#pragma once

#include <string>
#include <algorithm>
#include <functional>
#include <thread>

#include "jpeg-compressor/jpgd.h"
#include "jpeg-compressor/jpge.h"

#include "Utils.h"
#include "ImageObject.h"
#include "ImageUtils.h"
#include "Random.h"

class TextureSynthesiser {

public:
	using ProgressCallbackType = const std::function<void(float, std::string)>&;
	enum GenerationMode : int {
		BRUTE_FORCE,
		K_COHERENCE,
		PATCH_BASED
	};
	enum ValueDistanceMode: int {
		INPUT_INPUT,
		INPUT_OUTPUT
	};
private:
	using PixelImage = Image<Pixel>;
	using ReferenceImage = Image<int>;
	static constexpr int COLOR_COMPONENTS  = 3;
	static constexpr int UNSET_PIXEL_VALUE = -1;

private:
	Dimension			inputDimension;
	PixelImage			inputImage;
	std::string			inputImagePath;

	std::vector<int>	inputImageIDs;
	std::vector<std::vector<int>>
						inputSimilarIDs;

	Dimension			outputDimension;
	ReferenceImage		outputRefImage;
	PixelImage			outputImage;

	int					neighbourSize;
	float				similarityThreshold;
	GenerationMode		generationMode;
	float				coherenceThreshold;

public:
	TextureSynthesiser(
		std::string inputImagePath,
		Dimension outputDimension,
		int neighbourSize,
		float similarityThreshold,
		GenerationMode generationMode,
		float coherenceThreshold
	):
		inputImagePath(inputImagePath),
		outputDimension(outputDimension),
		outputRefImage(outputDimension.width, outputDimension.height),
		outputImage(outputDimension.width, outputDimension.height),
		neighbourSize(neighbourSize),
		similarityThreshold(similarityThreshold),
		generationMode(generationMode),
		coherenceThreshold(coherenceThreshold)
	{
		LoadInputImage();
	}

	void LoadInputImage(){
		int actualPixelSize;
		unsigned char *imageData = jpgd::decompress_jpeg_image_from_file(
			inputImagePath.c_str(),
			&inputDimension.width,
			&inputDimension.height,
			&actualPixelSize,
			COLOR_COMPONENTS
		);
		AssertRT(actualPixelSize == COLOR_COMPONENTS);

		inputImage.SetDimension(inputDimension);
		int inputImageRawSize = inputDimension.size() * 3;
		for (int i = 0; i < inputImageRawSize; i += 3){
			float r = float(imageData[i + 0]) / 255.f;
			float g = float(imageData[i + 1]) / 255.f;
			float b = float(imageData[i + 2]) / 255.f;
			inputImage.Data().emplace_back(r, g, b);
		}
		free(imageData);
	}

	float GetColorDistanceSquared(const Pixel& a, const Pixel& b){
		return
			(a.r - b.r)*(a.r - b.r) +
			(a.g - b.g)*(a.g - b.g) +
			(a.b - b.b)*(a.b - b.b);
	}

	inline int TileizeValue(int value, int max){
		if (value < 0){
			return (value + max) % max;
		} else {
			return value % max;
		}
	}

	inline Coordinate TileizeCoordinate(const Coordinate& coord, const Dimension& dimension){
		Coordinate tileizedCoord{coord};

		tileizedCoord.y = TileizeValue(coord.y, dimension.height);
		tileizedCoord.x = TileizeValue(coord.x, dimension.width);
		
		return tileizedCoord;
	};

	void FillReferenceOutputWithNoise(){
		RandomGenerator randomGenerator{0, double(inputDimension.size())};
		for (int hOut = 0; hOut < outputDimension.height; hOut++){
			for (int wOut = 0; wOut < outputDimension.width; wOut++){
				const int randomInputPosition = int(randomGenerator());
				outputRefImage.Data().push_back(randomInputPosition);
			}
		}
		AssertRT(outputRefImage.Data().size() == outputDimension.size());
	}

	template <ValueDistanceMode DistanceMode>
	float GetBlockDistance(const Coordinate& similarCoord, const Coordinate& originalCoord){
		// calculate the accumulated distance of two blocks in input image
		// TODO: if we are not interested in a cropped block, 
		// we should return from the for cycle instead of 
		// relying on the num of found pixels
		float sumOfDistances = 0.f;
		float foundPixelInBlock = 0.f;
		for (int hInBlock = -neighbourSize; hInBlock <= neighbourSize; ++hInBlock){
			for (int wInBlock = -neighbourSize; wInBlock <= neighbourSize; ++wInBlock){
				if ((hInBlock | wInBlock) == 0){
					hInBlock = INT_MAX - 1;
					wInBlock = INT_MAX - 1;
					break;
				} else {
					const Coordinate offsetedSimCrd{similarCoord.x + wInBlock, similarCoord.y + hInBlock};
					const Coordinate offsetedOrigCrd{originalCoord.x + wInBlock, originalCoord.y + hInBlock};
					if (offsetedSimCrd.x >= 0 &&
						offsetedSimCrd.x < inputDimension.width &&
						offsetedSimCrd.y >= 0 &&
						offsetedSimCrd.y < inputDimension.height &&
						(
							(DistanceMode == ValueDistanceMode::INPUT_OUTPUT) || (
								offsetedOrigCrd.x >= 0 &&
								offsetedOrigCrd.x < inputDimension.width &&
								offsetedOrigCrd.y >= 0 &&
								offsetedOrigCrd.y < inputDimension.height
							)
						)
					){
						const Pixel& similarPixel = inputImage.At(offsetedSimCrd);
						if (DistanceMode == ValueDistanceMode::INPUT_INPUT){
							const Pixel& originalPixel = inputImage.At(offsetedOrigCrd);
							sumOfDistances += GetColorDistanceSquared(similarPixel, originalPixel);
						} else {
							int inputPixelOffset = outputRefImage.At(TileizeCoordinate(offsetedOrigCrd, outputDimension));
							const Pixel& originalPixel = inputImage.At(inputPixelOffset);
							sumOfDistances += GetColorDistanceSquared(similarPixel, originalPixel);
						}
						foundPixelInBlock += 1.0f;
					}
				}
			}
		}
		const float validPixelCount = float((neighbourSize*2 + 1)*(neighbourSize + 1) - (neighbourSize + 1));
		if (foundPixelInBlock == validPixelCount){
			const float normalizedDistance = sumOfDistances / foundPixelInBlock;
			if (normalizedDistance <= similarityThreshold){
				// too big similarity makes the result noisy
				return FLT_MAX;
			} else {
				return normalizedDistance;
			}
		} else {
			return FLT_MAX;
		}
	}

	void BuildCoherenceMap(ProgressCallbackType callback){
		callback(0, "initializing id arrays");
		for (int hIn = 0; hIn < inputDimension.height; ++hIn){
			for (int wIn = 0; wIn < inputDimension.width; ++wIn){
				inputImageIDs.push_back(UNSET_PIXEL_VALUE);
				inputSimilarIDs.push_back(std::vector<int>{});
			}
		}
		AssertRT(inputImageIDs.size() == inputDimension.size());
		AssertRT(inputSimilarIDs.size() == inputDimension.size());

		callback(0, "loading similars");
		for (int hIn = neighbourSize; hIn < inputDimension.height - neighbourSize; ++hIn){
			for (int wIn = neighbourSize; wIn < inputDimension.width - neighbourSize; ++wIn){
				const int pixelOffset = hIn*inputDimension.width + wIn;
				// inputSimilarIDs[inputImageID] = [inputImageID1, inputImageID2, ...]
				AssertRT(inputImageIDs.size() > pixelOffset);
				if (inputImageIDs[pixelOffset] == UNSET_PIXEL_VALUE){
					const int& pixelID = pixelOffset;
					inputImageIDs[pixelOffset] = pixelID;
					AssertRT(inputSimilarIDs[pixelID].empty());
					inputSimilarIDs[pixelID].push_back(pixelOffset);
					int hInSimStart = (wIn + 1 < inputDimension.width - neighbourSize) ? hIn : hIn + 1;
					int wInSimStart = (wIn + 1 < inputDimension.width - neighbourSize) ? wIn + 1 : neighbourSize;
					for (int hInSim = hInSimStart; hInSim < inputDimension.height - neighbourSize; ++hInSim){
						for (int wInSim = wInSimStart; wInSim < inputDimension.width - neighbourSize; ++wInSim){
							const int similarPixelOffset = hInSim*inputDimension.width + wInSim;
							if (inputImageIDs[similarPixelOffset] == UNSET_PIXEL_VALUE){
								const Coordinate similarCoordinate{wInSim, hInSim};
								const Coordinate currentCoordinate{wIn, hIn};
								const float distance = GetBlockDistance<ValueDistanceMode::INPUT_INPUT>(similarCoordinate, currentCoordinate);
								if (distance < coherenceThreshold){
									inputImageIDs[similarPixelOffset] = pixelID;
									inputSimilarIDs[pixelID].push_back(similarPixelOffset);
								}
							}
						}
					}
					//std::cout << "similar for id-" << pixelID << ": " << inputSimilarIDs[pixelID].size() << std::endl;
					//AssertRT(inputSimilarIDs[pixelID].size() < 10);
				}
			}
			callback(float(hIn) / inputDimension.height, "building coherence");
		}
	}

	void SynthesiseTexture(ProgressCallbackType callback) {
		// walk over every pixel on the output image
		const float goodEnoughDistance = similarityThreshold * 1.4f;
		for (int hOut = 0; hOut < outputDimension.height; hOut++) {
			for (int wOut = 0; wOut < outputDimension.width; wOut++) {
				Coordinate outputPixelCoord{wOut, hOut};
				if (generationMode == GenerationMode::BRUTE_FORCE) {
					Coordinate candidateInputPixel;
					float neighbourhoodMinimalDistance = FLT_MAX;
					for (int hIn = 0; hIn < inputDimension.height; ++hIn) {
						for (int wIn = 0; wIn < inputDimension.width; ++wIn) {
							Coordinate inputPixelCoord{wIn, hIn};
							float inputNeighbourhoodDistance = GetBlockDistance<ValueDistanceMode::INPUT_OUTPUT>(inputPixelCoord, outputPixelCoord);
							if (inputNeighbourhoodDistance < neighbourhoodMinimalDistance) {
								neighbourhoodMinimalDistance = inputNeighbourhoodDistance;
								candidateInputPixel.y = hIn;
								candidateInputPixel.x = wIn;
								if (neighbourhoodMinimalDistance <= goodEnoughDistance) {
									hIn = inputDimension.height;
									break;
								}
							}
						}
					}
					outputRefImage.At(wOut, hOut) = candidateInputPixel.y * inputDimension.width + candidateInputPixel.x;
				} else if (generationMode == GenerationMode::K_COHERENCE) {
					float minDistance = FLT_MAX;
					Coordinate bestInputMatch;
					for (int hOutBlock = -neighbourSize; hOutBlock <= neighbourSize; ++hOutBlock) {
						for (int wOutBlock = -neighbourSize; wOutBlock <= neighbourSize; ++wOutBlock) {
							if ((wOutBlock | hOutBlock) == 0) {
								hOutBlock = INT_MAX - 1;
								break;
							}
							int inputNeighbourOffset = outputRefImage.At(
								TileizeValue(hOut + hOutBlock, outputDimension.height),
								TileizeValue(wOut + wOutBlock, outputDimension.width)
							);
							// get the pixelID of the current output reference
							if (inputImageIDs[inputNeighbourOffset] != UNSET_PIXEL_VALUE) {
								int pixelID = inputImageIDs[inputNeighbourOffset];
								for (const int similarOffset : inputSimilarIDs[pixelID]) {
									const Coordinate inputOffsetCoord = OffsetToCoordinate(similarOffset, inputDimension);
									float neighbourhoodDistance = GetBlockDistance<ValueDistanceMode::INPUT_OUTPUT>(
										inputOffsetCoord,
										outputPixelCoord
									);
									if (neighbourhoodDistance < minDistance) {
										minDistance = neighbourhoodDistance;
										bestInputMatch.x = inputOffsetCoord.x;
										bestInputMatch.y = inputOffsetCoord.y;
									}
								}
							}
						}
					}
					outputRefImage.At(wOut, hOut) = bestInputMatch.y * inputDimension.width + bestInputMatch.x;
				}
			}
			callback(float(hOut) / float(outputDimension.height), "filling output image");
		}
	}

	void Generate(ProgressCallbackType callback) {

		if (generationMode == PATCH_BASED) {
			GeneratePatchBased(callback);
		} else {

			// fill up the output image with noise from input image
			callback(0, "fill reference output with noise");
			FillReferenceOutputWithNoise();

			// create coherence map out of the input pixels
			if (generationMode == GenerationMode::K_COHERENCE) {
				callback(0, "loading coherence map");
				BuildCoherenceMap(callback);
			}

			SynthesiseTexture(callback);
		}
	}

	void GeneratePatchBased(ProgressCallbackType callback) {
		/*
		Megoldás menete:
			Változók: patch mérete, ütközõzóna mérete
			Lerakunk egy random patch-et
			Amíg van üres rész az outputon
				Lerakunk egy random patch-et
				Amíg van a patch-nek feldolgozatlan ütközõzónája
					Minden sorra/oszlopra megnézzük, hogy honnantól kéne belevágni
					Azaz az adott pixel és 1D környezete mennyire tér el az output-tól az adott helyen

		*/
		// fill output (to see unfilled pixels)
		for (int hOut = 0; hOut < outputDimension.height; hOut++) {
			for (int wOut = 0; wOut < outputDimension.width; wOut++) {
				outputRefImage.Data().push_back(0);
			}
		}
		AssertRT(outputRefImage.Data().size() == outputDimension.size());

		// init
		const int patchSize = 40;
		AssertRT(patchSize < inputDimension.width);
		AssertRT(patchSize < inputDimension.height);
		const int borderSize = 10;
		AssertRT(borderSize < patchSize);
		RandomGenerator randomWidth{0.0, double(inputDimension.width - patchSize)};
		RandomGenerator randomHeight{0.0, double(inputDimension.height - patchSize)};
		// place first patch
		{
			const int randomOffsetWidth = int(randomWidth());
			const int randomOffsetHeight = int(randomHeight());
			for (int h = 0; h < patchSize; ++h) {
				for (int w = 0; w < patchSize; ++w) {
					const int rWidth = randomOffsetWidth + w;
					const int rHeight = randomOffsetHeight + h;
					outputRefImage.At(h * outputDimension.width + w) = rHeight * inputDimension.width + rWidth;
				}
			}
		}
		// place second patch
		{
			for (int i = 1; i < outputDimension.width / (patchSize - borderSize); ++i) {
				const int randomOffsetWidth = int(randomWidth());
				const int randomOffsetHeight = int(randomHeight());
				const int widthFrom = i * (patchSize - borderSize);
				std::vector<int> outputRefs;
				std::vector<int> inputRefs;
				for (int h = 0; h < patchSize; ++h) {
					for (int w = widthFrom; w < widthFrom + patchSize; ++w) {
						const int rWidth = randomOffsetWidth + w - widthFrom;
						const int rHeight = randomOffsetHeight + h;
						AssertRT(rWidth < inputDimension.width);
						AssertRT(rHeight < inputDimension.height);
						const int outputOffset = h * outputDimension.width + w;
						const int inputOffset = rHeight * inputDimension.width + rWidth;
						if ((w-widthFrom) < borderSize) {
							outputRefs.push_back(outputOffset);
							inputRefs.push_back(inputOffset);
						} else {
							outputRefImage.At(outputOffset) = inputOffset;
						}
					}
				}
				AssertRT(outputRefs.size() == inputRefs.size());
				AssertRT(outputRefs.size() == patchSize*borderSize);
				for (int h = 0; h < patchSize; ++h) {
					int minOffset;
					float minValue = FLT_MAX;
					for (int w = 0; w < borderSize; ++w) {
						const int outputRef = outputRefs[h*borderSize + w];
						const int inputRef = inputRefs[h*borderSize + w];
						const Pixel& inputColor = inputImage.At(inputRef);
						const Pixel& outputColor = inputImage.At(outputRefImage.At(outputRef));
						float dist = GetColorDistanceSquared(inputColor, outputColor);
						if (dist < minValue) {
							minValue = dist;
							minOffset = w;
						}
					}
					for (int w = minOffset; w < borderSize; ++w) {
						const int outputRef = outputRefs[h*borderSize + w];
						const int inputRef = inputRefs[h*borderSize + w];
						/*if (w == minOffset) {
							outputRefImage.At(outputRef) = -1;
						} else */{
							outputRefImage.At(outputRef) = inputRef;
						}
					}
					std::cout << std::endl;
				}
			}
		}
	}

	void SaveToFile(std::string outputImagePath){
		std::vector<unsigned char> outputImageBuffer;
		std::for_each(
			outputRefImage.Data().cbegin(),
			outputRefImage.Data().cend(),
			[&](const int inputOffset){
				Pixel pixel{1, 0, 0};
				/*if (inputOffset != -1) */{
					pixel = inputImage.At(inputOffset);
				}
				outputImageBuffer.push_back(unsigned char(pixel.r*255.f));
				outputImageBuffer.push_back(unsigned char(pixel.g*255.f));
				outputImageBuffer.push_back(unsigned char(pixel.b*255.f));
			}
		);
		bool resultOfCompression = jpge::compress_image_to_jpeg_file(
			outputImagePath.c_str(),
			outputDimension.width,
			outputDimension.height,
			COLOR_COMPONENTS,
			outputImageBuffer.data()
		);
	}

};