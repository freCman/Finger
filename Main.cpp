#include"Header.h"

int main(){
	//capture = cvCreateCameraCapture(0);
	if(!capture){
		printf("Failure\n");
		return -1;
	}
	int pp;
	_beginthread(handCapture, 0, capture);
//	fingerCapture(capture);
	_beginthread(runningCapture,0,capture);
	_sleep(100000);
	cvReleaseCapture(&capture);
	
	return 0;
}