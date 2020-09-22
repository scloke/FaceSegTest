#pragma once

#ifdef FASTPORTRAIT_EXPORTS
#define FASTPORTRAIT_API __declspec(dllexport)
#else
#define FASTPORTRAIT_API __declspec(dllimport)
#endif

#pragma warning (disable : 26451)

#include "process.h"
#include "model.h"
#include <opencv2/imgproc/types_c.h>


extern "C" FASTPORTRAIT_API void initFP(int width, int height);

extern "C" FASTPORTRAIT_API void exitFP();

extern "C" FASTPORTRAIT_API bool segmentFP(BYTE * matstruct_in, BYTE * buffer_in, BYTE * matstruct_out, BYTE * buffer_out, int* duration);


#ifndef FASTPORTRAIT_H
#define FASTPORTRAIT_H

// utility functions
class utility
{
public:
	// BUFFER ACCESS
	// gets mat from matstruct and buffer
	static cv::Mat getMat(BYTE* matstruct, BYTE* buffer)
	{
		std::tuple<int, int, int, int, int, int> returnStruct = getMatStruct(matstruct);
		int width = std::get<0>(returnStruct);
		int height = std::get<1>(returnStruct);
		int type = std::get<4>(returnStruct);
		int step = std::get<5>(returnStruct);

		cv::Mat mat(height, width, type, buffer, step);

		return mat;
	}

	static std::tuple<int, int, int, int, int, int> getMatStruct(BYTE* matstruct)
	{
		int width = getIntFromBuffer(matstruct, 0);
		int height = getIntFromBuffer(matstruct, 4);
		int channels = getIntFromBuffer(matstruct, 8);
		int length = getIntFromBuffer(matstruct, 12);
		int basetype = getIntFromBuffer(matstruct, 16);

		int type = basetype + ((channels - 1) * 8);
		int step = length / height;

		return std::make_tuple(width, height, channels, length, type, step);
	}

	// gets int from buffer location
	static int getIntFromBuffer(BYTE* buffer, int offset)
	{
		int returnInt = *(int*)(buffer + offset);
		return returnInt;
	}

private:
};

class fastportrait
{
public:
	fastportrait(int width, int height);
	~fastportrait();

    cv::Mat segment(cv::Mat mat, bool blur);

private:
    const char* model_path = ".\\Dnc_SINet_bi_256_192_fp16.mnn";        // path to model, should be in same directory as active
    const char* face_weight = ".\\opencv_face_detector_uint8.pb";       // weights for face detector
    const char* face_config = ".\\opencv_face_detector.pbtxt";          // configuration file for face detector
    const int thread = 1;                                               // number of threads to use for mnn
    const float private_level = 0.41;                                   // private level for mnn model

	cv::Ptr<model> mnn_model;											// mnn model class
};

#endif
