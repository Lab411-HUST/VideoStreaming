package com.example.restreaming;

import java.util.EnumSet;
import java.util.Iterator;

import android.text.TextUtils;

public enum CameraEviron {
    
	POLICY_SUBJECT_EVIRON;

    public static final CameraEviron DEFAULT_POLICY = POLICY_SUBJECT_EVIRON;

    public static EnumSet<CameraEviron> fromString(String policiesString) {
        EnumSet<CameraEviron> policies = EnumSet.noneOf(CameraEviron.class);
        for (String policyString: policiesString.split("\\|")) {
            if (!TextUtils.isEmpty(policyString)) {
                CameraEviron policy = CameraEviron.valueOf(policyString);
                policies.add(policy);
            }
        }
        return policies;
    }

    public static String toString(EnumSet<CameraEviron> policies) {
        StringBuilder b = new StringBuilder();
        Iterator<CameraEviron> iter = policies.iterator();
        if (iter.hasNext()) {
            b.append(iter.next().toString());
            while (iter.hasNext()) {
                b.append('|');
                b.append(iter.next().toString());
            }
        }
        return b.toString();
    }

}
