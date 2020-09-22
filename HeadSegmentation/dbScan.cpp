// dbScan.cpp : Defines the dbScan class for the DLL.

#include "pch.h" // use pch.h in Visual Studio 2019

#include "dbScan.h"

// MEDIANSCAN FUNCTIONS
hseg::dbScan::dbScan()
{
}

hseg::dbScan::~dbScan()
{
}

int hseg::dbScan::scan(const cv::Mat& mat, const cv::Mat& mask, cv::Mat* labelsMat, float tolerance)
{
	int imageWidth = mat.cols;
	int imageHeight = mat.rows;
	int clusters = 0;

	float adjTolerance = tolerance * tolerance;
	indexSize = imageHeight * imageWidth;

	// set labels to unclassified
	if (labelsMat->empty()) {
		*labelsMat = cv::Mat(imageHeight, imageWidth, CV_32SC1);
	}
	else if (labelsMat->rows != imageHeight || labelsMat->cols != imageWidth || labelsMat->type() != CV_32SC1) {
		labelsMat->release();
		*labelsMat = cv::Mat(imageHeight, imageWidth, CV_32SC1);
	}

	if (mask.empty()) {
		labelsMat->setTo(UNCLASSIFIED);
	}
	else {
		labelsMat->setTo(NONE);
		labelsMat->setTo(UNCLASSIFIED, mask);
	}

	// create buffers and initialise
	int* labelsBuffer = static_cast<int*>(malloc(indexSize * sizeof(int)));
	int* clusterBuffer = static_cast<int*>(malloc(indexSize * sizeof(int)));
	cv::Vec3b* labBuffer = static_cast<cv::Vec3b*>(malloc(indexSize * sizeof(cv::Vec3b)));
	int neighbourLocBuffer[neighbourCount];
	int* offsetBuffer = static_cast<int*>(malloc(indexSize * sizeof(int)));
	for (int i = 0; i < neighbourCount; i++) {
		neighbourLocBuffer[i] = (neighbourLoc[i].y * imageWidth) + neighbourLoc[i].x;
	}
	int clusterIndex = 0, locationIndex = 0, clusterID = 1;

	if (labelsBuffer != NULL && clusterBuffer != NULL && labBuffer != NULL && offsetBuffer != NULL) {
		// copy labels to buffer
		memcpy(labelsBuffer, labelsMat->data, indexSize * sizeof(int));

		// copy mat to buffer
		if (mat.isContinuous()) {
			memcpy(labBuffer, mat.data, indexSize * sizeof(cv::Vec3b));
		}
		else {
			cv::Mat labMatCont = mat.clone();
			memcpy(labBuffer, labMatCont.data, indexSize * sizeof(cv::Vec3b));
			labMatCont.release();
		}

		cv::Size boundsSize(imageWidth, imageHeight);
		for (int y = 0; y < imageHeight; y++) {
			for (int x = 0; x < imageWidth; x++) {
				expandCluster(labBuffer, labelsBuffer, neighbourLocBuffer, clusterBuffer, offsetBuffer, boundsSize, cv::Point(x, y), adjTolerance, &clusterIndex, &locationIndex, &clusterID);
			}
		}

		memcpy(labelsMat->data, labelsBuffer, indexSize * sizeof(int));
	}

	// run watershed
	utility::watershedEx(mat, *labelsMat);

	(*labelsMat) -= 1;

	// clean up
	if (labelsBuffer != NULL) {
		free(labelsBuffer);
	}
	if (clusterBuffer != NULL) {
		free(clusterBuffer);
	}
	if (labBuffer != NULL) {
		free(labBuffer);
	}
	if (offsetBuffer != NULL) {
		free(offsetBuffer);
	}

	return clusterID - 1;
}

// expand clusters from a point
void hseg::dbScan::expandCluster(cv::Vec3b* labBuffer, int* labelsBuffer, int* neighbourLocBuffer, int* clusterBuffer, int* offsetBuffer, const cv::Size& boundsSize, const cv::Point& point, float adjTolerance, int* clusterIndex, int* locationIndex, int* clusterID)
{
	int pointIndex = (point.y * boundsSize.width) + point.x;
	if (labelsBuffer[pointIndex] == UNCLASSIFIED) {
		int offsetStart = 0;
		int offsetEnd = 0;
		calculateCluster(labBuffer, labelsBuffer, neighbourLocBuffer, offsetBuffer, &offsetEnd, pointIndex, adjTolerance, *clusterID);

		if (offsetStart == offsetEnd) {
			labelsBuffer[pointIndex] = UNKNOWN;
		}
		else {
			// set cluster id and get core point index
			labelsBuffer[pointIndex] = *clusterID;

			while (offsetStart < offsetEnd) {
				int intoffset2 = *(offsetBuffer + offsetStart);
				offsetStart++;
				calculateCluster(labBuffer, labelsBuffer, neighbourLocBuffer, offsetBuffer, &offsetEnd, intoffset2, adjTolerance, *clusterID);
			}

			// add origin point
			offsetBuffer[offsetEnd] = pointIndex;
			offsetEnd++;

			// store to buffer
			clusterBuffer[*clusterIndex] = *clusterID;
			clusterBuffer[(*clusterIndex) + 1] = offsetEnd;
			(*clusterIndex) += 2;
			(*clusterID)++;
		}
	}
}

void hseg::dbScan::calculateCluster(cv::Vec3b* labBuffer, int* labelsBuffer, int* neighbourLocBuffer, int* offsetBuffer, int* offsetEnd, int pointIndex, float adjTolerance, int currentClusterID)
{
	for (int i = 0; i < neighbourCount; i++) {
		int intoffset2 = pointIndex + neighbourLocBuffer[i];
		if (intoffset2 >= 0 && intoffset2 < indexSize && labelsBuffer[intoffset2] == UNCLASSIFIED) {
			int diff1 = (int)labBuffer[pointIndex][0] - (int)labBuffer[intoffset2][0];
			int diff2 = (int)labBuffer[pointIndex][1] - (int)labBuffer[intoffset2][1];
			int diff3 = (int)labBuffer[pointIndex][2] - (int)labBuffer[intoffset2][2];

			if ((diff1 * diff1) + (diff2 * diff2) + (diff3 * diff3) <= adjTolerance) {
				labelsBuffer[intoffset2] = currentClusterID;
				offsetBuffer[*offsetEnd] = intoffset2;
				(*offsetEnd)++;
			}
		}
	}
}
