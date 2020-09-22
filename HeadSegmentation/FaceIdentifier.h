#pragma once

#include "Global.h"
#include "Smoothing.h"
#include <deque>

#ifndef FACEIDENTIFIER_H
#define FACEIDENTIFIER_H

namespace hseg
{
	class faceidentifier
	{
	public:
		faceidentifier(int id, int identifierExpiry, int capacity);
		faceidentifier();
		~faceidentifier();

		int id, capacity, trackNum;
		std::deque<std::pair<long long, std::vector<cv::Rect>>> faceStore;	// store for face bounding rects
		cv::Rect lastTrackedFace;									// last tracked face bounding rect
		doubleSmoothing locationX, locationY;						// smoothing for tracking location
		singleSmoothing sizeX, sizeY;								// smoothing for tracking size
		medianSmoothing noseY;										// smoothing for nose y-location

		cv::Rect2f getRect2f();

	private:
		long int identifierExpiry;
	};
}

#endif
