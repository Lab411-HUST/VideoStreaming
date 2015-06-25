#include <jni.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.hpp>
#include <opencv/cxcore.hpp>
#include "bmpfmt.h"
#include <math.h>
#include <errno.h>
#include "curl/curl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <android/log.h>
#include <android/bitmap.h>
#include "curl/curl.h"
using namespace std;
using namespace cv;
extern "C" {
#define  LOG_TAG    "opencv"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
int vitrihientai=5;
int vitriguive=5;
int flag,flag1;
int LOW_THRESHOLD=80;
int MIN_PIXELS =450;
const int N =3;
IplImage **buf = 0;
IplImage *mhi = 0;
CvCapture* camera;
int last = 0;
CvPoint COGpoint;
CvPoint prevpoint;
CURL *curl;
int *hieu;
CURLcode res;
int dem1=0;
int dem2=0;
int xulymang(int *array)
{
	int i=0;
	int j=0;
	int dem=0;
	int *mang=new int[4];

	for(i=1;i<5;i++)
	{
		j=i-1;
		mang[j]=array[i]-array[j];
		//if(array[i]>0)
		//dem++;
	}
	for(j=0;j<4;j++)
	{
		if(mang[j]>=0)
		 dem++;
	}
	if(dem>=3)
		return 1;
	else 0;

}
JNIEXPORT void Java_com_example_streaming_StreamingHeadlessCamcorder_Start(JNIEnv* env, jobject thiz,jobject bitmap)
{
     //  LOGE("opencv Bat dau");
	prevpoint.x=COGpoint.x;
	prevpoint.y=COGpoint.y;
	int key;
    int width =320;
    int height=240;
    IplImage* img = cvCreateImageHeader(cvSize(width, height),IPL_DEPTH_8U,4);
    int *cv_data;
    if(hieu==NULL)
    {
    	hieu=new int[5];
    }
    if(curl==NULL)
    {
    	curl = curl_easy_init();
    }
    AndroidBitmap_lockPixels(env, bitmap, (void**)&cv_data);
    
    cvSetData(img, cv_data, width*4);
  //  LOGE("centermotion");
   if(img==NULL)
			{
				LOGE("Mat");
				//cvWaitKey(10);
				return;

			}
	CvSize size = cvSize(img->width,img->height); // get current frame size
    int i, idx1 = last, idx2;
    IplImage* diffImg;
	 if( !mhi || mhi->width != size.width || mhi->height != size.height ) {
         LOGE("cap phat ne");
        if( buf == 0 ) {
           LOGE("cap phat");
            buf = (IplImage**)malloc(N*sizeof(buf[0]));
            memset( buf, 0, N*sizeof(buf[0]));
        }

        for( i = 0; i < N; i++ ) {
            cvReleaseImage( &buf[i] );
            buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
            cvZero( buf[i] );
        }
        cvReleaseImage( &mhi );
        mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        cvZero( mhi ); // clear MHI at the beginning
      }
	 cvCvtColor(img, buf[last], CV_BGR2GRAY ); // convert frame to grayscale
            // *buf[last]=*grey_img;
    idx2 = (last + 1) % N; // index of (last - (N-1))th frame
    last = idx2;

    diffImg = buf[idx2];
    cvAbsDiff(buf[idx1], buf[idx2],diffImg ); // get difference between frames
   // cvEqualizeHist(diffImg,diffImg);
			cvThreshold(diffImg,diffImg,LOW_THRESHOLD,255,CV_THRESH_BINARY);
			cvEqualizeHist(diffImg,diffImg);
			int numberpixel=cvCountNonZero(diffImg);
                          if(diffImg==0)
                            LOGE("chan");
					if(numberpixel>MIN_PIXELS)
					{
						CvMoments* moment=new CvMoments();
						cvMoments(diffImg,moment,1);
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
    //  LOGI("X:%d,Y:%d",COGpoint.x,COGpoint.y);
     // jintArray toado = env->NewIntArray(2);
      //	jint *td= env->GetIntArrayElements(toado, NULL);
        //   td[0]=COGpoint.x;
        //   td[1]=COGpoint.y;
         //  env->ReleaseIntArrayElements(toado,td, NULL);

	if(COGpoint.x!=160&&COGpoint.y!=120)
{
               CvPoint p1;
               p1.x=COGpoint.x-40;
               p1.y=COGpoint.y-80;
               CvPoint p2;
               p2.x=COGpoint.x+40;
               p2.y=COGpoint.y+80;
        	  cvRectangle(img,p1,p2,CV_RGB(255,0,0),1);
}

if(COGpoint.x>250)
         {
	          //if((COGpoint.x-prevpoint.x)>=0)
	         // prevpoint.x=COGpoint.x;

	          dem1++;
	          if(dem1<5)
	        	  hieu[dem1]=COGpoint.x-360;
	          else
	          {
	        	  dem1=0;
	        	  int x=0;
	        	  x=xulymang(hieu);
	        	  if(x==1&curl!=NULL)
	        	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.163:12345/cgi-bin/camctrl/camctrl.cgi?move=right");
	        	  res = curl_easy_perform(curl);

	          }
	          /*
	          if(curl!=NULL&&dem1>=5)
	          {
        	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.163:12345/cgi-bin/camctrl/camctrl.cgi?move=right");
        	  res = curl_easy_perform(curl);
        	  dem1=0;
	          }
               */
         }
if(COGpoint.x<60)
{
	 //if((COGpoint.x-prevpoint.x)<=0)
	              dem1++;
		          if(dem1<5)
		        	  hieu[dem1]=60-COGpoint.x;
		          else
		         	  {
		         	        	  dem1=0;
		         	        	  int x=0;
		         	        	  x=xulymang(hieu);
		         	        	  if(x==1&curl!=NULL)
		         	        	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.163:12345/cgi-bin/camctrl/camctrl.cgi?move=left");
		         	        	  res = curl_easy_perform(curl);

		         	  }
}

   //  cvReleaseImage(&img);
   // return toado;

}
JNIEXPORT void JNICALL Java_com_example_streaming_StreamingHeadlessCamcorder_quayvitri1(JNIEnv* env, jobject thiz)
{
	 if(curl==NULL)
	    {
	    	curl = curl_easy_init();
	    }
	 if(curl!=NULL)
	 {
	curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.163:12345/cgi-bin/camctrl/camctrl.cgi?move=right");
    res = curl_easy_perform(curl);
	 }
}



}
