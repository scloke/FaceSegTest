#pragma once

#include "Global.h"
#include "Utility.h"
#include <execution>

#ifndef MEDIANSCAN_H
#define MEDIANSCAN_H

namespace hseg
{
	class dbScan
	{
#define UNKNOWN 0
#define UNCLASSIFIED -1
#define NONE -3

	public:
		dbScan();
		~dbScan();

		int scan(const cv::Mat& mat, const cv::Mat& mask, cv::Mat* labelsMat, float tolerance);

	private:
		static const int neighbourCount = 8;							// number of pixel neighbours
		int indexSize = 0;												// size of label mat vector
		std::vector<cv::Point> neighbourLoc{ cv::Point(-1, -1), cv::Point(0, -1), cv::Point(1, -1), cv::Point(-1, 0), cv::Point(1, 0), cv::Point(-1, 1), cv::Point(0, 1), cv::Point(1, 1) };

		void expandCluster(cv::Vec3b* labBuffer, int* labelsBuffer, int* neighbourLocBuffer, int* clusterBuffer, int* offsetBuffer, const cv::Size& boundsSize, const cv::Point& point, float adjTolerance, int* clusterIndex, int* locationIndex, int* clusterID);
		void calculateCluster(cv::Vec3b* labBuffer, int* labelsBuffer, int* neighbourLocBuffer, int* offsetBuffer, int* offsetEnd, int pointIndex, float adjTolerance, int currentClusterID);
	};
}

#endif
