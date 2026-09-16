package io.truthraw.debug;

import java.util.Locale;

/** Deterministic JSON report body for device-side source/CFA validation.
 *
 * Runtime timestamp/device strings are observational metadata only. They never
 * participate in scientific identity or inheritance.
 */
public final class VerificationReport {
    public static final String SCHEMA = "TruthRawAndroidVerificationReport/0.3";
    public static final String CLASSIFICATION = "DEVICE_VALIDATION_ONLY_NO_SCIENTIFIC_WRITEBACK";
    public static final String APP_VERSION = "0.3-debug";

    private VerificationReport() {}

    public static boolean isExactCfa(DngCfaHasher.Result cfa) {
        return cfa != null
                && cfa.width == IdentityContract.DECODED_CFA_WIDTH
                && cfa.height == IdentityContract.DECODED_CFA_HEIGHT
                && cfa.stripCount == IdentityContract.DECODED_CFA_STRIPS
                && cfa.decodedBytes == IdentityContract.DECODED_CFA_BYTES
                && IdentityContract.DECODED_CFA_SHA256.equalsIgnoreCase(cfa.decodedCfaSha256);
    }

    public static String build(
            String timestampUtc,
            String device,
            String androidVersion,
            String selectedName,
            Sha256.DigestResult source,
            DngCfaHasher.Result cfa) {
        boolean sourceExact = IdentityContract.isExactFrozenSource(source.byteCount, source.sha256);
        boolean cfaExact = isExactCfa(cfa);
        boolean pass = sourceExact && cfaExact;

        StringBuilder s = new StringBuilder(2600);
        s.append("{\n");
        field(s, "schema", SCHEMA, true, 1);
        field(s, "classification", CLASSIFICATION, true, 1);
        field(s, "app_version", APP_VERSION, true, 1);
        field(s, "timestamp_utc_observational", timestampUtc, true, 1);
        field(s, "device_observational", device, true, 1);
        field(s, "android_observational", androidVersion, true, 1);
        field(s, "selected_name_observational", selectedName, true, 1);
        field(s, "result", pass ? "PASS_EXACT_SOURCE_AND_DECODED_CFA" : "FAIL_IDENTITY_NOT_COMPLETE", true, 1);
        boolField(s, "source_and_cfa_identity_match", pass, true, 1);

        indent(s, 1).append("\"source\": {\n");
        numberField(s, "observed_bytes", source.byteCount, true, 2);
        field(s, "observed_sha256", source.sha256, true, 2);
        numberField(s, "frozen_bytes", IdentityContract.SOURCE_BYTES, true, 2);
        field(s, "frozen_sha256", IdentityContract.SOURCE_SHA256, true, 2);
        boolField(s, "identity_match", sourceExact, false, 2);
        indent(s, 1).append("},\n");

        indent(s, 1).append("\"decoded_cfa\": {\n");
        if (cfa == null) {
            s.append("    \"decoded\": false,\n");
            s.append("    \"identity_match\": false\n");
        } else {
            boolField(s, "decoded", true, true, 2);
            numberField(s, "width", cfa.width, true, 2);
            numberField(s, "height", cfa.height, true, 2);
            numberField(s, "strips", cfa.stripCount, true, 2);
            numberField(s, "decoded_bytes", cfa.decodedBytes, true, 2);
            field(s, "sha256", cfa.decodedCfaSha256, true, 2);
            boolField(s, "identity_match", cfaExact, false, 2);
        }
        indent(s, 1).append("},\n");

        indent(s, 1).append("\"frozen_downstream_references\": {\n");
        field(s, "scientific_master_sha256", IdentityContract.SCIENTIFIC_MASTER_SHA256, true, 2);
        field(s, "dynamic_authority_sha256", IdentityContract.DYNAMIC_AUTHORITY_SHA256, true, 2);
        field(s, "source_bound_p3_transform_sha256", IdentityContract.P3_TRANSFORM_SHA256, true, 2);
        s.append(String.format(Locale.ROOT, "    \"reference_l0\": %.17g,\n", IdentityContract.REFERENCE_L0));
        boolField(s, "scientific_master_recomputed_on_device", false, true, 2);
        boolField(s, "dynamic_authority_recomputed_on_device", false, true, 2);
        boolField(s, "hdr_projection_run_on_device", false, false, 2);
        indent(s, 1).append("},\n");

        indent(s, 1).append("\"scientific_rules\": {\n");
        boolField(s, "creates_new_sensor_evidence", false, true, 2);
        boolField(s, "scientific_writeback_allowed", false, true, 2);
        boolField(s, "appearance_or_transport_may_upgrade_authority", false, true, 2);
        boolField(s, "representation_may_exceed_source_but_knowledge_claims_may_not", true, false, 2);
        indent(s, 1).append("}\n");
        s.append("}\n");
        return s.toString();
    }

    private static void field(StringBuilder s, String key, String value, boolean comma, int level) {
        indent(s, level).append('"').append(escape(key)).append("\": \"")
                .append(escape(value == null ? "" : value)).append('"');
        if (comma) s.append(',');
        s.append('\n');
    }

    private static void numberField(StringBuilder s, String key, long value, boolean comma, int level) {
        indent(s, level).append('"').append(escape(key)).append("\": ").append(value);
        if (comma) s.append(',');
        s.append('\n');
    }

    private static void boolField(StringBuilder s, String key, boolean value, boolean comma, int level) {
        indent(s, level).append('"').append(escape(key)).append("\": ").append(value);
        if (comma) s.append(',');
        s.append('\n');
    }

    private static StringBuilder indent(StringBuilder s, int level) {
        for (int i = 0; i < level; i++) s.append("  ");
        return s;
    }

    private static String escape(String value) {
        StringBuilder out = new StringBuilder(value.length() + 16);
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            switch (c) {
                case '\\': out.append("\\\\"); break;
                case '"': out.append("\\\""); break;
                case '\n': out.append("\\n"); break;
                case '\r': out.append("\\r"); break;
                case '\t': out.append("\\t"); break;
                default:
                    if (c < 0x20) out.append(String.format(Locale.ROOT, "\\u%04x", (int)c));
                    else out.append(c);
            }
        }
        return out.toString();
    }
}
