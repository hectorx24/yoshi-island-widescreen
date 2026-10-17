package com.hector.yoshi;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.view.HapticFeedbackConstants;
import android.view.View;

/**
 * Event-driven tactile haptics for Yoshi's Island on Android.
 * Follows Android developer guidelines: subtle, short duration, event-based,
 * no continuous buzzing, throttled rain cooldown, and user-configurable intensity.
 */
public final class YoshiHaptics {
    public static final int EVENT_UI = 0;
    public static final int EVENT_JUMP = 1;
    public static final int EVENT_EGG = 2;
    public static final int EVENT_LANDING = 3;
    public static final int EVENT_SPLASH = 4;
    public static final int EVENT_DAMAGE = 5;
    public static final int EVENT_BOSS = 6;
    public static final int EVENT_RAIN = 7;

    private static Vibrator vibrator;
    private static int mode = 1; // 0 = Desactivada, 1 = Sutil (Recomendada), 2 = Normal
    private static long lastRainTime = 0;
    private static long lastEventTime = 0;

    public static void init(Context context) {
        vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        SharedPreferences prefs = context.getSharedPreferences("yoshi-options", Context.MODE_PRIVATE);
        mode = prefs.getInt("haptics", 1);
    }

    public static void setMode(Context context, int newMode) {
        mode = newMode;
        if (context != null) {
            context.getSharedPreferences("yoshi-options", Context.MODE_PRIVATE)
                   .edit().putInt("haptics", newMode).apply();
        }
    }

    public static int getMode() {
        return mode;
    }

    /** Called from native C code on emulated game events. */
    public static void onNativeHaptic(int eventId) {
        trigger(eventId, null);
    }

    public static void trigger(int event, View view) {
        if (mode == 0 || vibrator == null || !vibrator.hasVibrator()) return;

        long now = System.currentTimeMillis();
        // Global throttle: minimum 45ms between non-rain events to prevent overlap
        if (event != EVENT_RAIN && now - lastEventTime < 45) return;
        lastEventTime = now;

        if (event == EVENT_RAIN) {
            // Plan Maestro: minimum 1000ms cooldown for rain
            if (now - lastRainTime < 1000) return;
            lastRainTime = now;
        }

        float scale = (mode == 1) ? 0.6f : 1.0f; // Sutil (0.6x) vs Normal (1.0x)

        try {
            if (Build.VERSION.SDK_INT >= 29) {
                switch (event) {
                    case EVENT_UI:
                        if (view != null) {
                            view.performHapticFeedback(HapticFeedbackConstants.CLOCK_TICK);
                        } else {
                            vibrator.vibrate(VibrationEffect.createPredefined(VibrationEffect.EFFECT_CLICK));
                        }
                        return;
                    case EVENT_JUMP:
                        vibrator.vibrate(VibrationEffect.createOneShot(18, Math.min(255, (int)(75 * scale))));
                        return;
                    case EVENT_EGG:
                        vibrator.vibrate(VibrationEffect.createPredefined(VibrationEffect.EFFECT_CLICK));
                        return;
                    case EVENT_LANDING:
                        vibrator.vibrate(VibrationEffect.createOneShot(26, Math.min(255, (int)(90 * scale))));
                        return;
                    case EVENT_SPLASH:
                        vibrator.vibrate(VibrationEffect.createOneShot(35, Math.min(255, (int)(110 * scale))));
                        return;
                    case EVENT_DAMAGE:
                        vibrator.vibrate(VibrationEffect.createPredefined(VibrationEffect.EFFECT_HEAVY_CLICK));
                        return;
                    case EVENT_BOSS:
                        vibrator.vibrate(VibrationEffect.createOneShot(75, Math.min(255, (int)(210 * scale))));
                        return;
                    case EVENT_RAIN:
                        vibrator.vibrate(VibrationEffect.createOneShot(10, Math.min(255, (int)(35 * scale))));
                        return;
                }
            } else if (Build.VERSION.SDK_INT >= 26) {
                int duration = 20;
                int amp = (int)(80 * scale);
                switch (event) {
                    case EVENT_UI: duration = 12; amp = (int)(55 * scale); break;
                    case EVENT_JUMP: duration = 18; amp = (int)(75 * scale); break;
                    case EVENT_EGG: duration = 22; amp = (int)(95 * scale); break;
                    case EVENT_LANDING: duration = 26; amp = (int)(90 * scale); break;
                    case EVENT_SPLASH: duration = 32; amp = (int)(105 * scale); break;
                    case EVENT_DAMAGE: duration = 55; amp = (int)(160 * scale); break;
                    case EVENT_BOSS: duration = 75; amp = (int)(210 * scale); break;
                    case EVENT_RAIN: duration = 10; amp = (int)(35 * scale); break;
                }
                vibrator.vibrate(VibrationEffect.createOneShot(duration, Math.max(1, Math.min(255, amp))));
            } else {
                vibrator.vibrate(20);
            }
        } catch (Throwable ignored) {}
    }
}
