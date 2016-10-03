#pragma once

class ColorData {

public:
	float r;
	float g;
	float b;
	
	ColorData(float r = 0.f, float g = 0.f, float b = 0.f):
		r(r),
		g(g),
		b(b)
	{}
};

using Pixel = ColorData;

class Coordinate {

public:

	union {
		int x;
		int w;
		int width;
	};
	union {
		int y;
		int h;
		int height;
	};

	Coordinate(int x = 0, int y = 0):
		x(x),
		y(y)
	{}

};

class Dimension: public Coordinate {

public:

	Dimension(int width = 0, int height = 0):
		Coordinate(width, height)
	{}

	int size() const {
		return size_t(width * height);
	}

};

inline Coordinate OffsetToCoordinate(int offset, const Dimension& dimension){
	return Coordinate {
		offset%dimension.width,
		offset/dimension.width
	};
}