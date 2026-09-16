package io.truthraw.debug;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.InputStream;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class MainActivity extends Activity {
    private static final int REQUEST_OPEN_DNG = 1001;

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private Button openButton;
    private TextView statusView;
    private TextView detailsView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        openButton = findViewById(R.id.open_dng);
        statusView = findViewById(R.id.status);
        detailsView = findViewById(R.id.details);

        detailsView.setText(referenceText());
        openButton.setOnClickListener(v -> openDocument());
    }

    private void openDocument() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, REQUEST_OPEN_DNG);
    }

    @Override
    @SuppressWarnings("deprecation")
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_OPEN_DNG || resultCode != RESULT_OK || data == null || data.getData() == null) {
            return;
        }
        Uri uri = data.getData();
        openButton.setEnabled(false);
        statusView.setText("Hashing selected file…");
        executor.execute(() -> inspect(uri));
    }

    private void inspect(Uri uri) {
        final String displayName = displayName(uri);
        try (InputStream in = getContentResolver().openInputStream(uri)) {
            if (in == null) {
                throw new IllegalStateException("Content resolver returned no stream");
            }
            Sha256.DigestResult digest = Sha256.digest(in);
            final boolean exact = IdentityContract.isExactFrozenSource(digest.byteCount, digest.sha256);
            final String status = IdentityContract.admissionLabel(digest.byteCount, digest.sha256);
            final String detail = resultText(displayName, digest, exact);
            runOnUiThread(() -> {
                statusView.setText(status);
                detailsView.setText(detail);
                openButton.setEnabled(true);
            });
        } catch (Exception e) {
            runOnUiThread(() -> {
                statusView.setText("ERROR — source admission not completed");
                detailsView.setText("Selected: " + displayName + "\n\n" + e.getClass().getSimpleName() + ": " + e.getMessage() + "\n\n" + referenceText());
                openButton.setEnabled(true);
            });
        }
    }

    private String displayName(Uri uri) {
        try (Cursor cursor = getContentResolver().query(uri, new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (index >= 0) {
                    String value = cursor.getString(index);
                    if (value != null && !value.isEmpty()) {
                        return value;
                    }
                }
            }
        } catch (Exception ignored) {
            // Display name is informational only and never participates in admission.
        }
        return uri.toString();
    }

    private static String resultText(String name, Sha256.DigestResult digest, boolean exact) {
        StringBuilder s = new StringBuilder();
        s.append("Selected: ").append(name).append('\n');
        s.append("Observed bytes: ").append(digest.byteCount).append('\n');
        s.append("Observed SHA-256: ").append(digest.sha256).append("\n\n");
        if (exact) {
            s.append("Exact source admission is verified on-device. The identities below are frozen research references bound to this exact source; this debug client has NOT independently recomputed them.\n\n");
        } else {
            s.append("The file is not the frozen source. No decoded-CFA, Scientific-Master, Dynamic-Authority or HDR identity may be inherited from the reference capture.\n\n");
        }
        s.append(referenceText());
        return s.toString();
    }

    private static String referenceText() {
        return String.format(Locale.ROOT,
                "NON-CANONICAL DEBUG CLIENT\n" +
                "Scientific processing: NOT RUN\n" +
                "HDR projection: NOT RUN\n" +
                "Scientific writeback: FORBIDDEN\n\n" +
                "Frozen source bytes: %d\n" +
                "Frozen source SHA-256: %s\n" +
                "Decoded CFA SHA-256 (reference only): %s\n" +
                "Scientific Master SHA-256 (reference only): %s\n" +
                "Dynamic Authority SHA-256 (reference only): %s\n" +
                "Reference L0: %.17g\n" +
                "P3-D65 transform SHA-256 (L1 source-bound, not physical calibration): %s\n\n" +
                "Rule: Representation may exceed the source; knowledge claims may not exceed the evidence.",
                IdentityContract.SOURCE_BYTES,
                IdentityContract.SOURCE_SHA256,
                IdentityContract.DECODED_CFA_SHA256,
                IdentityContract.SCIENTIFIC_MASTER_SHA256,
                IdentityContract.DYNAMIC_AUTHORITY_SHA256,
                IdentityContract.REFERENCE_L0,
                IdentityContract.P3_TRANSFORM_SHA256);
    }

    @Override
    protected void onDestroy() {
        executor.shutdownNow();
        super.onDestroy();
    }
}
