package com.example.restreaming;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;
import org.devtcg.rojocam.ffmpeg.RtpOutputContext;
import org.devtcg.rojocam.ffmpeg.SwsScaler;
import org.devtcg.rojocam.util.IOUtils;

import com.example.restreaming.MainActivity.CommsThread;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ImageFormat;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.lang.ref.WeakReference;
import java.net.HttpURLConnection;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.URL;
import java.net.URLConnection;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CopyOnWriteArraySet;

public class StreamingHeadlessCamcorder  implements SurfaceHolder.Callback,Runnable {
	//socket
	  ServerSocket ss = null;
	   String mClientMsg = "";
	   Thread myCommsThread = null;
	   protected static final int MSG_ID = 0x1337;
	   public static final int SERVERPORT = 6000;
	   //
	//for webcam
	BluetoothChatService bluetooth;
	private byte[] mBuffer;
	private byte[] buffer;
	private int size;
	private int cameraId=0;
	private int cameraBase=0;
	private boolean cameraExists=false;
	///
    private static final String TAG = StreamingHeadlessCamcorder.class.getSimpleName();
    private final WeakReference<Context> mContext;
    private SurfaceView mDummySurfaceView;
    private SurfaceHolder mDummySurfaceHolder;
   
    private static final int NOT_RECORDING = 0;
    private static final int WAITING_FOR_SURFACE = 1;
    private static final int RECORDING = 2;

    private int mRecordingState = NOT_RECORDING;
   
    private final static int AUDIO_DATA_ID = 1;
    private final static int VIDEO_DATA_ID = 2;
    public static Bitmap mBackground;
    public static Bitmap mbitmap;
    private int[] mAudioFrameBufferDataLength = new int[1];
    private AudioTrack mAudioTrack = null;
    private Handler mHandler;
    private boolean mRun;
    private boolean mVideoOpened;
    private boolean mAudioOpened;
    //private SurfaceHolder mSurfaceHolder;
    Paint paint = new Paint();
    private int x;
    private int y;
    private int toado[];
    private String url;
    Thread mainLoop = null;
    private processimage proces;
    private class processimage extends Thread
    {
    	@Override
    	public void run()
    	{
    		super.run();
    		while(mRun)
    		{
    		if(mBackground!=null)
    		 {
    			synchronized (mBackground)
                {
                
    			mbitmap=mBackground;
    			int i=Start(mbitmap);
                }
    		 }
    		}
    		
    		
    	}
    	
    }
    ////
    SerialPort serialrx;
    SerialPort serialtx;
    private  OutputStream mOutputStream;
	private  InputStream mInputStream;
	private  ReadThread mReadThread;
	private  String mess;
	private class ReadThread extends Thread {    
		@Override
		public void run() {
			super.run();
			byte[] buffer = new byte[64];
			System.out.println("Bat dau doc\n");
			while(!isInterrupted()) {
			//System.out.println("chay");
				int size;
				
				try {
					
					if (mInputStream == null) 	
					   {
						System.out.println("khong doc duoc\n");
						return;
						}
					size = mInputStream.read(buffer);
					
					if (size>0) {
						
						onDataReceived(buffer, size);
					}
				} catch (IOException e) {
					
					e.printStackTrace();
					return;
				}
			}
		}
	}
	protected  void onDataReceived(final byte[] buffer,int size)
	{
		//System.out.println(size);
		byte[] data=new byte[size];
		int i=0;
		for(i=0;i<size;i++)
		data[i]=buffer[i];
		//data[size]='\0';
		try {
			mess = new String(data,"UTF-8");
		} catch (UnsupportedEncodingException e) 
		{
			e.printStackTrace();
		}
		System.out.println("data:"+mess);
	}
	//////
    
 
    private static FrameBuf sPleaseWaitFrame;
    private static ByteBuffer sourceBuf;
    private boolean mCanDoTorch;

    private int mPreviewFormat;
    private Size mPreviewSize;
    private int mPreviewBitsPerPixel;
    private int width=320;
    private int height=240;
    private final CopyOnWriteArraySet<RtpOutputContext> mReceivers =
            new CopyOnWriteArraySet<RtpOutputContext>();

    
    public StreamingHeadlessCamcorder(Context context) {
        mContext = new WeakReference<Context>(context);
    }
    
    protected Context getContext() {
        Context context = mContext.get();
        if (context == null) {
            throw new AssertionError();
        }
        return context;
    }
    public void start() {
    	Log.i(TAG,"start stream\n");
        if (isStarted()) {
            throw new IllegalStateException("Camcorder already started");
        }

        /* XXX: NOOOOOO!!!! */
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            public void run() {
            	System.out.println("sabshgahsvahvs\n");
                mRecordingState = WAITING_FOR_SURFACE;
                makeAndAddSurfaceView();
            }
        });
       
    }
    public void stop() {
        if (!isStarted()) {
            throw new IllegalStateException("Camcorder not started");
        }

        if (mRecordingState == RECORDING) {
            /* XXX: stopRecorder will set mRecordingState for us. */
            stopRecorder();
        } else {
            mRecordingState = NOT_RECORDING;
        }
        removeSurfaceView();
    }

    public boolean isStarted() {
        return mRecordingState != NOT_RECORDING;
    }
    private void makeAndAddSurfaceView() {
    	System.out.println("makeandsurface\n");
        SurfaceView dummyView = new SurfaceView(getContext());
        SurfaceHolder holder = dummyView.getHolder();
        holder.addCallback(this);
        holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);

        WindowManager wm = (WindowManager)getContext().getSystemService(Context.WINDOW_SERVICE);

        WindowManager.LayoutParams params = new WindowManager.LayoutParams(320,240,
                WindowManager.LayoutParams.TYPE_SYSTEM_ALERT,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
                        WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE,
                PixelFormat.TRANSLUCENT);
        params.gravity = Gravity.TOP | Gravity.RIGHT;
        params.x = params.y = getContext().getResources().getDimensionPixelOffset(
                R.dimen.preview_surface_offset);
        wm.addView(dummyView, params);
        
        mDummySurfaceView = dummyView;
    }
    private void removeSurfaceView() {
        WindowManager wm = (WindowManager)getContext().getSystemService(Context.WINDOW_SERVICE);
        wm.removeView(mDummySurfaceView);
        mDummySurfaceView = null;
    }
    
    private void startRecorder() throws IOException {
    	//String url="rtsp://192.168.11.163:554/live.sdp";
    	url=SettingsActivity.geturl();
        onRecorderInitialized(url);
        System.out.println("buoc 2 \n");
        mRecordingState = RECORDING;
       // inituart();
    }
    private void stopRecorder() {
        Log.i(TAG, "Stopping recorder...");

//        if (mRecordingState == RECORDING) {
//            onRecorderStopped(mCamera);
//            mCamera.stopPreview();
//        }
//        mCamera.release();
//        mCamera = null;
        mRecordingState = NOT_RECORDING;
        mRun=false;
        stopCamera();
       bluetooth.stop();
        
    }

    /**
     * Add a new peer that is to receive the camera feed.
     */
    public void addReceiver(RtpOutputContext rtpContext) {
        mReceivers.add(rtpContext);
    }

    /**
     * Remove a peer from this feed. The camera remains open when all
     * participants are removed but no data will be delivered to any parties.
     */
    public void surfaceChanged(SurfaceHolder holder, int format, int aWidth, int aHeight)
    {
    	Log.d(TAG, "surfaceChanged: format=" + format + "; width=" + width + "; height=" + height);
        System.out.println("surfacechanged\n");
        if (holder.getSurface() == null) {
            Log.d(TAG, "holder.getSurface() is null");
            return;
        }
        
        if (mRecordingState == WAITING_FOR_SURFACE) {
            try {
                Log.i(TAG, "Starting recorder...");
                startRecorder();
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    //setSurfaceSize(aWidth, aHeight);
    mainLoop = new Thread(this);
    mainLoop.start();
    }
    public void chaynao()
    {
    	mainLoop = new Thread(this);
        mainLoop.start();
    }
    public void surfaceCreated(SurfaceHolder aHolder)
    {
    	 
    	 Log.d(TAG, "surfaceCreated");
    	 mDummySurfaceHolder=aHolder;
    }
    public void surfaceDestroyed(SurfaceHolder holder) {
       // Log.d(TAG, "surfaceDestroyed");
      //  mDummySurfaceHolder = null;
    }
    public void setSurfaceSize(int aWidth, int aHeight)
    {
    synchronized (mDummySurfaceHolder)
        {
        mBackground = Bitmap.createScaledBitmap(mBackground, aWidth, aHeight, true);
        if (mVideoOpened)
        	{
        	//free old references to mBackground and other video resources
            nativeCloseVideo();
            mVideoOpened = nativeOpenVideo(mBackground) == 0;
            if (!mVideoOpened)
                {
            	nativeCloseVideo();
                Log.i(TAG, "unable to reopen video");
                }
        	}
        }
    }
    public void removeReceiver(RtpOutputContext rtpContext) {
        mReceivers.remove(rtpContext);
    }

    private static boolean isTorchModeSupported(Camera.Parameters params) {
        List<String> flashModes = params.getSupportedFlashModes();
        for (String flashMode: flashModes) {
            if (flashMode.equals(Camera.Parameters.FLASH_MODE_TORCH)) {
                return true;
            }
        }
        return false;
    }

    protected void onRecorderInitialized(String linkurl) {
    //Log.i(TAG,SettingsActivity.getselect());
    bluetooth=new BluetoothChatService(mHandler);
    String s1="CameraIP";
    String s2="Webcam";
    Setting(SettingsActivity.getselecteviron(),SettingsActivity.geturl(),Integer.parseInt(opencvsetting.getthresold()),Integer.parseInt(opencvsetting.getminpixel()));
   // if(SettingsActivity.getselect().equals(s1))
   // {	
    	Log.i(TAG, "chon camera IP");
        mBackground = Bitmap.createBitmap(320,240,Bitmap.Config.ARGB_8888); //ARGB_8888
        mbitmap=Bitmap.createBitmap(320,240,Bitmap.Config.ARGB_8888);
        if (nativeOpenFromFile(url) != 0)
            {
            nativeClose();
            Log.i(TAG, "nativeOpen() failed, throwing RuntimeException");
            throw new RuntimeException();
            }
        nativeCloseVideo();
        mVideoOpened = nativeOpenVideo(mBackground) == 0;
        if (mVideoOpened)
    	{
        nativeCloseVideo();
        mVideoOpened = nativeOpenVideo(mBackground) == 0;
        if (!mVideoOpened)
            {
        	nativeCloseVideo();
            Log.i(TAG, "unable to reopen video");
            //finish();
            }
    	}
        if (!mVideoOpened)
            nativeCloseVideo();
        if (!mVideoOpened)
            {
            Log.i(TAG, "unable to open a stream, throwing RuntimeException");
            throw new RuntimeException();
            }
   // }
    	///phan webcam
    //if(SettingsActivity.getselect().equals(s2))
   // {
//    	Log.i(TAG, "chon webcam");
//    	int ret = prepareCameraWithBase(cameraId, cameraBase);
//    	mBackground = Bitmap.createBitmap(640,480,Bitmap.Config.ARGB_8888); //ARGB_8888
//    	if(ret!=-1) cameraExists = true;
   // }
 
    }
    
    /////////////////////////////////////////
    @Override
    public void run() {
        Log.i(TAG, "entering run()");
        mRun = true;
//       proces=new processimage();
//        proces.start();
    ////phan camera IP
//        String s1="CameraIP";
//        String s2="Webcam";
//       this.myCommsThread=new Thread(new CommsThread());
//		 this.myCommsThread.start();
        //if(SettingsActivity.getselect().equals(s1))
        //{
        //bluetooth.quay(2);
        while (mRun)
            {
                    int dispatch = 0;
            		dispatch = nativeDecodeFrameFromFile();
            		while(dispatch==-1)
                   {
           		     dispatch=nativeOpenFromFile(url);
           		     System.out.println("ENd of file");
                   }
            	if(mDummySurfaceHolder!=null)
            	{
            		synchronized (mDummySurfaceHolder)
                	{
            	     Canvas canvas = null;
                     canvas = mDummySurfaceHolder.lockCanvas(null);
                    if (canvas != null)
                    {
                    	
                    	nativeUpdateBitmap();
                    	int i=Start(mBackground);
                    	if(CamcorderNodeService.isstreaming==true)
                    	{
                        sendFrame(getPleaseWaitFrame1(getContext(),320,240,17));
                        Log.i(TAG,"duoc phep gui");
                    	}
                        canvas.drawBitmap(mBackground,0,0,null);
                    	mDummySurfaceHolder.unlockCanvasAndPost(canvas);
                    }
                }
	                    
            }
            }
        
        nativeClose();
        Log.i(TAG, "leaving run()");
        
    }
    
    
    private void addCallbackBuffers(Camera camera, int numBuffers) {
        while (numBuffers-- > 0) {
            Size size = mPreviewSize;
            byte[] buf = new FrameBuf(size.width, size.height, mPreviewFormat).getBuffer();
            camera.addCallbackBuffer(buf);
        }
    }
    ///connect http
    private InputStream openConnectionGET(String urlString) throws IOException {

		 int respond = -1;
		 InputStream in = null;
		 HttpParams httpParameters = new BasicHttpParams();
		 int timeoutConnection =100;
		 HttpConnectionParams.setConnectionTimeout(httpParameters,timeoutConnection);
		 URL url = new URL(urlString);
		 URLConnection mConn = url.openConnection();
		 HttpURLConnection mURLConnection = (HttpURLConnection) mConn;
		 mURLConnection.setAllowUserInteraction(false);
		 mURLConnection.setInstanceFollowRedirects(true);
		 mURLConnection.setRequestMethod("GET");
		 mURLConnection.connect();
		 respond = mURLConnection.getResponseCode();
		 if (respond == HttpURLConnection.HTTP_OK)
		 in = mURLConnection.getInputStream();

		 return in;

		 }
    protected void onRecorderStopped(Camera recorder) {
        for (RtpOutputContext rtpContext: mReceivers) {
            IOUtils.closeQuietly(rtpContext);
        }
    }

    private static synchronized byte[] getPleaseWaitFrame(Context context,int width,int height, int pixelFormat) {
        if (sPleaseWaitFrame == null ||!sPleaseWaitFrame.compatibleWith(width, height, pixelFormat)) 
        {
            sPleaseWaitFrame = new FrameBuf(width,height, pixelFormat);
            Bitmap source = BitmapFactory.decodeResource(context.getResources(),
                    R.drawable.subject_warning_image);
          
            ByteBuffer sourceBuf = ByteBuffer.allocate(source.getRowBytes() * source.getHeight());
            source.copyPixelsToBuffer(sourceBuf);
            sourceBuf.rewind();

            byte[] sourceData = sourceBuf.array();
            byte[] destData = sPleaseWaitFrame.getBuffer();

            SwsScaler.scale(sourceData, SwsScaler.androidBitmapConfigToPixelFormat(mBackground.getConfig()),
                    source.getWidth(),source.getHeight(),
                    destData, SwsScaler.androidImageFormatToPixelFormat(pixelFormat),
                    width,height);
        }
        return sPleaseWaitFrame.getBuffer();
    }

    private static synchronized byte[] getPleaseWaitFrame1(Context context,int width,int height, int pixelFormat) {
        if (sPleaseWaitFrame == null) 
        {
        	 Log.w(TAG, "vao day nhe");
            sPleaseWaitFrame = new FrameBuf(width,height, pixelFormat);
            sourceBuf = ByteBuffer.allocate(mBackground.getRowBytes() * mBackground.getHeight());
        }
           mBackground.copyPixelsToBuffer(sourceBuf);
            sourceBuf.rewind();

            byte[] sourceData = sourceBuf.array();
            byte[] destData = sPleaseWaitFrame.getBuffer();
            SwsScaler.scale(sourceData, SwsScaler.androidBitmapConfigToPixelFormat(mBackground.getConfig()),
                    mBackground.getWidth(),mBackground.getHeight(),
                    destData, SwsScaler.androidImageFormatToPixelFormat(pixelFormat),
                    width,height);
        return sPleaseWaitFrame.getBuffer();
    }
    private void sendFrame(byte[] data) {
    	//Log.w(TAG, "Error writing to RTP participant: " +mPreviewFormat+"  "+mPreviewSize+"  "+mPreviewBitsPerPixel);
        /*
         * XXX: We're attempting to send out the encoded frames to all
         * participants as fast as they come in but obviously this doesn't
         * scale at all. We need to process each participant in its own
         * thread, using a sort of frame ringbuffer to keep the preview
         * frames going.
         */
        long now = System.nanoTime() / 1000;
        ArrayList<RtpOutputContext> toRemove = null;
        for (RtpOutputContext rtpContext: mReceivers) {
            try {
                rtpContext.writeFrame(data, now,
                        17,width,height,12);
            } catch (IOException e) {
                Log.w(TAG, "Error writing to RTP participant: " + rtpContext.getPeer());
                IOUtils.closeQuietly(rtpContext);
                if (toRemove == null) {
                    toRemove = new ArrayList<RtpOutputContext>();
                }
                toRemove.add(rtpContext);
            }
        }

        if (toRemove != null) {
            for (int i = 0; i < toRemove.size(); i++) {
                mReceivers.remove(toRemove.get(i));
            }
        }
    }
    //uart
    private void inituart()
    {
    	 buffer = new byte[64];
    	 mBuffer=new byte[1];
    	try {
			serialrx=new SerialPort(new File("/dev/ttyS2"),19200);
			//serialtx=new SerialPort(new File("/dev/ttyS1"),19200);
		} catch (SecurityException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		mInputStream = serialrx.getInputStream();
		mOutputStream=serialrx.getOutputStream();
		//mReadThread=new ReadThread();
		//mReadThread.start();
    }
    ///socket
   
	 class CommsThread implements Runnable {
		    public void run() {
		    	try
		    	{
		    	ss = new ServerSocket(6000);

                while (true) {
                    Socket socket = ss.accept();
                    BufferedReader in = new BufferedReader(
                    new InputStreamReader(socket.getInputStream()));
                    String str = in.readLine();
                    Log.i("received response from server", str);
//                    if(str.equals("trai"))
//                    	bluetooth.quay(1);
//                    if(str.equals("phai"))
//                    	bluetooth.quay(2);
                    in.close();
                    socket.close();
                }
            } catch (IOException e) {
                Log.e(TAG, e.getMessage());
            } catch (Exception e) {             
                Log.e(TAG, e.getMessage());
            }
		    }
		    }
	 ///
    private static class FrameBuf {
        private final int width;
        private final int height;
        private final int pixelFormat;

        private final byte[] buf;

        public FrameBuf(int width, int height, int pixelFormat) {
            float bytesPerPixel = ImageFormat.getBitsPerPixel(pixelFormat) / 8f;
            int bufSize = (int)(width * height * bytesPerPixel);
            buf = new byte[bufSize];

            this.width = width;
            this.height = height;
            this.pixelFormat = pixelFormat;
        }

        public boolean compatibleWith(int width, int height, int pixelFormat) {
            return (this.width == width && this.height == height &&
                    this.pixelFormat == pixelFormat);
        }

        public byte[] getBuffer() {
            return buf;
        }
    }
    static {
    	System.loadLibrary("avjni");
    	System.loadLibrary("opencv");
    	 System.loadLibrary("ImageProc");
    }
    //private static native void Start(Bitmap aBitmap);
    private native int nativeOpenFromFile(String aMediaFile);
    private native int Start(Bitmap abitmap);
    private native void Setting(String eviron,String serverurl,int thresold,int minpixel);
    private native void nativeClose();
    private native int nativeOpenVideo(Object aBitmapRef);
    private native void nativeCloseVideo();
    //private native int (byte[] aAudioFrameBufferRef, int[] aAudioFrameBufferCountRef);
    private native void nativeCloseAudio();
    private native int nativeDecodeFrameFromFile(); //never touch the bitmap here
    private native int nativeUpdateBitmap();
    private native void openFile(String url);
    private native void drawFrame(Bitmap bmp);
    ///
    public native int prepareCamera(int videoid);
    public native int prepareCameraWithBase(int videoid, int camerabase);
    public native void processCamera();
    public native void stopCamera();
    public native void pixeltobmp(Bitmap bitmap);
    
}
