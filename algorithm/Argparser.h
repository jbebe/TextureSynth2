#pragma once

#include <vector>
#include <string>

//void ParseArguments(int argc, char* argv[]){
//	std::vector<std::string> arguments;
//	for (int i = 0; i < argc; ++i){
//		arguments.emplace_back(argv[i]);
//	}
//	for (int i = 0; i < argc; ++i){
//		const std::string& arg = arguments[i];
//
//		if (arg[0] == '-'){
//			if (arg[1] == 'w'){
//				const std::string& argNext = arguments[i + 1];
//				outputWidth = std::stoi(argNext);
//				++i;
//				continue;
//			}
//			if (arg[1] == 'h'){
//				const std::string& argNext = arguments[i + 1];
//				outputHeight = std::stoi(argNext);
//				++i;
//				continue;
//			}
//			if (arg[1] == 'o'){
//				outputImagePath = arguments[i + 1];
//				++i;
//				continue;
//			}
//		} else{
//			std::ifstream inputImage{arg, std::ifstream::in};
//			if (inputImage.good()){
//				inputImagePath = arg;
//			} else{
//				std::cout << "cannot load image" << std::endl;
//			}
//		}
//
//	}
//}