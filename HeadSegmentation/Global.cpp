// Global.cpp : Defines the global class for the DLL.

#include "pch.h" // use pch.h in Visual Studio 2019

#include "Global.h"

// GAMMA FUNCTIONS
hseg::gamma::gamma()
{
	gammaLowInt = (int)(gammaLow * (float)gammaScale);
	gammaHighInt = (int)(gammaHigh * (float)gammaScale);
	gammaStepInt = (int)(gammaStep * (float)gammaScale);

	setGammaLUT();
}

hseg::gamma::~gamma()
{
	for (int i = 0; i < gammaLUT.size(); i++) {
		gammaLUT[i].release();
	}
	gammaLUT.clear();
}

// initialises the gamma lut mat
void hseg::gamma::setGammaLUT()
{
	int lutCount = (int)((float)(gammaHighInt - gammaLowInt) / (float)gammaStepInt);
	gammaLUT.reserve(lutCount);

	for (int gammaInt = gammaLowInt; gammaInt < gammaHighInt; gammaInt += gammaStepInt) {
		float invGamma = (float)gammaScale / (float)gammaInt;
		cv::Mat currentLUT(256, 1, CV_8UC1);
		for (int j = 0; j < 256; j++) {
			float convert = (float)j / 255.0f;
			convert = pow(convert, invGamma);
			currentLUT.at<BYTE>(j, 0) = (BYTE)MAX(MIN(convert * 255.0, 255), 0);
		}
		gammaLUT.emplace(std::make_pair(gammaInt, currentLUT));
	}
}

// gamma correction algorithm
void hseg::gamma::gammaCorrect(const cv::Mat& img_input, cv::Mat& img_output)
{
	int trimInt = 100;
	cv::Mat monoImg, smallImg, rangeMask;

	// convert to grayscale and resize down for gamma correction
	cv::cvtColor(img_input, monoImg, cv::COLOR_BGR2GRAY);
	cv::resize(monoImg, smallImg, cv::Size(gammaSmallSize, gammaSmallSize));

	// get the mask for all pixels within the brightness range for processing
	cv::inRange(smallImg, cv::Scalar::all(gammaRangeRestriction), cv::Scalar::all(245 - gammaRangeRestriction), rangeMask);

	// get the mean brightness for the pixels within the mask
	BYTE brightness = (BYTE)cv::mean(smallImg, rangeMask)[0];

	// get list of gamma values and corrected brightness
	for (int gammaInt = gammaLowInt; gammaInt < gammaHighInt; gammaInt += gammaStepInt) {
		BYTE correctedBrightness = gammaLUT[gammaInt].at<BYTE>(brightness, 0);
		if (correctedBrightness >= targetColourBrightness) {
			trimInt = gammaInt;
			break;
		}
	}

	// run gamma processing only if needed
	if (trimInt != 100) {
		cv::LUT(img_input, gammaLUT[trimInt], img_output);
	}
	else {
		img_output = img_input.clone();;
	}

	// clean up
	monoImg.release();
	smallImg.release();
	rangeMask.release();
}

// FRAMECONTAINER FUNCTIONS
hseg::frameContainer::frameContainer()
{
	width = 0;
	height = 0;
	smallWidth = 0;
	smallHeight = 0;
	tinyWidth = 0;
	tinyHeight = 0;
	smallScale = 0.0f;
	tinyScale = 0.0f;
}

hseg::frameContainer::~frameContainer()
{
	if (!gammaMat.empty()) {
		gammaMat.release();
	}
	if (!smallGammaMat.empty()) {
		smallGammaMat.release();
	}
	if (!smallMonoMat.empty()) {
		smallMonoMat.release();
	}
	if (!tinyLABMat.empty()) {
		tinyLABMat.release();
	}
}

// gets frame container from image
hseg::frameContainer hseg::frameContainer::getFrameContainer(const cv::Mat& img_input, hseg::gamma* gammacorrect)
{
	frameContainer frame;

	frame.width = img_input.cols;
	frame.height = img_input.rows;
	frame.size = cv::Size(frame.width, frame.height);
	frame.smallWidth = frameSmallWidth;
	frame.smallScale = (float)frameSmallWidth / (float)(img_input.cols);
	frame.smallHeight = (int)((float)(img_input.rows) * frame.smallScale);
	frame.smallSize = cv::Size(frame.smallWidth, frame.smallHeight);
	frame.tinyWidth = frameTinyWidth;
	frame.tinyScale = (float)frameTinyWidth / (float)(img_input.cols);
	frame.tinyHeight = (int)((float)(img_input.rows) * frame.tinyScale);
	frame.tinySize = cv::Size(frame.tinyWidth, frame.tinyHeight);
	frame.rect = cv::Rect(cv::Point(), frame.size);
	frame.smallRect = cv::Rect(cv::Point(), frame.smallSize);
	frame.tinyRect = cv::Rect(cv::Point(), frame.tinySize);

	cv::Mat tinyGammaMat;

	// resize to small size, and correct gamma
	gammacorrect->gammaCorrect(img_input, frame.gammaMat);
	cv::resize(frame.gammaMat, frame.smallGammaMat, frame.smallSize);
	cv::resize(frame.smallGammaMat, tinyGammaMat, frame.tinySize);
	cv::cvtColor(frame.smallGammaMat, frame.smallMonoMat, cv::COLOR_BGR2GRAY);

	// convert to lab colour
	cv::cvtColor(tinyGammaMat, frame.tinyLABMat, cv::COLOR_BGR2Lab);

	// clean up
	tinyGammaMat.release();

	return frame;
}

// POSE FUNCTIONS
hseg::pose::pose(float roll, float pitch, float yaw)
{
	hseg::pose::roll = roll;
	hseg::pose::pitch = pitch;
	hseg::pose::yaw = yaw;
}

hseg::pose::pose()
{
	hseg::pose::roll = 0.0f;
	hseg::pose::pitch = 0.0f;
	hseg::pose::yaw = 0.0f;
}
