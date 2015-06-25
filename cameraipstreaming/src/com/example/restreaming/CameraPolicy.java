package com.example.restreaming;

import android.text.TextUtils;

import java.util.EnumSet;
import java.util.Iterator;

public enum CameraPolicy {
    
    POLICY_SUBJECT_WARNING;

    public static final CameraPolicy DEFAULT_POLICY = POLICY_SUBJECT_WARNING;

    public static EnumSet<CameraPolicy> fromString(String policiesString) {
        EnumSet<CameraPolicy> policies = EnumSet.noneOf(CameraPolicy.class);
        for (String policyString: policiesString.split("\\|")) {
            if (!TextUtils.isEmpty(policyString)) {
                CameraPolicy policy = CameraPolicy.valueOf(policyString);
                policies.add(policy);
            }
        }
        return policies;
    }

    public static String toString(EnumSet<CameraPolicy> policies) {
        StringBuilder b = new StringBuilder();
        Iterator<CameraPolicy> iter = policies.iterator();
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
