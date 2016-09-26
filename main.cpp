#include <iostream>
#include <iomanip>
#include <chrono>

#include "TextureGenerator.h"

int main(int argc, char* argv[]){
	TextureGenerator textureGenerator {
		"2.jpg",
		Dimension{256, 256},
		/* neighbour size */ 5, // (2*x+1)
		/* similarityThreshold */ 0.02f, // if x<threshold -> skip
		TextureGenerator::Mode::BRUTE_FORCE,
		/*coherenceThreshold*/ 0.1f // if x>threshold -> skip
	};

	using namespace std::chrono;
	steady_clock::time_point startTime = steady_clock::now();
	textureGenerator.Generate([&](float percent, std::string msg){
		if (percent != 0.f){
			steady_clock::time_point currentTime = steady_clock::now();
			const float elapsedMS = float(duration_cast<microseconds>(currentTime - startTime).count());
			const float remainingMS = (elapsedMS / percent)*(1.f - percent);
			const int remainingS = int(remainingMS / (1e6f));
			std::cout << remainingS << "sec remaining";
		} else {
			std::cout << "+Inf sec remaining";
		}
		std::cout << " msg: " << msg << std::endl;
		std::cout.flush();
	});
	
	
	textureGenerator.SaveToFile("2_out.jpg");

	return 0;
}