package com.hector.yoshi;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.hardware.input.InputManager;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;
import org.libsdl.app.SDLActivity;
import java.util.HashSet;
import java.util.Set;

/**
 * Multitouch controls and in-game pause menu for Yoshi's Island on Android.
 * Renders authentic styled on-screen controls, discreet menu emblem with safe insets,
 * tactile haptic triggers, and an immersive parchment-themed pause menu in Spanish.
 */
final class YoshiControls extends View implements InputManager.InputDeviceListener {
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final SharedPreferences prefs;
    private final InputManager inputs;
    private final Set<Integer> held = new HashSet<Integer>();
    private boolean visible;
    private int mode; // 0 = Auto, 1 = Show, 2 = Hide

    YoshiControls(Context context) {
        super(context);
        prefs = context.getSharedPreferences("yoshi-options", Context.MODE_PRIVATE);
        inputs = (InputManager) context.getSystemService(Context.INPUT_SERVICE);
        mode = prefs.getInt("touch", 0);
        setFocusable(false);
        setContentDescription("Controles y opciones táctiles de Yoshi's Island");
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
                (sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) {
                gamepad = true;
            }
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

    private float scale() {
        float usableW = getWidth() - getPaddingLeft() - getPaddingRight();
        float usableH = getHeight() - getPaddingTop() - getPaddingBottom();
        if (usableW <= 0) usableW = getWidth();
        if (usableH <= 0) usableH = getHeight();
        return Math.min(usableH / 448f, usableW / 800f);
    }

    private float[][] buttons() {
        float s = scale();
        float pl = getPaddingLeft(), pr = getPaddingRight();
        float pt = getPaddingTop(), pb = getPaddingBottom();
        float w = getWidth() - pr, h = getHeight() - pb;
        float midX = pl + (getWidth() - pl - pr) / 2f;
        return new float[][] {
            {w - 84*s, h - 54*s, KeyEvent.KEYCODE_Z},           // B salto
            {w - 38*s, h - 100*s, KeyEvent.KEYCODE_X},          // A huevo
            {w - 130*s, h - 100*s, KeyEvent.KEYCODE_A},         // Y lengua
            {w - 84*s, h - 146*s, KeyEvent.KEYCODE_S},          // X
            {midX - 42*s, h - 25*s, KeyEvent.KEYCODE_SHIFT_RIGHT}, // Select
            {midX + 42*s, h - 25*s, KeyEvent.KEYCODE_ENTER},       // Start
            {pl + 40*s, pt + 36*s, KeyEvent.KEYCODE_C},         // L
            {w - 90*s, pt + 36*s, KeyEvent.KEYCODE_V}           // R
        };
    }

    private boolean near(float x, float y, float cx, float cy, float radius) {
        float dx = x - cx, dy = y - cy;
        return dx*dx + dy*dy <= radius*radius;
    }

    private void keysAt(float x, float y, Set<Integer> keys) {
        if (!visible) return;
        float s = scale(), pl = getPaddingLeft(), pb = getPaddingBottom();
        float cx = pl + 76*s, cy = getHeight() - pb - 80*s;
        if (near(x, y, cx, cy, 58*s)) {
            if (x < cx - 12*s) keys.add(KeyEvent.KEYCODE_DPAD_LEFT);
            if (x > cx + 12*s) keys.add(KeyEvent.KEYCODE_DPAD_RIGHT);
            if (y < cy - 12*s) keys.add(KeyEvent.KEYCODE_DPAD_UP);
            if (y > cy + 12*s) keys.add(KeyEvent.KEYCODE_DPAD_DOWN);
        }
        for (float[] b : buttons()) {
            if (near(x, y, b[0], b[1], 23*s)) keys.add((int)b[2]);
        }
    }

    private String getTouchModeDescription() {
        switch (mode) {
            case 0: return "Automático (ocultar con mando)";
            case 1: return "Siempre visibles";
            case 2: return "Ocultos";
            default: return "Automático";
        }
    }

    private String getHapticsDescription() {
        int h = YoshiHaptics.getMode();
        switch (h) {
            case 0: return "Desactivada";
            case 1: return "Sutil (Recomendada)";
            case 2: return "Normal";
            default: return "Sutil";
        }
    }

    private String getLanguageDescription() {
        boolean spanish = prefs.getBoolean("spanish", true);
        return spanish ? "Español (Wave USA Rev 1)" : "English (Original USA)";
    }

    private LinearLayout createMenuRow(Context ctx, String iconText, String titleText,
                                       final TextView[] outSubtitle, float dp) {
        LinearLayout row = new LinearLayout(ctx);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        int padH = (int)(14 * dp), padV = (int)(10 * dp);
        row.setPadding(padH, padV, padH, padV);

        StateListDrawable selector = new StateListDrawable();
        GradientDrawable pressed = new GradientDrawable();
        pressed.setColor(0xFFDED0B4);
        pressed.setCornerRadius(10 * dp);
        GradientDrawable normal = new GradientDrawable();
        normal.setColor(0xFFF3ECE0);
        normal.setCornerRadius(10 * dp);
        normal.setStroke((int)(1 * dp), 0xFFD8CCB2);
        selector.addState(new int[]{android.R.attr.state_pressed}, pressed);
        selector.addState(new int[]{android.R.attr.state_focused}, pressed);
        selector.addState(new int[]{}, normal);
        row.setBackground(selector);

        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.setMargins(0, 0, 0, (int)(8 * dp));
        row.setLayoutParams(lp);

        TextView icon = new TextView(ctx);
        icon.setText(iconText);
        icon.setTextSize(18);
        icon.setPadding(0, 0, (int)(12 * dp), 0);
        row.addView(icon);

        LinearLayout textCol = new LinearLayout(ctx);
        textCol.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams colLp = new LinearLayout.LayoutParams(
            0, LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f);
        textCol.setLayoutParams(colLp);

        TextView title = new TextView(ctx);
        title.setText(titleText);
        title.setTextSize(14);
        title.setTextColor(0xFF263C1B);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        textCol.addView(title);

        TextView subtitle = new TextView(ctx);
        subtitle.setTextSize(11);
        subtitle.setTextColor(0xFF5A7249);
        textCol.addView(subtitle);
        if (outSubtitle != null && outSubtitle.length > 0) outSubtitle[0] = subtitle;

        row.addView(textCol);
        return row;
    }

    private void options() {
        releaseAll();
        YoshiHaptics.trigger(YoshiHaptics.EVENT_UI, this);
        YoshiActivity.setGamePaused(true);

        final Context ctx = getContext();
        final Dialog dialog = new Dialog(ctx, android.R.style.Theme_DeviceDefault_Dialog_NoActionBar);
        if (dialog.getWindow() != null) {
            dialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        }

        float dp = getResources().getDisplayMetrics().density;
        int p12 = (int)(12 * dp), p16 = (int)(16 * dp);

        ScrollView scroll = new ScrollView(ctx);
        scroll.setVerticalScrollBarEnabled(false);

        LinearLayout root = new LinearLayout(ctx);
        root.setOrientation(LinearLayout.VERTICAL);

        GradientDrawable bg = new GradientDrawable();
        bg.setColor(0xFFFAF5EB);
        bg.setCornerRadius(16 * dp);
        bg.setStroke((int)(2 * dp), 0xFF355E24);
        root.setBackground(bg);

        // Header banner
        LinearLayout header = new LinearLayout(ctx);
        header.setOrientation(LinearLayout.VERTICAL);
        GradientDrawable headerBg = new GradientDrawable();
        headerBg.setColor(0xFF387627);
        headerBg.setCornerRadii(new float[]{16*dp, 16*dp, 16*dp, 16*dp, 0, 0, 0, 0});
        header.setBackground(headerBg);
        header.setPadding(p16, p12, p16, p12);

        TextView brand = new TextView(ctx);
        brand.setText("SUPER MARIO WORLD 2");
        brand.setTextSize(10);
        brand.setTextColor(0xFFD6F5BC);
        brand.setTypeface(Typeface.DEFAULT_BOLD);
        brand.setGravity(Gravity.CENTER);

        TextView title = new TextView(ctx);
        title.setText("YOSHI'S ISLAND");
        title.setTextSize(18);
        title.setTextColor(0xFFFFFFFF);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setGravity(Gravity.CENTER);

        header.addView(brand);
        header.addView(title);
        root.addView(header);

        // Options body
        LinearLayout body = new LinearLayout(ctx);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setPadding(p16, p12, p16, p16);

        // 1. Continuar
        final TextView[] subResume = new TextView[1];
        LinearLayout rowResume = createMenuRow(ctx, "▶", "Continuar partida", subResume, dp);
        subResume[0].setText("Reanudar la aventura actual");
        rowResume.setOnClickListener(new OnClickListener() {
            @Override public void onClick(View v) {
                YoshiHaptics.trigger(YoshiHaptics.EVENT_UI, v);
                dialog.dismiss();
            }
        });
        body.addView(rowResume);

        // 2. Guardar / Cargar (Ranuras)
        final TextView[] subSave = new TextView[1];
        LinearLayout rowSave = createMenuRow(ctx, "💾", "Guardar / Cargar partida", subSave, dp);
        subSave[0].setText("Abrir menú de ranuras rápidas");
        rowSave.setOnClickListener(new OnClickListener() {
            @Override public void onClick(View v) {
                YoshiHaptics.trigger(YoshiHaptics.EVENT_UI, v);
                dialog.dismiss();
                postDelayed(new Runnable() {
                    @Override public void run() {
                        SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_F11);
                        SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_F11);
                    }
                }, 100);
            }
        });
        body.addView(rowSave);

        // 3. Controles táctiles
        final TextView[] subTouch = new TextView[1];
        LinearLayout rowTouch = createMenuRow(ctx, "🎮", "Controles en pantalla", subTouch, dp);
        subTouch[0].setText(getTouchModeDescription());
        rowTouch.setOnClickListener(new OnClickListener() {
            @Override public void onClick(View v) {
                mode = (mode + 1) % 3;
                prefs.edit().putInt("touch", mode).apply();
                refreshController();
                subTouch[0].setText(getTouchModeDescription());
                YoshiHaptics.trigger(YoshiHaptics.EVENT_UI, v);
            }
        });
        body.addView(rowTouch);

        // 4. Vibración háptica
        final TextView[] subHaptic = new TextView[1];
        LinearLayout rowHaptic = createMenuRow(ctx, "📳", "Vibración háptica", subHaptic, dp);
        subHaptic[0].setText(getHapticsDescription());
        rowHaptic.setOnClickListener(new OnClickListener() {
            @Override public void onClick(View v) {
                int next = (YoshiHaptics.getMode() + 1) % 3;
                YoshiHaptics.setMode(ctx, next);
                subHaptic[0].setText(getHapticsDescription());
                YoshiHaptics.trigger(YoshiHaptics.EVENT_UI, v);
            }
        });
        body.addView(rowHaptic);

        // 5. Idioma
        final TextView[] subLang = new TextView[1];
        LinearLayout rowLang = createMenuRow(ctx, "🌐", "Idioma de la partida", subLang, dp);
        subLang[0].setText(getLanguageDescription());
        rowLang.setOnClickListener(new OnClickListener() {
            @Override public void onClick(View v) {
                boolean cur = prefs.getBoolean("spanish", true);
                boolean next = !cur;
                prefs.edit().putBoolean("spanish", next).apply();
                YoshiActivity.setSpanish(next);
                subLang[0].setText(getLanguageDescription());
                YoshiHaptics.trigger(YoshiHaptics.EVENT_UI, v);
                Toast.makeText(ctx, next ? "Idioma: Español (Wave USA Rev 1)" : "Language: English", Toast.LENGTH_SHORT).show();
            }
        });
        body.addView(rowLang);

        // 6. RetroAchievements
        final TextView[] subAch = new TextView[1];
        LinearLayout rowAch = createMenuRow(ctx, "🏆", "RetroAchievements", subAch, dp);
        subAch[0].setText("Iniciar sesión / Estado softcore");
        rowAch.setOnClickListener(new OnClickListener() {
            @Override public void onClick(View v) {
                YoshiHaptics.trigger(YoshiHaptics.EVENT_UI, v);
                dialog.dismiss();
                if (ctx instanceof Activity) {
                    YoshiAchievements.show((Activity)ctx);
                }
            }
        });
        body.addView(rowAch);

        root.addView(body);
        scroll.addView(root);
        dialog.setContentView(scroll);

        dialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override public void onDismiss(DialogInterface d) {
                YoshiActivity.setGamePaused(false);
            }
        });

        dialog.show();
    }

    @Override public boolean onTouchEvent(MotionEvent event) {
        float s = scale();
        float pr = getPaddingRight(), pt = getPaddingTop();
        // Menu button touch area in top-right with safe insets
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN &&
            near(event.getX(), event.getY(), getWidth() - pr - 26*s, pt + 24*s, 24*s)) {
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

        // Release buttons no longer held
        for (int key : held) {
            if (!next.contains(key)) SDLActivity.onNativeKeyUp(key);
        }

        // Press new buttons and provide tactile haptic response
        for (int key : next) {
            if (!held.contains(key)) {
                SDLActivity.onNativeKeyDown(key);
                if (key == KeyEvent.KEYCODE_Z) {
                    YoshiHaptics.trigger(YoshiHaptics.EVENT_JUMP, this);
                } else if (key == KeyEvent.KEYCODE_X) {
                    YoshiHaptics.trigger(YoshiHaptics.EVENT_EGG, this);
                } else {
                    YoshiHaptics.trigger(YoshiHaptics.EVENT_UI, this);
                }
            }
        }

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

    private void drawMenuButton(Canvas canvas, float s, float pr, float pt) {
        float bx = getWidth() - pr - 26*s;
        float by = pt + 24*s;
        float radius = 17*s;

        // Soft green disc
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(0xD02F5B22);
        canvas.drawCircle(bx, by, radius, paint);

        // Cream outline
        paint.setStyle(Paint.Style.STROKE);
        paint.setColor(0xE6F8F3DC);
        paint.setStrokeWidth(1.6f * s);
        canvas.drawCircle(bx, by, radius, paint);

        // Pause symbol (two rounded vertical bars ❚❚)
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(0xFFFDFCF8);
        float barW = 3.2f * s;
        float barH = 12.0f * s;
        float gap = 3.2f * s;
        float rad = 1.2f * s;
        canvas.drawRoundRect(bx - gap/2 - barW, by - barH/2, bx - gap/2, by + barH/2, rad, rad, paint);
        canvas.drawRoundRect(bx + gap/2, by - barH/2, bx + gap/2 + barW, by + barH/2, rad, rad, paint);
    }

    @Override protected void onDraw(Canvas canvas) {
        float s = scale();
        float pl = getPaddingLeft(), pr = getPaddingRight();
        float pt = getPaddingTop(), pb = getPaddingBottom();
        if (visible) {
            String[] names = {"B", "A", "Y", "X", "Select", "Start", "L", "R"};
            float[][] bs = buttons();
            for (int i = 0; i < bs.length; ++i) {
                circle(canvas, bs[i][0], bs[i][1], (i < 4 ? 22 : 20)*s, names[i], (int)bs[i][2]);
            }
            float x = pl + 76*s, y = getHeight() - pb - 80*s;
            circle(canvas, x - 34*s, y, 22*s, "<", KeyEvent.KEYCODE_DPAD_LEFT);
            circle(canvas, x + 34*s, y, 22*s, ">", KeyEvent.KEYCODE_DPAD_RIGHT);
            circle(canvas, x, y - 34*s, 22*s, "^", KeyEvent.KEYCODE_DPAD_UP);
            circle(canvas, x, y + 34*s, 22*s, "v", KeyEvent.KEYCODE_DPAD_DOWN);
        }
        drawMenuButton(canvas, s, pr, pt);
    }
}
