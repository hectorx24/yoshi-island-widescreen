package com.hector.yoshi;

import android.os.Bundle;
import android.view.WindowManager;
import android.view.View;
import android.view.WindowInsets;
import android.widget.TextView;
import org.libsdl.app.SDLActivity;
import java.io.*;

public final class YoshiActivity extends SDLActivity {
    @Override protected String[] getLibraries() { return new String[]{"SDL3", "YoshiIsland"}; }
    @Override protected String[] getArguments() {
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
                nativeSetenv("SNESRECOMP_WS_FRAME_NUMBER", "6211");
            } else {
                nativeSetenv("SNESRECOMP_WS_EXTRA", "0");
            }
            nativeSetenv("SNESRECOMP_DUMP_DIR", captures.getPath());
            String script = adaptive ? "tutorial.script" : "boot.script";
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
                String script = getIntent().getBooleanExtra("adaptive", false)
                    ? "tutorial.script" : "boot.script";
                try (InputStream in = getAssets().open(script);
                     FileOutputStream out = new FileOutputStream(new File(getFilesDir(), script))) {
                    byte[] buffer = new byte[4096]; int n;
                    while ((n = in.read(buffer)) != -1) out.write(buffer, 0, n);
                }
            }
            if (!config.exists()) {
                try (FileOutputStream out = new FileOutputStream(config)) {
                    out.write(("[General]\nAutosave=0\n[Graphics]\nFullscreen=1\nNewRenderer=1\n" +
                               "NoSpriteLimits=0\nIgnoreAspectRatio=0\n[Sound]\nEnableAudio=1\n" +
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
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        immersive();
    }
    private void immersive() {
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN |
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        // SDL's surface stays inside the platform safe area. No cutout is
        // assumed safe merely because the display is wide.
        if (android.os.Build.VERSION.SDK_INT >= 28) {
            WindowManager.LayoutParams params = getWindow().getAttributes();
            params.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
            getWindow().setAttributes(params);
        }
        if (android.os.Build.VERSION.SDK_INT >= 30) {
            View content = findViewById(android.R.id.content);
            content.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener() {
                @Override public WindowInsets onApplyWindowInsets(View view, WindowInsets insets) {
                    android.graphics.Insets safe = insets.getInsets(WindowInsets.Type.displayCutout() |
                                                                     WindowInsets.Type.systemBars());
                    view.setPadding(safe.left, safe.top, safe.right, safe.bottom);
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
}
