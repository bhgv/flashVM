package org.flashvm.example; /* Match the real JNI entrypoint names in bridge.cpp. */
import android.Manifest; /* Name the actual microphone permission required before STT. */
import android.app.Activity; /* Let the real host Activity own runtime permission dialogs. */
import android.content.Context; /* Pass only the application Context to native speech setup. */
import android.content.pm.PackageManager; /* Check whether the user has already granted microphone access. */
import java.nio.charset.StandardCharsets; /* Preserve standard UTF-8 at the native boundary. */
/* Android host integration source, not an automatically runnable desktop main program. */
public final class SpeechHost { /* Serialize all methods on the application's UI thread. */
    static { System.loadLibrary("flashvm_speech_example"); } /* Load the actual host JNI shared target. */
    private static final int MICROPHONE_REQUEST = 41; /* Let the Activity identify its own explicit permission request. */
    /* Bind actual Context/helper globals and explicitly create real Android TTS. */
    public static native int openTts(Context context); /* Use activity.getApplicationContext() from the host. */
    /* Submit standard UTF-8; NOT_FOUND means actual asynchronous TTS initialization is still pending. */
    public static native int speak(byte[] text); /* Keep requested text in the UI and retry after readiness instead of losing it. */
    /* Explicitly start actual recognition only after the host checks RECORD_AUDIO. */
    public static native int openStt(Context context); /* Recognition may use the configured system provider's processing policy. */
    /* Receive marker plus actual UTF-8; native errors raise an ordinary host exception. */
    public static native byte[] poll(); /* Null means no transcript yet unless an exception is raised. */
    /* Release sessions before native Context/class global references. */
    public static native void close(); /* Call from Activity.onDestroy or the host's explicit stop action. */
    /* Demonstrate actual microphone preparation without silently starting capture after a permission dialog. */
    public static boolean prepareMicrophone(Activity activity) { /* The UI decides when the user wants to listen. */
        if (activity.checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED) return true; /* Permit an explicit subsequent openStt call only with a real grant. */
        activity.requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO}, MICROPHONE_REQUEST); /* Ask through the actual Android runtime permission UI. */
        return false; /* Wait for the Activity's permission result; do not open the microphone yet. */
    } /* Finish explicit permission preparation. */
    /* Demonstrate actual UTF-8 submission after the host opens the real TTS helper. */
    public static int say(String text) { /* Return native readiness/error status to the UI. */
        return speak(text.getBytes(StandardCharsets.UTF_8)); /* Avoid JNI modified-UTF-8 text corruption. */
    } /* Finish host text submission. */
    /* Decode an actual transcript payload returned by the native polling example. */
    public static String transcript(byte[] event) { /* The host reads event[0] separately to distinguish partial/final. */
        if (event == null || event.length == 0) return null; /* Do not invent text when recognition has not produced a result. */
        return new String(event, 1, event.length - 1, StandardCharsets.UTF_8); /* Decode actual standard UTF-8 while excluding the final marker. */
    } /* Finish actual transcript decoding. */
} /* Finish Android preparation/integration helpers. */
