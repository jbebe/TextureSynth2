#pragma once

class HSVData {

public:
	float h;
	float s;
	float v;

	HSVData(float h = 0.f, float s = 0.f, float v = 0.f):
		h(h),
		s(s),
		v(v) {}
};

class RGBData {

public:
	float r;
	float g;
	float b;

	RGBData(float r = 0.f, float g = 0.f, float b = 0.f):
		r(r),
		g(g),
		b(b) {}
};

using Pixel = RGBData;

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

HSVData RgbToHsv(RGBData in) {
	HSVData     out;
	float       min, max, delta;

	min = in.r < in.g ? in.r : in.g;
	min = min  < in.b ? min : in.b;

	max = in.r > in.g ? in.r : in.g;
	max = max  > in.b ? max : in.b;

	out.v = max;                                // v
	delta = max - min;
	if (delta < 0.00001f) {
		out.s = 0;
		out.h = 0; // undefined, maybe nan?
		return out;
	}
	if (max > 0.0f) { // NOTE: if Max is == 0, this divide would cause a crash
		out.s = (delta / max);                  // s
	} else {
		// if max is 0, then r = g = b = 0              
		// s = 0, v is undefined
		out.s = 0.0;
		out.h = NAN;                            // its now undefined
		return out;
	}
	if (in.r >= max)                           // > is bogus, just keeps compilor happy
		out.h = (in.g - in.b) / delta;        // between yellow & magenta
	else
		if (in.g >= max)
			out.h = 2.0f + (in.b - in.r) / delta;  // between cyan & yellow
		else
			out.h = 4.0f + (in.r - in.g) / delta;  // between magenta & cyan

	out.h *= 60.0f;                              // degrees

	if (out.h < 0.0)
		out.h += 360.0f;

	return out;
}


RGBData HsvToRgb(HSVData in) {
	float       hh, p, q, t, ff;
	long        i;
	RGBData     out;

	if (in.s <= 0.0f) {       // < is bogus, just shuts up warnings
		out.r = in.v;
		out.g = in.v;
		out.b = in.v;
		return out;
	}
	hh = in.h;
	if (hh >= 360.0f) hh = 0.0f;
	hh /= 60.0f;
	i = (long)hh;
	ff = hh - i;
	p = in.v * (1.0f - in.s);
	q = in.v * (1.0f - (in.s * ff));
	t = in.v * (1.0f - (in.s * (1.0f - ff)));

	switch (i) {
	case 0:
		out.r = in.v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = in.v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = in.v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = in.v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = in.v;
		break;
	case 5:
	default:
		out.r = in.v;
		out.g = p;
		out.b = q;
		break;
	}
	return out;
}