// HeadSegmentation.cpp : Defines the exported functions for the DLL.

#include "pch.h" // use pch.h in Visual Studio 2019

#include "HeadSegmentation.h"

static hseg::headSegmentation* seg = NULL;
static hseg::facedetect* facedetector = NULL;
static const int setconcurrency = 4;

// GENERAL FUNCTIONS
void initSegmenter(int topIndex, int bottomIndex, int landmarks)
{
	// check thread concurrency
	int concurrentthreads = std::thread::hardware_concurrency();
	if (concurrentthreads == 0) {
		concurrentthreads = setconcurrency;
	}

	if (seg == NULL) {
		seg = new hseg::headSegmentation(concurrentthreads, topIndex, bottomIndex, landmarks);
	}

	if (facedetector == NULL) {
		facedetector = new hseg::facedetect();
	}
}

void exitSegmenter()
{
	if (seg != NULL) {
		delete seg;
		seg = NULL;
	}

	if (facedetector != NULL) {
		delete facedetector;
		facedetector = NULL;
	}
}

void resetSegmenter()
{
	if (seg != NULL) {
		seg->reset();
	}
}

void setSegment(BYTE* pointbuffer_in, int faceCount, BYTE* imgstruct_in, BYTE* imgbuffer_in, int* duration)
{
	if (seg != NULL) {
		// get input
		std::vector<std::vector<cv::Point2f>> detectedFacePoints = hseg::utility::getPoint2fVecOfVec(pointbuffer_in, seg->landmarks, faceCount);
		cv::Mat img_input = hseg::utility::getMat(imgstruct_in, imgbuffer_in);

		auto tstart = std::chrono::high_resolution_clock::now();

		seg->set(img_input, detectedFacePoints);

		auto tend = std::chrono::high_resolution_clock::now();
		*duration = (int)std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count();
	}
}

void getSegment(int* faceCount, int* trackID, BYTE* imgstruct_in, BYTE* imgbuffer_in, BYTE* imgstructmask_out, BYTE* imgbuffermask_out, int* duration)
{
	if (seg != NULL) {
		cv::Mat img_input = hseg::utility::getMat(imgstruct_in, imgbuffer_in);
		cv::Mat img_mask = hseg::utility::getMat(imgstructmask_out, imgbuffermask_out);

		std::vector<cv::Rect2f> matchedFaces;

		auto tstart = std::chrono::high_resolution_clock::now();

		seg->get(img_input, &img_mask, &matchedFaces, trackID);

		auto tend = std::chrono::high_resolution_clock::now();
		*duration = (int)std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count();

		seg->segtracker->matchedFacesDict.emplace(std::make_pair(*trackID, matchedFaces));
		*faceCount = (int)matchedFaces.size();
	}
}

void getFetch(int trackID, BYTE* rectbuffer_out)
{
	if (seg != NULL) {
		// set results
		if (seg->segtracker->matchedFacesDict.count(trackID) > 0) {
			int faceCount = (int)seg->segtracker->matchedFacesDict[trackID].size();
			if (faceCount > 0) {
				hseg::utility::setRect2fVec(rectbuffer_out, seg->segtracker->matchedFacesDict[trackID]);
			}
			seg->segtracker->matchedFacesDict.erase(trackID);
		}
	}
}

void FD_multiview_reinforce(BYTE* matstruct_in, BYTE* buffer_in, BYTE* faceresults, int* facecount)
{
	facedetector->FD_multiview_reinforce(matstruct_in, buffer_in, faceresults, facecount);
}

// HEADSEGMENTATION FUNCTIONS
// the topIndex and bottomIndex refer to two points that mark the face midline (usually top of head / nose and chin)
// landmarks refers to the number of landmark points
hseg::headSegmentation::headSegmentation(unsigned concurrentthreads, int topIndex, int bottomIndex, int landmarks)
{
	hseg::headSegmentation::concurrentthreads = concurrentthreads;
	hseg::headSegmentation::topIndex = topIndex;
	hseg::headSegmentation::bottomIndex = bottomIndex;
	hseg::headSegmentation::landmarks = landmarks;

	segtracker = new tracker();
	segsegmenter = new segmenter(concurrentthreads);
	gammacorrect = new gamma();

	reset();
}

hseg::headSegmentation::~headSegmentation()
{
	reset();

	delete segtracker;
	segtracker = NULL;

	delete segsegmenter;
	segsegmenter = NULL;

	delete gammacorrect;
	gammacorrect = NULL;
}

void hseg::headSegmentation::set(const cv::Mat& img, const std::vector<std::vector<cv::Point2f>>& detectedFacePoints)
{
	assert(img.empty() == false && img.channels() == 3);

	if (detectedFacePoints.size() > 0) {
		int maxlandmarks = 0;
		int minlandmarks = INT_MAX;
		for (int i = 0; i < (int)detectedFacePoints.size(); i++) {
			int landmarksize = (int)detectedFacePoints[i].size();
			maxlandmarks = MAX(maxlandmarks, landmarksize);
			minlandmarks = MIN(minlandmarks, landmarksize);
		}
		assert(maxlandmarks == landmarks && minlandmarks == landmarks);

		// get frame container for processing images
		frameContainer frame = hseg::frameContainer::getFrameContainer(img, gammacorrect);

		std::vector<std::vector<cv::Point2f>> smallFacePoints(detectedFacePoints.size());
		std::vector<std::vector<cv::Point2f>> tinyFacePoints(detectedFacePoints.size());
		for (int i = 0; i < detectedFacePoints.size(); i++) {
			smallFacePoints[i] = std::vector<cv::Point2f>(detectedFacePoints[i].size());
			tinyFacePoints[i] = std::vector<cv::Point2f>(detectedFacePoints[i].size());
			for (int j = 0; j < detectedFacePoints[i].size(); j++) {
				smallFacePoints[i][j] = cv::Point2f(detectedFacePoints[i][j].x * frame.smallScale, detectedFacePoints[i][j].y * frame.smallScale);
				tinyFacePoints[i][j] = cv::Point2f(detectedFacePoints[i][j].x * frame.tinyScale, detectedFacePoints[i][j].y * frame.tinyScale);
			}
		}

		// convert landmarks into rotated rects
		std::vector<cv::RotatedRect> smallFaceRects = convertPoints(smallFacePoints);

		// match identifiers to face rectangles
		std::unordered_map<int, int> finalMatches = processIdentifiers(frame, smallFaceRects);
		identifiersDict[0].locationX.enqueue(smallFaceRects[0].center.x);// REMOVE WHEN IDENTIFIERS FIXED
		identifiersDict[0].locationY.enqueue(smallFaceRects[0].center.y);// REMOVE WHEN IDENTIFIERS FIXED
		cv::Rect2f boundingRect = smallFaceRects[0].boundingRect2f();// REMOVE WHEN IDENTIFIERS FIXED
		float noseY = (smallFacePoints[0][nosePoint].y - boundingRect.y) / boundingRect.height;// REMOVE WHEN IDENTIFIERS FIXED
		if (abs(noseY - 0.5f) <= noseLimit) {// REMOVE WHEN IDENTIFIERS FIXED
			identifiersDict[0].noseY.enqueue(noseY);// REMOVE WHEN IDENTIFIERS FIXED
		}// REMOVE WHEN IDENTIFIERS FIXED

		// iterate through identifiers and get match
		float tinySmallScale = frame.tinyScale / frame.smallScale;
		for (auto itIden = identifiersDict.begin(); itIden != identifiersDict.end(); ++itIden) {
			if (finalMatches.count(itIden->second.id) > 0) {
				// match key found
				int identifierId = itIden->second.id;
				int matchKey = finalMatches[identifierId];
				cv::RotatedRect smallFaceRect = smallFaceRects[matchKey];

				// update tracker and identifier
				segtracker->setTracker(frame, &identifiersDict, identifierId, smallFaceRect);
			}
		}
	}
}

// no face points, just tracking
void hseg::headSegmentation::get(const cv::Mat& img, cv::Mat* mask, std::vector<cv::Rect2f>* matchedFaces, int* trackID)
{
	assert(img.empty() == false && img.channels() == 3);

	// get frame container for processing images
	frameContainer frame = hseg::frameContainer::getFrameContainer(img, gammacorrect);

	// iterate through identifiers and get match
	mask->setTo(0);
	float tinySmallScale = frame.tinyScale / frame.smallScale;
	*matchedFaces = std::vector<cv::Rect2f>();
	for (auto itIden = identifiersDict.begin(); itIden != identifiersDict.end(); ++itIden) {
		// match key found
		int identifierId = itIden->second.id;
		float smoothedNoseY = (float)identifiersDict[identifierId].noseY.getSmoothedValue();
		if (isnan(smoothedNoseY)) {
			smoothedNoseY = 0.5f;
		}

		// update tracker and identifier
		cv::Rect2f smallFaceRect;
		segtracker->getTracker(frame, &identifiersDict, identifierId, &smallFaceRect);

		cv::Rect2f tinyFaceRect(smallFaceRect.x * tinySmallScale, smallFaceRect.y * tinySmallScale, smallFaceRect.width * tinySmallScale, smallFaceRect.height * tinySmallScale);

		segsegmenter->segment(frame, &identifiersDict, identifierId, tinyFaceRect, mask, smoothedNoseY);

		if (!smallFaceRect.empty() && !isnan(smallFaceRect.x) && !isnan(smallFaceRect.width)) {
			matchedFaces->push_back(cv::Rect2f(smallFaceRect.x / frame.smallScale, smallFaceRect.y / frame.smallScale, smallFaceRect.width / frame.smallScale, smallFaceRect.height / frame.smallScale));
		}
	}

	// track by frame number
	*trackID = framenumberTrack;

	framenumberTrack++;
}

// reset segmenter
void hseg::headSegmentation::reset()
{
	framenumberTrack = 0;
	identifiersDict.clear();
	segtracker->reset();
}

// converts face points to rotated rects and resets the angle to point from chin to forehead
std::vector<cv::RotatedRect> hseg::headSegmentation::convertPoints(const std::vector<std::vector<cv::Point2f>>& detectedFacePoints)
{
	std::vector<cv::RotatedRect> detectedFaceRects(detectedFacePoints.size());
	for (int i = 0; i < detectedFacePoints.size(); i++) {
		cv::RotatedRect rect = cv::minAreaRect(detectedFacePoints[i]);
		rect = hseg::utility::resetRotatedRect(rect, detectedFacePoints[i], topIndex, bottomIndex);
		detectedFaceRects[i] = rect;
	}
	return detectedFaceRects;
}

// processes face identification
std::unordered_map<int, int> hseg::headSegmentation::processIdentifiers(const frameContainer& frame, const std::vector<cv::RotatedRect>& detectedFaceRects)
{
	// PLACEHOLDER FUNCTION
	// PLACEHOLDER FUNCTION
	// PLACEHOLDER FUNCTION
	if (identifiersDict.size() == 0) {
		faceidentifier identifier(0, identifierExpiry, storeCapacity);
		identifiersDict.emplace(std::make_pair(0, identifier));
	}

	std::unordered_map<int, int> finalMatches; // key: identifier id, value: match (detectedFaceRects) index
	finalMatches.emplace(std::make_pair(0, 0));
	return finalMatches;
	// PLACEHOLDER FUNCTION
	// PLACEHOLDER FUNCTION
	// PLACEHOLDER FUNCTION
}
