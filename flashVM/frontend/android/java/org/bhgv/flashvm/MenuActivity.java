/* Android test launcher: copy bundled SWFs to private storage, then select a real native mode. */
package org.bhgv.flashvm;

import android.app.Activity;
import android.content.Intent;
import android.content.ClipData;
import android.net.Uri;
import android.widget.ScrollView;
import android.os.Build;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
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
        help.setText("FlashVM 0.587 · ARM64\nВыберите тест и режим. Касание двигает розовый указатель. " +
                     "Для стрелок нужна физическая клавиатура. Назад — выйти из теста. " +
                     "Звук: PCM8 mono / PCM16 stereo / ADPCM. JIT может использовать fallback.");
        layout.addView(help);
        final CheckBox mute = new CheckBox(this);
        mute.setText("Без звука / Mute");
        layout.addView(mute);
        final CheckBox diagnostics = new CheckBox(this);
        diagnostics.setText("Диагностика звука: 15 секунд, сохранить WAV и отчёт");
        layout.addView(diagnostics);
        for (final String movie : new String[]{"01-animation-audio.swf", "02-input-audio.swf", "03-stop-sounds.swf"}) {
            for (final boolean jit : new boolean[]{false, true}) {
                Button button = new Button(this);
                button.setText(movie + " · " + (jit ? "JIT" : "Interpreter"));
                button.setOnClickListener(view -> launch(movie, jit, mute.isChecked(), diagnostics.isChecked()));
                layout.addView(button);
            }
        }
        Button share = new Button(this);
        share.setText("Отправить диагностику звука (ZIP)");
        share.setOnClickListener(view -> shareDiagnostics(share));
        layout.addView(share);
        ScrollView scroll = new ScrollView(this);
        scroll.addView(layout);
        setContentView(scroll);
    }

    /** Preserve Unicode-safe private paths and report copy failure instead of starting a broken VM. */
    private void launch(String movie, boolean jit, boolean mute, boolean diagnostics) {
        if (diagnostics && mute) {
            Toast.makeText(this, "Для диагностики отключите Mute", Toast.LENGTH_LONG).show();
            return;
        }
        if (diagnostics) {
            File folder = new File(getFilesDir(), "audio-diagnostics");
            if (!folder.isDirectory() && !folder.mkdirs()) {
                Toast.makeText(this, "Не удалось создать папку диагностики", Toast.LENGTH_LONG).show();
                return;
            }
            String base = movie.replace(".swf", "") + (jit ? "-jit" : "-interpreter");
            // Replace only this selected run, retaining other movies and execution modes.
            for (String suffix : new String[]{".wav", ".txt", ".wav.tmp", ".txt.tmp"}) {
                File previous = new File(folder, base + suffix);
                if (previous.exists() && !previous.delete()) {
                    Toast.makeText(this, "Не удалось заменить прошлую запись", Toast.LENGTH_LONG).show();
                    return;
                }
            }
            try (FileOutputStream info = new FileOutputStream(new File(folder, "device.txt"))) {
                android.media.AudioManager manager = (android.media.AudioManager)getSystemService(AUDIO_SERVICE);
                String audioProperties = manager == null ? "unavailable" :
                    "rate=" + manager.getProperty(android.media.AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE) +
                    " frames=" + manager.getProperty(android.media.AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
                String text = "FlashVM 0.587\nAndroid=" + Build.VERSION.RELEASE + " API=" +
                    Build.VERSION.SDK_INT + "\nmanufacturer=" + Build.MANUFACTURER +
                    " model=" + Build.MODEL + "\nABI=" + Arrays.toString(Build.SUPPORTED_ABIS) +
                    "\nAudioManager " + audioProperties + "\nNo microphone capture. WAV contains SDL-bound native PCM.\n";
                info.write(text.getBytes("UTF-8"));
            } catch (IOException error) {
                Toast.makeText(this, error.toString(), Toast.LENGTH_LONG).show();
                return;
            }
        }
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
        intent.putExtra("diagnostics", diagnostics);
        startActivity(intent);
    }
    /** Compress completed private evidence off the UI thread; share only on an explicit button tap. */
    private void shareDiagnostics(final Button button) {
        button.setEnabled(false);
        new Thread(() -> {
            try {
                File folder = new File(getFilesDir(), "audio-diagnostics");
                File[] files = folder.listFiles((dir, name) -> name.endsWith(".wav") || name.endsWith(".txt"));
                if (files == null || files.length == 0)
                    throw new IOException("Сначала запустите тест с диагностикой и дождитесь возврата в меню");
                boolean hasWave = false;
                for (File file : files) if (file.getName().endsWith(".wav")) hasWave = true;
                if (!hasWave) throw new IOException("Нет завершённого WAV: дождитесь окончания 15-секундного теста");
                Arrays.sort(files);
                File archive = new File(getCacheDir(), "audio-diagnostics.zip");
                File temporary = new File(getCacheDir(), "audio-diagnostics.zip.tmp");
                try (ZipOutputStream zip = new ZipOutputStream(new FileOutputStream(temporary))) {
                    byte[] buffer = new byte[8192];
                    for (File file : files) {
                        zip.putNextEntry(new ZipEntry(file.getName()));
                        try (FileInputStream input = new FileInputStream(file)) {
                            int count;
                            while ((count = input.read(buffer)) != -1) zip.write(buffer, 0, count);
                        }
                        zip.closeEntry();
                    }
                }
                // Android rename publishes a completed archive; existing readers retain the old inode.
                if (!temporary.renameTo(archive)) throw new IOException("Не удалось завершить ZIP");
                runOnUiThread(() -> {
                    button.setEnabled(true);
                    Uri uri = Uri.parse("content://org.bhgv.flashvm.diagnostics/audio-diagnostics.zip");
                    Intent share = new Intent(Intent.ACTION_SEND);
                    share.setType("application/zip");
                    share.putExtra(Intent.EXTRA_STREAM, uri);
                    share.setClipData(ClipData.newRawUri("FlashVM audio diagnostics", uri));
                    share.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    try { startActivity(Intent.createChooser(share, "Отправить диагностику FlashVM")); }
                    catch (android.content.ActivityNotFoundException error) {
                        Toast.makeText(this, error.toString(), Toast.LENGTH_LONG).show();
                    }
                });
            } catch (IOException error) {
                runOnUiThread(() -> {
                    button.setEnabled(true);
                    Toast.makeText(this, error.toString(), Toast.LENGTH_LONG).show();
                });
            }
        }, "FlashVM diagnostic ZIP").start();
    }
}
