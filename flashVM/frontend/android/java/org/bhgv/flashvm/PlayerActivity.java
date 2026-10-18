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
        return getIntent().getBooleanExtra("mute", false) ?
            new String[]{mode, "--stats", "--mute", path} : new String[]{mode, "--stats", path};
    }
}
