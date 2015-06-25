package com.example.restreaming;

import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.Preference.OnPreferenceChangeListener;

public class opencvsetting extends PreferenceActivity {
	
	 static ListPreference thresold;
	static  ListPreference minpixel;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		//mApplication = (Application) getApplication();
		//mSerialPortFinder = mApplication.mSerialPortFinder;

		addPreferencesFromResource(R.xml.serial_port_preferences);
		thresold = (ListPreference)findPreference("THRESOLD");
		//thresold.setSummary(thresold.getValue());
		System.out.println("dachon"+thresold.getValue());

		minpixel = (ListPreference)findPreference("MINPIXEL");
		//minpixel.setSummary(minpixel.getValue());
		System.out.println("dachon"+minpixel.getValue());
//	    minpixel.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
//			public boolean onPreferenceChange(Preference preference, Object newValue) {
//				preference.setSummary((String)newValue);
//				return true;
//			}
//		});
	}
	public static String getthresold()
	{
		thresold.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
		public boolean onPreferenceChange(Preference preference, Object newValue) {
			preference.setSummary((String)newValue);
			return true;
		}
	});
		return thresold.getValue();
	}
	public static String getminpixel()
	{
		minpixel.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
		public boolean onPreferenceChange(Preference preference, Object newValue) {
			preference.setSummary((String)newValue);
			return true;
		}
	});
		return minpixel.getValue();
	}
}
