#pragma once

#pragma warning (disable : 26451)

#pragma warning(push, 0)
#pragma warning (disable : 6294 6201 26439 26495)
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

#ifndef GLOBAL_H
#define GLOBAL_H

namespace hseg
{
	static const int labChannels = 3;							// lab colour space number of channels
	static const double largeSmoothing = 0.5;					// large smoothing value
	static const double mediumSmoothing = 0.3;					// medium smoothing value
	static const double smallSmoothing = 0.1;					// small smoothing value
	static const int identifierExpiry = 5 * 30;					// 5 seconds (assuming 30 fps)

	class gamma
	{
	public:
		gamma();
		~gamma();

		void gammaCorrect(const cv::Mat& img_input, cv::Mat& img_output);

	private:
		std::unordered_map<int, cv::Mat> gammaLUT;						// key contains the gamma * gammaScale
		float gammaLow = 0.1f;											// gamma search range
		float gammaHigh = 5.0f;											// gamma search range
		float gammaStep = 0.05f;										// gamma search range
		static const int gammaScale = 100;								// scaling for gamma -> int
		int gammaLowInt, gammaHighInt, gammaStepInt;					// gamma search range
		static const int targetColourBrightness = 160;					// target brightness value for the colour images
		static const int gammaSmallSize = 10;							// processing size to determine image brighness
		static const int gammaRangeRestriction = 8;						// restricts brightness search to remove black and white pixels

		void setGammaLUT();
	};

	class frameContainer
	{
	public:
		frameContainer();
		~frameContainer();

		cv::Mat gammaMat, smallGammaMat, smallMonoMat, tinyLABMat;
		float smallScale, tinyScale;
		int width, height, smallWidth, smallHeight, tinyWidth, tinyHeight;
		cv::Size size, smallSize, tinySize;
		cv::Rect rect, smallRect, tinyRect;

		static frameContainer getFrameContainer(const cv::Mat& img_input, hseg::gamma* gammacorrect);

	private:
		static const int frameTinyWidth = 80;							// tiny frame width for processing
		static const int frameSmallWidth = 160;							// small frame width for processing
		static const int processingBlurK = 3;							// median blur k value for processing images
	};

	struct pose
	{
	public:
		float roll;
		float pitch;
		float yaw;

		pose(float roll, float pitch, float yaw);
		pose();

	private:
	};
}

#endif