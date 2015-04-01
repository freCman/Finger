#include"Header.h"
void handCapture(void* capture1){
	//CvCapture* capture = (CvCapture*)capture1;
	CvSize size = getFrameSize(capture);
	IplImage* frame = NULL;
	IplImage* mask = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage* dstImage = cvCreateImage(size, IPL_DEPTH_8U, 3);
	IplImage* CrImage = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage* tmpImage = cvCreateImage(size, IPL_DEPTH_8U, 3);
	IplImage* backPro = cvCreateImage(size, IPL_DEPTH_8U, 1);
	CvRect rect = cvRect(size.width/2,size.height/3,(size.width-40)/2, size.height-40);
	CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_EPS|CV_TERMCRIT_ITER, 10, 5);
	IplImage* silh = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage* mhi = cvCreateImage(size, IPL_DEPTH_32F, 1);
	IplImage* buffer[4];
	IplImage* motion = cvCreateImage(size, IPL_DEPTH_8U, 3);
	IplImage* moveMask = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage* segmask = cvCreateImage(size, IPL_DEPTH_32F, 1);
	IplImage* orient = cvCreateImage(size, IPL_DEPTH_32F, 1);


	int curr,prev, last = 0;
	double timeStamp, threshold = 50;
	for(int i = 0 ; i < 4 ; i++)
		buffer[i] = cvCreateImage(size, IPL_DEPTH_8U, 1);

	float max;
	int histSize = 8;
	float valueRange[] = {100,180};
	float* ranges[] = {valueRange};
	CvHistogram *pHist = cvCreateHist(1, &histSize, CV_HIST_ARRAY, ranges, 1); 
	int k = 1,hValue;
	int t = 0;

	CvSeq* seq = NULL;
	CvMemStorage* storage = cvCreateMemStorage(0);

	CvPoint pt1,pt2;
	CvConnectedComp handTracking;
	CvRect trackRect;
	while(1){
		frame = cvQueryFrame(capture);
		if(!frame)
			break;
		cvFlip(frame, frame, 1);
		cvCopy(frame, dstImage);
		cvCopy(frame, testImage);
		cvCvtColor(frame, tmpImage, CV_BGR2YCrCb);
		cvInRangeS(tmpImage, cvScalar(0,133,77), cvScalar(255,173,127),mask);
		cvSplit(tmpImage, 0, CrImage,0,0);

		if(k < 10){
			cvSetImageROI(CrImage,rect);

			cvCalcHist(&CrImage, pHist, 0);
			cvGetMinMaxHistValue(pHist, 0, &max, 0,0);
			cvScale(pHist->bins, pHist->bins, max? 255/max : 0);
			cvResetImageROI(CrImage);
			k++;
			trackRect = rect;
		}

		cvCalcBackProject(&CrImage, backPro, pHist);
		cvAnd(backPro, mask, backPro);

		cvDilate(backPro,backPro,0,3);
		cvErode(backPro, backPro, 0,3);
		cvAndS(backPro, cvScalar(180,180,180), backPro);

		cvMeanShift(backPro, trackRect, criteria , &handTracking);
		trackRect = handTracking.rect;

		drawRect(pt1, pt2, trackRect);
		cvRectangle(testImage, pt1, pt2, CV_RGB(0,255,0), 2);

		cvCvtColor(frame, buffer[last],CV_BGR2GRAY);


		for(int i = 0 ; i < 4 ; i++)
			cvSetImageROI(buffer[i], trackRect);
		cvSetImageROI(silh, trackRect);
		cvSetImageROI(mhi, trackRect);
		cvSetImageROI(moveMask, trackRect);
		cvSetImageROI(motion, trackRect);
		cvSetImageROI(segmask, trackRect);
		cvSetImageROI(orient, trackRect);
		curr = last;
		prev = (curr+1) %4;
		last = prev;
		silh = buffer[prev];


		cvAbsDiff(buffer[prev], buffer[curr], silh);
		cvThreshold(silh, silh, threshold, 255, 0);
		timeStamp = (double) clock() / CLOCKS_PER_SEC;


		cvUpdateMotionHistory(silh, mhi, timeStamp, MHI_DURATION);	//silh과 timeStamp, MHI_DURATION을 이용하여 mhi를 갱신.
		double scale  = 255./MHI_DURATION;
		double t = MHI_DURATION - timeStamp;
		cvScale(mhi, moveMask, scale, t*scale);
		cvZero(motion);
		cvMerge(moveMask, 0,0,0, motion);


		cvCalcMotionGradient(mhi, moveMask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3);

		cvClearMemStorage(storage);
		seq = cvSegmentMotion(mhi, segmask, storage, timeStamp, MAX_TIME_DELTA);
		for(int i = 0 ; i < 4 ; i++)
			cvResetImageROI(buffer[i]);
		cvResetImageROI(silh);
		cvResetImageROI(mhi);
		cvResetImageROI(moveMask);
		cvResetImageROI(motion);
		cvResetImageROI(segmask);
		cvResetImageROI(orient);
		if(seq->total > 0)
			DrawMotionOrientation(seq, silh, mhi, orient,	moveMask, testImage, timeStamp,trackRect);

		char c = cvWaitKey(1);
		if(c == 27)
			break;


		cvShowImage("test", testImage);
	//	cvShowImage("silh", silh);
	//	cvShowImage("motion", motion);
	//	cvShowImage("dstImage", dstImage);
	//	cvShowImage("back", backPro);
		cvZero(motion);

	}
	
	cvReleaseImage(&mask);
	cvReleaseImage(&dstImage);
	cvReleaseHist(&pHist);
	_endthread();

}
void drawRect(CvPoint &pt1, CvPoint &pt2, CvRect rect){
	pt1 = cvPoint(rect.x,rect.y);
	pt2 = cvPoint(pt1.x + rect.width, pt1.y + rect.height);

}
void DrawMotionOrientation(CvSeq* seq, IplImage* silh, IplImage* mhi,IplImage* orient, IplImage* mask, IplImage* dstImage, double timeStamp, CvRect rect)
{
	int i, x, y;
	int count;
	CvRect comp_rect;
	CvScalar color;
	CvPoint center;
	double r, angle;
	CvSize size = cvGetSize(dstImage);
	int d;
	for( i = -1; i < seq->total; i++ )						//움직이는 물체보다 작을 때까지 반복문
	{
		if( i < 0 ) // case of the whole image				//i == -1
		{ 
			comp_rect = cvRect( rect.x, rect.y, rect.width, rect.height );	// Rect을 초기화.
			color = CV_RGB(0, 0, 255);						//파랑색으로.
			r = 50;
		}
		else // i-th motion component 
		{ 
			comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;	//sep의 i번쨰 index를 가져와서 
			// reject very small components
			comp_rect.x = comp_rect.x + rect.x;
			comp_rect.y = comp_rect.y + rect.y;
			if( comp_rect.width*comp_rect.height < 200 )			//그 가져온게 되게 작으면 
				continue;			//넘어가고
			color = CV_RGB(255,0,0);			//빨강으로 지정
			r = 30;
		}
		// select component ROI
		cvSetImageROI( silh, comp_rect );		//ROI 할당.
		cvSetImageROI( mhi, comp_rect );
		cvSetImageROI( orient, comp_rect );
		cvSetImageROI( mask, comp_rect );
		// calculate orientation
		angle = cvCalcGlobalOrientation( orient, mask, mhi,  timeStamp, MHI_DURATION);		//움직인 각도를 계산하여 반환
		// adjust for images with top-left origin
		angle = 360.0 - angle;					//윈쪽 상단이 원점인 영상에 대해 각도 조정.
		count = cvNorm(silh, NULL, CV_L1, NULL );	// silh의 V_L1 놈을 계산

		cvResetImageROI( mhi );
		cvResetImageROI( orient );
		cvResetImageROI( mask );
		cvResetImageROI( silh );

		// check for the case of little motion
		if( count < comp_rect.width*comp_rect.height * 0.001 )		//작은 움직임 처리 안함.
			continue;

		// draw a clock with arrow indicating the direction
		center = cvPoint( ( comp_rect.x + comp_rect.width/2),			//ROI영역의 가운데
			( comp_rect.y + comp_rect.height/2) );

		cvCircle( dstImage, center, cvRound(r*1.2), color, 3, CV_AA, 0 );	//원 그림.
		x = cvRound( center.x + r*cos(angle*CV_PI/180));			//value를 반올림 하여 정수로 반환.
		y = cvRound( center.y - r*sin(angle*CV_PI/180));
		cvLine( dstImage, center, cvPoint(x, y), color, 3, CV_AA, 0 );	//center에서 cvPint(x, y) 까지 선을 그린다.
		if(angle>240 && angle < 300){
			int a = abs(point.x - center.x) -360;
			int b = abs(point.y - center.y)-360;
			if( a < 50 && b < 50){
				//printf("x = %d\n", abs(point.x - center.x)-360);
				//printf("y = %d\n", abs(point.y - center.y)-360);
				printf("key event\n");
				keybd_event('A',0,0,0);
			}
		}
	}
}