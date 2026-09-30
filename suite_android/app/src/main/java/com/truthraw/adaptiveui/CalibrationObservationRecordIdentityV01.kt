package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest

/**
 * Canonical identity for optional calibration/observation records.
 *
 * Object key order never changes identity. Array order remains significant.
 * source_sha256_roots are normalized to lowercase hexadecimal before identity
 * is computed. The identity field itself is excluded from its own hash.
 */
object CalibrationObservationRecordIdentityV01 {
    const val SCHEMA = "D.RAW/CalibrationObservationRecordIdentity/0.1"
    private const val ID_FIELD = "record_identity_sha256"

    fun normalize(record: JSONObject): JSONObject {
        val out = JSONObject(record.toString())
        val roots = out.optJSONArray("source_sha256_roots")
        if (roots != null) {
            val normalized = JSONArray()
            for (i in 0 until roots.length()) {
                normalized.put(roots.optString(i).trim().lowercase())
            }
            out.put("source_sha256_roots", normalized)
        }
        out.remove(ID_FIELD)
        out.put(ID_FIELD, sha256(canonicalJson(out, ignoreIdentity = true)))
        return out
    }

    fun identity(record: JSONObject): String =
        sha256(canonicalJson(record, ignoreIdentity = true))

    fun canonicalJson(
        value: Any?,
        ignoreIdentity: Boolean = false,
    ): String =
        when (value) {
            null,
            JSONObject.NULL -> "null"
            is JSONObject -> {
                val keys = value.keys().asSequence().toList().sorted()
                buildString {
                    append('{')
                    var first = true
                    for (key in keys) {
                        if (ignoreIdentity && key == ID_FIELD) continue
                        if (!first) append(',')
                        first = false
                        append(JSONObject.quote(key))
                        append(':')
                        append(canonicalJson(value.opt(key), ignoreIdentity))
                    }
                    append('}')
                }
            }
            is JSONArray -> buildString {
                append('[')
                for (i in 0 until value.length()) {
                    if (i > 0) append(',')
                    append(canonicalJson(value.opt(i), ignoreIdentity))
                }
                append(']')
            }
            is String -> JSONObject.quote(value)
            is Number,
            is Boolean -> JSONObject.valueToString(value)
            else -> JSONObject.quote(value.toString())
        }

    private fun sha256(value: String): String {
        val digest =
            MessageDigest.getInstance("SHA-256")
                .digest(value.toByteArray(Charsets.UTF_8))
        return digest.joinToString("") {
            "%02x".format(it.toInt() and 0xff)
        }
    }
}
