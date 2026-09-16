package io.truthraw.debug;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.widget.Button;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class MainActivity extends Activity {
    private static final int REQUEST_OPEN_DNG = 1001;
    private static final int REQUEST_CREATE_REPORT = 1002;

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private Button openButton;
    private Button exportButton;
    private TextView statusView;
    private TextView detailsView;
    private String lastReport;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        openButton = findViewById(R.id.open_dng);
        exportButton = findViewById(R.id.export_report);
        statusView = findViewById(R.id.status);
        detailsView = findViewById(R.id.details);
        detailsView.setText(referenceText());
        exportButton.setEnabled(false);
        openButton.setOnClickListener(v -> openDocument());
        exportButton.setOnClickListener(v -> createReportDocument());
    }

    private void openDocument() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, REQUEST_OPEN_DNG);
    }

    private void createReportDocument() {
        if (lastReport == null) return;
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/json");
        intent.putExtra(Intent.EXTRA_TITLE, "truthraw-device-verification-v0.3.json");
        startActivityForResult(intent, REQUEST_CREATE_REPORT);
    }

    @Override
    @SuppressWarnings("deprecation")
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null || data.getData() == null) return;
        if (requestCode == REQUEST_OPEN_DNG) {
            Uri uri = data.getData();
            openButton.setEnabled(false);
            exportButton.setEnabled(false);
            lastReport = null;
            statusView.setText("Hashing selected file…");
            executor.execute(() -> inspect(uri));
        } else if (requestCode == REQUEST_CREATE_REPORT) {
            writeReport(data.getData());
        }
    }

    private void inspect(Uri uri) {
        final String displayName = displayName(uri);
        try (InputStream in = getContentResolver().openInputStream(uri)) {
            if (in == null) throw new IllegalStateException("Content resolver returned no stream");
            Sha256.DigestResult digest = Sha256.digest(in);
            boolean exactSource = IdentityContract.isExactFrozenSource(digest.byteCount, digest.sha256);
            if (!exactSource) {
                String report = buildReport(displayName, digest, null);
                publish(IdentityContract.admissionLabel(digest.byteCount, digest.sha256), resultText(displayName, digest, null), report);
                return;
            }
            runOnUiThread(() -> statusView.setText("Source exact. Verifying decoded CFA…"));
            File cached = copyToCache(uri);
            try {
                DngCfaHasher.Result cfa = DngCfaHasher.hash(cached);
                boolean cfaExact = VerificationReport.isExactCfa(cfa);
                String status = cfaExact
                        ? "PASS — exact source + decoded CFA verified on-device"
                        : "FAIL — source bytes exact but decoded CFA identity mismatch";
                publish(status, resultText(displayName, digest, cfa), buildReport(displayName, digest, cfa));
            } finally {
                if (!cached.delete()) cached.deleteOnExit();
            }
        } catch (Exception e) {
            publish("ERROR — admission/CFA verification not completed",
                    "Selected: " + displayName + "\n\n" + e.getClass().getSimpleName() + ": " + e.getMessage() + "\n\n" + referenceText(),
                    null);
        }
    }

    private String buildReport(String displayName, Sha256.DigestResult digest, DngCfaHasher.Result cfa) {
        String device = Build.MANUFACTURER + " " + Build.MODEL;
        String android = Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")";
        return VerificationReport.build(Instant.now().toString(), device, android, displayName, digest, cfa);
    }

    private File copyToCache(Uri uri) throws Exception {
        File out = File.createTempFile("truthraw-source-", ".dng", getCacheDir());
        try (InputStream in = getContentResolver().openInputStream(uri); FileOutputStream fos = new FileOutputStream(out)) {
            if (in == null) throw new IllegalStateException("Content resolver returned no second stream");
            byte[] buffer = new byte[1024 * 1024];
            int n;
            while ((n = in.read(buffer)) != -1) fos.write(buffer, 0, n);
        }
        return out;
    }

    private void writeReport(Uri uri) {
        String report = lastReport;
        if (report == null) return;
        executor.execute(() -> {
            try (OutputStream out = getContentResolver().openOutputStream(uri)) {
                if (out == null) throw new IllegalStateException("Content resolver returned no output stream");
                out.write(report.getBytes(StandardCharsets.UTF_8));
                out.flush();
                runOnUiThread(() -> statusView.setText("Verification report saved — scientific result unchanged"));
            } catch (Exception e) {
                runOnUiThread(() -> statusView.setText("ERROR — report not saved: " + e.getMessage()));
            }
        });
    }

    private void publish(String status, String details, String report) {
        runOnUiThread(() -> {
            statusView.setText(status);
            detailsView.setText(details);
            lastReport = report;
            exportButton.setEnabled(report != null);
            openButton.setEnabled(true);
        });
    }

    private String displayName(Uri uri) {
        try (Cursor cursor = getContentResolver().query(uri, new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (index >= 0) {
                    String value = cursor.getString(index);
                    if (value != null && !value.isEmpty()) return value;
                }
            }
        } catch (Exception ignored) {
            // Informational only; never participates in scientific admission.
        }
        return uri.toString();
    }

    private static String resultText(String name, Sha256.DigestResult digest, DngCfaHasher.Result cfa) {
        StringBuilder s = new StringBuilder();
        s.append("Selected: ").append(name).append('\n');
        s.append("Observed bytes: ").append(digest.byteCount).append('\n');
        s.append("Observed file SHA-256: ").append(digest.sha256).append("\n\n");
        if (cfa == null) {
            s.append("The file is not the exact frozen source. No downstream scientific identity is inherited.\n\n");
        } else {
            s.append("Decoded CFA: ").append(cfa.width).append('x').append(cfa.height).append('\n');
            s.append("CFA strips: ").append(cfa.stripCount).append('\n');
            s.append("Decoded CFA bytes: ").append(cfa.decodedBytes).append('\n');
            s.append("Decoded CFA SHA-256: ").append(cfa.decodedCfaSha256).append('\n');
            s.append("CFA identity match: ").append(VerificationReport.isExactCfa(cfa) ? "PASS" : "FAIL").append("\n\n");
            s.append("Source and decoded CFA are independently verified by this APK. Scientific Master, Dynamic Authority and HDR projection remain frozen references until their exact runtimes are ported.\n\n");
        }
        s.append(referenceText());
        return s.toString();
    }

    private static String referenceText() {
        return String.format(Locale.ROOT,
                "NON-CANONICAL DEBUG CLIENT v0.3\n" +
                "Source byte/SHA admission: IMPLEMENTED\n" +
                "Uncompressed 16-bit DNG CFA verification: IMPLEMENTED\n" +
                "Device validation report export: IMPLEMENTED\n" +
                "Scientific Master recomputation: NOT RUN\n" +
                "Dynamic Authority recomputation: NOT RUN\n" +
                "HDR projection: NOT RUN\n" +
                "Scientific writeback: FORBIDDEN\n\n" +
                "Frozen source bytes: %d\n" +
                "Frozen source SHA-256: %s\n" +
                "Decoded CFA SHA-256: %s\n" +
                "Scientific Master SHA-256 (reference): %s\n" +
                "Dynamic Authority SHA-256 (reference): %s\n" +
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
