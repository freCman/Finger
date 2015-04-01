#include"Header.h"
void runningCapture(void* capture1){
	//CvCapture *capture = (CvCapture*)capture1;
	CvSize size = getFrameSize(capture);
	IplImage *frame = NULL;
	IplImage *img = cvCreateImage(size, IPL_DEPTH_8U, 3);
	IplImage *hsvImage = cvCreateImage(size, IPL_DEPTH_8U,3);
	IplImage *hImage = cvCreateImage(size, IPL_DEPTH_8U,1);
	IplImage *mask = cvCreateImage(size, IPL_DEPTH_8U,1);
	IplImage *backProject = cvCreateImage(size, IPL_DEPTH_8U,1);
	IplImage *dstImage = cvCreateImage(size, IPL_DEPTH_8U,3);
	IplImage *cannyImage = cvCreateImage(size, IPL_DEPTH_8U, 1);
	CvRect keyboard = cvRect(20,size.height/2, size.width-60, (size.height-60)/2);
	int correctionValue = 3;
	int histSize = 8;
	float valueRange[] = {0,200};
	float* ranges[] = {valueRange};
	int z = 0;
	CvHistogram *pHist = cvCreateHist(1, &histSize, CV_HIST_ARRAY, ranges);

	CvConnectedComp keyTrackComp;
	CvBox2D keyTrackBox;
	CvRect track_window;
	CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_EPS|CV_TERMCRIT_ITER, 10, 5);
	int t = 1;
	float max_val;
	CvPoint pt1,pt2;
	int k, hValue;
	int pt1_x,pt1_y, pt2_x, pt2_y;

	while(100){
		frame = cvQueryFrame(capture);
		if(!frame)
			break;

		cvFlip(frame, frame,1);
		cvCopy(frame, dstImage);
		cvCvtColor(frame, hsvImage, CV_BGR2HSV);

		cvSetImageROI(dstImage, keyboard);
		cvSetImageROI(hsvImage, keyboard);
		cvSetImageROI(mask, keyboard);
		cvSetImageROI(hImage, keyboard);
		int vmin = 0, vmax = 120, smin = 0;
		cvInRangeS(hsvImage, cvScalar(0,smin, MIN(vmin, vmax)), cvScalar(200, 200, MAX(vmin, vmax)), mask);
		cvSplit(hsvImage, hImage, 0,0,0);
		cvResetImageROI(hsvImage);
		cvResetImageROI(mask);
		cvResetImageROI(hImage);
		if(t == 1){
			cvSetImageROI(hImage, keyboard);
			cvSetImageROI(mask, keyboard);
			cvCalcHist(&hImage, pHist, 0 , mask);

			cvResetImageROI(mask);
			cvResetImageROI(hImage);
			track_window = keyboard;
			t++;
		}
		cvCalcBackProject(&hImage, backProject, pHist);
		cvAnd(backProject, mask, backProject);
		
		cvDilate(backProject,backProject,0,5);
		cvErode(backProject, backProject, 0,5);
		
		
		cvAndS(backProject, cvScalar(255,255,255), backProject);
		cvCamShift(backProject, track_window, criteria, &keyTrackComp, &keyTrackBox);

		point.x = keyTrackBox.center.x;
		point.y = keyTrackBox.center.y;
		pt1_x = keyTrackBox.size.height/2+correctionValue;
		pt1_y = keyTrackBox.size.width/2+correctionValue;
		pt2_x = keyTrackBox.size.height/2-correctionValue;
		pt2_y = keyTrackBox.size.width/2-correctionValue;
		pt1 = cvPoint(keyTrackBox.center.x - pt1_x, keyTrackBox.center.y - pt1_y);
		pt2 = cvPoint(keyTrackBox.center.x + pt2_x, keyTrackBox.center.y + pt2_y);


		track_window = keyTrackComp.rect;
		cvResetImageROI(dstImage);
		cvCopy(frame, testImage);
		cvRectangle(testImage, pt1, pt2, CV_RGB(0,0,255), 2);
		//cvEllipseBox(dstImage, keyTrackBox, CV_RGB(255,0,0), 3, CV_AA, 0);

		char c = cvWaitKey(1);
		if(c == 27)
			break;

		cvShowImage("test", testImage);
		//cvShowImage("hsvImage", hsvImage);
		//cvShowImage("mask", mask);
		//cvShowImage("back", backProject);
		//cvShowImage("hImage", hImage);

	}



	cvDestroyAllWindows();
	//cvReleaseCapture(&capture);
	cvReleaseImage(&img);
	cvReleaseImage(&hsvImage);
	cvReleaseImage(&hImage);
	cvReleaseImage(&dstImage);
	cvReleaseImage(&backProject);
	cvReleaseImage(&mask);
	cvReleaseHist(&pHist);
	_endthread();
}