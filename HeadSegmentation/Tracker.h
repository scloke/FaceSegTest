#pragma once

#include "Global.h"
#include "Utility.h"
#include "FaceIdentifier.h"

#ifndef TRACKER_H
#define TRACKER_H

namespace hseg
{
	class tracker
	{
	public:
		tracker();
		~tracker();

		// RESET VARIABLES
		std::unordered_map<long long, cv::Mat> matStore;			// store for id-matrices
		std::unordered_map<int, std::vector<cv::Rect2f>> matchedFacesDict;	// temporary store for matched faces

		void setTracker(const frameContainer& frame, std::unordered_map<int, faceidentifier>* identifiersDict, int identifierId, const cv::RotatedRect& smallFaceRect);
		void getTracker(const frameContainer& frame, std::unordered_map<int, faceidentifier>* identifiersDict, int identifierId, cv::Rect2f* smallFaceRect);
		void reset();

	private:
		const float templateMatchCutoff = 0.7f;						// threshold for template matching
		const float templateMatchEdgeCutoff = 0.8f;					// threshold for template matching when the image touches the frame edge (prevent erronous matches)
		const float searchExpansion = 1.0;							// expansion factor for search rect
		const float searchExtended = 2.0;							// expansion factor for search rect
		const float searchMultiplier = 1.5;							// multiplier for search distance

		long long combineId(int identifierID, int trackNum);
		cv::Rect2f sortTrackMatches(const std::vector<std::pair<double, cv::Rect2f>>& matchResults, float trackThreshold, float templateMatchCutoffLocal);
	};

	struct DistanceEquivalence
	{
		float threshold = 0.0f;
		bool operator()(const cv::Point2f& point1, const cv::Point2f& point2) {
			float distance = sqrtf((point1.x - point2.x) * (point1.x - point2.x) + (point1.y - point2.y) * (point1.y - point2.y));
			return distance <= threshold;
		}
	};
}

#endif
