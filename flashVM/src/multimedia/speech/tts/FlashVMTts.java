package org.flashvm.multimedia; /* Keep this package/class name in the Android app's JNI configuration. */
import android.content.Context; /* Borrow the application's real Context. */
import android.os.Handler; /* Marshal system-service work to Android's main thread. */
import android.os.Looper; /* Select the actual application's main Looper. */
import android.speech.tts.TextToSpeech; /* Use the real installed Android synthesis service. */
import java.nio.charset.StandardCharsets; /* Convert standard UTF-8 without JNI modified-UTF-8 behavior. */
/* Real platform adapter; include this source in the Android host app, not the Linux C++ build. */
public final class FlashVMTts { /* Own one real TextToSpeech instance. */
    private final Handler main = new Handler(Looper.getMainLooper()); /* Serialize service lifecycle on the UI thread. */
    private volatile int state = -5; /* Report NOT_FOUND until asynchronous initialization completes. */
    private volatile boolean closed = false; /* Prevent queued requests from reviving a closed service. */
    private TextToSpeech engine; /* Own the native Android service connection on the main thread. */
    /* Construct explicit TTS state; microphone access is never needed by this class. */
    public FlashVMTts(Context supplied) { /* The JNI caller supplies an actual application Context. */
        final Context context = supplied.getApplicationContext(); /* Avoid retaining an Activity/UI lifetime. */
        main.post(new Runnable() { /* Create the real service on Android's main Looper. */
            @Override public void run() { /* Begin deferred native-service construction. */
                if (closed) return; /* Do not create resources after an early native close. */
                try { /* Convert native-service construction errors into the C-visible state. */
                    engine = new TextToSpeech(context, new TextToSpeech.OnInitListener() { /* Connect to the installed system synthesizer. */
                        @Override public void onInit(int result) { /* Receive actual asynchronous service readiness. */
                            if (!closed) state = result == TextToSpeech.SUCCESS ? 0 : -1; /* Preserve real engine initialization failure. */
                        } /* Finish readiness callback. */
                    }); /* Finish native service construction. */
                } catch (RuntimeException error) { state = -1; } /* Expose actual platform failure on subsequent native operations. */
            } /* Finish deferred service construction. */
        }); /* Finish posting the constructor operation. */
    } /* Finish explicit helper construction. */
    /* Query readiness or the last platform failure without scheduling speech. */
    public int status() { return closed ? -1 : state; } /* Match FVM_OK, FVM_NOT_FOUND and FVM_ERROR. */
    /* Accept validated standard UTF-8 and queue actual native synthesis on the main Looper. */
    public int speak(byte[] bytes) { /* This method reports scheduling, not audible completion. */
        if (status() != 0) return status(); /* Do not lose text while the real engine is initializing. */
        final String text = new String(bytes, StandardCharsets.UTF_8); /* Preserve supplementary Unicode and ordinary text. */
        main.post(new Runnable() { /* Serialize submission with stop/close. */
            @Override public void run() { /* Begin actual TextToSpeech submission. */
                if (closed || engine == null) return; /* Reject stale queued work after close. */
                try { /* Preserve system-service errors for the next status call. */
                    if (engine.speak(text, TextToSpeech.QUEUE_ADD, null, "flashvm") == TextToSpeech.ERROR) state = -1; /* Use real synthesis/output, not a shell command. */
                } catch (RuntimeException error) { state = -1; } /* Report native provider failure asynchronously. */
            } /* Finish actual text submission. */
        }); /* Finish scheduling speech. */
        return 0; /* The helper accepted work; no completion event is promised. */
    } /* Finish native text enqueueing. */
    /* Cancel this helper's current/queued native speech without shutting down the service. */
    public int stop() { /* Preserve the open helper for future text. */
        if (closed) return -1; /* Do not schedule operations after close. */
        main.post(new Runnable() { /* Keep native cancellation on the main thread. */
            @Override public void run() { /* Begin real service cancellation. */
                if (engine != null) { /* Stop only a successfully created service instance. */
                    try { if (engine.stop() == TextToSpeech.ERROR) state = -1; } /* Invoke actual Android cancellation. */
                    catch (RuntimeException error) { state = -1; } /* Expose platform failure on the next native call. */
                } /* Finish real engine cancellation. */
            } /* Finish deferred cancellation. */
        }); /* Finish scheduling native stop. */
        return 0; /* Report accepted cancellation scheduling. */
    } /* Finish stop. */
    /* Release the service; the posted Runnable retains this receiver until cleanup completes. */
    public void close() { /* Called once by native session cleanup, and safe if repeated by an app. */
        closed = true; /* Block new/queued speech immediately. */
        main.post(new Runnable() { /* Release actual service ownership on its required thread. */
            @Override public void run() { /* Begin real service shutdown. */
                if (engine != null) { /* Release only a created native synthesizer. */
                    try { engine.stop(); } /* Cancel actual playback even if a later shutdown operation fails. */
                    catch (RuntimeException error) { state = -1; } /* Preserve cancellation failure without skipping resource release. */
                    try { engine.shutdown(); } /* Release the actual engine even when stop failed. */
                    catch (RuntimeException error) { state = -1; } /* Void cleanup exposes native shutdown failure only in state/diagnostics. */
                    engine = null; /* Drop this helper's native-service reference. */
                } /* Finish engine cleanup. */
            } /* Finish deferred shutdown. */
        }); /* Finish scheduling native cleanup. */
    } /* Finish helper close. */
} /* Finish the real Android TTS adapter. */
