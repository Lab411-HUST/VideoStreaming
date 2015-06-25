
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
int bien=0;
int vitrihientai=5;
int vitriguive=5;
int flag,flag1;
int LOW_THRESHOLD=100;
int MIN_PIXELS =500;
const int N =3;
IplImage **buf = 0;
IplImage *mhi = 0;
CvCapture* camera;
int last = 0;
CvPoint COGpoint;
CvPoint prevpoint;
CvRect bndRect;
CvPoint pt1, pt2;
CvPoint pt_center1,pt_center2;
CURL *curl;
int *hieu;
int *updatepoint;
const char* mserverurl;
bool outdoor=false;
CURLcode res;
int dem1=0;
int dem2=0;
int *mang;
jint toado;
//
int tam_x;
int tam_y;
int chiso;
int dt[10];
int xcenter[10];
int ycenter[10];
int quay=0;
int dem=0;
int xulymangam(int *array)
{
	int i=0;
	int j=0;
	int dem11=0;
	for(i=1;i<15;i++)
	{
		j=i-1;
		mang[j]=array[i]-array[j];
	}
	for(j=0;j<14;j++)
	{
		if(mang[j]<0)
                 { 
		 dem11++;
                 LOGE("am");
                 }
	}
	if(dem11>=10)
		return 1;
	else
              return 0;


}
int xulymangduong(int *array)
{
	int i=0;
	int j=0;
	int dem11=0;
	for(i=1;i<15;i++)
	{
		j=i-1;
		mang[j]=array[i]-array[j];
	}
	for(j=0;j<14;j++)
	{
		if(mang[j]>0)
               {
		 dem11++;
                 LOGE("duong");
                } 
	}
	if(dem11>=10)
	  return 1;
	else 
          return 0;


}
JNIEXPORT jint JNICALL Java_com_example_restreaming_StreamingHeadlessCamcorder_Setting(JNIEnv* env, jobject thiz,jstring eviron,jstring serverurl,jint thresold,jint minpixel)
{
  jboolean blnIsCopy;
  const char* meviron= (env)->GetStringUTFChars(eviron, &blnIsCopy);
  mserverurl=(env)->GetStringUTFChars(serverurl,&blnIsCopy);
__android_log_print(ANDROID_LOG_INFO, "com.example.setting", "opening eviron %s", meviron);
__android_log_print(ANDROID_LOG_INFO, "com.example.setting", "opening serverurl %s", mserverurl);
int n=strcmp(meviron,"outdoor");
__android_log_print(ANDROID_LOG_INFO, "com.example.setting", "opening n %d",n);
if(n==0)
outdoor=true;
else
outdoor=false;
(env)->ReleaseStringUTFChars(eviron,meviron);
(env)->ReleaseStringUTFChars(serverurl,mserverurl);
LOW_THRESHOLD=thresold;
MIN_PIXELS=minpixel;
if(updatepoint==NULL)
updatepoint=new int[5];
if(hieu==NULL)
    {
    	hieu=new int[16];
    }
   if(mang==NULL)
   {
     mang=new int[15];
   }
    if(curl==NULL)
    {
    	curl = curl_easy_init();
    }
}
int dtmax(int *dt,int numpeople)
{
   int i=0;
   int chiso=0;
   int max=dt[0];


   for(i=1;i<numpeople;i++)
   {
     if(dt[i]>max)
     {
    	 max=dt[i];
    	 chiso=i;
     }

   }
   return chiso;

}
JNIEXPORT jint Java_com_example_restreaming_StreamingHeadlessCamcorder_Start(JNIEnv* env, jobject thiz,jobject bitmap)
{
     //  LOGE("opencv Bat dau");
        bndRect = cvRect(0,0,0,0);
	int avgX = 0;
	int prevX = 0;
	int numpeople = 0;
	int closestToLeft = 0;
	int closestToRight = 320;
	prevpoint.x=COGpoint.x;
	prevpoint.y=COGpoint.y;
	int key;
    int width =320;
    int height=240;
    IplImage* img = cvCreateImageHeader(cvSize(width, height),IPL_DEPTH_8U,4);
    int *cv_data;
    AndroidBitmap_lockPixels(env, bitmap, (void**)&cv_data);
    cvSetData(img, cv_data, width*4);
   if(img==NULL)
			{
				LOGE("Mat");
				return 0;

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
     cvSmooth(img, img, CV_GAUSSIAN,3);
	 cvCvtColor(img, buf[last], CV_BGR2GRAY ); // convert frame to grayscale
            // *buf[last]=*grey_img;
    idx2 = (last+1) % N; // index of (last - (N-1))th frame
    last = idx2;
    
    diffImg = buf[idx2];
    cvAbsDiff(buf[idx1], buf[idx2],diffImg ); // get difference between frames
    cvThreshold(diffImg,diffImg,LOW_THRESHOLD,255,CV_THRESH_BINARY);

//////
if(outdoor)
{
             cvDilate(diffImg, diffImg, 0,18);
             cvErode(diffImg, diffImg, 0, 10);
             CvMoments* moment=new CvMoments();
             CvMemStorage* storage = cvCreateMemStorage(0);
             CvSeq* contour = 0;
             cvFindContours( diffImg, storage, &contour, sizeof(CvContour), CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
             int numpeople=0;
             toado=0;
             for( ;contour != 0; contour = contour->h_next )
             {
            	 int area = abs(cvContourArea(contour, CV_WHOLE_SEQ));
            	 numpeople++;
            	 cvMoments(contour, moment, 0);
            	 int x_mass_centre = (moment->m10/moment->m00);
            	 int y_mass_centre = (moment->m01/moment->m00);
            	 dt[numpeople-1]=area;

            	 xcenter[numpeople-1]=x_mass_centre;
            	 ycenter[numpeople-1]=y_mass_centre;
            	// __android_log_print(ANDROID_LOG_INFO, "com.example.setting", "x center %d",x_mass_centre);
             //Get a bounding rectangle around the moving object.
            	 if(area>6000)
            	 {
             bndRect = cvBoundingRect(contour, 0);
             pt1.x = bndRect.x;
             pt1.y = bndRect.y;
             pt2.x = bndRect.x + bndRect.width;
             pt2.y = bndRect.y + bndRect.height;

             }
             cvRectangle(img, pt1, pt2, CV_RGB(0,255,0), 1);
             }
             ///delay 15 frame k xu ly vi khi quay co rat nhieu nhieu
             if(quay==1)
             {
            	 LOGI("delay");
                dem++;
                if(dem<10)
                return 0;
             }

             if(dem==10)
             {
            	 quay=0;
            	 dem=0;
             }
             if(numpeople!=0)
             {
              chiso=dtmax(dt,numpeople);
              tam_x=xcenter[chiso];
              tam_y=ycenter[chiso];
              }
             if(numpeople==0)
             {
            	 tam_x=160;
             }
           // __android_log_print(ANDROID_LOG_INFO, "com.example.setting", "tam %d",tam_x);
            pt_center1.x=tam_x-40;
            pt_center2.x=tam_x+40;
            pt_center1.y=tam_y-60;
            pt_center2.y=tam_y+60;
            updatepoint[idx2]=tam_x;
                        	 if(quay==0)
                        	 {
                        	 if(tam_x>170)
                        	          {
                                 dem1++;
                       	          if(dem1<16)
                       	        	  hieu[dem1]=tam_x;
                       	          else
                       	          {
                       	        	  dem1=0;
                       	        	  int x=0;
                       	        	  x=xulymangduong(hieu);
                       	        	  if(x==1)
                       //{
                       	        	 // curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.163:12345/cgi-bin/camctrl/camctrl.cgi?move=right");
                       	        	 //res = curl_easy_perform(curl);
                       //}
                       	        	  //
                                               {
                       	        		                        	 	        	 toado=1;
                       	        		                        	 	        	  quay=1;
                       	        		                        	 	        	curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.165:12345/cgi-bin/camctrl/camctrl.cgi?move=right");
                       	        		                        	 	        	res = curl_easy_perform(curl);
                       	        		                        	 	        	LOGI("gui lenh quay phai");
                       	        		                        	 	        	LOGI("gui ve server\n");
                       	        		                        	 	            curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.108/vlc2/test.php?id=1");
                       	        		                        	 	            res = curl_easy_perform(curl);


                                               }

                       	          }
//                        	 	         if(updatepoint[idx1]<240 && updatepoint[idx1]!=160) {
//                        	 	        	 toado=1;
//                        	 	        	  quay=1;
//                        	 	        	curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.165:12345/cgi-bin/camctrl/camctrl.cgi?move=right");
//                        	 	        	res = curl_easy_perform(curl);
//                        	 	        	LOGI("gui lenh quay phai");
//                        	 	        	LOGI("gui ve server\n");
//                        	 	            curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.108/vlc2/test.php?id=1");
//                        	 	            res = curl_easy_perform(curl);
//
//                        	               }
                        	          }
                             if(tam_x<150)
                             {
                                 LOGI("X:%d",tam_x);
                          	 //if((COGpoint.x-prevpoint.x)<=0)
                          	              dem2++;
                          		          if(dem2<16)
                          		        	  hieu[dem2]=tam_x;
                          		          else
                          		         	  {               //x=0;
                          		         	        	  dem2=0;
                          		         	        	  int x=0;
                          		         	        	  x=xulymangam(hieu);
                          		         	        	  if(x==1)
                          //{
                          		         	        	//  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.163:12345/cgi-bin/camctrl/camctrl.cgi?move=left");
                          		         	        	 // res = curl_easy_perform(curl);
                          //}
                          		         	        	  //
                                                                   {
                          		         	        		 toado=2;
                          		         	        		            	            	  quay=1;
                          		         	        		            	            	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.165:12345/cgi-bin/camctrl/camctrl.cgi?move=left");
                          		         	        		            	            	  res = curl_easy_perform(curl);
                          		         	        		            	            	  LOGI("gui lenh quay trai");
                          		         	        		            	            	  LOGI("gui ve server\n");
                          		         	        		            	            	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.108/vlc2/test.php?id=1");
                          		         	        		            	            	  res = curl_easy_perform(curl);

                                                                  }

                          		         	  }

//            	              if(updatepoint[idx1]>80 && updatepoint[idx1]!=160)
//            	              {
//            	            	  toado=2;
//            	            	  quay=1;
//            	            	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.165:12345/cgi-bin/camctrl/camctrl.cgi?move=left");
//            	            	  res = curl_easy_perform(curl);
//            	            	  LOGI("gui lenh quay trai");
//            	            	  LOGI("gui ve server\n");
//            	            	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.108/vlc2/test.php?id=1");
//            	            	  res = curl_easy_perform(curl);
//            	              }
                             }
                         }
              if(tam_x!=160)
             cvRectangle(img,pt_center1,pt_center2, CV_RGB(255,0,0), 1);
             //}


             //__android_log_print(ANDROID_LOG_INFO, "com.example.opencv", "numberpeople:%d",numpeople);
  }
else
{
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
	if(COGpoint.x!=160&&COGpoint.y!=120)
{
               CvPoint p1;
               p1.x=COGpoint.x-40;
               p1.y=COGpoint.y-80;
               CvPoint p2;
               p2.x=COGpoint.x+40;
               p2.y=COGpoint.y+80;
               cvRectangle(img,p1,p2,CV_RGB(0,255,0),1);
}
     toado=0;
     updatepoint[idx2]=COGpoint.x;
if(COGpoint.x>280)
         {
	         if(updatepoint[idx1]<280 && updatepoint[idx1]!=160)

	           {
	        	 toado=1;
               }

         }
if(COGpoint.x<80)
{
	              
	              if(updatepoint[idx1]>80 && updatepoint[idx1]!=160)
	              {
	            	 toado=2;

	              }
}

////
/*
                        CvMemStorage* storage = cvCreateMemStorage();
                        CvSeq* first_contour = NULL;
                        int Nc = cvFindContours(diffImg, storage, &first_contour, sizeof(CvContour), CV_RETR_LIST);
                       LOGI("Total Contours Detected: %d\n", Nc);
                        CvSeq* c=first_contour;
                        for( c; c!=NULL; c=c->h_next ) {
                          // cvCvtColor( img_8uc1, img_8uc3, CV_GRAY2BGR );
                           cvDrawContours(img,c,CV_RGB(250,0,0),CV_RGB(0,0,250),0,2,8);
                         for( int i=0; i<c->total; ++i) {
                         CvPoint* p = CV_GET_SEQ_ELEM(CvPoint, c, i);
  
                                                        }
                                  
                                                  }
                        CvMoments* Moments;
                        CvHuMoments *HuMoments;
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
                  LOGI("X:%d",COGpoint.x);
	          dem1++;
	          if(dem1<6)
	        	  hieu[dem1]=COGpoint.x;
	          else
	          {
	        	  dem1=0;
	        	  int x=0;
	        	  x=xulymangduong(hieu);
	        	  if(x==1 && curl!=NULL)
                      {
	        	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.163:12345/cgi-bin/camctrl/camctrl.cgi?move=right");
	        	 res = curl_easy_perform(curl);

	        	  
                       
	        	  LOGI("gui ve server\n");
	        	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.108/vlc2/test.php?id=1");
	        	  res = curl_easy_perform(curl);
                        }

	          }
	          
         }
if(COGpoint.x<80)
{
	 //if((COGpoint.x-prevpoint.x)<=0)
                      LOGI("X:%d",COGpoint.x);
	              dem2++;
		          if(dem2<6)
		        	  hieu[dem2]=COGpoint.x;
		          else
		         	  {               //x=0;
		         	        	  dem2=0;
		         	        	  int x=0;
		         	        	  x=xulymangam(hieu);
		         	        	  if(x==1 && curl!=NULL)
                                            {
		         	        	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.163:12345/cgi-bin/camctrl/camctrl.cgi?move=left");
		         	        	  res = curl_easy_perform(curl);

		         	        	  
                                         
		         	        	  LOGI("gui lenh nao");
		         	        	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.108/vlc2/test.php?id=1");
	        	                          res = curl_easy_perform(curl);
                                            }

		         	  }
}
*/
    //cvReleaseImage(&img);

   }

 return toado;
}

JNIEXPORT jint Java_com_example_restreaming_StreamingHeadlessCamcorder_Start1(JNIEnv* env, jobject thiz,jobject bitmap)
{
    
         //  LOGE("opencv Bat dau");
	prevpoint.x=COGpoint.x;
	prevpoint.y=COGpoint.y;
	int key;
    int width =352;
    int height=288;
    IplImage* img = cvCreateImageHeader(cvSize(width, height),IPL_DEPTH_8U,4);
    int *cv_data;
    if(hieu==NULL)
    {
    	hieu=new int[7];
    }
   if(mang==NULL)
{
  mang=new int[6];
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
				return 0;

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
    //cvSmooth(img, img, CV_BLUR, 3);
	 cvCvtColor(img, buf[last], CV_BGR2GRAY ); // convert frame to grayscale
            // *buf[last]=*grey_img;
    idx2 = (last + 1) % N; // index of (last - (N-1))th frame
    last = idx2;
    
    diffImg = buf[idx2];
   // Mat abs_dst;
  //  Mat dst;
   // int kernel_size = 3;
 // int scale = 1;
 // int delta = 0;
//  int ddepth = CV_16S;
   
   // Laplacian((Mat)buf[idx2], dst, ddepth, kernel_size, scale, delta, BORDER_DEFAULT);
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
    //if(COGpoint.x!=160)
    //  LOGI("X:%d",COGpoint.x);
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
     toado=0;
 //jintArray toado;
      	     //jint *td= env->GetIntArrayElements(toado, NULL);
          // td[0]=0;
          // td[0]=COGpoint.x;
           //td[1]=COGpoint.y;
          // env->ReleaseIntArrayElements(toado,td, NULL);
if(COGpoint.x>200)
         {
	          //if((COGpoint.x-prevpoint.x)>=0)
	         // prevpoint.x=COGpoint.x;
                   LOGI("X:%d",COGpoint.x);
	          dem1++;
	          if(dem1<7)
	        	  hieu[dem1]=COGpoint.x;
	          else
	          {
	        	  dem1=0;
	        	  int x=0;
	        	  x=xulymangduong(hieu);
	        	  if(x==1&curl!=NULL)
//{
	        	 // curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.163:12345/cgi-bin/camctrl/camctrl.cgi?move=right");
	        	 //res = curl_easy_perform(curl);
//}
	        	  //
                        {
                          toado=1;
	        	  LOGI("gui ve server\n");
	        	 // curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.135/vlc/test.php?id=1");
	        	 // res = curl_easy_perform(curl);
                        }

	          }
	          
         }
if(COGpoint.x<100)
{
            LOGI("X:%d",COGpoint.x);
	 //if((COGpoint.x-prevpoint.x)<=0)
	              dem2++;
		          if(dem2<7)
		        	  hieu[dem2]=COGpoint.x;
		          else
		         	  {               //x=0;
		         	        	  dem2=0;
		         	        	  int x=0;
		         	        	  x=xulymangam(hieu);
		         	        	  if(x==1&curl!=NULL)
//{                                        
		         	        	//  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.163:12345/cgi-bin/camctrl/camctrl.cgi?move=left");
		         	        	 // res = curl_easy_perform(curl);
//}
		         	        	  //
                                         {
                                                  toado=2;
		         	        	  LOGI("gui lenh nao");
		         	        	  //curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.11.135/vlc/test.php?id=1");
	        	                          //res = curl_easy_perform(curl);
                                        }

		         	  }
}
else
{
  dem2=0;
  dem1=0;
}
   // cvReleaseImage(&img);
    return toado;

}



}
