package org.flashvm.multimedia; /* Keep this name when supplying the JNI helper-class reference. */
import android.Manifest; /* Name the real platform microphone permission. */
import android.content.Context; /* Borrow the application's actual Context. */
import android.content.Intent; /* Configure the installed recognition service explicitly. */
import android.content.pm.PackageManager; /* Check already-granted microphone permission. */
import android.os.Bundle; /* Read actual recognition result payloads. */
import android.os.Handler; /* Serialize native service work on the main thread. */
import android.os.Looper; /* Use the application's real main Looper. */
import android.speech.RecognitionListener; /* Receive actual recognition events. */
import android.speech.RecognizerIntent; /* Select free-form recognition and partial-result behavior. */
import android.speech.SpeechRecognizer; /* Use the installed real Android speech recognizer. */
import java.nio.charset.StandardCharsets; /* Encode standard UTF-8 for the C ABI. */
import java.util.ArrayList; /* Read the Java platform's standard result list, not a C++ STL container. */
/* Real platform adapter; the host obtains permission before explicitly creating this helper. */
public final class FlashVMStt implements RecognitionListener { /* Own one bounded utterance-recognition lifecycle. */
    private final Handler main = new Handler(Looper.getMainLooper()); /* Marshal required Android APIs to the UI thread. */
    private volatile int state = -5; /* Report NOT_FOUND while recognition service setup is pending. */
    private volatile boolean closed = false; /* Prevent callbacks/queued operations after close. */
    private SpeechRecognizer engine; /* Own the native recognition-service connection on the main thread. */
    private byte[] pending; /* Keep only the latest unconsumed partial/final transcript atomically. */
    /* Explicit construction can activate the microphone; it never requests permissions silently. */
    public FlashVMStt(Context supplied) { /* JNI supplies a real app Context and calls only after host authorization. */
        final Context context = supplied.getApplicationContext(); /* Avoid retaining a short-lived Activity. */
        if (context.checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) throw new SecurityException("RECORD_AUDIO must be granted by the host"); /* Require Android API 23+ runtime permission first. */
        if (!SpeechRecognizer.isRecognitionAvailable(context)) throw new UnsupportedOperationException("No system recognition service"); /* Reject unavailable real providers. */
        main.post(new Runnable() { /* Open capture on the main Looper as required by the platform. */
            @Override public void run() { /* Begin explicit native recognition setup. */
                if (closed) return; /* Do not create capture after an early close. */
                try { /* Convert native service failures into public status. */
                    engine = SpeechRecognizer.createSpeechRecognizer(context); /* Connect to the actual configured recognizer. */
                    engine.setRecognitionListener(FlashVMStt.this); /* Register this helper for real platform callbacks. */
                    Intent request = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH); /* Request real free-form speech recognition. */
                    request.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM); /* Use the installed engine's default language. */
                    request.putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, true); /* Permit useful partial transcripts. */
                    request.putExtra(RecognizerIntent.EXTRA_PREFER_OFFLINE, true); /* Ask the provider for offline processing; this is advisory, not a guarantee. */
                    engine.startListening(request); /* Explicitly activate actual microphone recognition. */
                    state = 0; /* Report that native service setup succeeded. */
                } catch (RuntimeException error) { state = -1; } /* Expose real service/device/permission failure. */
            } /* Finish explicit native recognition startup. */
        }); /* Finish scheduling main-thread service creation. */
    } /* Finish explicit recognizer construction. */
    /* Return readiness or a real platform recognition error. */
    public int status() { return closed ? -1 : state; } /* Preserve the shared FVM result-code meanings. */
    /* Publish one paired final marker and standard UTF-8 transcript without a native data race. */
    private synchronized void publish(Bundle results, boolean complete) { /* Only the latest unconsumed hypothesis is retained. */
        if (closed) return; /* Ignore callbacks after native session cleanup. */
        ArrayList<String> values = results.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION); /* Read actual provider hypotheses. */
        if (values == null || values.isEmpty()) return; /* Empty recognition callbacks do not invent text. */
        byte[] text = values.get(0).getBytes(StandardCharsets.UTF_8); /* Select the actual highest-ranked hypothesis. */
        pending = new byte[text.length + 1]; /* Reserve a marker plus the complete UTF-8 payload. */
        pending[0] = (byte)(complete ? 1 : 0); /* Atomically pair final/partial state with this transcript. */
        System.arraycopy(text, 0, pending, 1, text.length); /* Copy real recognized text into the event payload. */
    } /* Finish transcript publication. */
    /* Transfer one complete Java event to native retention; null means no transcript yet. */
    public synchronized byte[] take() { /* Native code retains the result across size queries. */
        byte[] result = pending; /* Snapshot one atomic marker/text event. */
        pending = null; /* Consume exactly this Java queue entry. */
        return result; /* JNI copies it without modified-UTF-8 conversions. */
    } /* Finish atomic result retrieval. */
    /* Stop capture and release service ownership; no implicit restart is scheduled. */
    public void close() { /* Native close is the sole required owner cleanup operation. */
        closed = true; /* Block future callbacks and deferred startup immediately. */
        main.post(new Runnable() { /* Release the actual recognizer on the required main thread. */
            @Override public void run() { /* Begin actual microphone/service shutdown. */
                if (engine != null) { /* Release only a created recognition service. */
                    try { engine.cancel(); } /* Stop actual microphone recognition before releasing service ownership. */
                    catch (RuntimeException error) { state = -1; } /* Preserve cancellation failure without skipping destroy. */
                    try { engine.destroy(); } /* Release the actual service even when cancellation failed. */
                    catch (RuntimeException error) { state = -1; } /* Void cleanup cannot return a synchronous C error. */
                    engine = null; /* Drop the owned system-service reference. */
                } /* Finish real recognizer cleanup. */
            } /* Finish deferred shutdown. */
        }); /* Finish scheduling actual capture cleanup. */
    } /* Finish explicit recognizer close. */
    /* Native readiness changes do not synthesize transcripts. */
    @Override public void onReadyForSpeech(Bundle parameters) { if (!closed) state = 0; } /* Report actual service readiness. */
    /* Speech start needs no additional host text event. */
    @Override public void onBeginningOfSpeech() { } /* Keep real recognition lifecycle under Android control. */
    /* RMS updates are not transcripts and are deliberately not retained. */
    @Override public void onRmsChanged(float value) { } /* Avoid unnecessary per-audio-frame storage. */
    /* Raw audio callbacks are not copied into the text bridge. */
    @Override public void onBufferReceived(byte[] audio) { } /* Preserve the recognizer's native buffer ownership. */
    /* End-of-speech alone does not imply a final transcript has arrived. */
    @Override public void onEndOfSpeech() { } /* Wait for actual result/error callbacks. */
    /* Expose real recognition failure, including timeout/no-match; never fabricate text. */
    @Override public void onError(int code) { if (!closed) state = -1; } /* Re-open explicitly if the host wants another utterance. */
    /* Publish the actual final transcript without restarting the microphone. */
    @Override public void onResults(Bundle results) { publish(results, true); } /* Preserve finality in the atomic payload. */
    /* Publish the newest actual partial transcript while recognition continues. */
    @Override public void onPartialResults(Bundle results) { publish(results, false); } /* Older unconsumed partial hypotheses may be replaced. */
    /* Provider-specific extension events are outside the initial text bridge contract. */
    @Override public void onEvent(int code, Bundle parameters) { } /* Do not reinterpret unknown native extensions. */
} /* Finish the real Android STT adapter. */
