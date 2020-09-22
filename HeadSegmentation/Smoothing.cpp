// Smoothing.cpp : Defines the smoothing classes for the DLL.

#include "pch.h" // use pch.h in Visual Studio 2019

#include "Smoothing.h"

hseg::medianSmoothing::medianSmoothing(int capacity)
{
	m_capacity = capacity;
}

hseg::medianSmoothing::medianSmoothing(const medianSmoothing& smoothing)
{
	m_capacity = smoothing.m_capacity;
	m_store = std::vector<double>(smoothing.m_store);
}

hseg::medianSmoothing::medianSmoothing()
{
	m_capacity = 0;
}

hseg::medianSmoothing::~medianSmoothing()
{
	m_store.clear();
}

void hseg::medianSmoothing::enqueue(double value)
{
	m_store.push_back(value);

	while (m_store.size() > m_capacity) {
		m_store.erase(m_store.begin());
	}
}

double hseg::medianSmoothing::getSmoothedValue()
{
	if (m_store.size() == 0) {
		return std::numeric_limits<double>::quiet_NaN();
	}
	else if (m_store.size() >= minSmoothingCount) {
		int halfIndex = ((int)m_store.size() - 1) / 2;
		std::vector<double> storeMed(m_store);
		std::sort(storeMed.begin(), storeMed.end());

		return storeMed[halfIndex];
	}
	else {
		return m_store.back();
	}
}

int hseg::medianSmoothing::count()
{
	return (int)m_store.size();
}

void hseg::medianSmoothing::clear()
{
	m_store.clear();
}

hseg::singleSmoothing::singleSmoothing(int capacity)
{
	m_capacity = capacity;
}

hseg::singleSmoothing::singleSmoothing(const singleSmoothing& smoothing)
{
	m_capacity = smoothing.m_capacity;
	m_store = std::vector<std::pair<double, double>>(smoothing.m_store);
}

hseg::singleSmoothing::singleSmoothing()
{
	m_capacity = 0;
}

hseg::singleSmoothing::~singleSmoothing()
{
	m_store.clear();
}

void hseg::singleSmoothing::enqueue(double value, double smoothing)
{
	m_store.push_back(std::make_pair(value, smoothing));

	while (m_store.size() > m_capacity) {
		m_store.erase(m_store.begin());
	}
}

// get the result of the last smoothed value
double hseg::singleSmoothing::getSmoothedQueueResult(int minSmoothingCount)
{
	if (m_store.size() == 0) {
		return std::numeric_limits<double>::quiet_NaN();
	}
	else if (m_store.size() >= minSmoothingCount) {
		std::vector<double> smoothedQueue = getSmoothedQueue();
		return smoothedQueue.back();
	}
	else {
		return m_store.back().first;
	}
}

// runs single exponential smoothing method on the contained queue values
std::vector<double> hseg::singleSmoothing::getSmoothedQueue()
{
	if (m_store.size() > 0) {
		std::vector<double> queue;

		for (int i = 0; i < m_store.size(); i++) {
			switch (i) {
			case 0:
			{
				queue.push_back(m_store[i].first);
				break;
			}
			default:
			{
				double St = (m_store[i].second * m_store[i].first) + ((1.0 - m_store[i].second) * queue[i - 1]);
				queue.push_back(St);
				break;
			}
			}
		}

		return queue;
	}
	else {
		return std::vector<double>();
	}
}

int hseg::singleSmoothing::count()
{
	return (int)m_store.size();
}

void hseg::singleSmoothing::clear()
{
	m_store.clear();
}

hseg::doubleSmoothing::doubleSmoothing(int capacity)
{
	m_capacity = capacity;
}

hseg::doubleSmoothing::doubleSmoothing(const doubleSmoothing& smoothing)
{
	m_capacity = smoothing.m_capacity;
	m_store = std::vector<double>(smoothing.m_store);
}

hseg::doubleSmoothing::doubleSmoothing()
{
	m_capacity = 0;
}

hseg::doubleSmoothing::~doubleSmoothing()
{
	m_store.clear();
}

void hseg::doubleSmoothing::enqueue(double value)
{
	m_store.push_back(value);

	while (m_store.size() > m_capacity) {
		m_store.erase(m_store.begin());
	}
}

// get the result of the last smoothed value
// gets the predicted values iAhead steps after the last value, if supplied
double hseg::doubleSmoothing::getSmoothedQueueResult(int minSmoothingCount, double smoothing, int ahead)
{
	if (m_store.size() == 0) {
		return std::numeric_limits<double>::quiet_NaN();
	}
	else if (m_store.size() >= minSmoothingCount) {
		std::vector<double> smoothedQueue = getSmoothedQueue(smoothing, smoothing, ahead);
		return smoothedQueue.back();
	}
	else {
		return m_store.back();
	}
}

// runs the Holt-Winters double exponential smoothing method on the contained queue values
std::vector<double> hseg::doubleSmoothing::getSmoothedQueue(double alpha, double beta, int ahead)
{
	if (m_store.size() > 0) {
		std::vector<double> queueS, queueB;

		for (int i = 0; i < m_store.size(); i++) {
			switch (i) {
			case 0:
			{
				queueS.push_back(m_store[i]);
				queueB.push_back(m_store[i]);
				break;
			}
			case 1:
			{
				double S1 = 0;
				double B1 = 0;
				doubleSmoothInit(m_store[i - 1], m_store[i], &S1, &B1);
				queueS.push_back(S1);
				queueB.push_back(B1);
				break;
			}
			default:
			{
				double St = 0;
				double Bt = 0;
				doubleSmooth(queueS[i - 1], queueB[i - 1], alpha, beta, m_store[i], &St, &Bt);
				queueS.push_back(St);
				queueB.push_back(Bt);
				break;
			}
			}
		}

		// forecast ahead
		if (queueS.size() > 0) {
			for (int i = 0; i < ahead; i++) {
				double last = queueS[queueS.size() - 1];
				switch (queueS.size()) {
				case 1:
				{
					queueS.push_back(last);
					break;
				}
				default:
				{
					double nextLast = queueS[queueS.size() - 2];
					double step = last - nextLast;
					queueS.push_back(last + step);
					break;
				}
				}
			}
		}

		return queueS;
	}
	else {
		return std::vector<double>();
	}
}

int hseg::doubleSmoothing::count()
{
	return (int)m_store.size();
}

void hseg::doubleSmoothing::clear()
{
	m_store.clear();
}

//initialises the Holt-Winters double exponential smoothing method
inline void hseg::doubleSmoothing::doubleSmoothInit(double X0, double X1, double* S1, double* B1)
{
	*S1 = X1;
	*B1 = X1 - X0;
}

// runs the Holt-Winters double exponential smoothing method
inline void hseg::doubleSmoothing::doubleSmooth(double Stp, double Btp, double alpha, double beta, double Xt, double* St, double* Bt)
{
	*St = (alpha * Xt) + ((1.0 - alpha) * (Stp + Btp));
	*Bt = (beta * (*St - Stp)) + ((1.0 - beta) * Btp);
}

hseg::noSmoothing::noSmoothing()
{
	lastValue = std::numeric_limits<double>::quiet_NaN();
}

hseg::noSmoothing::noSmoothing(const noSmoothing& smoothing)
{
	lastValue = smoothing.lastValue;
}

hseg::noSmoothing::~noSmoothing()
{
}

void hseg::noSmoothing::enqueue(double value)
{
	lastValue = value;
}

// get the result of the last smoothed value
// gets the predicted values iAhead steps after the last value, if supplied
double hseg::noSmoothing::getSmoothedResult()
{
	return lastValue;
}

hseg::rotatedRectSmoothing::rotatedRectSmoothing(int minSmoothing, float smoothing)
{
	hseg::rotatedRectSmoothing::minSmoothing = minSmoothing;
	hseg::rotatedRectSmoothing::smoothing = smoothing;
	x = noSmoothing();
	y = noSmoothing();
	angle = doubleSmoothing(doubleSmoothingCapacity);
	width = medianSmoothing(medianSmoothingCapacity);
	height = medianSmoothing(medianSmoothingCapacity);
}

hseg::rotatedRectSmoothing::rotatedRectSmoothing()
{
	minSmoothing = 1;
	hseg::rotatedRectSmoothing::smoothing = mediumSmoothing;
	x = noSmoothing();
	y = noSmoothing();
	angle = doubleSmoothing(doubleSmoothingCapacity);
	width = medianSmoothing(medianSmoothingCapacity);
	height = medianSmoothing(medianSmoothingCapacity);
}

hseg::rotatedRectSmoothing::rotatedRectSmoothing(const rotatedRectSmoothing& smoothingRect)
{
	hseg::rotatedRectSmoothing::minSmoothing = smoothingRect.minSmoothing;
	hseg::rotatedRectSmoothing::smoothing = smoothingRect.smoothing;
	x = noSmoothing(smoothingRect.x);
	y = noSmoothing(smoothingRect.y);
	angle = doubleSmoothing(smoothingRect.angle);
	width = medianSmoothing(smoothingRect.width);
	height = medianSmoothing(smoothingRect.height);
}

hseg::rotatedRectSmoothing::~rotatedRectSmoothing()
{
	x.~noSmoothing();
	y.~noSmoothing();
	angle.~doubleSmoothing();
	width.~medianSmoothing();
	height.~medianSmoothing();
}

// enqueues rotated rect
void hseg::rotatedRectSmoothing::enqueue(cv::RotatedRect& rotatedRect)
{
	// only enqueue if valid rotated rect
	if (!rotatedRect.size.empty()) {
		x.enqueue(rotatedRect.center.x);
		y.enqueue(rotatedRect.center.y);

		float smoothedAngle = (float)angle.getSmoothedQueueResult(minSmoothing, smoothing, 0);
		float angleDiff = 0.0f;
		if (!std::isnan(smoothedAngle)) {
			angleDiff = abs(fmodf((smoothedAngle - rotatedRect.angle) + 180.0f + 90.0f, 180.0f) - 90.0f);
		}

		float smoothedWidth = (float)width.getSmoothedValue();
		float smoothedHeight = (float)height.getSmoothedValue();
		if (angleDiff < 90.0f) {
			float correctedAngle = fmodf(rotatedRect.angle + 180.0f + 90.0f, 180.0f) - 90.0f;
			if (!std::isnan(smoothedAngle)) {
				float correctedAngleDiff = abs(fmodf((smoothedAngle - correctedAngle) + 180.0f + 90.0f, 180.0f) - 90.0f);
				if (correctedAngleDiff <= 45.0f * maxFaceDifference) {
					angle.enqueue(correctedAngle);
				}
			}
			else {
				angle.enqueue(correctedAngle);
			}

			if ((!std::isnan(smoothedWidth)) && (!std::isnan(smoothedHeight))) {
				float maxDiff = MAX(MAX(rotatedRect.size.width / smoothedWidth, smoothedWidth / rotatedRect.size.width), MAX(rotatedRect.size.height / smoothedHeight, smoothedHeight / rotatedRect.size.height));
				if (maxDiff <= 1.0 + maxFaceDifference) {
					width.enqueue(rotatedRect.size.width);
					height.enqueue(rotatedRect.size.height);
				}
			}
			else {
				width.enqueue(rotatedRect.size.width);
				height.enqueue(rotatedRect.size.height);
			}
		}
		else {
			float correctedAngle = fmodf(rotatedRect.angle + 90.0f + 180.0f + 90.0f, 180.0f) - 90.0f;
			if (!std::isnan(smoothedAngle)) {
				float correctedAngleDiff = abs(fmodf((smoothedAngle - correctedAngle) + 180.0f + 90.0f, 180.0f) - 90.0f);
				if (correctedAngleDiff <= 45.0f * maxFaceDifference) {
					angle.enqueue(correctedAngle);
				}
			}
			else {
				angle.enqueue(correctedAngle);
			}

			if ((!std::isnan(smoothedWidth)) && (!std::isnan(smoothedHeight))) {
				float maxDiff = MAX(MAX(rotatedRect.size.height / smoothedWidth, smoothedWidth / rotatedRect.size.height), MAX(rotatedRect.size.width / smoothedHeight, smoothedHeight / rotatedRect.size.width));
				if (maxDiff <= 1.0 + maxFaceDifference) {
					width.enqueue(rotatedRect.size.height);
					height.enqueue(rotatedRect.size.width);
				}
			}
			else {
				width.enqueue(rotatedRect.size.height);
				height.enqueue(rotatedRect.size.width);
			}
		}
	}
}

// gets smoothed rotated rect
cv::RotatedRect hseg::rotatedRectSmoothing::getRotatedRect()
{
	if (count() == 0) {
		return cv::RotatedRect();
	}
	else {
		return cv::RotatedRect(cv::Point2f((float)x.getSmoothedResult(), (float)y.getSmoothedResult()),
			cv::Size2f((float)width.getSmoothedValue(), (float)height.getSmoothedValue()),
			(float)angle.getSmoothedQueueResult(minSmoothing, smoothing));
	}
}

int hseg::rotatedRectSmoothing::count()
{
	return angle.count();
}

// matrix smoothing class
hseg::matSmoothing::matSmoothing(int acctype, int outtype)
{
	hseg::matSmoothing::acctype = acctype;
	hseg::matSmoothing::outtype = outtype;
	cleared = true;
}

hseg::matSmoothing::matSmoothing()
{
	hseg::matSmoothing::acctype = CV_32F;
	hseg::matSmoothing::outtype = CV_8U;
	cleared = true;
}

hseg::matSmoothing::~matSmoothing()
{
	if (!accumulator.empty()) {
		accumulator.release();
	}
}

cv::Size hseg::matSmoothing::size()
{
	cv::Size returnSize;
	if (!accumulator.empty()) {
		returnSize = cv::Size(accumulator.cols, accumulator.rows);
	}
	return returnSize;
}

bool hseg::matSmoothing::empty()
{
	return accumulator.empty();
}

void hseg::matSmoothing::clear()
{
	if (!accumulator.empty()) {
		accumulator.setTo(0);
	}
	cleared = true;
}

void hseg::matSmoothing::enqueue(cv::Mat item, float smoothing, cv::Mat mask)
{
	// create accumlator if empty
	if (accumulator.empty()) {
		accumulator = cv::Mat(item.rows, item.cols, CV_MAKETYPE(acctype, item.channels()));
	}

	if (item.size() == accumulator.size() && item.channels() == accumulator.channels()) {
		int maskType = CV_MAKETYPE(CV_8U, item.channels());
		if (cleared) {
			if (mask.empty()) {
				item.convertTo(accumulator, acctype);
			}
			else if (mask.size() == accumulator.size() && mask.type() == maskType) {
				cv::Mat convertItem;
				item.convertTo(convertItem, acctype);
				cv::copyTo(convertItem, accumulator, mask);
				convertItem.release();
			}
			cleared = false;
		}
		else {
			if (mask.empty()) {
				cv::accumulateWeighted(item, accumulator, smoothing);
			}
			else if (mask.size() == accumulator.size() && mask.type() == maskType) {
				cv::accumulateWeighted(item, accumulator, smoothing, mask);
			}
		}
	}
}

cv::Mat hseg::matSmoothing::getSmoothedMat()
{
	cv::Mat returnMat;
	accumulator.convertTo(returnMat, outtype);
	cv::boxFilter(returnMat, returnMat, -1, cv::Size(5, 5), cv::Point(-1, -1), true, cv::BORDER_ISOLATED);
	return returnMat;
}
