#pragma once

#pragma warning (disable : 26451)

#include "Global.h"
#include <mutex>

#ifndef SMOOTHING_H
#define SMOOTHING_H

namespace hseg
{
	static const int minSmoothingCount = 3;							// do not use smoothing until at least this number of values have accumulated
	static const int medianSmoothingCapacity = 5;					// capacity for median smoothing store
	static const int doubleSmoothingCapacity = 20;					// capacity for double smoothing store
	static const double maxFaceDifference = 0.25;					// maximum angle difference before discarding value

	// median smoothing class
	class medianSmoothing
	{
	public:
		medianSmoothing(int capacity);
		medianSmoothing(const medianSmoothing& smoothing);
		medianSmoothing();
		~medianSmoothing();

		void enqueue(double value);
		double getSmoothedValue();
		int count();
		void clear();

	private:
		int m_capacity;
		std::vector<double> m_store;
	};

	// single smoothing class
	class singleSmoothing
	{
	public:
		singleSmoothing(int capacity);
		singleSmoothing(const singleSmoothing& smoothing);
		singleSmoothing();
		~singleSmoothing();

		void enqueue(double value, double smoothing);
		double getSmoothedQueueResult(int minSmoothingCount);
		std::vector<double> getSmoothedQueue();
		int count();
		void clear();

	private:
		int m_capacity;
		std::vector<std::pair<double, double>> m_store;
	};

	// Holt-Winters double smoothing class
	class doubleSmoothing
	{
	public:
		doubleSmoothing(int capacity);
		doubleSmoothing(const doubleSmoothing& smoothing);
		doubleSmoothing();
		~doubleSmoothing();

		void enqueue(double value);
		double getSmoothedQueueResult(int minSmoothingCount, double smoothing, int ahead = 0);
		std::vector<double> getSmoothedQueue(double alpha, double beta, int ahead = 0);
		int count();
		void clear();

	private:
		int m_capacity;
		std::vector<double> m_store;

		static inline void doubleSmoothInit(double X0, double X1, double* S1, double* B1);
		static inline void doubleSmooth(double Stp, double Btp, double alpha, double beta, double Xt, double* St, double* Bt);
	};

	// no smoothing
	class noSmoothing
	{
	public:
		noSmoothing();
		noSmoothing(const noSmoothing& smoothing);
		~noSmoothing();

		void enqueue(double value);
		double getSmoothedResult();

	private:
		double lastValue;
	};

	// smoothing for rotated rects
	class rotatedRectSmoothing
	{
	public:
		rotatedRectSmoothing(int minSmoothing, float smoothing);
		rotatedRectSmoothing();
		rotatedRectSmoothing(const rotatedRectSmoothing& smoothingRect);
		~rotatedRectSmoothing();

		void enqueue(cv::RotatedRect& rotatedRect);
		cv::RotatedRect getRotatedRect();
		int count();

	private:
		int minSmoothing;
		double smoothing;
		noSmoothing x, y;
		doubleSmoothing angle;
		medianSmoothing width, height;
	};

	// matrix smoothing class
	class matSmoothing
	{
	public:
		matSmoothing(int acctype, int outtype);
		matSmoothing();
		~matSmoothing();

		cv::Size size();
		bool empty();
		void clear();
		void enqueue(cv::Mat item, float smoothing, cv::Mat mask = cv::Mat());
		cv::Mat getSmoothedMat();

	private:
		cv::Mat accumulator;
		bool cleared;
		int acctype, outtype;
	};
}

#endif
