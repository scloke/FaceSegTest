#pragma once

#include "Global.h"
#include "Utility.h"

#ifndef FACEDETECT_H
#define FACEDETECT_H

namespace hseg
{
	class facedetect
	{
#define CALIB_PI 3.14159265358979323846
#define CALIB_PI_2 1.57079632679489661923
#define CALIB_DEGREES 0
#define CALIB_RADIANS 1

	public:
		facedetect();
		~facedetect();

		void FD_multiview_reinforce(BYTE* matstruct_in, BYTE* buffer_in, BYTE* faceresults, int* facecount);

	private:
		float scale = 1.2f;											//scale factor for scan windows
		static const int landmarkCount = 68;						// number of landmarks
		static const int min_neighbors = 2;							//how many neighbors each candidate rectangle should have to retain it
		static const int min_object_width = 48;						//Minimum possible face size. Faces smaller than that are ignored.
		static const int max_object_width = 0;						//Maximum possible face size. Faces larger than that are ignored. It is the largest posible when max_object_width=0.
		static const int midlineTop = 27;							// point index for determining the midline angle
		static const int midlineBottom = 8;							// point index for determining the midline angle
		static const int upperFaceLeft = 0;							// point index for dividing the upper face
		static const int upperFaceRight = 16;						// point index for dividing the upper face
		static const int smallWidth = 640;							// scale width for face detect

		std::vector<int> leftEyePoints, rightEyePoints, leftEarPoints, rightEarPoints, posePoints;
		typedef int* facedetect_multiview_reinforce(int* pbybuffer,
			int* pbyimage, int width, int height, int step,			//input image, it must be gray (single-channel) image!
			float scale,
			int min_neighbors,
			int min_object_width,
			int max_object_width,
			int doLandmark);										// landmark detection

		std::vector<short> buffer;									//buffer memory for storing face detection results, !!its size must be 0x20000 Bytes!!
		facedetect_multiview_reinforce* FDMR = NULL;
		HINSTANCE hFaceDetectDLL = NULL;
		cv::Ptr<cv::CLAHE> clahe;
		std::vector<std::tuple<cv::RotatedRect, std::vector<cv::Point2f>>> outResults;

		std::vector<std::vector<cv::Point2f>> FD_multiview_reinforce(cv::Mat mat);
		std::vector<std::tuple<cv::RotatedRect, std::vector<cv::Point2f>>> getFaceResults(std::vector<std::vector<cv::Point2f>> detectResults);
		void setFaceResults(BYTE* faceresults, const std::vector<std::tuple<cv::RotatedRect, std::vector<cv::Point2f>>>& outResults);
	};
}

#endif
