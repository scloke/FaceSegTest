// FaceDetect.cpp : Defines the FaceDetect class for the DLL.

#include "pch.h" // use pch.h in Visual Studio 2019

#include "FaceDetect.h"

// LIBFACEDETECTOR FUNCTIONS
hseg::facedetect::facedetect()
{
	if (leftEyePoints.empty()) {
		leftEyePoints = std::vector<int>({ 36, 37, 38, 39, 40, 41 });
	}
	if (rightEyePoints.empty()) {
		rightEyePoints = std::vector<int>({ 42, 43, 44, 45, 46, 47 });
	}
	if (leftEarPoints.empty()) {
		leftEarPoints = std::vector<int>({ 1, 2 });
	}
	if (rightEarPoints.empty()) {
		rightEarPoints = std::vector<int>({ 14, 15 });
	}
	if (posePoints.empty()) {
		posePoints = std::vector<int>({ 33, 48, 54 }); // order of points is: nose tip, left mouth outer angle, right mouth outer angle
	}

	hFaceDetectDLL = LoadLibrary(L"libfacedetect-x64.dll");
	if (hFaceDetectDLL)
	{
		FDMR = (facedetect_multiview_reinforce*)GetProcAddress(hFaceDetectDLL, MAKEINTRESOURCEA(4));
		buffer = std::vector<short>(0x20000 / sizeof(short));
	}

	// use CLAHE to improve contrast for face detection
	clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
}

hseg::facedetect::~facedetect()
{
	if (hFaceDetectDLL) {
		FreeLibrary(hFaceDetectDLL);
	}

	hFaceDetectDLL = NULL;
	FDMR = NULL;
	buffer.clear();
	clahe.release();
}

void hseg::facedetect::FD_multiview_reinforce(BYTE* matstruct_in, BYTE* buffer_in, BYTE* faceresults, int* facecount)
{
	if (faceresults == NULL) {
		cv::Mat mat = hseg::utility::getMat(matstruct_in, buffer_in);
		std::vector<std::vector<cv::Point2f>> detectResults = FD_multiview_reinforce(mat);
		outResults = getFaceResults(detectResults);
		mat.release();

		*facecount = (int)outResults.size();
	}
	else {
		setFaceResults(faceresults, outResults);
	}
}

std::vector<std::vector<cv::Point2f>> hseg::facedetect::FD_multiview_reinforce(cv::Mat mat)
{
	int width = mat.cols;
	int height = mat.rows;
	int channels = mat.channels();
	int step = (int)mat.step;
	int facecount = 0;
	float smallscale = 1.0f;

	assert(channels == 1 || channels == 3);

	if (width > smallWidth) {
		smallscale = (float)smallWidth / (float)width;
		int smallHeight = (int)((float)height * smallscale);

		if (channels == 1) {
			cv::Mat smallMat;
			cv::resize(mat, smallMat, cv::Size(smallWidth, smallHeight), 0.0, 0.0, cv::INTER_LINEAR);
			clahe->apply(smallMat, smallMat);

			facecount = *FDMR((int*)buffer.data(), (int*)smallMat.data, smallMat.cols, smallMat.rows, (int)(smallMat.step), scale, min_neighbors, min_object_width, max_object_width, 1);

			smallMat.release();
		}
		else if (channels == 3) {
			cv::UMat monoMat;
			cv::cvtColor(mat, monoMat, cv::COLOR_BGR2GRAY);
			cv::Mat smallMat;
			cv::resize(monoMat, smallMat, cv::Size(smallWidth, smallHeight), 0.0, 0.0, cv::INTER_LINEAR);
			clahe->apply(smallMat, smallMat);

			facecount = *FDMR((int*)buffer.data(), (int*)smallMat.data, smallMat.cols, smallMat.rows, (int)(smallMat.step), scale, min_neighbors, min_object_width, max_object_width, 1);

			monoMat.release();
			smallMat.release();
		}
	}
	else {
		smallscale = 1.0f;
		if (channels == 1) {
			cv::Mat smallMat;
			clahe->apply(mat, smallMat);

			facecount = *FDMR((int*)buffer.data(), (int*)smallMat.data, smallMat.cols, smallMat.rows, (int)(smallMat.step), scale, min_neighbors, min_object_width, max_object_width, 1);
			smallMat.release();
		}
		else if (channels == 3) {
			cv::Mat smallMat;
			cv::cvtColor(mat, smallMat, cv::COLOR_BGR2GRAY);
			clahe->apply(smallMat, smallMat);

			facecount = *FDMR((int*)buffer.data(), (int*)smallMat.data, smallMat.cols, smallMat.rows, (int)(smallMat.step), scale, min_neighbors, min_object_width, max_object_width, 1);

			smallMat.release();
		}
	}

	std::vector<std::vector<cv::Point2f>> returnResults;
	for (int i = 0; i < facecount; i++) {
		int offset = 2 + (142 * i);

		// extend rectangle to include landmarks
		std::vector<cv::Point2f> landmarks;
		for (int j = 0; j < landmarkCount; j++) {
			short pointX = buffer[offset + 6 + (j * 2)];
			short pointY = buffer[offset + 6 + (j * 2) + 1];
			landmarks.push_back(cv::Point2f((float)pointX / smallscale, (float)pointY / smallscale));
		}

		returnResults.push_back(landmarks);
	}

	return returnResults;
}

// get face results from detector
std::vector<std::tuple<cv::RotatedRect, std::vector<cv::Point2f>>> hseg::facedetect::getFaceResults(std::vector<std::vector<cv::Point2f>> detectResults)
{
	std::vector<std::tuple<cv::RotatedRect, std::vector<cv::Point2f>>> returnResults;
	for (int i = 0; i < detectResults.size(); i++) {
		std::vector<cv::Point2f> points = detectResults[i];
		cv::RotatedRect rotatedrect = cv::minAreaRect(points);

		cv::RotatedRect resetRect = utility::resetRotatedRect(rotatedrect, points, midlineTop, midlineBottom);

		returnResults.push_back(std::make_tuple(resetRect, points));
	}
	return returnResults;
}

// sets face results to buffer
void hseg::facedetect::setFaceResults(BYTE* faceresults, const std::vector<std::tuple<cv::RotatedRect, std::vector<cv::Point2f>>>& outResults)
{
	int faceresultsize = utility::rotatedrectstructsize + (landmarkCount * utility::pointfstructsize);
	for (int i = 0; i < outResults.size(); i++) {
		BYTE* currentface = faceresults + (i * faceresultsize);

		cv::RotatedRect rotatedrect = std::get<0>(outResults[i]);
		utility::setRotatedRect(currentface, rotatedrect);

		std::vector<cv::Point2f> points = std::get<1>(outResults[i]);
		if (points.size() == landmarkCount) {
			for (int j = 0; j < landmarkCount; j++) {
				cv::Point2f point = points[j];
				utility::setPoint2f(currentface + utility::rotatedrectstructsize + (j * utility::pointfstructsize), point);
			}
		}
		else {
			for (int j = 0; j < landmarkCount; j++) {
				cv::Point2f point(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
				utility::setPoint2f(currentface + utility::rotatedrectstructsize + (j * utility::pointfstructsize), point);
			}
		}
	}
}
