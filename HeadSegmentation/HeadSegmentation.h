#pragma once

#ifdef HEADSEGMENTATION_EXPORTS
#define SEGMENTATION_API __declspec(dllexport)
#else
#define SEGMENTATION_API __declspec(dllimport)
#endif

#pragma warning (disable : 26451)

#include "Global.h"
#include "Utility.h"
#include "FaceIdentifier.h"
#include "Tracker.h"
#include "Segmenter.h"
#include "FaceDetect.h"

extern "C" SEGMENTATION_API void initSegmenter(int topIndex, int bottomIndex, int landmarks);

extern "C" SEGMENTATION_API void exitSegmenter();

extern "C" SEGMENTATION_API void resetSegmenter();

extern "C" SEGMENTATION_API void setSegment(BYTE* pointbuffer_in, int faceCount, BYTE* imgstruct_in, BYTE* imgbuffer_in, int* duration);

extern "C" SEGMENTATION_API void getSegment(int* faceCount, int* trackID, BYTE * imgstruct_in, BYTE * imgbuffer_in, BYTE * imgstruct_out, BYTE * imgbuffer_out, int* duration);

extern "C" SEGMENTATION_API void getFetch(int trackID, BYTE * rectbuffer_out);

extern "C" SEGMENTATION_API void FD_multiview_reinforce(BYTE * matstruct_in, BYTE * buffer_in, BYTE * faceresults, int* facecount);

#ifndef SEGMENTATION_H
#define SEGMENTATION_H

namespace hseg
{
	class headSegmentation
	{
	public:
		headSegmentation(unsigned concurrentthreads, int topIndex, int bottomIndex, int landmarks);
		~headSegmentation();

		int landmarks;													// and the number of landmark points
		tracker* segtracker;											// face tracker class
		segmenter* segsegmenter;										// face segmenter class
		gamma* gammacorrect;											// gamma correction class

		void set(const cv::Mat& img, const std::vector<std::vector<cv::Point2f>>& detectedFacePoints);
		void get(const cv::Mat& img, cv::Mat* mask, std::vector<cv::Rect2f>* matchedFaces, int* trackID);
		void reset();

	private:
		static const int nosePoint = 33;								// i-bug nose point
		const float noseLimit = 0.10f;									// limit to nose displacement

		// VARIABLES
		int topIndex, bottomIndex;										// index of points that mark the face midline
		int storeCapacity;												// capacity for identifier smoothing store
		unsigned concurrentthreads;										// thread concurrency

		// RESET VARIABLES
		std::unordered_map<int, faceidentifier> identifiersDict;		// face identifiers
		long int framenumberTrack;										// number of frames processed

		std::vector<cv::RotatedRect> convertPoints(const std::vector<std::vector<cv::Point2f>>& detectedFacePoints);
		std::unordered_map<int, int> processIdentifiers(const frameContainer& frame, const std::vector<cv::RotatedRect>& detectedFaceRects);
	};
}

#endif
