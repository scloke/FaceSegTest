// FastPortrait.cpp : Defines the exported functions for the DLL.

#include "pch.h" // use pch.h in Visual Studio 2019

#include "FastPortrait.h"

static cv::Ptr<fastportrait> fp;

void initFP(int width, int height)
{
	if (fp == NULL) {
		fp = new fastportrait(width, height);
	}
}

void exitFP()
{
	if (fp != NULL) {
		fp = NULL;
	}
}

bool segmentFP(BYTE* matstruct_in, BYTE* buffer_in, BYTE* matstruct_out, BYTE* buffer_out, int* duration)
{
	cv::Mat inMat = utility::getMat(matstruct_in, buffer_in);
	cv::Mat outMat = utility::getMat(matstruct_out, buffer_out);

	cv::Mat mask;

	auto tProcessingStart = std::chrono::high_resolution_clock::now();
	mask = fp->segment(inMat, false);
	auto tProcessingEnd = std::chrono::high_resolution_clock::now();
	*duration = (int)std::chrono::duration_cast<std::chrono::microseconds>(tProcessingEnd - tProcessingStart).count();

	if (mask.channels() == 3) {
		mask.copyTo(outMat);
	}
	else {
		cv::cvtColor(mask, outMat, CV_GRAY2BGR);
	}
	mask.release();

	return true;
}

fastportrait::fastportrait(int width, int height)
{
	mnn_model = new model(model_path, face_weight, face_config, NULL, height, width, thread, private_level);
}

fastportrait::~fastportrait()
{
	if (mnn_model != NULL) {
		mnn_model = NULL;
	}
}

cv::Mat fastportrait::segment(cv::Mat mat, bool blur)
{
	cv::Mat image, ori_image;
	image = mat.clone();
	ori_image = mat.clone();
	auto pre_out = mnn_model->preprocess(image);
	auto infer_out = mnn_model->infer(image);
	cv::Mat mask = mnn_model->postprocessmask(infer_out, ori_image, get<0>(pre_out), get<1>(pre_out), get<2>(pre_out), true, 13);

	// keep edges sharp for comparison
	if (!blur) {
		cv::threshold(mask, mask, 127, 255, CV_THRESH_BINARY);
	}

	return mask;
}
