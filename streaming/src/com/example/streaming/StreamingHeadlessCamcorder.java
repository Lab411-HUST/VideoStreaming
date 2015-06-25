package com.example.streaming;
import org.devtcg.rojocam.ffmpeg.RtpOutputContext;
import org.devtcg.rojocam.ffmpeg.SwsScaler;
import org.devtcg.rojocam.util.IOUtils;

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
import android.util.Log;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CopyOnWriteArraySet;

public class StreamingHeadlessCamcorder implements SurfaceHolder.Callback,Runnable {
    private static final String TAG = StreamingHeadlessCamcorder.class.getSimpleName();
    private final WeakReference<Context> mContext;
    private SurfaceView mDummySurfaceView;
    private SurfaceHolder mDummySurfaceHolder;
   
    private static final int NOT_RECORDING = 0;
    private static final int WAITING_FOR_SURFACE = 1;
    private static final int RECORDING = 2;

    private int mRecordingState = NOT_RECORDING;
    /**
     * We offer a way to warn the camera subject that they are about to be
     * filmed by playing a sound (see UserAlertHelper) and also by flashing the
     * camera's LED for a period before actually sending the live camera feed.
     * This is part of a soon-to-be configurable policy to protect the privacy
     * of individuals that may be under surveillance (e.g. my girlfriend).
     */
  //  private SubjectWarning mSubjectWarning
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
    			Start(mbitmap);
                }
    		 }
    		}
    		
    		
    	}
    	
    }
    
 
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
    	Log.d(TAG,"start stream\n");
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
        holder.setType(SurfaceHolder.SURFACE_TYPE_NORMAL);

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
    	String url="rtsp://192.168.11.163:554/live.sdp";
        onRecorderInitialized(url);
        System.out.println("buoc 2 \n");
        mRecordingState = RECORDING;
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
        mDummySurfaceHolder = holder;
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
    public void surfaceCreated(SurfaceHolder aHolder)
    {
    	 
    	 Log.d(TAG, "surfaceCreated");
    }
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "surfaceDestroyed");
        mDummySurfaceHolder = null;
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
      	url=linkurl;
    	paint.setColor(Color.RED);
    	paint.setStyle(Paint.Style.STROKE);
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
       // mBackground = Bitmap.createScaledBitmap(mBackground,480,320, true);
        if (mVideoOpened)
    	{
    	//free old references to mBackground and other video resources
        nativeCloseVideo();
        mVideoOpened = nativeOpenVideo(mBackground) == 0;
       // mVideoOpened = nativeOpenVideo(mbitmap) == 0;
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
        	
 
    }
    
    /////////////////////////////////////////
    @Override
    public void run() {
        Log.i(TAG, "entering run()");
        mRun = true;
        proces=new processimage();
        proces.start();
        while (mRun)
            {
        	
            int dispatch = 0;
//           synchronized (mDummySurfaceHolder)
//                {
//            	if (mRun)
//            		{
            		dispatch = nativeDecodeFrameFromFile();
            		while(dispatch==-1)
                   {
           		 dispatch=nativeOpenFromFile("rtsp://192.168.11.163:554/live.sdp");
           		 System.out.println("ENd of file");
                   }
//            		if (dispatch == VIDEO_DATA_ID)
//	                    {
//	                    Canvas canvas = null;
//	                    try {
//	                        canvas =mDummySurfaceHolder.lockCanvas(null);    
	                        nativeUpdateBitmap();
//	                        
//	                        //System.out.println("gui nao\n");
//	                        
                        sendFrame(getPleaseWaitFrame1(getContext(),320,240,17));
//	                        
//	                      // canvas.drawBitmap(mBackground, 0, 0, null);
//	                        }
//	                    finally
//	                        {
//	                        if (canvas != null)
//	                            {
//	                        	mDummySurfaceHolder.unlockCanvasAndPost(canvas);
//	                            }
//	                        }
//	                    }
//            		}
//                }
//            }
//        synchronized (mDummySurfaceHolder)
//            {
//            nativeClose();
//            }
            }
        
        nativeClose();
      //  mAudioTrack.flush();
       // mAudioTrack.stop();
      //  mAudioTrack.release();

        Log.i(TAG, "leaving run()");

    }
    
    
    private void addCallbackBuffers(Camera camera, int numBuffers) {
        while (numBuffers-- > 0) {
            Size size = mPreviewSize;
            byte[] buf = new FrameBuf(size.width, size.height, mPreviewFormat).getBuffer();
            camera.addCallbackBuffer(buf);
        }
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
    }
    //private static native void Start(Bitmap aBitmap);
    private native int nativeOpenFromFile(String aMediaFile);
    private native void Start(Bitmap abitmap);
    private native void nativeClose();
    private native int nativeOpenVideo(Object aBitmapRef);
    private native void nativeCloseVideo();
    //private native int (byte[] aAudioFrameBufferRef, int[] aAudioFrameBufferCountRef);
    private native void nativeCloseAudio();
    private native int nativeDecodeFrameFromFile(); //never touch the bitmap here
    private native int nativeUpdateBitmap();
    private native void openFile(String url);
    private native void drawFrame(Bitmap bmp);
}
