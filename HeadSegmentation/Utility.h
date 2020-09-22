#pragma once

#include "Global.h"
#include <execution>

#ifndef UTILITY_H
#define UTILITY_H

namespace hseg
{
	class utility
	{
	public:
		utility();
		~utility();

		// STRUCTURE SIZES
		static const int pointfstructsize = 8;						// size of structures
		static const int posestructsize = 12;						// size of structures
		static const int rectstructsize = 20;						// size of structures
		static const int rotatedrectstructsize = 28;				// size of structures

		// INPUT PROCESSING
		static cv::Mat getMat(BYTE* matstruct, BYTE* buffer);
		static std::tuple<int, int, int, int, int, int> getMatStruct(BYTE* matstruct);
		static cv::Point2f getPoint2f(BYTE* pointfstruct);
		static std::vector<cv::Point2f> getPoint2fVec(BYTE* pointfstruct, int pointcount);
		static std::vector<std::vector<cv::Point2f>> getPoint2fVecOfVec(BYTE* pointfstruct, int pointcount, int veccount);
		static void setPoint2f(BYTE* pointfstruct, cv::Point2f point);
		static void setRect2f(BYTE* rectstruct, const cv::Rect2f& rect);
		static void setRect2fVec(BYTE* rectstruct, const std::vector<cv::Rect2f>& rects);
		static void setRotatedRect(BYTE* rotatedrectstruct, const cv::RotatedRect& rotatedRect);

		// PRIMITIVES
		static int getIntFromBuffer(BYTE* buffer, int offset);
		static float getFloatFromBufferEx(BYTE* buffer, int* offset);
		static void setFloatToBuffer(BYTE* buffer, int offset, float value);
		static void setFloatToBufferEx(BYTE* buffer, int* offset, float value);

		// GEOMETRY
		static cv::RotatedRect resetRotatedRect(const cv::RotatedRect& rotatedRect, const std::vector<cv::Point2f>& points, int topIndex, int bottomIndex);
		static cv::RotatedRect resetRotatedRect(const cv::RotatedRect& rotatedRect, float midline);
		static float getMidline(const cv::RotatedRect& rotatedRect, const std::vector<cv::Point2f>& points, int topIndex, int bottomIndex);
		static double getdistance(const cv::Point2f& p1, const cv::Point2f& p2);
		static cv::Point2f getMidPoint(const cv::Point2f& p1, const cv::Point2f& p2);
		static cv::Point2f getCenterPoint(const cv::Rect2f& rect);
		static cv::Point2f getInterPoint(const cv::Point2f& p1, const cv::Point2f& p2, float fraction);

		// TEMPLATES
		// converts mat to vec
		template<typename T> static std::vector<T> matToVec(const cv::Mat& mat)
		{
			std::vector<T> returnVec;
			if (mat.isContinuous()) {
				returnVec = mat.reshape(0, 1);
			}
			else {
				cv::Mat matCont = mat.clone();
				returnVec = matCont.reshape(0, 1);
				matCont.release();
			}
			return returnVec;
		}

		// converts vec to mat
		template<typename T> static cv::Mat vecToMat(const std::vector<T>& vec, int width, int height, int type)
		{
			cv::Mat returnMat(height, width, type);
			memcpy(returnMat.data, vec.data(), vec.size() * sizeof(T));
			return returnMat;
		}

		template<typename T> static void vecToMat(const std::vector<T>& vec, cv::Mat* mat)
		{
			memcpy(mat->data, vec.data(), vec.size() * sizeof(T));
		}

		// checks if a value is present in the vector
		template<typename T> static bool findVal(const std::vector<T>& vec, T value)
		{
			if (vec.size() >= parallelLimit) {
				return std::find(std::execution::par_unseq, vec.begin(), vec.end(), value) != vec.end();
			}
			else {
				return std::find(std::execution::seq, vec.begin(), vec.end(), value) != vec.end();
			}
		}

		// WATERSHED
		struct WSNode
		{
			int next;
			int mask_ofs;
			int img_ofs;
		};

		// Queue for WSNodes
		struct WSQueue
		{
			WSQueue() { first = last = 0; }
			int first, last;
		};

		static int allocWSNodes(std::vector<WSNode>& storage);
		static void watershedEx(const cv::Mat& src, cv::Mat& dst);

	private:
		static const int parallelLimit = 100;						// limit before parallel processing
	};
}

#endif
