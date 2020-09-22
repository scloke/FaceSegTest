#pragma once

#include "Global.h"
#include "FaceIdentifier.h"
#include "Utility.h"
#include "dbScan.h"

#ifndef SEGMENTER_H
#define SEGMENTER_H

namespace hseg
{
	class segmenter
	{
	public:
		segmenter(unsigned concurrentthreads);
		segmenter();
		~segmenter();

		void segment(const frameContainer& frame, std::unordered_map<int, faceidentifier>* identifiersDict, int identifierId, const cv::Rect2f& tinyRect, cv::Mat* returnmask, float smoothedNoseY);

	private:
		dbScan* scan;												// medianScan segmenter class
		unsigned concurrentthreads;										// thread concurrency
		const float noseCenterY = 0.50f;								// nose point center
		const float noseDispMultipler = 5.0f;							// multiplier for nose displacement correction
		const float calcThreshold = 3.0f;								// used to calculate threshold
		const float threshAdjFactor = 40.0f;							// used to calculate threshold
		const float outlineVertExpansion = 1.75f;						// vertical expansion for face outline
		const float outlineHorzExpansion = 1.75f;						// horizontal expansion for face outline
		const double contourEpsilon = 1.0;								// epsilon for mask contour smoothing
		const float maskExtendUpper = 1.7f;								// extension factor for process mask
		const float maskExtendLower = 1.10f;							// extension factor for process mask
		const float maskExtendSide = 1.05f;								// extension factor for process mask
		const float includeFractionHead = 0.5f;							// fraction of pixels to be included in dominant clusters
		const float includeFractionLowerBody = 0.8f;					// fraction of pixels to be included in dominant clusters
		const int maskTransform = 2;									// morphology border width for mask transform

		enum grabCutEnum : BYTE { background = 0, possibleBackground = 2, foreground = 1, possibleForeground = 3 }; // enum for grabcut

		std::vector<cv::Point> getFaceOutline(const cv::RotatedRect& rect, const cv::Rect& boundingRect, float dispY);
		std::vector<cv::Point> getBodyOutline(const cv::Rect& tinyBoundingRect, const cv::Size& tinySize, const cv::Rect& lowerBodyRect, float dispY);
		std::vector<cv::Mat> processMask(const cv::Mat& mat, cv::Mat* labels, const std::vector<cv::Point>& outlineVec, float includeFraction);
		cv::RotatedRect extendRect(const cv::RotatedRect& rect, float upperExpansion, float lowerExpansion, float sideExpansion);
		std::vector<std::tuple<int, int, int>> getLabelCount(int* labelsBuffer, const cv::Size& labelsSize, const cv::Mat& mask);
		void getDominantMask(int* labelsBuffer, const cv::Size& labelsSize, const std::vector<std::vector<int>>& dominantLabelsVec, std::vector<cv::Mat>* maskVec);
		float getNoseDisp(float refHeight, float smoothedNoseY);
	};
}

#endif
