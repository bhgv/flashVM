/* Android Surface/audio/lifecycle host supplied by the separately installed SDL2 SDK. */
package org.bhgv.flashvm;

import org.libsdl.app.SDLActivity;

/** Keep JNI/native ownership in SDLActivity; this subclass selects only our library and argv. */
public final class PlayerActivity extends SDLActivity {
    /** Load the actual ARM64 SDL library before the dependent VM/player library. */
    @Override protected String[] getLibraries() {
        return new String[]{"SDL2", "flashvm_android"};
    }

    /** SDL prepends the program name; never add a second argv[0] here. */
    @Override protected String[] getArguments() {
        String mode = getIntent().getBooleanExtra("jit", false) ? "--jit" : "--no-jit";
        String path = getIntent().getStringExtra("movie");
        boolean diagnostics = getIntent().getBooleanExtra("diagnostics", false);
        String base = new java.io.File(getFilesDir(), "audio-diagnostics/" +
            new java.io.File(path).getName().replace(".swf", "") +
            (getIntent().getBooleanExtra("jit", false) ? "-jit" : "-interpreter")).getAbsolutePath();
        // SDL has loaded its JNI library before invoking getArguments; no VM has started yet.
        nativeSetenv("FVM_AUDIO_TRACE", diagnostics ? base : "");
        if (diagnostics) return new String[]{mode, "--stats", "--frames", "360", path};
        return getIntent().getBooleanExtra("mute", false) ?
            new String[]{mode, "--stats", "--mute", path} : new String[]{mode, "--stats", path};
    }
}
