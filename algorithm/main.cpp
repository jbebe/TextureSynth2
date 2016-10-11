#include <iostream>
#include <iomanip>
#include <chrono>

//#define DEBUG

#include "TextureSynthesiser.h"

int main(int argc, char* argv[]){
	
	using namespace std::chrono;
	steady_clock::time_point startTime = steady_clock::now();
	auto generateCallback = [&](float percent, std::string msg) {
		if (percent != 0.f) {
			steady_clock::time_point currentTime = steady_clock::now();
			const float elapsedMS = float(duration_cast<microseconds>(currentTime - startTime).count());
			const float remainingMS = (elapsedMS / percent)*(1.f - percent);
			const int remainingS = int(remainingMS / (1e6f));
			std::cout << remainingS << "sec remaining";
		} else {
			std::cout << "+Inf sec remaining";
		}
		if (msg.empty() == false) {
			std::cout << " msg: " << msg << std::endl;
		}
		std::cout.flush();
	};

	TextureSynthesiser textureGenerator {
		"2.jpg",
		/* output dimension */ Dimension{64, 64},
		/* neighbour size */ 5, // (2*x+1)
		/* similarityThreshold */ 0.02f, // if x<threshold -> skip
		TextureSynthesiser::GenerationMode::PATCH_BASED,
		/*coherenceThreshold*/ 0.2f // if x>threshold -> skip
	};

	textureGenerator.Generate(generateCallback);
	textureGenerator.SaveToFile("2_out.jpg");

	return 0;
}
