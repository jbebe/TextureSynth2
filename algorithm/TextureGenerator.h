#pragma once

#include <string>
#include <algorithm>
#include <functional>

#include "jpeg-compressor/jpgd.h"
#include "jpeg-compressor/jpge.h"

#include "Utils.h"
#include "ImageObject.h"
#include "ImageUtils.h"
#include "Random.h"

class TextureGenerator {

public:
	using ProgressCallbackType = const std::function<void(float, std::string)>&;
	enum Mode : int {
		BRUTE_FORCE,
		K_COHERENCE
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
	Mode				generationMode;
	float				coherenceThreshold;

public:
	TextureGenerator(
		std::string inputImagePath,
		Dimension outputDimension,
		int neighbourSize,
		float similarityThreshold,
		Mode generationMode,
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

	float GetColorDistanceSquared(const ColorData& a, const ColorData& b){
		return
			(a.r - b.r)*(a.r - b.r) +
			(a.g - b.g)*(a.g - b.g) +
			(a.b - b.b)*(a.b - b.b);
	}

	inline int TileizeValue(int value, int max){
		if (value < 0){
			return (value + max) % max;
		} else{
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

	float GetBlockDistance(const Coordinate& similarCoord, const Coordinate& originalCoord, bool originalIsInput = false){
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
							(originalIsInput == false) || (
								offsetedOrigCrd.x >= 0 &&
								offsetedOrigCrd.x < inputDimension.width &&
								offsetedOrigCrd.y >= 0 &&
								offsetedOrigCrd.y < inputDimension.height
							)
						)
					){
						const Pixel& similarPixel = inputImage.At(offsetedSimCrd);
						if (originalIsInput){
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
		if (foundPixelInBlock == float((neighbourSize*2+1)*(neighbourSize+1) - (neighbourSize+1))){
			const float normalizedDistance = sumOfDistances / foundPixelInBlock;
			if (normalizedDistance <= similarityThreshold){
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
								const float distance = GetBlockDistance(similarCoordinate, currentCoordinate, true);
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

	void Generate(ProgressCallbackType callback){

		// fill up the output image with noise from input image
		callback(0, "fill reference output with noise");
		FillReferenceOutputWithNoise();

		// create coherence map out of the input pixels
		if (generationMode == Mode::K_COHERENCE){
			callback(0, "loading coherence map");
			BuildCoherenceMap(callback);
		}

		// walk over every pixel on the output image
		for (int hOut = 0; hOut < outputDimension.height; hOut++){
			for (int wOut = 0; wOut < outputDimension.width; wOut++){
				Coordinate outputPixelCoord{wOut, hOut};
				if (generationMode == Mode::BRUTE_FORCE){
					Coordinate candidateInputPixel;
					float neighbourhoodMinimalDistance = FLT_MAX;
					for (int hIn = 0; hIn < inputDimension.height; ++hIn){
						for (int wIn = 0; wIn < inputDimension.height; ++wIn){
							Coordinate inputPixelCoord{wIn, hIn};
							float inputNeighbourhoodDistance = GetBlockDistance(inputPixelCoord, outputPixelCoord);
							if (inputNeighbourhoodDistance < neighbourhoodMinimalDistance){
								neighbourhoodMinimalDistance = inputNeighbourhoodDistance;
								candidateInputPixel.y = hIn;
								candidateInputPixel.x = wIn;
							}
						}
					}
					outputRefImage.At(wOut, hOut) = candidateInputPixel.y * inputDimension.width + candidateInputPixel.x;
				} else if (generationMode == Mode::K_COHERENCE){
					float minDistance = FLT_MAX;
					Coordinate bestInputMatch;
					for (int hOutBlock = -neighbourSize; hOutBlock <= neighbourSize; ++hOutBlock){
						for (int wOutBlock = -neighbourSize; wOutBlock <= neighbourSize; ++wOutBlock){
							if ((wOutBlock | hOutBlock) == 0){
								wOutBlock = INT_MAX - 1;
								hOutBlock = INT_MAX - 1;
								break;
							}
							int inputNeighbourOffset = outputRefImage.At(
								TileizeValue(hOut + hOutBlock, outputDimension.height),
								TileizeValue(wOut + wOutBlock, outputDimension.width)
							);
							// get the pixelID of the current output reference
							if (inputImageIDs[inputNeighbourOffset] != UNSET_PIXEL_VALUE){
								int pixelID = inputImageIDs[inputNeighbourOffset];
								for (const int similarOffset : inputSimilarIDs[pixelID]){
									const Coordinate inputOffsetCoord = OffsetToCoordinate(similarOffset, inputDimension);
									float neighbourhoodDistance = GetBlockDistance(
										inputOffsetCoord,
										outputPixelCoord
									);
									if (neighbourhoodDistance < minDistance){
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
			callback(float(hOut)/float(outputDimension.height), "filling output image");
		}
	}

	void SaveToFile(std::string outputImagePath){
		std::vector<unsigned char> outputImageBuffer;
		std::for_each(
			outputRefImage.Data().cbegin(),
			outputRefImage.Data().cend(),
			[&](const int inputOffset){
				const Pixel& pixel = inputImage.At(inputOffset);
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