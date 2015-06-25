package com.example.restreaming;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.ServerSocket;
import java.net.Socket;


import org.devtcg.rojocam.util.DetachableResultReceiver;

import android.hardware.Camera;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.ResultReceiver;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.view.Menu;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends Activity {
	///socket
	   ServerSocket ss = null;
	   String mClientMsg = "";
	   Thread myCommsThread = null;
	   protected static final int MSG_ID = 0x1337;
	   public static final int SERVERPORT = 6000;
	   ///
	private static final String TAG = MainActivity.class.getSimpleName();
    private static final int COMMAND_NONE = 0;
    private static final int COMMAND_START = 1;
    private static final int COMMAND_STOP = 2;
    private static final int COMMAND_SETTING=3;

    private WifiManager mWifiMgr;

    private int mPendingCommand;

    private Button mStart;
    private Button mStop;
    private Button msetting;
    private Button mopencvsetting;
    private TextView mWifiState;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		 mStart = (Button)findViewById(R.id.start);
		 mStop = (Button)findViewById(R.id.stop);
		 msetting=(Button)findViewById(R.id.settings);
		 mopencvsetting=(Button)findViewById(R.id.opencvsetting);
		 
		 
		// getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		 mStart.setOnClickListener(new Button.OnClickListener(){
				@Override
				public void onClick(View v) {
					 mPendingCommand = COMMAND_START;
		             buttonstart();
		             //   break;
				}});
		 mStop.setOnClickListener(new Button.OnClickListener(){
				@Override
				public void onClick(View v) {
					mPendingCommand = COMMAND_STOP;
	                buttonstop();
				}});
		 msetting.setOnClickListener(new Button.OnClickListener(){
				@Override
				public void onClick(View v) {
					mPendingCommand = COMMAND_SETTING;
	                buttonsetting();
				}});
		 mopencvsetting.setOnClickListener(new Button.OnClickListener(){
					public void onClick(View v) {
						startActivity(new Intent(MainActivity.this, opencvsetting.class));
					}
				});
	     
		 
	}
	private void buttonstart()
	{
		 CamcorderNodeService.activateCameraNode(this,sStaticReceiver);
	}
	private void buttonstop()
	{
		CamcorderNodeService.deactivateCameraNode(this, sStaticReceiver);
		
	}
	private void buttonsetting()
	{
		SettingsActivity.show(this);
	}
	
	 private static String formatAddr(int ipAddr) {
	        StringBuilder ipStr = new StringBuilder();
	        ipStr.append(ipAddr & 0xff);
	        for (int i = 8; i <= 24; i += 8) {
	            ipStr.append('.');
	            ipStr.append((ipAddr >> i) & 0xff);
	        }
	        return ipStr.toString();
	    }
	private void updateWifiState() {
        WifiInfo wifiInfo = mWifiMgr.getConnectionInfo();
        if (wifiInfo != null) {
            String ip = formatAddr(wifiInfo.getIpAddress());
            mWifiState.setText(getString(R.string.wifi_state_connected, ip));
        } else {
            mWifiState.setText(getString(R.string.wifi_state_disconnected));
        }
    }
	 private final BroadcastReceiver mWifiEventReceiver = new BroadcastReceiver() {
	        @Override
	        public void onReceive(Context context, Intent intent) {
	            String action = intent.getAction();
	            if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
	                updateWifiState();
	            }
	        }
	    };
	    ///socket
	 private static final DetachableResultReceiver sStaticReceiver = new DetachableResultReceiver(new Handler());
	 Handler myUpdateHandler = new Handler() {
		    public void handleMessage(Message msg) {
		        switch (msg.what) {
		        case MSG_ID:       
		            break;
		        default:
		            break;
		        }
		        super.handleMessage(msg);
		    }
		   };
		   
		   
	 class CommsThread implements Runnable {
		    public void run() {
		    	System.out.println("chay sever");
		        Socket s = null;
		        try {
		            ss = new ServerSocket(SERVERPORT);
		        } catch (IOException e) {
		            e.printStackTrace();
		        }
		        while (!Thread.currentThread().isInterrupted()) {
		            //Message m = new Message();
		            //m.what = MSG_ID;
		            try {
		                if (s == null)
		                    s = ss.accept();
		                BufferedReader input = new BufferedReader(new InputStreamReader(s.getInputStream()));
		                String st = null;
		                st = input.readLine();
		                mClientMsg = st;
		                if(!mClientMsg.equals("null"))
		                System.out.println("du lieu"+mClientMsg);
		               // myUpdateHandler.sendMessage(m);
		            } catch (IOException e) {
		                e.printStackTrace();
		            }
		        }
		    }
		    }
	 ////

}
