// Utility.cpp : Defines the utility functions for the DLL.

#include "pch.h" // use pch.h in Visual Studio 2019

#include "Utility.h"

// UTILITY FUNCTIONS
hseg::utility::utility()
{
}

hseg::utility::~utility()
{
}

// INPUT PROCESSING
cv::Mat hseg::utility::getMat(BYTE* matstruct, BYTE* buffer)
{
	std::tuple<int, int, int, int, int, int> returnStruct = getMatStruct(matstruct);
	int width = std::get<0>(returnStruct);
	int height = std::get<1>(returnStruct);
	int type = std::get<4>(returnStruct);
	int step = std::get<5>(returnStruct);

	if (width == 0 || height == 0) {
		return cv::Mat();
	}
	else {
		cv::Mat mat(height, width, type, buffer, step);
		return mat;
	}
}

std::tuple<int, int, int, int, int, int> hseg::utility::getMatStruct(BYTE* matstruct)
{
	int width = getIntFromBuffer(matstruct, 0);
	int height = getIntFromBuffer(matstruct, 4);
	int channels = getIntFromBuffer(matstruct, 8);
	int length = getIntFromBuffer(matstruct, 12);
	int basetype = getIntFromBuffer(matstruct, 16);

	int type = basetype + ((channels - 1) * 8);
	if (length == 0) {
		return std::make_tuple(0, 0, 0, 0, 0, 0);
	}
	else {
		int step = length / height;
		return std::make_tuple(width, height, channels, length, type, step);
	}
}

// gets point2f from pointfstruct
cv::Point2f hseg::utility::getPoint2f(BYTE* pointfstruct)
{
	int offset = 0;
	float x = getFloatFromBufferEx(pointfstruct, &offset);
	float y = getFloatFromBufferEx(pointfstruct, &offset);

	return cv::Point2f(x, y);
}

// gets vector of point2f from pointfstruct
std::vector<cv::Point2f> hseg::utility::getPoint2fVec(BYTE* pointfstruct, int pointcount)
{
	std::vector<cv::Point2f> returnPoints(pointcount);
	for (int i = 0; i < pointcount; i++) {
		// get point
		returnPoints[i] = getPoint2f(pointfstruct + (i * pointfstructsize));
	}
	return returnPoints;
}

// gets vector of vector of point2f from pointfstruct
std::vector<std::vector<cv::Point2f>> hseg::utility::getPoint2fVecOfVec(BYTE* pointfstruct, int pointcount, int veccount)
{
	std::vector<std::vector<cv::Point2f>> returnVecOfVec(veccount);
	for (int i = 0; i < veccount; i++) {
		// get vec
		returnVecOfVec[i] = getPoint2fVec(pointfstruct + (i * pointfstructsize * pointcount), pointcount);
	}
	return returnVecOfVec;
}

// sets point2f to pointfstruct
void hseg::utility::setPoint2f(BYTE* pointfstruct, cv::Point2f point)
{
	setFloatToBuffer(pointfstruct, 0, point.x);
	setFloatToBuffer(pointfstruct, 4, point.y);
}

// sets rect to rectstruct
void hseg::utility::setRect2f(BYTE* rectstruct, const cv::Rect2f& rect)
{
	int offset = 0;
	setFloatToBufferEx(rectstruct, &offset, rect.x);
	setFloatToBufferEx(rectstruct, &offset, rect.y);
	setFloatToBufferEx(rectstruct, &offset, rect.width);
	setFloatToBufferEx(rectstruct, &offset, rect.height);
}

// sets vector of rect to rectstruct
void hseg::utility::setRect2fVec(BYTE* rectstruct, const std::vector<cv::Rect2f>& rects)
{
	for (int i = 0; i < rects.size(); i++) {
		// set rect
		setRect2f(rectstruct + (i * rectstructsize), rects[i]);
	}
}

// sets rotated rect to rotatedrectstruct
void hseg::utility::setRotatedRect(BYTE* rotatedrectstruct, const cv::RotatedRect& rotatedRect)
{
	setFloatToBuffer(rotatedrectstruct, 0, rotatedRect.center.x);
	setFloatToBuffer(rotatedrectstruct, 4, rotatedRect.center.y);
	setFloatToBuffer(rotatedrectstruct, 8, rotatedRect.angle);
	setFloatToBuffer(rotatedrectstruct, 12, rotatedRect.size.width);
	setFloatToBuffer(rotatedrectstruct, 16, rotatedRect.size.height);
}

// PRIMITIVES
// gets int from buffer location
int hseg::utility::getIntFromBuffer(BYTE* buffer, int offset)
{
	int returnInt = *(int*)(buffer + offset);
	return returnInt;
}

float hseg::utility::getFloatFromBufferEx(BYTE* buffer, int* offset)
{
	float returnFloat = *(float*)(buffer + *offset);
	(*offset) += sizeof(float);
	return returnFloat;
}

// sets float to buffer location
void hseg::utility::setFloatToBuffer(BYTE* buffer, int offset, float value)
{
	BYTE* floatbuffer = reinterpret_cast<BYTE*>(&value);
	memcpy(buffer + offset, floatbuffer, 4);
}

void hseg::utility::setFloatToBufferEx(BYTE* buffer, int* offset, float value)
{
	BYTE* floatbuffer = reinterpret_cast<BYTE*>(&value);
	memcpy(buffer + *offset, floatbuffer, sizeof(float));
	(*offset) += sizeof(float);
}

// GEOMETRY
// sets the rotated rect to an upright angle (ie. head up)
cv::RotatedRect hseg::utility::resetRotatedRect(const cv::RotatedRect& rotatedRect, const std::vector<cv::Point2f>& points, int topIndex, int bottomIndex)
{
	float midline = getMidline(rotatedRect, points, topIndex, bottomIndex);
	return resetRotatedRect(rotatedRect, midline);
}

cv::RotatedRect hseg::utility::resetRotatedRect(const cv::RotatedRect& rotatedRect, float midline)
{
	std::vector<std::tuple<double, double, double>> compareAngles;
	for (double angle = -360.0; angle <= 360.0; angle += 90.0) {
		double modifiedAngle = rotatedRect.angle + angle;
		double difference = abs(modifiedAngle - midline);
		compareAngles.push_back(std::make_tuple(modifiedAngle, difference, fmod(angle + 360.0, 180.0)));
	}

	// sort ascending according to difference
	std::sort(compareAngles.begin(), compareAngles.end(), [](auto& left, auto& right) {
		return std::get<1>(left) < std::get<1>(right);
	});

	cv::RotatedRect resetRotatedRect = cv::RotatedRect(rotatedRect.center, (float)std::get<2>(compareAngles[0]) == 0.0 ? rotatedRect.size : cv::Size2f(rotatedRect.size.height, rotatedRect.size.width), (float)std::get<0>(compareAngles[0]));

	return resetRotatedRect;
}

// gets the midline angle of the rotated rect from its points
float hseg::utility::getMidline(const cv::RotatedRect& rotatedRect, const std::vector<cv::Point2f>& points, int topIndex, int bottomIndex)
{
	double approxAngle = 0.0;
	if (points[topIndex].x != points[bottomIndex].x) {
		approxAngle = fmod(((atan2(points[topIndex].y - points[bottomIndex].y, points[topIndex].x - points[bottomIndex].x) * 180 / M_PI) + 90.0 + 360.0 + 180.0), 360.0) - 180.0;
	}
	else {
		if (points[topIndex].y > points[bottomIndex].y) {
			approxAngle = 180.0;
		}
	}

	std::vector<std::pair<double, double>> compareAngles;
	for (double angle = -360.0; angle <= 360.0; angle += 90.0) {
		double modifiedAngle = rotatedRect.angle + angle;
		double difference = abs(modifiedAngle - approxAngle);
		compareAngles.push_back(std::make_pair(modifiedAngle, difference));
	}

	// sort ascending according to difference
	std::sort(compareAngles.begin(), compareAngles.end(), [](auto& left, auto& right) {
		return left.second < right.second;
	});

	return (float)compareAngles[0].first;
}

// gets distance between points
double hseg::utility::getdistance(const cv::Point2f& p1, const cv::Point2f& p2)
{
	return (double)sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
}

// gets mid-point of line
cv::Point2f hseg::utility::getMidPoint(const cv::Point2f& p1, const cv::Point2f& p2)
{
	return cv::Point2f((float)((p1.x + p2.x) / 2.0), (float)((p1.y + p2.y) / 2.0));
}

// gets center point of rectangle
cv::Point2f hseg::utility::getCenterPoint(const cv::Rect2f& rect)
{
	return cv::Point2f((float)(rect.x + ((rect.width - 1.0) / 2.0)), (float)(rect.y + ((rect.height - 1.0) / 2.0)));
}

// get intermediate point as a fraction from p1->p2
cv::Point2f hseg::utility::getInterPoint(const cv::Point2f& p1, const cv::Point2f& p2, float fraction)
{
	return cv::Point2f((float)(p1.x + ((p2.x - p1.x) * fraction)), (float)(p1.y + ((p2.y - p1.y) * fraction)));
}

// WATERSHED
int hseg::utility::allocWSNodes(std::vector<hseg::utility::WSNode>& storage)
{
	int sz = (int)storage.size();
	int newsz = MAX(128, sz * 3 / 2);

	storage.resize(newsz);
	if (sz == 0)
	{
		storage[0].next = 0;
		sz = 1;
	}
	for (int i = sz; i < newsz - 1; i++)
		storage[i].next = i + 1;
	storage[newsz - 1].next = 0;
	return sz;
}

//the modified version of watershed algorithm from OpenCV
void hseg::utility::watershedEx(const cv::Mat& src, cv::Mat& dst)
{
	// https://github.com/Seaball/watershed_with_mask

	// Labels for pixels
	const int IN_QUEUE = -2; // Pixel visited
	// possible bit values = 2^8
	const int NQ = 256;

	cv::Size size = src.size();
	int channel = src.channels();
	// Vector of every created node
	std::vector<WSNode> storage;
	int free_node = 0, node;
	// Priority queue of queues of nodes
	// from high priority (0) to low priority (255)
	WSQueue q[NQ];
	// Non-empty queue with highest priority
	int active_queue;
	int i, j;
	// Color differences
	int db, dg, dr;
	int subs_tab[513];

	// MAX(a,b) = b + MAX(a-b,0)
#define ws_max(a,b) ((b) + subs_tab[(a)-(b)+NQ])
	// MIN(a,b) = a - MAX(a-b,0)
#define ws_min(a,b) ((a) - subs_tab[(a)-(b)+NQ])

	// Create a new node with offsets mofs and iofs in queue idx
#define ws_push(idx,mofs,iofs)          \
        {                                       \
    if (!free_node)                    \
    free_node = allocWSNodes(storage); \
    node = free_node;                   \
    free_node = storage[free_node].next; \
    storage[node].next = 0;             \
    storage[node].mask_ofs = mofs;      \
    storage[node].img_ofs = iofs;       \
    if (q[idx].last)                   \
    storage[q[idx].last].next = node; \
    else                                \
  q[idx].first = node;            \
  q[idx].last = node;                 \
        }

	// Get next node from queue idx
#define ws_pop(idx,mofs,iofs)           \
        {                                       \
    node = q[idx].first;                \
    q[idx].first = storage[node].next;  \
    if (!storage[node].next)           \
    q[idx].last = 0;                \
    storage[node].next = free_node;     \
    free_node = node;                   \
    mofs = storage[node].mask_ofs;      \
    iofs = storage[node].img_ofs;       \
        }

	// Get highest absolute channel difference in diff
#define c_diff(ptr1,ptr2,diff)           \
        {                                        \
    db = std::abs((ptr1)[0] - (ptr2)[0]); \
    dg = std::abs((ptr1)[1] - (ptr2)[1]); \
    dr = std::abs((ptr1)[2] - (ptr2)[2]); \
    diff = ws_max(db, dg);                \
    diff = ws_max(diff, dr);              \
    assert(0 <= diff && diff <= 255);  \
        }

	//get absolute difference in diff
#define c_gray_diff(ptr1,ptr2,diff)		\
        {									\
    diff = std::abs((ptr1)[0] - (ptr2)[0]);	\
    assert(0 <= diff&&diff <= 255);		\
        }

	CV_Assert(src.type() == CV_8UC3 || src.type() == CV_8UC1 && dst.type() == CV_32SC1);
	CV_Assert(src.size() == dst.size());

	// Current pixel in input image
	const uchar* img = src.ptr();
	// Step size to next row in input image
	int istep = int(src.step / sizeof(img[0]));

	// Current pixel in mask image
	int* mask = dst.ptr<int>();
	// Step size to next row in mask image
	int mstep = int(dst.step / sizeof(mask[0]));

	for (i = 0; i < 256; i++)
		subs_tab[i] = 0;
	for (i = 256; i <= 512; i++)
		subs_tab[i] = i - 256;

	//for (j = 0; j < size.width; j++)
	//mask[j] = mask[j + mstep*(size.height - 1)] = 0;

	// initial phase: put all the neighbor pixels of each marker to the ordered queue -
	// determine the initial boundaries of the basins
	for (i = 1; i < size.height - 1; i++) {
		img += istep; mask += mstep;
		mask[0] = mask[size.width - 1] = 0; // boundary pixels

		for (j = 1; j < size.width - 1; j++) {
			int* m = mask + j;
			if (m[0] < 0)
				m[0] = 0;
			if (m[0] == 0 && (m[-1] > 0 || m[1] > 0 || m[-mstep] > 0 || m[mstep] > 0))
			{
				// Find smallest difference to adjacent markers
				const uchar* ptr = img + j * channel;
				int idx = 256, t;
				if (m[-1] > 0) {
					if (channel == 3) {
						c_diff(ptr, ptr - channel, idx);
					}
					else {
						c_gray_diff(ptr, ptr - channel, idx);
					}
				}
				if (m[1] > 0) {
					if (channel == 3) {
						c_diff(ptr, ptr + channel, t);
					}
					else {
						c_gray_diff(ptr, ptr + channel, t);
					}
					idx = ws_min(idx, t);
				}
				if (m[-mstep] > 0) {
					if (channel == 3) {
						c_diff(ptr, ptr - istep, t);
					}
					else {
						c_gray_diff(ptr, ptr - istep, t);
					}
					idx = ws_min(idx, t);
				}
				if (m[mstep] > 0) {
					if (channel == 3) {
						c_diff(ptr, ptr + istep, t);
					}
					else {
						c_gray_diff(ptr, ptr + istep, t);
					}
					idx = ws_min(idx, t);
				}

				// Add to according queue
				assert(0 <= idx && idx <= 255);
				ws_push(idx, i * mstep + j, i * istep + j * channel);
				m[0] = IN_QUEUE;//initial unvisited
			}
		}
	}
	// find the first non-empty queue
	for (i = 0; i < NQ; i++)
		if (q[i].first)
			break;

	// if there is no markers, exit immediately
	if (i == NQ)
		return;

	active_queue = i;//first non-empty priority queue
	img = src.ptr();
	mask = dst.ptr<int>();

	// recursively fill the basins
	for (;;)
	{
		int mofs, iofs;
		int lab = 0, t;
		int* m;
		const uchar* ptr;

		// Get non-empty queue with highest priority
		// Exit condition: empty priority queue
		if (q[active_queue].first == 0)
		{
			for (i = active_queue + 1; i < NQ; i++)
				if (q[i].first)
					break;
			if (i == NQ)
			{
				std::vector<WSNode>().swap(storage);
				break;
			}
			active_queue = i;
		}

		// Get next node
		ws_pop(active_queue, mofs, iofs);
		int top = 1, bottom = 1, left = 1, right = 1;
		if (0 <= mofs && mofs < mstep)//pixel on the top
			top = 0;
		if ((mofs % mstep) == 0)//pixel in the left column
			left = 0;
		if ((mofs + 1) % mstep == 0)//pixel in the right column
			right = 0;
		if (mstep * (size.height - 1) <= mofs && mofs < mstep * size.height)//pixel on the bottom
			bottom = 0;

		// Calculate pointer to current pixel in input and marker image
		m = mask + mofs;
		ptr = img + iofs;
		int diff, temp;
		// Check surrounding pixels for labels to determine label for current pixel
		if (left) {//the left point can be visited
			t = m[-1];
			if (t > 0) {
				lab = t;
				if (channel == 3) {
					c_diff(ptr, ptr - channel, diff);
				}
				else {
					c_gray_diff(ptr, ptr - channel, diff);
				}
			}
		}
		if (right) {// Right point can be visited
			t = m[1];
			if (t > 0) {
				if (lab == 0) {//and this point didn't be labeled before
					lab = t;
					if (channel == 3) {
						c_diff(ptr, ptr + channel, diff);
					}
					else {
						c_gray_diff(ptr, ptr + channel, diff);
					}
				}
				else if (t != lab) {
					if (channel == 3) {
						c_diff(ptr, ptr + channel, temp);
					}
					else {
						c_gray_diff(ptr, ptr + channel, temp);
					}
					diff = ws_min(diff, temp);
					if (diff == temp)
						lab = t;
				}
			}
		}
		if (top) {
			t = m[-mstep]; // Top
			if (t > 0) {
				if (lab == 0) {//and this point didn't be labeled before
					lab = t;
					if (channel == 3) {
						c_diff(ptr, ptr - istep, diff);
					}
					else {
						c_gray_diff(ptr, ptr - istep, diff);
					}
				}
				else if (t != lab) {
					if (channel == 3) {
						c_diff(ptr, ptr - istep, temp);
					}
					else {
						c_gray_diff(ptr, ptr - istep, temp);
					}
					diff = ws_min(diff, temp);
					if (diff == temp)
						lab = t;
				}
			}
		}
		if (bottom) {
			t = m[mstep]; // Bottom
			if (t > 0) {
				if (lab == 0) {
					lab = t;
				}
				else if (t != lab) {
					if (channel == 3) {
						c_diff(ptr, ptr + istep, temp);
					}
					else {
						c_gray_diff(ptr, ptr + istep, temp);
					}
					diff = ws_min(diff, temp);
					if (diff == temp)
						lab = t;
				}
			}
		}
		// Set label to current pixel in marker image
		assert(lab != 0);//lab must be labeled with a nonzero number
		m[0] = lab;

		// Add adjacent, unlabeled pixels to corresponding queue
		if (left) {
			if (m[-1] == 0)//left pixel with marker 0
			{
				if (channel == 3) {
					c_diff(ptr, ptr - channel, t);
				}
				else {
					c_gray_diff(ptr, ptr - channel, t);
				}
				ws_push(t, mofs - 1, iofs - channel);
				active_queue = ws_min(active_queue, t);
				m[-1] = IN_QUEUE;
			}
		}

		if (right)
		{
			if (m[1] == 0)//right pixel with marker 0
			{
				if (channel == 3) {
					c_diff(ptr, ptr + channel, t);
				}
				else {
					c_gray_diff(ptr, ptr + channel, t);
				}
				ws_push(t, mofs + 1, iofs + channel);
				active_queue = ws_min(active_queue, t);
				m[1] = IN_QUEUE;
			}
		}

		if (top)
		{
			if (m[-mstep] == 0)//top pixel with marker 0
			{
				if (channel == 3) {
					c_diff(ptr, ptr - istep, t);
				}
				else {
					c_gray_diff(ptr, ptr - istep, t);
				}
				ws_push(t, mofs - mstep, iofs - istep);
				active_queue = ws_min(active_queue, t);
				m[-mstep] = IN_QUEUE;
			}
		}

		if (bottom) {
			if (m[mstep] == 0)//down pixel with marker 0
			{
				if (channel == 3) {
					c_diff(ptr, ptr + istep, t);
				}
				else {
					c_gray_diff(ptr, ptr + istep, t);
				}
				ws_push(t, mofs + mstep, iofs + istep);
				active_queue = ws_min(active_queue, t);
				m[mstep] = IN_QUEUE;
			}
		}
	}
}
