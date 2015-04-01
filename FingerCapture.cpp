#include"Header.h"
void fingerCapture(CvCapture* capture){
	CvSize size = getFrameSize(capture);

	IplImage* motion = cvCreateImage(size, 8, 3);
	IplImage *frame = NULL;
	IplImage *mask = cvCreateImage(size, IPL_DEPTH_8U,1);
	IplImage *segmask = cvCreateImage(size, IPL_DEPTH_32F,1);
	IplImage *mhi = cvCreateImage(size, IPL_DEPTH_32F, 1);
	IplImage *orient = cvCreateImage(size, IPL_DEPTH_32F, 1);
	IplImage *dstImage = cvCreateImage(size, IPL_DEPTH_8U, 3);
	IplImage *handImage = cvCreateImage(size, IPL_DEPTH_8U, 3);
	IplImage *hImage = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage *baseImage = cvCreateImage(size, IPL_DEPTH_8U, 3);
	IplImage *baseImage2 = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage *cannyImage = cvCreateImage(size, IPL_DEPTH_8U, 1);
	IplImage *tmp = cvCreateImage(size, IPL_DEPTH_8U, 1);
	CvRect rect = cvRect(20, size.height/2, size.width-60, (size.height-60)/2); 
	CvPoint pt1,pt2;

	CvSeq* seq = NULL;
	CvMemStorage* storage = cvCreateMemStorage(0);
	IplImage *buffer[4];
	IplImage* silh;// = cvCreateImage(size, IPL_DEPTH_8U, 1);
	
	int y = 0 ;
	double threshold = 50;
	double timeStamp;
	int last = 0;
	int prev, curr;
	for(int i = 0 ; i < 4 ; i++){
		buffer[i] = cvCreateImage(size, IPL_DEPTH_8U, 1);
		cvZero(buffer[i]);
	}
	while(1){
		frame = cvQueryFrame(capture);
		if(!frame)
			break;
		
		cvCvtColor(frame, buffer[last],CV_BGR2GRAY);
		curr = last;
		prev = (curr+1) % 4;
		last = prev;
		silh = buffer[prev];
		cvAbsDiff(buffer[prev], buffer[curr], silh);		//이전과 현재의 차이 -> 즉 이동 데이터를 silh에 저장한다.
		cvThreshold(silh, silh, threshold, 255, 0);			//화소가 50이상 차이나는 영상에 대해 흰색, 나머지는 검은색으로 처리한다.
		timeStamp  = (double) clock()/CLOCKS_PER_SEC;		
		cvUpdateMotionHistory(silh, mhi, timeStamp, MHI_DURATION);	//silh과 timeStamp, MHI_DURATION을 이용하여 mhi를 갱신.


		// convert MHI to blue 8u image				//파란 8비트 정수 이미지로 바꿈.
		// mapping: [timeStamp -  MHI_DURATION, timeStamp]  ->  [0, 255] 
		double scale = 255./MHI_DURATION;				
		double t = MHI_DURATION - timeStamp;
		cvScale(mhi, mask, scale, t*scale);		//mhi를 255./1.0으로 스케일링하고 mask에 저장. 0~255 값으로.
		cvZero(motion);							
		cvMerge(mask, 0,0,0,motion);			//아무튼 여차저차 scaling 한게 motion에 들어감


		cvCalcMotionGradient(mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3);		//mhi에서 각도 계산 유효한 화소를 mask에 저장. 마스크 크기 3

		cvClearMemStorage(storage);					//메모리 비우기.
		seq = cvSegmentMotion(mhi, segmask, storage, timeStamp, MAX_TIME_DELTA);		//mhi를 통해 이동하는 물체를 분할, 반환값을 seq에 저장. 

		cvCopy(frame, dstImage);

		if(seq->total > 0){
		int i, x, y;
		int count;
		CvRect comp_rect;
		CvScalar color;
		CvPoint center;
		double r, angle;
		CvSize size = cvGetSize(dstImage);
		for( i = -1; i < seq->total; i++ ){						//움직이는 물체보다 작을 때까지 반복문
		if( i < 0 ){ // case of the whole image				//i == -1 

		comp_rect = cvRect( 0, 0, size.width, size.height );	// Rect을 초기화.
		color = CV_RGB(0, 0, 255);						//파랑색으로.
		r = 100;
		}
		else{ // i-th motion component 
		comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;	//움직인 물체를 저장하는 sep의 i번쨰 index를 가져와서 
		// reject very small components
		if( comp_rect.width*comp_rect.height < 100 )			//그 가져온게 되게 작으면 
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
		//		count = cvCountNonZero(silh);

		cvResetImageROI( mhi );
		cvResetImageROI( orient );
		cvResetImageROI( mask );
		cvResetImageROI( silh );

		// check for the case of little motion
		if( count < comp_rect.width*comp_rect.height * 0.001 )		//작은 움직임 처리 안함.
		continue;

		// draw a clock with arrow indicating the direction
		center = cvPoint( (comp_rect.x + comp_rect.width/2),			//ROI영역의 가운데
		(comp_rect.y + comp_rect.height/2) );

		cvCircle( dstImage, center, cvRound(r*1.2), color, 3, CV_AA, 0 );	//원 그림.
		x = cvRound( center.x + r*cos(angle*CV_PI/180));			//value를 반올림 하여 정수로 반환.
		y = cvRound( center.y - r*sin(angle*CV_PI/180));
		cvLine( dstImage, center, cvPoint(x, y), color, 3, CV_AA, 0 );	//center에서 cvPint(x, y) 까지 선을 그린다.
		}


		}

		char key = cvWaitKey(20);
		if(key ==  27)
			break;
		//cvShowImage("frame", frame);
		//cvShowImage("mask", mask);
		cvShowImage("dstImage", dstImage);
		cvShowImage("motion", motion);
	//	cvShowImage("mhi", mhi);
	//	cvShowImage("seq", segmask);	
	}
	for(int i = 0 ; i < 4 ; i++){
		cvReleaseImage(&buffer[i]);
	}
	cvReleaseImage(&mhi);
	cvReleaseImage(&orient);
	cvReleaseImage(&mask);
	cvReleaseImage(&segmask);
	cvReleaseImage(&dstImage);
}
