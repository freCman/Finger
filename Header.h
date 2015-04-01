#ifndef HEADER_H
#define HEADER_H
#include<opencv/cv.h>
#include<opencv/highgui.h>
#include<stdio.h>
#include<time.h>
#include <process.h>
#include<math.h>
#include<Windows.h>
const double MHI_DURATION = 1;
const double MAX_TIME_DELTA = 0.5;
const double MIN_TIME_DELTA = 0.05;
static CvCapture* capture = cvCreateCameraCapture(0);
static CvPoint point;
CvSize getFrameSize(CvCapture* capture);
static IplImage* testImage = cvCreateImage(getFrameSize(capture), IPL_DEPTH_8U, 3);
void DrawMotionOrientation(CvSeq* seq, IplImage* silh, IplImage* mhi,		   
			               IplImage* orient, IplImage* mask, 
			               IplImage* dstImage, double timeStamp, CvRect rect);
void drawKeyboardROI(CvRect rect, IplImage* dstImage );
void runningCapture(void* capture1);
void fingerCapture(CvCapture* capture);
void handCapture(void* capture1);
void drawRect(CvPoint &pt1, CvPoint &pt2, CvRect rect);
#endif