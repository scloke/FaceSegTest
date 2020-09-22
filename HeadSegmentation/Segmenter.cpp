// Segmenter.cpp : Defines the face segmenter class for the DLL.

#include "pch.h" // use pch.h in Visual Studio 2019

#include "Segmenter.h"

// SEGMENTER FUNCTIONS
hseg::segmenter::segmenter(unsigned concurrentthreads)
{
	hseg::segmenter::concurrentthreads = concurrentthreads;
	scan = new dbScan();
}

hseg::segmenter::segmenter()
{
	hseg::segmenter::concurrentthreads = 0;
	scan = new dbScan();
}

hseg::segmenter::~segmenter()
{
	delete scan;
	scan = NULL;
}

void hseg::segmenter::segment(const frameContainer& frame, std::unordered_map<int, faceidentifier>* identifiersDict, int identifierId, const cv::Rect2f& tinyRect, cv::Mat* returnmask, float smoothedNoseY)
{
	if (tinyRect.area() > 0) {
		cv::RotatedRect tinyFaceRect(tinyRect.tl(), cv::Point2f(tinyRect.x + tinyRect.width, tinyRect.y), cv::Point2f(tinyRect.x + tinyRect.width, tinyRect.y + tinyRect.height));

		cv::Point2f points[4];
		tinyFaceRect.points(points);
		cv::Rect tinyBoundingRect = cv::boundingRect(std::vector<cv::Point2f>{ points[0], points[1], points[2], points[3] });
		float dispY = getNoseDisp((float)(tinyBoundingRect.height), smoothedNoseY);

		cv::Point2f boundingCenter((float)(tinyBoundingRect.width - 1) / 2.0f, ((float)(tinyBoundingRect.height - 1) / 2.0f) + dispY);
		cv::Rect boundingRect = cv::Rect((int)((float)tinyBoundingRect.x - ((float)tinyBoundingRect.width * (outlineHorzExpansion - 1.0f) / 2.0f)), (int)(((float)tinyBoundingRect.y - ((float)tinyBoundingRect.height * (outlineVertExpansion - 1.0f))) + dispY), (int)((float)tinyBoundingRect.width * outlineHorzExpansion), (int)((float)tinyBoundingRect.height * (1.0f + ((outlineVertExpansion - 1.0f) * 1.5f))));
		boundingRect = boundingRect & frame.tinyRect;

		cv::Rect lowerBodyRect(tinyBoundingRect.x - tinyBoundingRect.width, (int)(tinyBoundingRect.y + tinyBoundingRect.height - dispY - (tinyBoundingRect.height * 0.25f)), (int)(tinyBoundingRect.width * 3.0f), (int)(frame.tinySize.height - (tinyBoundingRect.y + tinyBoundingRect.height - dispY - (tinyBoundingRect.height * 0.25f))));
		lowerBodyRect = lowerBodyRect & frame.tinyRect;

		cv::Rect combinedRect = boundingRect | lowerBodyRect;
		cv::Rect cropBoundingRect(boundingRect.x - combinedRect.x, boundingRect.y - combinedRect.y, boundingRect.width, boundingRect.height);
		cv::Rect cropLowerBodyRect(lowerBodyRect.x - combinedRect.x, lowerBodyRect.y - combinedRect.y, lowerBodyRect.width, lowerBodyRect.height);

		// get adjusted threshold
		cv::Scalar mean, stdDev;
		cv::meanStdDev(frame.tinyLABMat(combinedRect), mean, stdDev);
		float adjThreshold = (float)(stdDev[0] * calcThreshold / threshAdjFactor);

		// run dbScan
		cv::Mat mask = cv::Mat::zeros(combinedRect.size(), CV_8UC1);
		if (!cropBoundingRect.empty()) {
			mask(cropBoundingRect).setTo(255);
		}
		if (!cropLowerBodyRect.empty()) {
			mask(cropLowerBodyRect).setTo(255);
		}
		cv::Mat labels;
		scan->scan(frame.tinyLABMat(combinedRect), mask, &labels, adjThreshold);

		mask.setTo(0);
		cv::Mat excludeMask(combinedRect.size(), CV_8UC1);
		excludeMask.setTo(255);
		if (!cropBoundingRect.empty()) {
			std::vector<cv::Point> outlineVec = getFaceOutline(tinyFaceRect, boundingRect, dispY);
			std::vector<cv::Mat> clusterMaskVec = processMask(frame.tinyLABMat(boundingRect), &labels(cropBoundingRect), outlineVec, includeFractionHead);
			if (!clusterMaskVec[0].empty()) {
				cv::bitwise_or(mask(cropBoundingRect), clusterMaskVec[0], mask(cropBoundingRect));
				clusterMaskVec[0].release();
			}
			if (!clusterMaskVec[1].empty()) {
				cv::bitwise_and(excludeMask(cropBoundingRect), clusterMaskVec[1], excludeMask(cropBoundingRect));
				clusterMaskVec[1].release();
			}
		}
		if (!cropLowerBodyRect.empty()) {
			std::vector<cv::Point> outlineVec = getBodyOutline(tinyBoundingRect, frame.tinySize, lowerBodyRect, dispY);
			std::vector<cv::Mat> clusterMaskVec = processMask(frame.tinyLABMat(lowerBodyRect), &labels(cropLowerBodyRect), outlineVec, includeFractionLowerBody);
			if (!clusterMaskVec[0].empty()) {
				cv::bitwise_or(mask(cropLowerBodyRect), clusterMaskVec[0], mask(cropLowerBodyRect));
				clusterMaskVec[0].release();
			}
			if (!clusterMaskVec[1].empty()) {
				cv::bitwise_and(excludeMask(cropLowerBodyRect), clusterMaskVec[1], excludeMask(cropLowerBodyRect));
				clusterMaskVec[1].release();
			}
		}

		cv::Mat frameMask(frame.tinySize, CV_8UC1);

		cv::Rect expandedCombinedRect(combinedRect.x - maskTransform, combinedRect.y - maskTransform, combinedRect.width + (2 * maskTransform), combinedRect.height + (2 * maskTransform));
		expandedCombinedRect = expandedCombinedRect & frame.tinyRect;
		cv::Rect expandedCropMask(combinedRect.x - expandedCombinedRect.x, combinedRect.y - expandedCombinedRect.y, combinedRect.width, combinedRect.height);

		cv::Mat compareMask(frame.tinySize, CV_8UC1);
		cv::Mat morphMask = cv::Mat::zeros(expandedCombinedRect.size(), CV_8UC1);
		mask.copyTo(morphMask(expandedCropMask));
		cv::dilate(morphMask, morphMask, cv::Mat(), cv::Point(-1, -1), maskTransform, cv::BORDER_CONSTANT, cv::Scalar(0));
		frameMask.setTo(1);
		frameMask(expandedCombinedRect).setTo(0, morphMask);

		// erode mask and set to foreground
		morphMask.setTo(0);
		mask.copyTo(morphMask(expandedCropMask));
		cv::erode(morphMask, morphMask, cv::Mat(), cv::Point(-1, -1), maskTransform, cv::BORDER_CONSTANT, cv::Scalar(0));
		frameMask(expandedCombinedRect).setTo(2, morphMask);

		// erode exclude mask and set to background
		morphMask.setTo(255);
		excludeMask.copyTo(morphMask(expandedCropMask));
		cv::erode(morphMask, morphMask, cv::Mat(), cv::Point(-1, -1), maskTransform, cv::BORDER_CONSTANT, cv::Scalar(255));
		frameMask(expandedCombinedRect).setTo(1, morphMask);

		// output mask
		cv::resize(frameMask, *returnmask, frame.size, 0.0, 0.0, cv::INTER_NEAREST);

		// set markers for watershed
		cv::Mat markers;
		(*returnmask).convertTo(markers, CV_32S);

		utility::watershedEx(frame.gammaMat, markers);

		// convert to background, border, and mask pixels
		cv::compare(markers, 2, compareMask, cv::CMP_EQ);

		returnmask->setTo(0);
		returnmask->setTo(255, compareMask);

		// clean up
		markers.release();
		morphMask.release();
		compareMask.release();
		frameMask.release();
		excludeMask.release();
		labels.release();
		mask.release();
	}
}

// get face outline contour
std::vector<cv::Point> hseg::segmenter::getFaceOutline(const cv::RotatedRect& rect, const cv::Rect& boundingRect, float dispY)
{
	cv::RotatedRect mainRect = cv::RotatedRect(rect);
	mainRect.center = cv::Point2f(mainRect.center.x - boundingRect.x, mainRect.center.y - boundingRect.y);

	cv::RotatedRect extendedRect = extendRect(mainRect, maskExtendUpper, maskExtendLower, maskExtendSide);
	cv::RotatedRect sideextendedRect = extendRect(mainRect, 1.0f, 1.0f, maskExtendSide);

	cv::Point2f points[4];
	mainRect.points(points);
	cv::Point2f extendedPoints[4];
	extendedRect.points(extendedPoints);
	cv::Point2f sideextendedPoints[4];
	sideextendedRect.points(sideextendedPoints);
	cv::Point2f pointL = utility::getMidPoint(extendedPoints[0], extendedPoints[3]);
	cv::Point2f point0 = utility::getMidPoint(pointL, extendedPoints[0]);
	cv::Point2f point7 = utility::getMidPoint(pointL, extendedPoints[3]);
	cv::Point2f point1 = utility::getInterPoint(sideextendedPoints[0], sideextendedPoints[1], 0.7f);
	cv::Point2f point6 = utility::getInterPoint(sideextendedPoints[3], sideextendedPoints[2], 0.7f);
	cv::Point2f point2 = utility::getInterPoint(sideextendedPoints[1], extendedPoints[1], 0.5f);
	cv::Point2f point5 = utility::getInterPoint(sideextendedPoints[2], extendedPoints[2], 0.5f);
	cv::Point2f pointU = utility::getMidPoint(extendedPoints[1], extendedPoints[2]);
	cv::Point2f point3 = utility::getMidPoint(pointU, extendedPoints[1]);
	cv::Point2f point4 = utility::getMidPoint(pointU, extendedPoints[2]);
	std::vector<cv::Point> outlineVec{ 
		cv::Point((int)(point0.x), (int)(point0.y + dispY)), 
		cv::Point((int)(point1.x), (int)(point1.y + dispY)), 
		cv::Point((int)(point2.x), (int)(point2.y + dispY)), 
		cv::Point((int)(point3.x), (int)(point3.y + dispY)), 
		cv::Point((int)(point4.x), (int)(point4.y + dispY)), 
		cv::Point((int)(point5.x), (int)(point5.y + dispY)), 
		cv::Point((int)(point6.x), (int)(point6.y + dispY)), 
		cv::Point((int)(point7.x), (int)(point7.y + dispY)) 
	};

	return outlineVec;
}

std::vector<cv::Point> hseg::segmenter::getBodyOutline(const cv::Rect& tinyBoundingRect, const cv::Size& tinySize, const cv::Rect& lowerBodyRect, float dispY)
{
	cv::Point point0((int)(tinyBoundingRect.x - lowerBodyRect.x), (int)(tinyBoundingRect.y - lowerBodyRect.y + (tinyBoundingRect.height * 0.75f)));
	cv::Point point7((int)(tinyBoundingRect.x - lowerBodyRect.x + tinyBoundingRect.width - 1), (int)(tinyBoundingRect.y - lowerBodyRect.y + (tinyBoundingRect.height * 0.75f)));
	cv::Point point1((int)(tinyBoundingRect.x - lowerBodyRect.x - (tinyBoundingRect.width * 0.5f)), (int)(tinyBoundingRect.y - lowerBodyRect.y + tinyBoundingRect.height));
	cv::Point point6((int)(tinyBoundingRect.x - lowerBodyRect.x + tinyBoundingRect.width - 1 + (tinyBoundingRect.width * 0.5f)), (int)(tinyBoundingRect.y - lowerBodyRect.y + tinyBoundingRect.height));
	cv::Point point2((int)(tinyBoundingRect.x - lowerBodyRect.x - tinyBoundingRect.width), (int)(tinyBoundingRect.y - lowerBodyRect.y + tinyBoundingRect.height + (tinyBoundingRect.width * 0.75f)));
	cv::Point point5((int)(tinyBoundingRect.x - lowerBodyRect.x + tinyBoundingRect.width - 1 + tinyBoundingRect.width), (int)(tinyBoundingRect.y - lowerBodyRect.y + tinyBoundingRect.height + (tinyBoundingRect.width * 0.75f)));
	cv::Point point3((int)(tinyBoundingRect.x - lowerBodyRect.x - tinyBoundingRect.width), (int)(tinySize.height - lowerBodyRect.y - 1.0f));
	cv::Point point4((int)(tinyBoundingRect.x - lowerBodyRect.x + tinyBoundingRect.width - 1 + tinyBoundingRect.width), (int)(tinySize.height - lowerBodyRect.y - 1.0f));

	std::vector<cv::Point> outlineVec{ 
		cv::Point(point0.x, (int)(point0.y + dispY)), 
		cv::Point(point1.x, (int)(point1.y + dispY)), 
		cv::Point(point2.x, (int)(point2.y + dispY)), 
		cv::Point(point3.x, (int)(point3.y + dispY)), 
		cv::Point(point4.x, (int)(point4.y + dispY)), 
		cv::Point(point5.x, (int)(point5.y + dispY)), 
		cv::Point(point6.x, (int)(point6.y + dispY)), 
		cv::Point(point7.x, (int)(point7.y + dispY))
	};

	return outlineVec;
}

// get the mask from the labels and face rotated rect
std::vector<cv::Mat> hseg::segmenter::processMask(const cv::Mat& mat, cv::Mat* labels, const std::vector<cv::Point>& outlineVec, float includeFraction)
{
	cv::Mat mask = cv::Mat::zeros(labels->size(), CV_8UC1);
	int labelsCount = labels->rows * labels->cols;

	int* labelsBuffer = static_cast<int*>(malloc(labelsCount * sizeof(int)));
	if (labelsBuffer != NULL) {
		// copy labels to buffer
		if (labels->isContinuous()) {
			memcpy(labelsBuffer, labels->data, labelsCount * sizeof(int));
		}
		else {
			cv::Mat tempLabels = labels->clone();
			memcpy(labelsBuffer, tempLabels.data, labelsCount * sizeof(int));
			tempLabels.release();
		}
	}

	cv::fillConvexPoly(mask, outlineVec, 255);

	std::vector<std::tuple<int, int, int>> innerCount = getLabelCount(labelsBuffer, labels->size(), mask);

	// get labels and means of clusters within the inner ellipse
	std::vector<int> dominantLabels;
	std::vector<int> externalLabels;
	dominantLabels.reserve(innerCount.size());
	float excludeFraction = 1.0f - includeFraction;
	for (int i = 0; i < innerCount.size(); i++) {
		int key = std::get<0>(innerCount[i]);
		float totalPixelCount = (float)std::get<1>(innerCount[i]);
		float innerPixelCount = (float)std::get<2>(innerCount[i]);
		float outerPixelCount = totalPixelCount - innerPixelCount;
		if (innerPixelCount / totalPixelCount >= includeFraction) {
			dominantLabels.push_back(key);
		}
		else if (outerPixelCount / totalPixelCount >= excludeFraction) {
			externalLabels.push_back(key);
		}
	}

	std::vector<std::vector<int>> clusterLabelsVec{ dominantLabels, externalLabels };
	std::vector<cv::Mat> clusterMaskVec(2);
	getDominantMask(labelsBuffer, labels->size(), clusterLabelsVec, &clusterMaskVec);

	// clean up
	free(labelsBuffer);
	mask.release();

	return clusterMaskVec;
}

// extends the rotated rect vertically
cv::RotatedRect hseg::segmenter::extendRect(const cv::RotatedRect& rect, float upperExpansion, float lowerExpansion, float sideExpansion)
{
	cv::Point2f points[4];
	rect.points(points);

	std::vector<cv::Point2f> topextendedPoints(4), sideextendedPoints(4);
	topextendedPoints[0] = cv::Point2f(points[1].x - (points[1].x - points[0].x) * lowerExpansion, points[1].y - (points[1].y - points[0].y) * lowerExpansion);
	topextendedPoints[1] = cv::Point2f(points[0].x + (points[1].x - points[0].x) * upperExpansion, points[0].y + (points[1].y - points[0].y) * upperExpansion);
	topextendedPoints[2] = cv::Point2f(points[3].x + (points[2].x - points[3].x) * upperExpansion, points[3].y + (points[2].y - points[3].y) * upperExpansion);
	topextendedPoints[3] = cv::Point2f(points[2].x - (points[2].x - points[3].x) * lowerExpansion, points[2].y - (points[2].y - points[3].y) * lowerExpansion);

	sideextendedPoints[0] = cv::Point2f(topextendedPoints[3].x - (topextendedPoints[3].x - topextendedPoints[0].x) * sideExpansion, topextendedPoints[3].y - (topextendedPoints[3].y - topextendedPoints[0].y) * sideExpansion);
	sideextendedPoints[1] = cv::Point2f(topextendedPoints[2].x - (topextendedPoints[2].x - topextendedPoints[1].x) * sideExpansion, topextendedPoints[2].y - (topextendedPoints[2].y - topextendedPoints[1].y) * sideExpansion);
	sideextendedPoints[2] = cv::Point2f(topextendedPoints[1].x + (topextendedPoints[2].x - topextendedPoints[1].x) * sideExpansion, topextendedPoints[1].y + (topextendedPoints[2].y - topextendedPoints[1].y) * sideExpansion);
	sideextendedPoints[3] = cv::Point2f(topextendedPoints[0].x + (topextendedPoints[3].x - topextendedPoints[0].x) * sideExpansion, topextendedPoints[0].y + (topextendedPoints[3].y - topextendedPoints[0].y) * sideExpansion);

	cv::RotatedRect extendedRect = cv::minAreaRect(sideextendedPoints);
	extendedRect = utility::resetRotatedRect(extendedRect, rect.angle);
	return extendedRect;
}

// gets the count for all labels
std::vector<std::tuple<int, int, int>> hseg::segmenter::getLabelCount(int* labelsBuffer, const cv::Size& labelsSize, const cv::Mat& mask)
{
	assert(mask.empty() || (mask.cols == labelsSize.width && mask.rows == labelsSize.height));

	int labelsCount = labelsSize.height * labelsSize.width;

	int max = *std::max_element(labelsBuffer, labelsBuffer + labelsCount);
	int labelsize = max + 1;

	std::vector<std::tuple<int, int, int>> returnCount;
	returnCount.reserve(labelsize);

	BYTE* maskBuffer = mask.empty() ? static_cast<BYTE*>(malloc(1)) : static_cast<BYTE*>(malloc(labelsCount));
	std::atomic<int>* cBuffer = new std::atomic<int>[labelsize];
	std::atomic<int>* cmBuffer = mask.empty() ? new std::atomic<int>[1] : new std::atomic<int>[labelsize];

	// initialize sum buffers
	for (int i = 0; i < labelsize; i++) {
		cBuffer[i].store(0);
		if (!mask.empty()) {
			cmBuffer[i].store(0);
		}
	}

	if (cBuffer != NULL && cmBuffer != NULL) {
		// copy mask to buffer if not empty
		if (!mask.empty()) {
			memcpy(maskBuffer, mask.data, labelsCount);
		}

		std::vector<int> indexVec(labelsCount);
		std::iota(indexVec.begin(), indexVec.end(), 0);

		if (mask.empty()) {
			std::for_each(std::execution::par_unseq, indexVec.begin(), indexVec.end(), [&labelsBuffer, &cBuffer](int& v) {
				int label = labelsBuffer[v];
				if (label >= 0) {
					cBuffer[label].fetch_add(1);
				}
			});
		}
		else {
			std::for_each(std::execution::par_unseq, indexVec.begin(), indexVec.end(), [&labelsBuffer, &maskBuffer, &cBuffer, &cmBuffer](int& v) {
				int label = labelsBuffer[v];
				if (label >= 0) {
					cBuffer[label].fetch_add(1);
					if (maskBuffer[v] > 0) {
						cmBuffer[label].fetch_add(1);
					}
				}
			});
		}

		for (int i = 0; i < labelsize; i++) {
			int count = cBuffer[i].load();
			int mcount = mask.empty() ? 0 : cmBuffer[i].load();
			if (count > 0) {
				returnCount.push_back(std::make_tuple(i, count, mcount));
			}
		}
	}

	// clean up
	free(maskBuffer);
	delete[] cBuffer;
	delete[] cmBuffer;

	return returnCount;
}

// gets the mask of all dominant labels
void hseg::segmenter::getDominantMask(int* labelsBuffer, const cv::Size& labelsSize, const std::vector<std::vector<int>>& dominantLabelsVec, std::vector<cv::Mat>* maskVec)
{
	if (dominantLabelsVec.size() > 0 && dominantLabelsVec.size() == maskVec->size()) {
		int labelsCount = labelsSize.height * labelsSize.width;

		std::vector<int> dominantCountVec(dominantLabelsVec.size());
		std::vector<int*> dominantBufferVec(dominantLabelsVec.size());
		std::vector<BYTE*> maskBufferVec(dominantLabelsVec.size());
		for (int i = 0; i < dominantLabelsVec.size(); i++) {
			int dominantSize = (int)dominantLabelsVec[i].size();
			dominantCountVec[i] = dominantSize;
			dominantBufferVec[i] = dominantSize > 0 ? static_cast<int*>(malloc(dominantSize * sizeof(int))) : NULL;
			memcpy(dominantBufferVec[i], dominantLabelsVec[i].data(), dominantSize * sizeof(int));

			maskBufferVec[i] = dominantSize > 0 ? static_cast<BYTE*>(calloc(labelsCount, sizeof(int))) : NULL;
		}

		std::vector<int> indexVec(labelsCount);
		std::iota(indexVec.begin(), indexVec.end(), 0);
		int dominantCountVecSize = (int)dominantCountVec.size();

		std::for_each(std::execution::par_unseq, indexVec.begin(), indexVec.end(), [&dominantCountVec, &dominantCountVecSize, &labelsBuffer, &dominantBufferVec, &maskBufferVec](int& v) {
			for (int i = 0; i < dominantCountVecSize; i++) {
				for (int j = 0; j < dominantCountVec[i]; j++) {
					if (labelsBuffer[v] == dominantBufferVec[i][j]) {
						maskBufferVec[i][v] = 255;
						break;
					}
				}
			}
		});

		for (int i = 0; i < dominantLabelsVec.size(); i++) {
			if (dominantBufferVec[i] != NULL) {
				free(dominantBufferVec[i]);
				dominantBufferVec[i] = NULL;
			}
			if (maskBufferVec[i] != NULL) {
				if ((*maskVec)[i].empty()) {
					(*maskVec)[i] = cv::Mat(labelsSize, CV_8UC1);
				}
				memcpy((*maskVec)[i].data, maskBufferVec[i], labelsCount);

				free(maskBufferVec[i]);
				maskBufferVec[i] = NULL;
			}
			else {
				// return empty mask
				if (!(*maskVec)[i].empty()) {
					(*maskVec)[i].release();
					(*maskVec)[i] = cv::Mat();
				}
			}
		}
	}
}

float hseg::segmenter::getNoseDisp(float refHeight, float smoothedNoseY)
{
	return (noseCenterY - smoothedNoseY) * refHeight * noseDispMultipler;
}
