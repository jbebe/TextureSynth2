#pragma once

#include <vector>
#include <cassert>
#include <shared_mutex>

#include "Utils.h"
#include "ImageUtils.h"

template <class DataType>
class Image {

public:

	Dimension dimension;
	std::vector<DataType> data;

public:

	Image(int width = 0, int height = 0):
		dimension(width, height)
	{
		data.reserve(width*height);
	}

	void SetDimension(const Dimension& d){
		dimension.width = d.width;
		dimension.height = d.height;
	}
	
	DataType& At(unsigned int offset){
		AssertRT(offset < data.size());
		return data.data()[offset];
	}

	DataType& At(unsigned int x, unsigned int y){
		AssertRT(x * y < unsigned int(dimension.size()));
		AssertRT(x * y < unsigned int(data.size()));
		return data[y * dimension.width + x];
	}

	DataType& At(const Coordinate& coord){
		return At(coord.x, coord.y);
	}

	std::vector<DataType>& Data(){
		return data;
	}

	void Set(unsigned int x, unsigned int y, const DataType& variable) {
		At(x, y) = variable;
	}

};

template <typename DataType>
class ThreadSafeImage: public Image<DataType> {

private:
	mutable std::shared_mutex dataSM;

public:
	ThreadSafeImage(int width = 0, int height = 0):
		Image<DataType>::Image(width, height)
	{}

	const DataType& ConstAt(const Coordinate& coord) {
		std::shared_lock<std::shared_mutex> lock(dataSM);
		return Image<DataType>::At(coord);
	}

	void Set(unsigned int x, unsigned int y, const DataType& variable) {
		std::unique_lock<std::shared_mutex> lock{dataSM};
		Image<DataType>::At(x, y) = variable;
	}

};