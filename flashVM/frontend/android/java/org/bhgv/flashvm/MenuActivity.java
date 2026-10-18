/* Android test launcher: copy bundled SWFs to private storage, then select a real native mode. */
package org.bhgv.flashvm;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

/** Own the ordinary Android menu; no VM runs until the user selects a test. */
public final class MenuActivity extends Activity {
    /** Build a readable test menu without storage/microphone permissions. */
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(24, 24, 24, 24);
        TextView help = new TextView(this);
        help.setText("FlashVM 0.582 · ARM64\nВыберите тест и режим. Касание двигает розовый указатель. " +
                     "Для стрелок нужна физическая клавиатура. Назад — выйти из теста. " +
                     "Звук: PCM8 mono / PCM16 stereo / ADPCM. JIT может использовать fallback.");
        layout.addView(help);
        final CheckBox mute = new CheckBox(this);
        mute.setText("Без звука / Mute");
        layout.addView(mute);
        for (final String movie : new String[]{"01-animation-audio.swf", "02-input-audio.swf"}) {
            for (final boolean jit : new boolean[]{false, true}) {
                Button button = new Button(this);
                button.setText(movie + " · " + (jit ? "JIT" : "Interpreter"));
                button.setOnClickListener(view -> launch(movie, jit, mute.isChecked()));
                layout.addView(button);
            }
        }
        setContentView(layout);
    }

    /** Preserve Unicode-safe private paths and report copy failure instead of starting a broken VM. */
    private void launch(String movie, boolean jit, boolean mute) {
        File destination = new File(getFilesDir(), movie);
        try (InputStream input = getAssets().open(movie);
             FileOutputStream output = new FileOutputStream(destination)) {
            byte[] buffer = new byte[8192];
            int count;
            while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
        } catch (java.io.IOException error) {
            Toast.makeText(this, error.toString(), Toast.LENGTH_LONG).show();
            return;
        }
        Intent intent = new Intent(this, PlayerActivity.class);
        intent.putExtra("movie", destination.getAbsolutePath());
        intent.putExtra("jit", jit);
        intent.putExtra("mute", mute);
        startActivity(intent);
    }
}
