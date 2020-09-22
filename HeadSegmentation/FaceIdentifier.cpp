// FaceIdentifier.cpp : Defines the face identifier class for the DLL.

#include "pch.h" // use pch.h in Visual Studio 2019

#include "FaceIdentifier.h"

// FACEIDENTIFIER FUNCTIONS
hseg::faceidentifier::faceidentifier(int id, int identifierExpiry, int capacity)
{
	hseg::faceidentifier::id = id;
	hseg::faceidentifier::capacity = capacity;
	hseg::faceidentifier::identifierExpiry = identifierExpiry;
	trackNum = 0;
	faceStore = std::deque<std::pair<long long, std::vector<cv::Rect>>>();
	lastTrackedFace = cv::Rect();
	locationX = doubleSmoothing(doubleSmoothingCapacity);
	locationY = doubleSmoothing(doubleSmoothingCapacity);
	sizeX = singleSmoothing(doubleSmoothingCapacity);
	sizeY = singleSmoothing(doubleSmoothingCapacity);
	noseY = medianSmoothing(doubleSmoothingCapacity);
}

hseg::faceidentifier::faceidentifier()
{
	id = 0;
	capacity = 0;
	identifierExpiry = 0;
	trackNum = 0;
	faceStore = std::deque<std::pair<long long, std::vector<cv::Rect>>>();
	lastTrackedFace = cv::Rect();
	locationX = doubleSmoothing(0);
	locationY = doubleSmoothing(0);
	sizeX = singleSmoothing(0);
	sizeY = singleSmoothing(0);
	noseY = medianSmoothing(0);
}

hseg::faceidentifier::~faceidentifier()
{
	locationX.~doubleSmoothing();
	locationY.~doubleSmoothing();
	sizeX.~singleSmoothing();
	sizeY.~singleSmoothing();
	noseY.~medianSmoothing();
}

// gets smoothed rect
cv::Rect2f hseg::faceidentifier::getRect2f()
{
	float smoothX = (float)locationX.getSmoothedQueueResult(minSmoothingCount, mediumSmoothing, 0);
	float smoothY = (float)locationY.getSmoothedQueueResult(minSmoothingCount, mediumSmoothing, 0);
	float smoothWidth = (float)sizeX.getSmoothedQueueResult(minSmoothingCount);
	float smoothHeight = (float)sizeY.getSmoothedQueueResult(minSmoothingCount);
	return cv::Rect2f((float)(smoothX - ((smoothWidth - 1.0) / 2.0)), (float)(smoothY - ((smoothHeight - 1.0) / 2.0)), smoothWidth, smoothHeight);
}
