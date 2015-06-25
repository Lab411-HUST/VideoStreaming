#include <jni.h>
#include <iostream>
//#include "cv.h"
//#include "cxcore.h"
//#include "highgui.h"
//#include <opencv2/core/core.hpp>
//#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.hpp>
#include <opencv/cxcore.hpp>
#include "bmpfmt.h"
//#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/calib3d/calib3d.hpp>
//#include <opencv2/imgproc/imgproc_c.h>
#include <math.h>
#include <errno.h>
#include <android/log.h>
//#include "curl/curl.h"
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#define vitri0 0
#define vitri1 1
#define vitri2 2
#define vitri3 3
#define vitri4 4
#define vitri5 5
#define vitri6 6
#define vitri7 7
#define vitri8 8
#define vitri9 9
#define  LOG_TAG    "opencv"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
using namespace std;
using namespace cv;

extern "C" 
{

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
int server_socket,client_socket;
void* sendurl(void* arg);
void quit(char* mesg,int inter);

int vitrihientai=5;
int vitriguive=5;
int flag,flag1;
int LOW_THRESHOLD =75;
int MIN_PIXELS = 100;
const int N = 4;
IplImage **buf = 0;
IplImage *mhi = 0;
CvCapture* camera;
int last = 0;
CvPoint COGpoint;
CvPoint prevpoint;
//IplImage *prevImg,*currtImg;
void centermotion(IplImage* currtFrame)
{
        LOGE("centermotion");
	if(currtFrame==NULL)
			{
				printf("mat");
				//cvWaitKey(10);
				return;

			}
	CvSize size = cvSize(currtFrame->width,currtFrame->height); // get current frame size
    int i, idx1 = last, idx2;
    IplImage* diffImg;
	// if( !mhi || mhi->width != size.width || mhi->height != size.height ) {
        if( buf == 0 ) {
            buf = (IplImage**)malloc(N*sizeof(buf[0]));
            memset( buf, 0, N*sizeof(buf[0]));
        }

        for( i = 0; i < N; i++ ) {
            cvReleaseImage( &buf[i] );
            buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
            cvZero( buf[i] );
        }
        //cvReleaseImage( &mhi );
      //  mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
       // cvZero( mhi ); // clear MHI at the beginning
    //  }
	 cvCvtColor(currtFrame, buf[last], CV_BGR2GRAY ); // convert frame to grayscale

    idx2 = (last + 1) % N; // index of (last - (N-1))th frame
    last = idx2;

    diffImg = buf[idx2];
    cvAbsDiff(buf[idx1], buf[idx2],diffImg ); // get difference between frames


	       // if(currtImg!=NULL)
	       // {

			//prevImg=currtImg;
			//IplImage* temimage=cvCreateImage(cvGetSize(currtFrame),IPL_DEPTH_8U,1);
	       // currtImg=cvCreateImage(cvGetSize(currtFrame),IPL_DEPTH_8U,1);
	        //}
			//currtImg=temimage;
	        //cvCvtColor(currtFrame,currtImg,CV_RGB2GRAY);
			//cvAbsDiff(currtImg,prevImg,diffImg);
			cvThreshold(diffImg,diffImg,LOW_THRESHOLD,255,CV_THRESH_BINARY);
			//CvPoint poin=findCOG(diffImg);
			int numberpixel=cvCountNonZero(diffImg);
			//printf("%d  ",numberpixel);
					if(numberpixel>MIN_PIXELS)
					{
						//printf("vao day");
						CvMoments* moment=new CvMoments();
						cvMoments(diffImg,moment,1);
						//double m00 = cvGetSpatialMoment(moment,0,0) ;
			           // double m10 = cvGetSpatialMoment(moment,1,0) ;
			            //double m01 = cvGetSpatialMoment(moment,0,1);
						double m00 =moment->m00;
					    double m10 =moment->m10;
					    double m01 =moment->m01;
						if(m00!=0)
						{
							int xcenter=(int)(m10/m00+0.5);
							int ycenter=(int)(m01/m00+0.5);
							COGpoint.x=xcenter;
							COGpoint.y=ycenter;
						}
						else
						{
							COGpoint.x=160;
							COGpoint.y=120;
						}
					}
					else
					{
						COGpoint.x=160;
						COGpoint.y=120;
					}
LOGI("X:%d,Y:%d",COGpoint.x,COGpoint.y);

}


//JNIEXPORT jbooleanArray JNICALL Java_bavuvan_motion_MainActivity_getSourceImage(
//		JNIEnv* env, jobject thiz, ) {
//	if (pImage == NULL) {
//		LOGE("No source image.");
//		return 0;
//	}
//	cvFlip(pImage);
//	int width = pImage->width;
//	int height = pImage->height;
//	int rowStep = pImage->widthStep;
//	int headerSize = 54;
//	int imageSize = rowStep * height;
//	int fileSize = headerSize + imageSize;
//	unsigned char* image = new unsigned char[fileSize];
//	struct bmpfile_header* fileHeader = (struct bmpfile_header*) (image);
//	fileHeader->magic[0] = 'B';
//	fileHeader->magic[1] = 'M';
//	fileHeader->filesz = fileSize;
//	fileHeader->creator1 = 0;
//	fileHeader->creator2 = 0;
//	fileHeader->bmp_offset = 54;
//	struct bmp_dib_v3_header_t* imageHeader =
//			(struct bmp_dib_v3_header_t*) (image + 14);
//	imageHeader->header_sz = 40;
//	imageHeader->width = width;
//	imageHeader->height = height;
//	imageHeader->nplanes = 1;
//	imageHeader->bitspp = 24;
//	imageHeader->compress_type = 0;
//	imageHeader->bmp_bytesz = imageSize;
//	imageHeader->hres = 0;
//	imageHeader->vres = 0;
//	imageHeader->ncolors = 0;
//	imageHeader->nimpcolors = 0;
//	memcpy(image + 54, pImage->imageData, imageSize);
//	jbooleanArray bytes = env->NewBooleanArray(fileSize);
//	if (bytes == 0) {
//		LOGE("Error in creating the image.");
//		delete[] image;
//		return 0;
//	}
//	env->SetBooleanArrayRegion(bytes, 0, fileSize, (jboolean*) image);
//	delete[] image;
//	return bytes;
//}


void JNICALL Java_com_example_opencv_Motiondetect_Start(JNIEnv *env, jobject obj)
//int main()
{
      LOGE("bat dau chay");
	//pthread_t thread_s;
	int key;
	//CURL *curl;
	//CURLcode res;
	//curl = curl_easy_init();
	prevpoint.x=160;
	prevpoint.y=120;
	flag=0;
	flag1=0;

        LOGE("bat dau chay adad");
	IplImage* img= cvLoadImage("/data/image.jpg");
        LOGE("bat dau chay hjagydw");
        if(img==NULL)
       {
        LOGE("k doc duoc");
	//cvWaitKey(30);
        return;
       }
	centermotion(img);
	//if(COGpoint.x!=160)
	//cvCircle(img,COGpoint,10,CV_RGB(255,0,0),1);



/*
	if((COGpoint.x-prevpoint.x)>200)
	          {
	        	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.11:12345/cgi-bin/camctrl/camctrl.cgi?move=right");
	        	  res = curl_easy_perform(curl);
	        	  if((COGpoint.y-prevpoint.y)>250)
	        	  {
	        		  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.11:12345/cgi-bin/camctrl/camctrl.cgi?move=up");
	        		  res = curl_easy_perform(curl);
	        	  }
	        	  if((COGpoint.y-prevpoint.y)<-250)
	        	   {
	        	          		  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.11:12345/cgi-bin/camctrl/camctrl.cgi?move=down");
	        	          		  res = curl_easy_perform(curl);
	        	   }

	          }
	          if((COGpoint.x-prevpoint.x)<-200)
	                    {
	                  	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.11:12345/cgi-bin/camctrl/camctrl.cgi?move=left");
	                  	  res = curl_easy_perform(curl);
	                  	  if((COGpoint.y-prevpoint.y)>250)
	                  	  {
	                  		  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.11:12345/cgi-bin/camctrl/camctrl.cgi?move=up");
	                  		  res = curl_easy_perform(curl);
	                  	  }
	                  	  if((COGpoint.y-prevpoint.y)<-250)
	                  	   {
	                  	      curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.11:12345/cgi-bin/camctrl/camctrl.cgi?move=down");
	                  	      res = curl_easy_perform(curl);
	                  	   }

	                    }

			  if(COGpoint.x>580)
			  {
				  curl_easy_setopt(curl, CURLOPT_URL,"http://192.168.11.11:12345/cgi-bin/camctrl/camctrl.cgi?move=right");
				  res = curl_easy_perform(curl);
			  }

			  if(COGpoint.x<50)
			  {
				  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.11:12345/cgi-bin/camctrl/camctrl.cgi?move=left");
				  res = curl_easy_perform(curl);
	          }
*/

//	cvShowImage("cameraIP",img);
//	cvSaveImage("/sdcard/out.bmp", img);
//	getSourceImage(img);
	//if (img == NULL) {
//			LOGE("No source image.");
	//		return;
		//}
/*
		cvFlip(img);
		int width = img->width;
		int height = img->height;
		int rowStep = img->widthStep;
		int headerSize = 54;
		int imageSize = rowStep * height;
		int fileSize = headerSize + imageSize;
		unsigned char* image = new unsigned char[fileSize];
		struct bmpfile_header* fileHeader = (struct bmpfile_header*) (image);
		fileHeader->magic[0] = 'B';
		fileHeader->magic[1] = 'M';
		fileHeader->filesz = fileSize;
		fileHeader->creator1 = 0;
		fileHeader->creator2 = 0;
		fileHeader->bmp_offset = 54;
		struct bmp_dib_v3_header_t* imageHeader =
				(struct bmp_dib_v3_header_t*) (image + 14);
		imageHeader->header_sz = 40;
		imageHeader->width = width;
		imageHeader->height = height;
		imageHeader->nplanes = 1;
		imageHeader->bitspp = 24;
		imageHeader->compress_type = 0;
		imageHeader->bmp_bytesz = imageSize;
		imageHeader->hres = 0;
		imageHeader->vres = 0;
		imageHeader->ncolors = 0;
		imageHeader->nimpcolors = 0;
		memcpy(image + 54, img->imageData, imageSize);
		jbooleanArray bytes = env->NewBooleanArray(fileSize);
		if (bytes == 0) {
//			LOGE("Error in creating the image.");
			delete[] image;
			return;
		}
		env->SetBooleanArrayRegion(bytes, 0, fileSize, (jboolean*) image);
		delete[] image;
		
*/
//	if(cvWaitKey(30) >= 0 )
//	      break;
//	}
//	for(int i=0;i<N;i++)
//	{
//      cvReleaseImage(&buf[i]);
//	}
//	cvDestroyWindow("cameraIP");
//	return 0;

}
void* sendurl(void* arg)
{

}
void quit(char* msg, int retval)
{
if (retval == 0) {
fprintf(stdout, (msg == NULL ? "" : msg));
fprintf(stdout, "\n");
} else {
fprintf(stderr, (msg == NULL ? "" : msg));
fprintf(stderr, "\n");
}
/*
if (client_socket) close(client_socket);
if (server_socket) close(server_socket);
*/
if (camera) cvReleaseCapture(&camera);
pthread_mutex_destroy(&mutex);
exit(retval);
}

}
