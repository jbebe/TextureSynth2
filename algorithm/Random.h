#pragma once

#include <ctime>
#include <random>

//#define FREEZE_RANDOM

class RandomGenerator {

private:

	std::mt19937 generator;
	std::uniform_real_distribution<> rand_get;

public:

	RandomGenerator(double from, double to):
#ifdef FREEZE_RANDOM
		generator(0),
#else
		generator((unsigned int)std::time(nullptr)),
#endif
		rand_get(double(from), double(to)){}

	double operator () (){
		return rand_get(generator);
	}

};