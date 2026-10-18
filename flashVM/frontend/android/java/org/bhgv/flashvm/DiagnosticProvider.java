/* Share only the user-requested diagnostic ZIP through a read-only, permission-granted URI. */
package org.bhgv.flashvm;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import java.io.File;
import java.io.FileNotFoundException;

/** No external storage permission, AndroidX dependency or exposure of arbitrary private paths. */
public final class DiagnosticProvider extends ContentProvider {
    /** The provider owns no resources until an explicit read request arrives. */
    @Override public boolean onCreate() { return true; }

    /** Restrict every operation to the one generated archive, regardless of URI escaping. */
    private File archive(Uri uri) {
        if (!"org.bhgv.flashvm.diagnostics".equals(uri.getAuthority()) ||
            !"/audio-diagnostics.zip".equals(uri.getPath()) || uri.getQuery() != null)
            throw new IllegalArgumentException("Unknown diagnostic URI");
        return new File(getContext().getCacheDir(), "audio-diagnostics.zip");
    }

    /** Expose the explicit ZIP as a read-only file descriptor to a granted recipient. */
    @Override public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        if (!"r".equals(mode)) throw new FileNotFoundException("Read only");
        return ParcelFileDescriptor.open(archive(uri), ParcelFileDescriptor.MODE_READ_ONLY);
    }

    /** Standard attachment MIME type for Android's share targets. */
    @Override public String getType(Uri uri) { archive(uri); return "application/zip"; }

    /** Supply only the requested attachment name and length, never its private filesystem path. */
    @Override public Cursor query(Uri uri, String[] projection, String selection,
                                  String[] arguments, String sortOrder) {
        File file = archive(uri);
        String[] columns = projection == null ?
            new String[]{OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE} : projection;
        MatrixCursor result = new MatrixCursor(columns, 1);
        MatrixCursor.RowBuilder row = result.newRow();
        for (String column : columns) {
            if (OpenableColumns.DISPLAY_NAME.equals(column)) row.add(file.getName());
            else if (OpenableColumns.SIZE.equals(column)) row.add(file.length());
            else row.add(null);
        }
        return result;
    }

    /** Reject writes: callers can read only the completed archive selected by the user. */
    @Override public Uri insert(Uri uri, ContentValues values) { throw new UnsupportedOperationException(); }
    /** Reject deletion of private diagnostic evidence by recipients. */
    @Override public int delete(Uri uri, String selection, String[] arguments) { throw new UnsupportedOperationException(); }
    /** Reject mutation of private diagnostic evidence by recipients. */
    @Override public int update(Uri uri, ContentValues values, String selection, String[] arguments) {
        throw new UnsupportedOperationException();
    }
}
