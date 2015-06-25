package com.example.restreaming;

import org.devtcg.rojocam.rtsp.SimpleRtspServer;
import org.devtcg.rojocam.util.ReferenceCounter;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.ResultReceiver;
import android.util.Log;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;

public class CamcorderNodeService extends Service {
    private static final String TAG = CamcorderNodeService.class.getSimpleName();
    public static boolean isstreaming=false;
    private static final int RTSP_PORT = 5454;

    public static final String ACTION_ACTIVATE_CAMERA_NODE =
            "org.devtcg.rojocam.intent.action.ACTIVATE_CAMERA_NODE";
    public static final String ACTION_DEACTIVATE_CAMERA_NODE =
            "org.devtcg.rojocam.intent.action.DEACTIVATE_CAMERA_NODE";

    public static enum State {
        ACTIVE, STREAMING, DEACTIVE
    }
    private StreamingHeadlessCamcorder camcorder;
    private static State sState = State.DEACTIVE;

    public static final int RESULT_CODE_ERROR = 1;
    public static final int RESULT_CODE_SUCCESS = 2;

    public static final String RESULT_ERROR_TEXT = "error-text";

    private static final String EXTRA_RECEIVER = "org.devtcg.rojocam.receiver";

    private SimpleRtspServer mRtspServer;

    private UPnPPortMapper mPortMapper;

    /**
     * Wake lock active only when the camera is active (that is, a stream is
     * being sent).
     */
    private WakeLock mCaptureLock;

    private UserAlertHelper mUserAlertHelper;

    private final Handler mHandler = new Handler();

    public static void activateCameraNode(Context context, ResultReceiver receiver) {
    	Log.i(TAG,"activateCameraNode");
        Intent intent = new Intent(ACTION_ACTIVATE_CAMERA_NODE, null,
                context, CamcorderNodeService.class);
        intent.putExtra(EXTRA_RECEIVER, receiver);
        context.startService(intent);
    }

    public static void deactivateCameraNode(Context context, ResultReceiver receiver) {
        /* XXX: Lock will be released once stop is handled. */
        Intent intent = new Intent(ACTION_DEACTIVATE_CAMERA_NODE, null,
                context, CamcorderNodeService.class);
        intent.putExtra(EXTRA_RECEIVER, receiver);
        context.startService(intent);
    }

    /**
     * Taking advantage of the fact that the controller and service run in the
     * same process to communicate light state information through process
     * globals. It's fine, trust me :)
     */
    static boolean isActive() {
        return sState != State.DEACTIVE;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
    	Log.i(TAG,"onstartCommand");
        if (mUserAlertHelper == null) {
            mUserAlertHelper = new UserAlertHelper(this);
        }

        String action = intent.getAction();
        ResultReceiver receiver = (ResultReceiver)intent.getParcelableExtra(EXTRA_RECEIVER);
        if (ACTION_ACTIVATE_CAMERA_NODE.equals(action)) {
            activateNode(receiver);
            return START_REDELIVER_INTENT;
        } else {
            if (ACTION_DEACTIVATE_CAMERA_NODE.equals(action)) {
                deactivateNode(receiver);
            } else {
                Log.d(TAG, "Unsupported start command: " + intent);
            }
            return START_NOT_STICKY;
        }
    }

    private void invalidState(State oldState, State newState) {
        throw new IllegalStateException("Cannot transition from " + oldState + " to " + newState + " directly");
    }

    private void respondToNewState(State oldState, State newState) {
        mUserAlertHelper.changeState(newState);

        if (newState == State.STREAMING) {
            takeCaptureLock();
        } 
        else {
            if (oldState == State.STREAMING) {
                releaseCaptureLock();
            }

            /*
             * XXX: Sanity check. This will melt my phones if we accidentally
             * leave the capture lock on :)
             */
            ensureCaptureLockReleased();
        }
    }

    private void changeState(State newState) {
    	
        if (Looper.getMainLooper() != Looper.myLooper()) {
            throw new IllegalStateException("State changes can only occur on the main thread");
        }
         Log.i(TAG,"changestate");
        if (sState != newState) {
        	
            State oldState = sState;
            switch (oldState) {
                case DEACTIVE:
                    if (newState != State.ACTIVE) {
                        invalidState(oldState, newState);
                    }
                    break;
            }
            sState = newState;
            respondToNewState(oldState, newState);
        } else {
            Log.w(TAG, "Redundant state change (already at state " + newState + ")");
        }
    }

    private void activateNode(ResultReceiver receiver) {
    	Log.i(TAG,"activateNode");
        if (sState != State.DEACTIVE) {
            Log.d(TAG, "Node already active.");
            return;
        }

        if (mRtspServer != null) {
            throw new IllegalStateException("Deactive nodes can't have an active RTSP server...");
        }

        try {
        	
            mRtspServer = new SimpleRtspServer();
           
            mRtspServer.bind(new InetSocketAddress((InetAddress)null, RTSP_PORT));//mo socket voi cong 5454
            Log.i(TAG,"ok kiem");
            mRtspServer.registerMedia("test1.rtp", new CamcorderMediaHandler(mCamcorderRef));
            Log.i(TAG,"ok kiem 222");
            mRtspServer.start();
            //mPortMapper = UPnPPortMapper.mapPortIfNecessary(this, "rojocam mapping", RTSP_PORT);
           
            changeState(State.ACTIVE);
            if (receiver != null) {
                receiver.send(RESULT_CODE_SUCCESS, null);
            }
        } catch (IOException e) {
        	System.out.println("asaa\n");
            Log.e(TAG, "Error starting node", e);

            if (receiver != null) {
                Bundle resultData = new Bundle();
                // resultData.putString(RESULT_ERROR_TEXT,getString(R.string.error_starting, e.getMessage()));
                receiver.send(RESULT_CODE_ERROR, resultData);
            }
        }
       camcorder =new StreamingHeadlessCamcorder(CamcorderNodeService.this);
	   camcorder.start();
    }

    private void deactivateNode(ResultReceiver receiver) {
        if (sState == State.DEACTIVE) {
        	
        	Log.i(TAG, "deactive");
            Log.d(TAG, "Node not active.");
            
            if (mRtspServer != null) {
                throw new IllegalStateException("Deactive nodes shouldn't have an active RTSP server...");
            }
        } else {
            if (mRtspServer == null) {
                throw new IllegalStateException("Active nodes must have an RTSP server running...");
            }

            if (mRtspServer != null) {
                mRtspServer.shutdown();
                mRtspServer = null;
            }

            if (mPortMapper != null) {
                mPortMapper.unmapPorts();
            }

            changeState(State.DEACTIVE);
        }
        
        stopSelf();

        if (receiver != null) {
            receiver.send(RESULT_CODE_SUCCESS, null);
        }
        camcorder.stop();
    }

    private void takeCaptureLock() {
        if (mCaptureLock == null) {
            PowerManager pm = (PowerManager)getSystemService(POWER_SERVICE);
            mCaptureLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);
        }
        mCaptureLock.acquire();
        Log.i(TAG, "Acquired capture lock.");
    }

    private void releaseCaptureLock() {
        if (mCaptureLock == null || !mCaptureLock.isHeld()) {
            throw new IllegalStateException("Releasing capture lock without first acquiring it");
        }
        mCaptureLock.release();
        Log.i(TAG, "Released capture lock.");
    }

    private synchronized void ensureCaptureLockReleased() {
        if (mCaptureLock != null && mCaptureLock.isHeld()) {
            /* By throwing an exception here the OS should release the lock. */
            throw new IllegalStateException("Capture lock is held!");
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private final ReferenceCounter<StreamingHeadlessCamcorder> mCamcorderRef =
            new ReferenceCounter<StreamingHeadlessCamcorder>() {
        @Override
        protected StreamingHeadlessCamcorder onCreate() {
        	System.out.println("hellodsdhsdhs\n");
            //Log.d(TAG, "Creating camera instance from thread " + Thread.currentThread().getId());
            Log.d(TAG, "Creating camera instance from thread " + Thread.currentThread().getId());
            mHandler.post(new Runnable() {
                public void run() {
                    if (sState != State.STREAMING) {
                        changeState(State.STREAMING);
                    }
                }
            });

          //  StreamingHeadlessCamcorder camcorder =
			  //  new StreamingHeadlessCamcorder(CamcorderNodeService.this);
			//camcorder.start();
			return camcorder;
        }

        @Override
        protected void onDestroy(StreamingHeadlessCamcorder instance) {
            Log.d(TAG, "Destroying camera instance from thread " + Thread.currentThread().getId());

            instance.stop();

            mHandler.post(new Runnable() {
                public void run() {
                    if (sState == State.STREAMING) {
                        changeState(State.ACTIVE);
                    }
                }
            });
        }
    };
}
