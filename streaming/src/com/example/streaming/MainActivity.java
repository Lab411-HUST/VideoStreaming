package com.example.streaming;
import java.io.IOException;
import org.devtcg.rojocam.util.DetachableResultReceiver;

import android.hardware.Camera;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.ResultReceiver;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends Activity {
	private static final String TAG = MainActivity.class.getSimpleName();
    private static final int COMMAND_NONE = 0;
    private static final int COMMAND_START = 1;
    private static final int COMMAND_STOP = 2;

    private WifiManager mWifiMgr;

    private int mPendingCommand;

    private Button mStart;
    private Button mStop;

    private TextView mWifiState;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		 mStart = (Button)findViewById(R.id.start);
		 mStop = (Button)findViewById(R.id.stop);
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
	     
		 
	}
	private void buttonstart()
	{
		 CamcorderNodeService.activateCameraNode(this,sStaticReceiver);
	}
	private void buttonstop()
	{
		CamcorderNodeService.deactivateCameraNode(this, sStaticReceiver);
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
	 private static final DetachableResultReceiver sStaticReceiver =
	            new DetachableResultReceiver(new Handler());

}
