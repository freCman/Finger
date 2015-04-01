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
		cvAbsDiff(buffer[prev], buffer[curr], silh);		//������ ������ ���� -> �� �̵� �����͸� silh�� �����Ѵ�.
		cvThreshold(silh, silh, threshold, 255, 0);			//ȭ�Ұ� 50�̻� ���̳��� ���� ���� ���, �������� ���������� ó���Ѵ�.
		timeStamp  = (double) clock()/CLOCKS_PER_SEC;		
		cvUpdateMotionHistory(silh, mhi, timeStamp, MHI_DURATION);	//silh�� timeStamp, MHI_DURATION�� �̿��Ͽ� mhi�� ����.


		// convert MHI to blue 8u image				//�Ķ� 8��Ʈ ���� �̹����� �ٲ�.
		// mapping: [timeStamp -  MHI_DURATION, timeStamp]  ->  [0, 255] 
		double scale = 255./MHI_DURATION;				
		double t = MHI_DURATION - timeStamp;
		cvScale(mhi, mask, scale, t*scale);		//mhi�� 255./1.0���� �����ϸ��ϰ� mask�� ����. 0~255 ������.
		cvZero(motion);							
		cvMerge(mask, 0,0,0,motion);			//�ƹ�ư �������� scaling �Ѱ� motion�� ��


		cvCalcMotionGradient(mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3);		//mhi���� ���� ��� ��ȿ�� ȭ�Ҹ� mask�� ����. ����ũ ũ�� 3

		cvClearMemStorage(storage);					//�޸� ����.
		seq = cvSegmentMotion(mhi, segmask, storage, timeStamp, MAX_TIME_DELTA);		//mhi�� ���� �̵��ϴ� ��ü�� ����, ��ȯ���� seq�� ����. 

		cvCopy(frame, dstImage);

		if(seq->total > 0){
		int i, x, y;
		int count;
		CvRect comp_rect;
		CvScalar color;
		CvPoint center;
		double r, angle;
		CvSize size = cvGetSize(dstImage);
		for( i = -1; i < seq->total; i++ ){						//�����̴� ��ü���� ���� ������ �ݺ���
		if( i < 0 ){ // case of the whole image				//i == -1 

		comp_rect = cvRect( 0, 0, size.width, size.height );	// Rect�� �ʱ�ȭ.
		color = CV_RGB(0, 0, 255);						//�Ķ�������.
		r = 100;
		}
		else{ // i-th motion component 
		comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;	//������ ��ü�� �����ϴ� sep�� i���� index�� �����ͼ� 
		// reject very small components
		if( comp_rect.width*comp_rect.height < 100 )			//�� �����°� �ǰ� ������ 
		continue;			//�Ѿ��
		color = CV_RGB(255,0,0);			//�������� ����
		r = 30;
		}
		// select component ROI
		cvSetImageROI( silh, comp_rect );		//ROI �Ҵ�.
		cvSetImageROI( mhi, comp_rect );
		cvSetImageROI( orient, comp_rect );
		cvSetImageROI( mask, comp_rect );
		// calculate orientation
		angle = cvCalcGlobalOrientation( orient, mask, mhi,  timeStamp, MHI_DURATION);		//������ ������ ����Ͽ� ��ȯ
		// adjust for images with top-left origin
		angle = 360.0 - angle;					//���� ����� ������ ���� ���� ���� ����.
		count = cvNorm(silh, NULL, CV_L1, NULL );	// silh�� V_L1 ���� ���
		//		count = cvCountNonZero(silh);

		cvResetImageROI( mhi );
		cvResetImageROI( orient );
		cvResetImageROI( mask );
		cvResetImageROI( silh );

		// check for the case of little motion
		if( count < comp_rect.width*comp_rect.height * 0.001 )		//���� ������ ó�� ����.
		continue;

		// draw a clock with arrow indicating the direction
		center = cvPoint( (comp_rect.x + comp_rect.width/2),			//ROI������ ���
		(comp_rect.y + comp_rect.height/2) );

		cvCircle( dstImage, center, cvRound(r*1.2), color, 3, CV_AA, 0 );	//�� �׸�.
		x = cvRound( center.x + r*cos(angle*CV_PI/180));			//value�� �ݿø� �Ͽ� ������ ��ȯ.
		y = cvRound( center.y - r*sin(angle*CV_PI/180));
		cvLine( dstImage, center, cvPoint(x, y), color, 3, CV_AA, 0 );	//center���� cvPint(x, y) ���� ���� �׸���.
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
