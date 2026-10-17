package com.hector.yoshi;

import android.os.Bundle;
import android.view.WindowManager;
import android.view.View;
import android.view.WindowInsets;
import android.widget.TextView;
import android.widget.RelativeLayout;
import org.libsdl.app.SDLActivity;
import java.io.*;

public final class YoshiActivity extends SDLActivity {
    private YoshiControls controls;
    static native void configurePresentation(int soft, boolean lighting);
    static native void setGamePaused(boolean paused);
    static native boolean isGamePaused();
    static native void setSpanish(boolean enabled);
    static native boolean isSpanish();
    @Override protected String[] getLibraries() { return new String[]{"SDL3", "YoshiIsland"}; }
    private String regressionScript() {
        if (getIntent().getBooleanExtra("castle_test", false)) return "castle_diagnostic.script";
        return getIntent().getBooleanExtra("level_test", false) ? "level_route.script" :
            (getIntent().getBooleanExtra("adaptive", false) ? "tutorial.script" : "boot.script");
    }
    @Override protected String[] getArguments() {
        android.content.SharedPreferences options = getSharedPreferences("yoshi-options", MODE_PRIVATE);
        int soft = options.getInt("display", 1);
        boolean lighting = options.getBoolean("lighting", true);
        boolean spanish = options.getBoolean("spanish", true);
        nativeSetenv("SNESRECOMP_LANG", spanish ? "es" : "en");
        if (getIntent().getBooleanExtra("display_test", false)) {
            soft = 0; lighting = false;
            nativeSetenv("SNESRECOMP_PRESENT_TEST_CYCLE", "1");
            nativeSetenv("SNESRECOMP_PRESENT_FRAME", "6000");
            nativeSetenv("SNESRECOMP_PRESENT_CAPTURE",
                new File(getFilesDir(), "presentation.bgrx").getPath());
        }
        if (getIntent().getBooleanExtra("level_test", false) ||
            getIntent().getBooleanExtra("castle_test", false)) {
            nativeSetenv("SNESRECOMP_WS_FRAME_DIR", new File(getFilesDir(), "captures").getPath());
            nativeSetenv("SNESRECOMP_WS_FRAME_NUMBERS", "7819,8020");
            nativeSetenv("SNESRECOMP_PRESENT_FRAME",
                getIntent().getBooleanExtra("castle_test", false) ? "8331" : "8020");
            nativeSetenv("SNESRECOMP_PRESENT_CAPTURE", new File(getFilesDir(), "viewport.bgrx").getPath());
        }
        configurePresentation(soft, lighting);
        YoshiAchievements.attach(this);
        if (getIntent().getBooleanExtra("save_diag", false))
            nativeSetenv("SNESRECOMP_SAVE_DIAG",
                new File(getFilesDir(), "save-diagnostic.txt").getPath());
        if (getIntent().getBooleanExtra("input_diag", false))
            nativeSetenv("SNESRECOMP_INPUT_DIAG",
                new File(getFilesDir(), "input-diagnostic.txt").getPath());
        if (getIntent().getBooleanExtra("controller_test", false)) {
            nativeSetenv("SNESRECOMP_OVERLAY_SELFTEST_PAD", "60");
            nativeSetenv("SNESRECOMP_TEST_LOG",
                new File(getFilesDir(), "controller-test.txt").getPath());
        }
        if (getIntent().getBooleanExtra("regression", false)) {
            File captures = new File(getFilesDir(), "captures");
            captures.mkdirs();
            boolean adaptive = getIntent().getBooleanExtra("adaptive", false);
            if (adaptive) {
                nativeSetenv("SNESRECOMP_WS_DIAG", "1");
                nativeSetenv("SNESRECOMP_WS_DIAG_FILE",
                    new File(captures, "adaptive-width.txt").getPath());
                nativeSetenv("SNESRECOMP_WS_FRAME_FILE",
                    new File(captures, "wide-after_move.bgrx").getPath());
                nativeSetenv("SNESRECOMP_WS_FRAME_NUMBER",
                    getIntent().getBooleanExtra("castle_test", false) ? "8331" :
                    (getIntent().getBooleanExtra("level_test", false) ? "8020" : "6211"));
            } else {
                nativeSetenv("SNESRECOMP_WS_EXTRA", "0");
            }
            nativeSetenv("SNESRECOMP_DUMP_DIR", captures.getPath());
            String script = regressionScript();
            return new String[]{"--no-launcher", "--config", new File(getFilesDir(), "config.ini").getPath(),
                "--rom", new File(getFilesDir(), "yoshi-usa11.sfc").getPath(),
                "--script", new File(getFilesDir(), script).getPath()};
        }
        return new String[]{"--no-launcher", "--config", new File(getFilesDir(), "config.ini").getPath(),
                            "--rom", new File(getFilesDir(), "yoshi-usa11.sfc").getPath()};
    }
    @Override protected void onCreate(Bundle state) {
        try {
            File rom = new File(getFilesDir(), "yoshi-usa11.sfc");
            if (!rom.exists()) {
                File temporary = new File(getFilesDir(), "rom.tmp");
                try (InputStream in = getAssets().open("yoshi-usa11.sfc");
                     FileOutputStream out = new FileOutputStream(temporary)) {
                    byte[] buffer = new byte[65536]; int n;
                    while ((n = in.read(buffer)) != -1) out.write(buffer, 0, n);
                    out.getFD().sync();
                }
                if (!temporary.renameTo(rom)) throw new IOException("Cannot install local game data");
            }
            File config = new File(getFilesDir(), "config.ini");
            if (getIntent().getBooleanExtra("regression", false)) {
                String script = regressionScript();
                try (InputStream in = getAssets().open(script);
                     FileOutputStream out = new FileOutputStream(new File(getFilesDir(), script))) {
                    byte[] buffer = new byte[4096]; int n;
                    while ((n = in.read(buffer)) != -1) out.write(buffer, 0, n);
                }
            }
            File[] targets = new File[] { getExternalFilesDir(null), getFilesDir() };
            for (File baseDir : targets) {
                if (baseDir == null) continue;
                File savesDir = new File(baseDir, "saves");
                savesDir.mkdirs();
                File sram = new File(savesDir, "save.srm");
                if (!sram.exists()) {
                    try (InputStream in = getAssets().open("seed_save.srm");
                         FileOutputStream out = new FileOutputStream(sram)) {
                        byte[] buffer = new byte[4096]; int n;
                        while ((n = in.read(buffer)) != -1) out.write(buffer, 0, n);
                    } catch (IOException ignored) {}
                }
            }
            if (!config.exists()) {
                try (FileOutputStream out = new FileOutputStream(config)) {
                    out.write(("[General]\nAutosave=0\n[Graphics]\nFullscreen=1\nNewRenderer=1\n" +
                               "NoSpriteLimits=1\nIgnoreAspectRatio=0\n[Sound]\nEnableAudio=1\n" +
                               "AudioFreq=32000\nAudioChannels=2\nAudioSamples=512\n" +
                               "[GamepadMap]\nEnableGamepad1=true\n").getBytes("UTF-8"));
                }
            }
        } catch (IOException error) {
            super.onCreate(state);
            TextView message = new TextView(this);
            message.setText("Could not prepare the locally supplied game: " + error.getMessage());
            setContentView(message);
            return;
        }
        super.onCreate(state);
        YoshiHaptics.init(this);
        if (!getIntent().getBooleanExtra("regression", false)) {
            controls = new YoshiControls(this);
            mLayout.addView(controls, new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT));
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        immersive();
    }
    private void immersive() {
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN |
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        // SDL surface draws edge-to-edge into display cutouts on landscape short edges
        if (android.os.Build.VERSION.SDK_INT >= 28) {
            WindowManager.LayoutParams params = getWindow().getAttributes();
            params.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(params);
        }
        // Request 120 Hz display refresh rate when supported (presentation only, keeping 60 Hz SNES simulation)
        if (android.os.Build.VERSION.SDK_INT >= 23) {
            try {
                android.view.Display display = (android.os.Build.VERSION.SDK_INT >= 30) ? getDisplay() : getWindowManager().getDefaultDisplay();
                if (display != null) {
                    android.view.Display.Mode[] modes = display.getSupportedModes();
                    android.view.Display.Mode best120 = null;
                    if (modes != null) {
                        for (android.view.Display.Mode m : modes) {
                            if (Math.round(m.getRefreshRate()) >= 119) {
                                if (best120 == null || m.getRefreshRate() > best120.getRefreshRate()) {
                                    best120 = m;
                                }
                            }
                        }
                    }
                    WindowManager.LayoutParams params = getWindow().getAttributes();
                    if (best120 != null) {
                        params.preferredDisplayModeId = best120.getModeId();
                    }
                    params.preferredRefreshRate = 120.0f;
                    getWindow().setAttributes(params);
                }
            } catch (Throwable ignored) {}
        }
        if (android.os.Build.VERSION.SDK_INT >= 30) {
            View content = findViewById(android.R.id.content);
            content.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener() {
                @Override public WindowInsets onApplyWindowInsets(View view, WindowInsets insets) {
                    android.graphics.Insets safe = insets.getInsets(WindowInsets.Type.displayCutout() |
                                                                     WindowInsets.Type.systemBars());
                    view.setPadding(0, 0, 0, 0);
                    if (controls != null) {
                        controls.setPadding(safe.left, safe.top, safe.right, safe.bottom);
                    }
                    return insets;
                }
            });
            content.requestApplyInsets();
        }
    }
    @Override public void onWindowFocusChanged(boolean focused) {
        super.onWindowFocusChanged(focused);
        if (focused) immersive();
    }
    @Override public void onConfigurationChanged(android.content.res.Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        immersive();
        View content = findViewById(android.R.id.content);
        if (content != null) content.requestApplyInsets();
    }
    @Override protected void onPause() {
        if (controls != null) controls.releaseAll();
        super.onPause();
    }
}
