// Tracker.cpp : Defines the face tracker class for the DLL.

#include "pch.h" // use pch.h in Visual Studio 2019

#include "Tracker.h"

// TRACKER FUNCTIONS
hseg::tracker::tracker()
{
}

hseg::tracker::~tracker()
{
}

// sets face image and rect to tracking store
void hseg::tracker::setTracker(const hseg::frameContainer& frame, std::unordered_map<int, faceidentifier>* identifiersDict, int identifierId, const cv::RotatedRect& smallFaceRect)
{
	cv::Rect boundingRect = smallFaceRect.boundingRect() & frame.smallRect;
	std::vector<cv::Point2f> vertices{
		cv::Point2f((float)boundingRect.x, (float)(boundingRect.y + boundingRect.height - 1)),
		cv::Point2f((float)boundingRect.x, (float)boundingRect.y),
		cv::Point2f((float)(boundingRect.x + boundingRect.width - 1), (float)boundingRect.y),
		cv::Point2f((float)(boundingRect.x + boundingRect.width - 1), (float)(boundingRect.y + boundingRect.height - 1)) };

	std::vector<cv::Rect> halfRectList(4);
	for (int i = 0; i < 4; i++) {
		int i2 = (i + 1) % 4;
		int i3 = (i + 2) % 4;
		int i4 = (i + 3) % 4;

		cv::Point2f point1 = vertices[i];
		cv::Point2f point2 = vertices[i2];
		cv::Point2f point3 = hseg::utility::getMidPoint(point2, vertices[i3]);
		cv::Point2f point4 = hseg::utility::getMidPoint(vertices[i4], point1);

		cv::RotatedRect halfRect = cv::minAreaRect(std::vector<cv::Point2f>{ point1, point2, point3, point4 });
		cv::Rect halfBoundingRect = boundingRect & halfRect.boundingRect();
		halfBoundingRect = cv::Rect(halfBoundingRect.x - boundingRect.x, halfBoundingRect.y - boundingRect.y, halfBoundingRect.width, halfBoundingRect.height);
		halfRectList[i] = halfBoundingRect;
	}

	// trim queue to capacity
	while ((*identifiersDict)[identifierId].faceStore.size() > (*identifiersDict)[identifierId].capacity - 1) {
		(*identifiersDict)[identifierId].faceStore.pop_front();
	}

	// store identifier tracking information
	cv::Mat smallCrop = frame.smallMonoMat(boundingRect).clone();

	long long combined = combineId((*identifiersDict)[identifierId].id, (*identifiersDict)[identifierId].trackNum);

	matStore.emplace(std::make_pair(combined, smallCrop));

	(*identifiersDict)[identifierId].faceStore.push_back(std::make_pair(combined, halfRectList));

	if ((*identifiersDict)[identifierId].lastTrackedFace.empty()) {
		(*identifiersDict)[identifierId].lastTrackedFace = boundingRect;
	}
	else {
		(*identifiersDict)[identifierId].sizeX.enqueue(boundingRect.width, mediumSmoothing);
		(*identifiersDict)[identifierId].sizeY.enqueue(boundingRect.height, mediumSmoothing);
		float smoothedWidth = (float)(*identifiersDict)[identifierId].sizeX.getSmoothedQueueResult(minSmoothingCount);
		float smoothedHeight = (float)(*identifiersDict)[identifierId].sizeY.getSmoothedQueueResult(minSmoothingCount);

		(*identifiersDict)[identifierId].lastTrackedFace = cv::Rect((int)boundingRect.x, (int)boundingRect.y, (int)smoothedWidth, (int)smoothedHeight);
	}

	// increment track number
	(*identifiersDict)[identifierId].trackNum++;
}

// gets tracking rect from identifier and image
void hseg::tracker::getTracker(const hseg::frameContainer& frame, std::unordered_map<int, faceidentifier>* identifiersDict, int identifierId, cv::Rect2f* smallFaceRect)
{
	*smallFaceRect = cv::Rect2f();
	if ((*identifiersDict)[identifierId].lastTrackedFace.area() != 0) {
		cv::Rect boundingRect = (*identifiersDict)[identifierId].lastTrackedFace;
		cv::Rect searchRect((int)(boundingRect.x - ((searchExpansion / 2.0) * boundingRect.width)), (int)(boundingRect.y - ((searchExpansion / 2.0) * boundingRect.height)), (int)MIN((searchExpansion + 1) * boundingRect.width, frame.smallWidth), (int)MIN((searchExpansion + 1) * boundingRect.height, frame.smallHeight));

		// check for bounding rectangle overlapping the frame edge
		cv::Rect edgeRect = boundingRect & frame.smallRect;
		bool edge = (edgeRect.area() != boundingRect.area());
		float templateMatchCutoffLocal = edge ? templateMatchEdgeCutoff : templateMatchCutoff;

		searchRect = searchRect & frame.smallRect;
		cv::Rect extendedRect((int)(boundingRect.x - ((searchExtended / 2.0) * boundingRect.width)), (int)(boundingRect.y - ((searchExtended / 2.0) * boundingRect.height)), (int)MIN((searchExtended + 1) * boundingRect.width, frame.smallWidth), (int)MIN((searchExtended + 1) * boundingRect.height, frame.smallHeight));
		extendedRect = extendedRect & frame.smallRect;

		int storeSize = (int)(*identifiersDict)[identifierId].faceStore.size();

		std::vector<std::pair<double, cv::Rect2f>> matchResults;
		matchResults.reserve(storeSize);

		// loop backwards to find match
		for (int i = storeSize; i-- > 0; ) {
			long long currentID = (*identifiersDict)[identifierId].faceStore[i].first;
			int currentWidth = matStore[currentID].cols;
			int currentHeight = matStore[currentID].rows;

			if (searchRect.height >= currentHeight && searchRect.width >= currentWidth) {
				cv::Mat result(searchRect.height - currentHeight + 1, searchRect.width - currentWidth + 1, CV_32FC1);
				cv::matchTemplate(frame.smallMonoMat(searchRect), matStore[currentID], result, cv::TM_CCOEFF_NORMED);

				// get location
				double min = 0.0;
				double max = 0.0;
				cv::Point maxLoc;
				cv::minMaxLoc(result, &min, &max, NULL, &maxLoc);

				// clean up
				result.release();

				cv::Rect2f currentTracking = cv::Rect2f(0.0f, 0.0f, (float)frame.smallWidth, (float)frame.smallHeight) & cv::Rect2f((float)(searchRect.x + maxLoc.x), (float)(searchRect.y + maxLoc.y), (float)currentWidth, (float)currentHeight);
				if (max > templateMatchCutoffLocal && !currentTracking.empty()) {
					// high match found, terminate search
					matchResults.push_back(std::make_pair(max, currentTracking));
					break;
				}
				else {
					// rerun search on extended rect
					cv::Mat result(extendedRect.height - currentHeight + 1, extendedRect.width - currentWidth + 1, CV_32FC1);
					cv::matchTemplate(frame.smallMonoMat(extendedRect), matStore[currentID], result, cv::TM_CCOEFF_NORMED);

					// get location
					double min = 0.0;
					double max = 0.0;
					cv::Point maxLoc;
					cv::minMaxLoc(result, &min, &max, NULL, &maxLoc);

					// clean up
					result.release();

					currentTracking = cv::Rect2f(0.0f, 0.0f, (float)frame.smallWidth, (float)frame.smallHeight) & cv::Rect2f((float)(extendedRect.x + maxLoc.x), (float)(extendedRect.y + maxLoc.y), (float)currentWidth, (float)currentHeight);
					if (max > templateMatchCutoffLocal && !currentTracking.empty()) {
						// high match found, terminate search
						matchResults.push_back(std::make_pair(max, currentTracking));
						break;
					}
				}
			}
		}

		if (matchResults.size() == 0) {
			for (int i = storeSize; i-- > 0; ) {
				long long currentID = (*identifiersDict)[identifierId].faceStore[i].first;

				// get store index
				int storeIndex = -1;
				for (int i = (int)(*identifiersDict)[identifierId].faceStore.size(); i-- > 0; ) {
					if ((*identifiersDict)[identifierId].faceStore[i].first == currentID) {
						storeIndex = i;
						break;
					}
				}

				if (storeIndex >= 0) {
					int currentWidth = matStore[currentID].cols;
					int currentHeight = matStore[currentID].rows;

					int storeRectSize = (int)(*identifiersDict)[identifierId].faceStore[storeIndex].second.size();

					std::vector<std::pair<double, cv::Rect2f>> currentMatchResults;
					cv::Rect cropRect(0, 0, currentWidth, currentHeight);
					for (int i = 0; i < storeRectSize; i++) {
						cv::Rect subRect = (*identifiersDict)[identifierId].faceStore[storeIndex].second[i] & cropRect;
						if (searchRect.height >= subRect.height && searchRect.width >= subRect.width) {
							cv::Mat result(searchRect.height - subRect.height + 1, searchRect.width - subRect.width + 1, CV_32FC1);
							cv::matchTemplate(frame.smallMonoMat(searchRect), matStore[currentID](subRect), result, cv::TM_CCOEFF_NORMED);

							// get location
							double min = 0.0;
							double max = 0.0;
							cv::Point maxLoc;
							cv::minMaxLoc(result, &min, &max, NULL, &maxLoc);

							cv::Rect currentTracking = cv::Rect2f(0.0f, 0.0f, (float)frame.smallWidth, (float)frame.smallHeight) & cv::Rect2f((float)(searchRect.x - subRect.x + maxLoc.x), (float)(searchRect.y - subRect.y + maxLoc.y), (float)currentWidth, (float)currentHeight);
							currentMatchResults.push_back(std::make_pair(max, currentTracking));

							result.release();
						}
					}

					// for sub-rects, do not re-search whole image as higher chance of false positive
					if (currentMatchResults.size() > 0) {
						// sort descending according to max value
						std::sort(std::execution::seq, currentMatchResults.begin(), currentMatchResults.end(), [](auto& left, auto& right) {
							return left.first > right.first;
						});

						double currentMax = currentMatchResults[0].first;
						cv::Rect2f currentTracking = currentMatchResults[0].second;
						if (currentMax > templateMatchCutoffLocal) {
							// high match found, terminate search
							matchResults.push_back(std::make_pair(currentMax, currentTracking));
							break;
						}
					}
				}
			}
		}

		if (matchResults.size() > 0) {
			float trackThreshold = (float)(boundingRect.width + boundingRect.height) / 50.0f;
			cv::Rect2f tracking = sortTrackMatches(matchResults, trackThreshold, templateMatchCutoffLocal);
			cv::Point2f trackingCenter = hseg::utility::getCenterPoint(tracking);

			// check for abrupt shift greater than the maximum tracking distance
			cv::Point2f lastTrackedCenter = hseg::utility::getCenterPoint(boundingRect);
			double maxSearchDistance = searchMultiplier * sqrt(((searchRect.width - boundingRect.width) / 2.0) * ((searchRect.width - boundingRect.width) / 2.0) + ((searchRect.height - boundingRect.height) / 2.0) * ((searchRect.height - boundingRect.height) / 2.0));
			double actualDistance = hseg::utility::getdistance(lastTrackedCenter, trackingCenter);
			if (actualDistance <= maxSearchDistance) {
				(*identifiersDict)[identifierId].sizeX.enqueue((double)tracking.width, smallSmoothing);
				(*identifiersDict)[identifierId].sizeY.enqueue((double)tracking.height, smallSmoothing);

				if ((*identifiersDict)[identifierId].sizeX.count() >= minSmoothingCount) {
					float smoothedWidth = (float)(*identifiersDict)[identifierId].sizeX.getSmoothedQueueResult(minSmoothingCount);
					float smoothedHeight = (float)(*identifiersDict)[identifierId].sizeY.getSmoothedQueueResult(minSmoothingCount);
					tracking = cv::Rect2f((float)(trackingCenter.x - ((smoothedWidth - 1.0) / 2.0)), (float)(trackingCenter.y - ((smoothedHeight - 1.0) / 2.0)), smoothedWidth, smoothedHeight);
				}
				(*identifiersDict)[identifierId].lastTrackedFace = cv::Rect((int)tracking.x, (int)tracking.y, (int)tracking.width, (int)tracking.height);

				*smallFaceRect = tracking;
			}
			else {
				*smallFaceRect = (*identifiersDict)[identifierId].getRect2f();
			}

			// crop to frame
			*smallFaceRect = *smallFaceRect & cv::Rect2f(0.0f, 0.0f, (float)frame.smallWidth, (float)frame.smallHeight);
		}
		else {
			*smallFaceRect = (*identifiersDict)[identifierId].getRect2f();
		}
	}
}

// reset variables
void hseg::tracker::reset()
{
	matchedFacesDict.clear();

	std::unordered_map<long long, cv::Mat>::iterator storeIt;
	for (storeIt = matStore.begin(); storeIt != matStore.end(); ++storeIt) {
		storeIt->second.release();
	}
	matStore.clear();
}

// combines two 32 bit int into a 64 bit long long int
long long hseg::tracker::combineId(int identifierID, int trackNum)
{
	long long combine = (long long)identifierID << 32 | trackNum;
	return combine;
}

// sort matches from tracking and get a grouped result
cv::Rect2f hseg::tracker::sortTrackMatches(const std::vector<std::pair<double, cv::Rect2f>>& matchResults, float trackThreshold, float templateMatchCutoffLocal)
{
	// uses partition to group matches
	if (matchResults.size() == 1) {
		return matchResults[0].second;
	}
	else if (matchResults.size() > 1) {
		int resultsSize = (int)matchResults.size();
		std::vector<cv::Point2f> centerVec(resultsSize);
		for (int i = 0; i < resultsSize; i++) {
			centerVec[i] = hseg::utility::getCenterPoint(matchResults[i].second);
		}

		DistanceEquivalence eqPredicate;
		eqPredicate.threshold = trackThreshold;
		std::vector<int> labelsClusterVec;

		int partitionCount = cv::partition(centerVec, labelsClusterVec, eqPredicate);

		std::vector<std::tuple<float, float, float, float, float>> partitionWeightsVec(partitionCount);
		float totalWeights = 0.0;
		for (int i = 0; i < partitionCount; i++) {
			float partitionWeights = 0.0;
			float partitionX = 0.0;
			float partitionY = 0.0;
			float partitionWidth = 0.0;
			float partitionHeight = 0.0;

			for (int j = 0; j < labelsClusterVec.size(); j++) {
				if (labelsClusterVec[j] == i) {
					float weight = (float)matchResults[j].first - templateMatchCutoffLocal;
					partitionWeights += weight;
					totalWeights += weight;
					partitionX += centerVec[j].x * weight;
					partitionY += centerVec[j].y * weight;
					partitionWidth += matchResults[j].second.width * weight;
					partitionHeight += matchResults[j].second.height * weight;
				}
			}
			partitionWeightsVec[i] = std::make_tuple(partitionWeights, partitionX / partitionWeights, partitionY / partitionWeights, partitionWidth / partitionWeights, partitionHeight / partitionWeights);
		}

		// sort descending according to partition weight
		std::sort(std::execution::seq, partitionWeightsVec.begin(), partitionWeightsVec.end(), [](auto& left, auto& right) {
			return std::get<0>(left) > std::get<0>(right);
		});

		std::tuple<float, float, float, float, float> topPartition = partitionWeightsVec[0];
		return cv::Rect2f((float)(std::get<1>(topPartition) - ((std::get<3>(topPartition) - 1.0) / 2.0)), (float)(std::get<2>(topPartition) - ((std::get<4>(topPartition) - 1.0) / 2.0)), (float)std::get<3>(topPartition), (float)std::get<4>(topPartition));
	}
	else {
		return cv::Rect2f();
	}
}
