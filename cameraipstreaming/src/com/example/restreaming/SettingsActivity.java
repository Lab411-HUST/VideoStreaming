package com.example.restreaming;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.EnumSet;





public class SettingsActivity extends PreferenceActivity implements OnPreferenceChangeListener {
	
	//bluetooth
    private static final int REQUEST_CONNECT_DEVICE = 1;
    private static final int REQUEST_ENABLE_BT = 2;
    public static final int STATE_NONE = 0;       // we're doing nothing
    public static final int STATE_LISTEN = 1;     // now listening for incoming connections
    public static final int STATE_CONNECTING = 2; // now initiating an outgoing connection
    public static final int STATE_CONNECTED = 3;  // now connected to a remote device
 // Name of the connected device
    private String mConnectedDeviceName = null;
    // Array adapter for the conversation thread
    private ArrayAdapter<String> mConversationArrayAdapter;
    // String buffer for outgoing messages
    private StringBuffer mOutStringBuffer;
    // Local Bluetooth adapter
    private BluetoothAdapter mBluetoothAdapter = null;
    // Member object for the chat services
  
 // Member object for the chat services
    private BluetoothChatService mChatService = null;
    //
 // Message types sent from the BluetoothChatService Handler

    public static final int MESSAGE_STATE_CHANGE = 1;
    public static final int MESSAGE_READ = 2;
    public static final int MESSAGE_WRITE = 3;
    public static final int MESSAGE_DEVICE_NAME = 4;
    public static final int MESSAGE_TOAST = 5;
   
    
    public static final String DEVICE_NAME = "device_name";
    public static final String TOAST = "toast";
    /////
    private static ListPreference mPolicy;
    private static ListPreference meviron;
    private static EditTextPreference mPassword;

   

    private static final String KEY_POLICY = "device";
    private static final String KEY_EVIRON="environments";
    private static final String KEY_PASSWORD = "url";

    public static SharedPreferences getPrefs(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context);
    }

    public static EnumSet<CameraPolicy> getPolicy(Context context) {
        return CameraPolicy.fromString(getPrefs(context).getString(KEY_POLICY, ""));
    }
    public static EnumSet<CameraEviron> getPolicyeviron(Context context)
    {
    	return CameraEviron.fromString(getPrefs(context).getString(KEY_EVIRON, ""));
    }
    public static void show(Context context) {
        Intent intent = new Intent(context, SettingsActivity.class);
        context.startActivity(intent);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        addPreferencesFromResource(R.xml.settings);

        mPolicy = (ListPreference)findPreference(KEY_POLICY);
        mPolicy.setOnPreferenceChangeListener(this);
         
        meviron=(ListPreference)findPreference(KEY_EVIRON);
        meviron.setOnPreferenceChangeListener(this);
        mPassword = (EditTextPreference)findPreference(KEY_PASSWORD);
        mPassword.setOnPreferenceChangeListener(this);
      
        ///bluetooth
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        // If the adapter is null, then Bluetooth is not supported
        if (mBluetoothAdapter == null) {
            Toast.makeText(this, "Bluetooth is not available", Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        
//        final Button buttonSetup = (Button)findViewById(R.id.ButtonSetup);
//        buttonSetup.setOnClickListener(new View.OnClickListener() {
//			public void onClick(View v) {
//				startActivity(new Intent(SettingsActivity.this, opencvsetting.class));
//			}
//        });
       // mTitle = (TextView) findViewById(R.id.title_left_text);
       // mTitle.setText("hello");
      //  mTitle = (TextView) findViewById(R.id.title_right_text);
    }
    @Override
    public void onStart() {
        super.onStart();
        

        // If BT is not on, request that it be enabled.
        // setupChat() will then be called during onActivityResult
        if (!mBluetoothAdapter.isEnabled()) {
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
        // Otherwise, setup the chat session
        } else {
            if (mChatService == null) setupChat();
        }
    }
    ///
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.option_menu, menu);
        return true;
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
    	System.out.println("chon item");
        switch (item.getItemId()) {
        case R.id.scan:
        	
            // Launch the DeviceListActivity to see devices and do scan
            Intent serverIntent = new Intent(this, DeviceListActivity.class);
            startActivityForResult(serverIntent, REQUEST_CONNECT_DEVICE);
            return true;
        case R.id.discoverable:
            // Ensure this device is discoverable by others
            ensureDiscoverable();
            return true;
        }
        return false;
    }
    private void ensureDiscoverable() {
        //if(D) Log.d(TAG, "ensure discoverable");
        if (mBluetoothAdapter.getScanMode() !=
            BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE) {
            Intent discoverableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_DISCOVERABLE);
            discoverableIntent.putExtra(BluetoothAdapter.EXTRA_DISCOVERABLE_DURATION, 300);
            startActivity(discoverableIntent);
        }
    }
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
        case REQUEST_CONNECT_DEVICE:
            // When DeviceListActivity returns with a device to connect
            if (resultCode == Activity.RESULT_OK) {
                // Get the device MAC address
                String address = data.getExtras()
                                     .getString(DeviceListActivity.EXTRA_DEVICE_ADDRESS);
                // Get the BLuetoothDevice object
                BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(address);
                // Attempt to connect to the device
                mChatService.connect(device);
            }
            break;
        case REQUEST_ENABLE_BT:
            // When the request to enable Bluetooth returns
            if (resultCode == Activity.RESULT_OK) {
                // Bluetooth is now enabled, so set up a chat session
                setupChat();
            } else {
                // User did not enable Bluetooth or an error occured
                
                Toast.makeText(this, R.string.bt_not_enabled_leaving, Toast.LENGTH_SHORT).show();
                finish();
            }
        }
    }
    private void setupChat() {
        
        // Initialize the array adapter for the conversation thread
        //mConversationArrayAdapter = new ArrayAdapter<String>(this, R.layout.message);
        //mConversationView = (ListView) findViewById(R.id.in);
        //mConversationView.setAdapter(mConversationArrayAdapter);

        // Initialize the compose field with a listener for the return key
        //mOutEditText = (EditText) findViewById(R.id.edit_text_out);
       // mOutEditText.setOnEditorActionListener(mWriteListener);

        // Initialize the send button with a listener that for click events
       // mSendButton = (Button) findViewById(R.id.button_send);
       // mSendButton.setOnClickListener(new OnClickListener() {
            //public void onClick(View v) {
                // Send a message using content of the edit text widget
              //  TextView view = (TextView) findViewById(R.id.edit_text_out);
              //  String message = view.getText().toString();
             //   sendMessage(message);
           // }
      //  });

        // Initialize the BluetoothChatService to perform bluetooth connections
        mChatService = new BluetoothChatService(mHandler);

        // Initialize the buffer for outgoing messages
        mOutStringBuffer = new StringBuffer("");
    }
    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_STATE_CHANGE:
                switch (msg.arg1) {
                case BluetoothChatService.STATE_CONNECTED:
                    //mTitle.setText(R.string.title_connected_to);
                    //mTitle.append(mConnectedDeviceName);
                    //mConversationArrayAdapter.clear();
                    break;
                case BluetoothChatService.STATE_CONNECTING:
                    //mTitle.setText(R.string.title_connecting);
                    break;
                case BluetoothChatService.STATE_LISTEN:
                case BluetoothChatService.STATE_NONE:
                   // mTitle.setText(R.string.title_not_connected);
                    break;
                }
                break;
            case MESSAGE_WRITE:
                //byte[] writeBuf = (byte[]) msg.obj;
                // construct a string from the buffer
               // String writeMessage = new String(writeBuf);
                //mConversationArrayAdapter.add("Me:  " + writeMessage);
                break;
            case MESSAGE_READ:
                byte[] readBuf = (byte[]) msg.obj;
                // construct a string from the valid bytes in the buffer
                String readMessage = new String(readBuf, 0, msg.arg1);
               // mConversationArrayAdapter.add(mConnectedDeviceName+":  " + readMessage);
                Toast.makeText(getApplicationContext(),readMessage,Toast.LENGTH_SHORT).show();
                break;
            case MESSAGE_DEVICE_NAME:
                // save the connected device's name
                mConnectedDeviceName = msg.getData().getString(DEVICE_NAME);
                Toast.makeText(getApplicationContext(), "Connected to "
                               + mConnectedDeviceName, Toast.LENGTH_SHORT).show();
                break;
            case MESSAGE_TOAST:
                Toast.makeText(getApplicationContext(), msg.getData().getString(TOAST),
                               Toast.LENGTH_SHORT).show();
                break;
            }
        }
    };

    @Override
    protected void onResume() {
        super.onResume();
        updateState();
    }

    private int getPolicyIndex(String policy) {
        String[] values = getResources().getStringArray(R.array.policyValues);
        for (int i = 0; i < values.length; i++) {
            if (values[i].equals(policy)) {
                return i;
            }
        }
        throw new IllegalArgumentException("Unknown policy=" + policy);
    }
    private int getPolicyevironIndex(String policy) {
        String[] values = getResources().getStringArray(R.array.evironValues);
        for (int i = 0; i < values.length; i++) {
            if (values[i].equals(policy)) {
                return i;
            }
        }
        throw new IllegalArgumentException("Unknown policy=" + policy);
    }

    private void updateState() {

        //String url = mPassword.getText();
        //if (TextUtils.isEmpty(url)) {
        //    mPassword.setSummary(R.string.no_password_set);
       // } else {
        //    mPassword.setSummary(R.string.password_protected);
       // }
       
        
        int policyIndex = getPolicyIndex("POLICY_SUBJECT_WARNING");
        String[] policySummaries = getResources().getStringArray(R.array.policySummary);
        mPolicy.setSummary(mPolicy.getEntry() + ": " + policySummaries[policyIndex]);
        int evironIndex=getPolicyevironIndex("POLICY_SUBJECT_EVIRON");
        String[] evironSummaries=getResources().getStringArray(R.array.evironSummary);
        meviron.setSummary(meviron.getEntry()+":"+evironSummaries[evironIndex]);
      //  String abc=mPolicy.getEntry().toString();
      //  System.out.println(abc);
    }
   public static String getselectpolicy()
    {
	    
    	return mPolicy.getEntry().toString();
    }
   public static String getselecteviron()
   {
	    
   	return meviron.getEntry().toString();
   }
   public static String geturl()
   {
	 String url = mPassword.getText();
	 return url;
   }
    private final Runnable mUpdateState = new Runnable() {
        public void run() {
            updateState();
        }
    };

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        System.out.println("onPreferenceChange: preference=" + preference.getKey() +
                "; newValue=" + newValue);
        mHandler.post(mUpdateState);
        return true;
    }
}
