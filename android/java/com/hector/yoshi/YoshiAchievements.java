package com.hector.yoshi;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.InputType;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Toast;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.ref.WeakReference;
import java.net.URL;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import javax.net.ssl.HttpsURLConnection;

/** Optional native rc_client transport. Never stores a password/token. */
final class YoshiAchievements {
    private static WeakReference<Activity> activity = new WeakReference<Activity>(null);
    private static final ExecutorService network = Executors.newFixedThreadPool(2);
    static native void initialize();
    static native String status();
    private static native void login(String username, String password);
    private static native void logout();
    private static native void reply(long id, int code, byte[] body);
    private static volatile boolean testOffline;

    static void attach(Activity owner) {
        activity = new WeakReference<Activity>(owner);
        initialize();
        testOffline = owner.getIntent().getBooleanExtra("achievements_test", false);
    }
    static void show(final Activity owner) {
        final LinearLayout content = new LinearLayout(owner);
        content.setOrientation(LinearLayout.VERTICAL);
        int pad = (int)(16 * owner.getResources().getDisplayMetrics().density);
        content.setPadding(pad, 0, pad, 0);
        final EditText username = new EditText(owner);
        username.setHint("Usuario de RetroAchievements");
        username.setSingleLine(true);
        username.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        final EditText password = new EditText(owner);
        password.setHint("Contraseña (solo esta sesión)");
        password.setSingleLine(true);
        password.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        content.addView(username); content.addView(password);
        new AlertDialog.Builder(owner).setTitle("RetroAchievements — Softcore")
            .setMessage(status() + "\n\nOpcional. Las credenciales no se guardan en el dispositivo. Puedes jugar sin conexión en cualquier momento. Los logros pendientes se sincronizarán al reconectar durante la sesión actual.")
            .setView(content)
            .setPositiveButton("Iniciar sesión", new DialogInterface.OnClickListener() {
                @Override public void onClick(DialogInterface dialog, int which) {
                    String name = username.getText().toString().trim();
                    String secret = password.getText().toString();
                    if (!name.isEmpty() && !secret.isEmpty()) login(name, secret);
                    else Toast.makeText(owner, "Introduce usuario y contraseña", Toast.LENGTH_SHORT).show();
                    password.getText().clear();
                }})
            .setNeutralButton("Cerrar sesión", new DialogInterface.OnClickListener() {
                @Override public void onClick(DialogInterface dialog, int which) { logout(); password.getText().clear(); }
            }).setNegativeButton("Volver", null).show();
    }
    // Invoked by native code. Responses are queued for the simulation thread.
    static void dispatch(final long id, final String url, final String post, final String contentType) {
        network.execute(new Runnable() {
            @Override public void run() {
                HttpsURLConnection connection = null;
                int code = -1;
                byte[] bytes = new byte[0];
                try {
                    if (testOffline) throw new java.io.IOException("Offline test");
                    URL target = new URL(url);
                    if (!"https".equals(target.getProtocol()) ||
                        !"retroachievements.org".equals(target.getHost()) ||
                        (target.getPort() != -1 && target.getPort() != 443))
                        throw new java.io.IOException("Unsupported service URL");
                    connection = (HttpsURLConnection)target.openConnection();
                    connection.setConnectTimeout(10000); connection.setReadTimeout(10000);
                    connection.setInstanceFollowRedirects(false);
                    connection.setRequestProperty("User-Agent", "YoshiIslandAndroid/0.1 (softcore)");
                    if (post != null) {
                        byte[] payload = post.getBytes("UTF-8");
                        connection.setRequestMethod("POST"); connection.setDoOutput(true);
                        connection.setRequestProperty("Content-Type", contentType);
                        connection.setFixedLengthStreamingMode(payload.length);
                        try (OutputStream out = connection.getOutputStream()) { out.write(payload); }
                        finally { java.util.Arrays.fill(payload, (byte)0); }
                    }
                    code = connection.getResponseCode();
                    InputStream stream = code >= 400 ? connection.getErrorStream() : connection.getInputStream();
                    if (stream != null) try (InputStream in = stream;
                        ByteArrayOutputStream out = new ByteArrayOutputStream()) {
                        byte[] buffer = new byte[8192]; int count;
                        while ((count = in.read(buffer)) != -1) {
                            if (out.size() + count > 4 * 1024 * 1024) throw new java.io.IOException("Response too large");
                            out.write(buffer, 0, count);
                        }
                        bytes = out.toByteArray();
                    }
                } catch (Exception unavailable) {
                    // No request/credential/server-body logging.
                    code = -1; bytes = new byte[0];
                } finally { if (connection != null) connection.disconnect(); }
                reply(id, code, bytes);
                java.util.Arrays.fill(bytes, (byte)0);
            }
        });
    }
    static void notice(final String message) {
        final Activity owner = activity.get();
        if (owner != null) owner.runOnUiThread(new Runnable() {
            @Override public void run() { Toast.makeText(owner, message, Toast.LENGTH_LONG).show(); }
        });
    }
}
