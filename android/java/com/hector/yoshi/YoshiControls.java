package com.hector.yoshi;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.hardware.input.InputManager;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import org.libsdl.app.SDLActivity;
import java.util.HashSet;
import java.util.Set;

/** Optional multitouch controls. Native controller events continue through SDL;
 * this view only emits the same keyboard events as the documented keybinds. */
final class YoshiControls extends View implements InputManager.InputDeviceListener {
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final SharedPreferences prefs;
    private final InputManager inputs;
    private final Set<Integer> held = new HashSet<Integer>();
    private boolean visible;
    private int mode;

    YoshiControls(Context context) {
        super(context);
        prefs = context.getSharedPreferences("yoshi-options", Context.MODE_PRIVATE);
        inputs = (InputManager) context.getSystemService(Context.INPUT_SERVICE);
        mode = prefs.getInt("touch", 0); // Auto / Show / Hide.
        setFocusable(false);
        setContentDescription("Yoshi touch controls and options");
        refreshController();
    }
    @Override protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        inputs.registerInputDeviceListener(this, null);
    }
    @Override protected void onDetachedFromWindow() {
        releaseAll();
        inputs.unregisterInputDeviceListener(this);
        super.onDetachedFromWindow();
    }
    @Override public void onInputDeviceAdded(int id) { refreshController(); }
    @Override public void onInputDeviceRemoved(int id) { refreshController(); }
    @Override public void onInputDeviceChanged(int id) { refreshController(); }
    private void refreshController() {
        boolean gamepad = false;
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice d = InputDevice.getDevice(id);
            if (d == null) continue;
            int sources = d.getSources();
            if ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
                (sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)
                gamepad = true;
        }
        visible = mode == 1 || (mode == 0 && !gamepad);
        if (!visible) releaseAll();
        invalidate();
    }
    void releaseAll() {
        for (int key : held) SDLActivity.onNativeKeyUp(key);
        held.clear();
        invalidate();
    }
    private float scale() { return Math.min(getHeight() / 448f, getWidth() / 800f); }
    private float[][] buttons() {
        float s = scale(), w = getWidth(), h = getHeight();
        return new float[][] {
            {w - 84*s, h - 54*s, KeyEvent.KEYCODE_Z}, // B jump
            {w - 38*s, h - 100*s, KeyEvent.KEYCODE_X}, // A egg
            {w - 130*s, h - 100*s, KeyEvent.KEYCODE_A}, // Y tongue
            {w - 84*s, h - 146*s, KeyEvent.KEYCODE_S}, // X
            {w/2 - 42*s, h - 25*s, KeyEvent.KEYCODE_SHIFT_RIGHT},
            {w/2 + 42*s, h - 25*s, KeyEvent.KEYCODE_ENTER},
            {40*s, 36*s, KeyEvent.KEYCODE_C},
            {w - 90*s, 36*s, KeyEvent.KEYCODE_V}
        };
    }
    private boolean near(float x, float y, float cx, float cy, float radius) {
        float dx = x - cx, dy = y - cy;
        return dx*dx + dy*dy <= radius*radius;
    }
    private void keysAt(float x, float y, Set<Integer> keys) {
        if (!visible) return;
        float s = scale(), cx = 76*s, cy = getHeight() - 80*s;
        if (near(x, y, cx, cy, 58*s)) {
            if (x < cx - 12*s) keys.add(KeyEvent.KEYCODE_DPAD_LEFT);
            if (x > cx + 12*s) keys.add(KeyEvent.KEYCODE_DPAD_RIGHT);
            if (y < cy - 12*s) keys.add(KeyEvent.KEYCODE_DPAD_UP);
            if (y > cy + 12*s) keys.add(KeyEvent.KEYCODE_DPAD_DOWN);
        }
        for (float[] b : buttons())
            if (near(x, y, b[0], b[1], 23*s)) keys.add((int)b[2]);
    }
    private void options() {
        releaseAll();
        new AlertDialog.Builder(getContext()).setTitle("Yoshi Island")
            .setItems(new String[]{"Touch controls", "Save / load state", "Close game menu"},
                new DialogInterface.OnClickListener() {
                @Override public void onClick(DialogInterface dialog, int which) {
                    if (which == 0) {
                        new AlertDialog.Builder(getContext()).setTitle("Touch controls")
                            .setSingleChoiceItems(new String[]{"Auto (hide with a controller)", "Show", "Hide"},
                                mode, new DialogInterface.OnClickListener() {
                                @Override public void onClick(DialogInterface choice, int value) {
                                    mode = value;
                                    prefs.edit().putInt("touch", value).apply();
                                    refreshController();
                                    choice.dismiss();
                                }}).setNegativeButton("Cancel", null).show();
                    } else {
                        int key = which == 1 ? KeyEvent.KEYCODE_F11 : KeyEvent.KEYCODE_ESCAPE;
                        SDLActivity.onNativeKeyDown(key);
                        SDLActivity.onNativeKeyUp(key);
                    }
                }}).setNegativeButton("Back", null).show();
    }
    @Override public boolean onTouchEvent(MotionEvent event) {
        float s = scale();
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN &&
            near(event.getX(), event.getY(), getWidth() - 24*s, 24*s, 22*s)) {
            options();
            return true;
        }
        Set<Integer> next = new HashSet<Integer>();
        int action = event.getActionMasked(), up = event.getActionIndex();
        if (action != MotionEvent.ACTION_CANCEL && action != MotionEvent.ACTION_UP) {
            for (int i = 0; i < event.getPointerCount(); ++i) {
                if (action == MotionEvent.ACTION_POINTER_UP && i == up) continue;
                keysAt(event.getX(i), event.getY(i), next);
            }
        }
        if (action == MotionEvent.ACTION_DOWN && next.isEmpty()) return false;
        for (int key : held) if (!next.contains(key)) SDLActivity.onNativeKeyUp(key);
        for (int key : next) if (!held.contains(key)) SDLActivity.onNativeKeyDown(key);
        held.clear();
        held.addAll(next);
        invalidate();
        return true;
    }
    private void circle(Canvas canvas, float x, float y, float radius, String label, int key) {
        paint.setColor(held.contains(key) ? 0x884a8cbb : 0x44202c38);
        paint.setStyle(Paint.Style.FILL);
        canvas.drawCircle(x, y, radius, paint);
        paint.setColor(0x99ffffff);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(scale());
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize((label.length() > 2 ? 8 : 13) * scale());
        canvas.drawText(label, x, y - (paint.ascent() + paint.descent())/2, paint);
    }
    @Override protected void onDraw(Canvas canvas) {
        float s = scale();
        if (visible) {
            String[] names = {"B", "A", "Y", "X", "Select", "Start", "L", "R"};
            float[][] bs = buttons();
            for (int i = 0; i < bs.length; ++i)
                circle(canvas, bs[i][0], bs[i][1], (i < 4 ? 22 : 20)*s, names[i], (int)bs[i][2]);
            float x = 76*s, y = getHeight() - 80*s;
            circle(canvas, x - 34*s, y, 22*s, "<", KeyEvent.KEYCODE_DPAD_LEFT);
            circle(canvas, x + 34*s, y, 22*s, ">", KeyEvent.KEYCODE_DPAD_RIGHT);
            circle(canvas, x, y - 34*s, 22*s, "^", KeyEvent.KEYCODE_DPAD_UP);
            circle(canvas, x, y + 34*s, 22*s, "v", KeyEvent.KEYCODE_DPAD_DOWN);
        }
        circle(canvas, getWidth() - 24*s, 24*s, 16*s, "...", -1);
    }
}
