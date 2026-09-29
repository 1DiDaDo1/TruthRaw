package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import org.json.JSONArray
import org.json.JSONObject
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.channels.FileChannel
import kotlin.math.PI
import kotlin.math.atan2
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sqrt

/**
 * Read-only backside signal-support audit for one selected DNG CFA observation.
 *
 * This is deliberately narrower than the scientific decoder. It samples the
 * actual source CFA payload only when the topology is simple enough to prove:
 * classic TIFF/DNG, uncompressed unsigned 16-bit, one sample per pixel, strips.
 *
 * It never edits source bytes and it never enables Dark Chroma correction.
 * Its only current authority is to provide a measured blocking signal when the
 * selected source payload is near-black dominated. Unsupported layouts remain
 * UNKNOWN instead of being guessed.
 */
object BacksideSignalSupportAudit {
    private const val SAMPLE_GRID_STEP = 64
    private const val MAX_IFD_ENTRIES = 4096
    private const val MAX_VALUE_BYTES = 2 * 1024 * 1024
    private const val NEAR_BLACK_NORMALIZED = 0.010
    private const val LOW_SIGNAL_NORMALIZED = 0.020
    private const val NEAR_BLACK_FRACTION_MIN = 0.70
    private const val LOW_SIGNAL_P90_MAX = 0.020
    private const val OPTICAL_FIELD_RADIAL_BINS = 12
    private const val OPTICAL_FIELD_AZIMUTH_BINS = 12

    private data class FieldSample(
        val x: Int,
        val y: Int,
        val phase: Int,
        val value: Double,
    )

    private data class Entry(
        val tag: Int,
        val type: Int,
        val count: Long,
        val valueOffset: Long,
        val inlineBytes: ByteArray,
    )

    fun analyze(
        resolver: ContentResolver,
        uri: Uri,
        parsedContainer: JSONObject,
        primaryRaw: JSONObject?,
        sourceSha256: String,
    ): JSONObject {
        if (primaryRaw == null) {
            return unavailable(sourceSha256, "NO_PRIMARY_RAW_CFA_CANDIDATE")
        }

        val rawPath = primaryRaw.optString("path", "")
        val ifdOffset = findIfdOffset(parsedContainer, rawPath)
            ?: return unavailable(sourceSha256, "RAW_IFD_OFFSET_NOT_FOUND")

        val pfd = resolver.openFileDescriptor(uri, "r")
            ?: return unavailable(sourceSha256, "SOURCE_OPEN_FAILED")
        val statSize = pfd.statSize

        return try {
            ParcelFileDescriptor.AutoCloseInputStream(pfd).use { input ->
                val channel = input.channel
                val fileSize = if (statSize >= 0L) statSize else channel.size()
                analyzeChannel(
                    channel = channel,
                    fileSize = fileSize,
                    ifdOffset = ifdOffset,
                    sourceSha256 = sourceSha256,
                )
            }
        } catch (e: Exception) {
            unavailable(
                sourceSha256,
                "FAIL_CLOSED_" + (e.message ?: e.javaClass.simpleName),
            )
        }
    }

    private fun analyzeChannel(
        channel: FileChannel,
        fileSize: Long,
        ifdOffset: Long,
        sourceSha256: String,
    ): JSONObject {
        val header = readAt(channel, 0L, 8)
        if (header.size != 8) return unavailable(sourceSha256, "TIFF_HEADER_TOO_SHORT")

        val order = when {
            header[0] == 'I'.code.toByte() && header[1] == 'I'.code.toByte() ->
                ByteOrder.LITTLE_ENDIAN
            header[0] == 'M'.code.toByte() && header[1] == 'M'.code.toByte() ->
                ByteOrder.BIG_ENDIAN
            else -> return unavailable(sourceSha256, "NOT_CLASSIC_TIFF_BYTE_ORDER")
        }

        val entries = readIfdEntries(channel, fileSize, order, ifdOffset)
            ?: return unavailable(sourceSha256, "RAW_IFD_UNREADABLE")

        val width = firstUnsigned(channel, fileSize, order, entries[256])
            ?: return unavailable(sourceSha256, "WIDTH_UNKNOWN")
        val height = firstUnsigned(channel, fileSize, order, entries[257])
            ?: return unavailable(sourceSha256, "HEIGHT_UNKNOWN")
        val bits = unsignedValues(channel, fileSize, order, entries[258]).firstOrNull() ?: 1L
        val compression = firstUnsigned(channel, fileSize, order, entries[259]) ?: 1L
        val fillOrder = firstUnsigned(channel, fileSize, order, entries[266]) ?: 1L
        val samplesPerPixel = firstUnsigned(channel, fileSize, order, entries[277]) ?: 1L
        val rowsPerStripTag = firstUnsigned(channel, fileSize, order, entries[278])
        val sampleFormat = firstUnsigned(channel, fileSize, order, entries[339]) ?: 1L
        val stripOffsets = unsignedValues(channel, fileSize, order, entries[273])
        val stripByteCounts = unsignedValues(channel, fileSize, order, entries[279])
        val blackRepeat = unsignedValues(channel, fileSize, order, entries[50713])
        val blackLevels = numericValues(channel, fileSize, order, entries[50714])
        val whiteLevels = numericValues(channel, fileSize, order, entries[50717])
        val activeArea = unsignedValues(channel, fileSize, order, entries[50829])

        val topology = JSONObject()
            .put("width", width)
            .put("height", height)
            .put("bits_per_sample", bits)
            .put("compression", compression)
            .put("fill_order", fillOrder)
            .put("samples_per_pixel", samplesPerPixel)
            .put("sample_format", sampleFormat)
            .put("strip_count", stripOffsets.size)
            .put("strip_byte_count_count", stripByteCounts.size)

        if (
            width <= 0L ||
            height <= 0L ||
            width > Int.MAX_VALUE ||
            height > Int.MAX_VALUE ||
            bits != 16L ||
            compression != 1L ||
            fillOrder != 1L ||
            samplesPerPixel != 1L ||
            sampleFormat != 1L ||
            stripOffsets.isEmpty() ||
            stripOffsets.size != stripByteCounts.size ||
            blackLevels.isEmpty() ||
            whiteLevels.isEmpty()
        ) {
            return unavailable(sourceSha256, "UNSUPPORTED_OR_INCOMPLETE_RAW_TOPOLOGY")
                .put("topology", topology)
        }

        val rowsPerStrip = when {
            rowsPerStripTag != null && rowsPerStripTag > 0L -> rowsPerStripTag
            stripOffsets.size == 1 -> height
            else -> return unavailable(sourceSha256, "ROWS_PER_STRIP_UNKNOWN")
                .put("topology", topology)
        }

        val repeatRows = blackRepeat.getOrNull(0)?.toInt()?.coerceAtLeast(1) ?: 1
        val repeatCols = blackRepeat.getOrNull(1)?.toInt()?.coerceAtLeast(1) ?: 1
        val repeatCount = repeatRows * repeatCols
        if (blackLevels.size < repeatCount) {
            return unavailable(sourceSha256, "BLACK_LEVEL_REPEAT_INCOMPLETE")
                .put("topology", topology)
        }

        val white = whiteLevels.first()
        if (!white.isFinite()) {
            return unavailable(sourceSha256, "WHITE_LEVEL_INVALID")
                .put("topology", topology)
        }

        val samples = ArrayList<Double>()
        val phaseSamples = Array(4) { ArrayList<Double>() }
        val fieldSamples = ArrayList<FieldSample>()
        val sourceWidth = width.toInt()
        val sourceHeight = height.toInt()
        val bytesPerRow = sourceWidth.toLong() * 2L

        val activeTop: Int
        val activeLeft: Int
        val activeBottom: Int
        val activeRight: Int
        if (
            activeArea.size >= 4 &&
            activeArea[0] >= 0L &&
            activeArea[0] < height &&
            activeArea[1] >= 0L &&
            activeArea[1] < width &&
            activeArea[2] > activeArea[0] &&
            activeArea[2] <= height &&
            activeArea[3] > activeArea[1] &&
            activeArea[3] <= width
        ) {
            activeTop = activeArea[0].toInt()
            activeLeft = activeArea[1].toInt()
            activeBottom = activeArea[2].toInt()
            activeRight = activeArea[3].toInt()
        } else {
            activeTop = 0
            activeLeft = 0
            activeBottom = sourceHeight
            activeRight = sourceWidth
        }
        topology.put(
            "sampled_active_area",
            JSONArray()
                .put(activeTop)
                .put(activeLeft)
                .put(activeBottom)
                .put(activeRight),
        )

        var baseY = activeTop
        while (baseY < activeBottom) {
            for (dy in 0..1) {
                val y = baseY + dy
                if (y >= activeBottom) continue

                val stripIndex = (y.toLong() / rowsPerStrip).toInt()
                if (stripIndex !in stripOffsets.indices) continue

                val rowInStrip = y.toLong() % rowsPerStrip
                val rowOffset = stripOffsets[stripIndex] + rowInStrip * bytesPerRow
                val stripStart = stripOffsets[stripIndex]
                val stripEnd = stripStart + stripByteCounts[stripIndex]

                if (
                    rowOffset < stripStart ||
                    rowOffset + bytesPerRow > stripEnd ||
                    rowOffset + bytesPerRow > fileSize
                ) {
                    continue
                }

                val row = readAt(channel, rowOffset, bytesPerRow.toInt())
                if (row.size != bytesPerRow.toInt()) continue
                val rb = ByteBuffer.wrap(row).order(order)

                var baseX = activeLeft
                while (baseX < activeRight) {
                    for (dx in 0..1) {
                        val x = baseX + dx
                        if (x >= activeRight) continue
                        val sample = u16(rb.getShort(x * 2))
                        val blackIndex =
                            (y % repeatRows) * repeatCols + (x % repeatCols)
                        val black = blackLevels[blackIndex]
                        val denominator = white - black
                        if (!black.isFinite() || denominator <= 0.0) continue

                        val normalized = (sample.toDouble() - black) / denominator
                        val phase = ((y and 1) shl 1) or (x and 1)
                        samples += normalized
                        phaseSamples[phase] += normalized
                        fieldSamples += FieldSample(
                            x = x,
                            y = y,
                            phase = phase,
                            value = normalized,
                        )
                    }
                    baseX += SAMPLE_GRID_STEP
                }
            }
            baseY += SAMPLE_GRID_STEP
        }

        if (samples.size < 256) {
            return unavailable(sourceSha256, "INSUFFICIENT_PAYLOAD_SAMPLES")
                .put("topology", topology)
                .put("sample_count", samples.size)
        }

        samples.sort()
        val p01 = percentile(samples, 0.01)
        val p10 = percentile(samples, 0.10)
        val p50 = percentile(samples, 0.50)
        val p90 = percentile(samples, 0.90)
        val p99 = percentile(samples, 0.99)
        val mean = samples.average()
        var sum2 = 0.0
        var nearBlack = 0L
        var lowSignal = 0L
        var atOrBelowBlack = 0L
        for (v in samples) {
            val d = v - mean
            sum2 += d * d
            if (v <= 0.0) atOrBelowBlack++
            if (v <= NEAR_BLACK_NORMALIZED) nearBlack++
            if (v <= LOW_SIGNAL_NORMALIZED) lowSignal++
        }
        val stddev = sqrt(max(0.0, sum2 / samples.size.toDouble()))
        val nearBlackFraction = nearBlack.toDouble() / samples.size.toDouble()
        val lowSignalFraction = lowSignal.toDouble() / samples.size.toDouble()
        val atOrBelowBlackFraction = atOrBelowBlack.toDouble() / samples.size.toDouble()

        val state =
            if (
                nearBlackFraction >= NEAR_BLACK_FRACTION_MIN &&
                p90 <= LOW_SIGNAL_P90_MAX
            ) {
                "NEAR_BLACK_DOMINATED"
            } else {
                "MEASURED_SIGNAL_PRESENT_OR_MIXED"
            }

        val opticalFieldSignal =
            opticalFieldSignalProfile(
                samples = fieldSamples,
                activeTop = activeTop,
                activeLeft = activeLeft,
                activeBottom = activeBottom,
                activeRight = activeRight,
            )

        val phaseJson = JSONArray()
        for (phase in phaseSamples.indices) {
            val values = phaseSamples[phase]
            values.sort()
            phaseJson.put(
                JSONObject()
                    .put("phase_index", phase)
                    .put("sample_count", values.size)
                    .put(
                        "p50_normalized_above_black",
                        if (values.isNotEmpty()) percentile(values, 0.50) else JSONObject.NULL,
                    )
                    .put(
                        "p90_normalized_above_black",
                        if (values.isNotEmpty()) percentile(values, 0.90) else JSONObject.NULL,
                    ),
            )
        }

        return JSONObject()
            .put("schema", "D.RAW/BacksideSignalSupport/0.1")
            .put("status", "MEASURED_SOURCE_PAYLOAD_SAMPLE_AVAILABLE")
            .put("source_sha256", sourceSha256)
            .put("authority", "SOURCE_PAYLOAD_MEASURED_WITHIN_SELECTED_DNG")
            .put("selected_source_is_untouched_adc_proof", false)
            .put("sample_grid_step", SAMPLE_GRID_STEP)
            .put("sample_count", samples.size)
            .put("topology", topology.put("rows_per_strip", rowsPerStrip))
            .put(
                "normalization",
                JSONObject()
                    .put("formula", "(raw_code-black_level)/(white_level-black_level)")
                    .put("black_repeat_rows", repeatRows)
                    .put("black_repeat_cols", repeatCols)
                    .put("black_levels", JSONArray(blackLevels))
                    .put("white_level", white)
                    .put("negative_values_preserved", true)
                    .put("clamping_applied", false),
            )
            .put(
                "global",
                JSONObject()
                    .put("p01_normalized_above_black", p01)
                    .put("p10_normalized_above_black", p10)
                    .put("p50_normalized_above_black", p50)
                    .put("p90_normalized_above_black", p90)
                    .put("p99_normalized_above_black", p99)
                    .put("mean_normalized_above_black", mean)
                    .put("stddev_normalized_above_black", stddev)
                    .put("fraction_at_or_below_black", atOrBelowBlackFraction)
                    .put("fraction_le_0_01", nearBlackFraction)
                    .put("fraction_le_0_02", lowSignalFraction),
            )
            .put("cfa_phase_summary", phaseJson)
            .put(
                "observation_optical_field_signal_v0_1",
                opticalFieldSignal,
            )
            .put("signal_support_state", state)
            .put(
                "research_thresholds",
                JSONObject()
                    .put("authority", "CONSERVATIVE_BLOCKING_HEURISTIC_ONLY")
                    .put("near_black_normalized", NEAR_BLACK_NORMALIZED)
                    .put("low_signal_normalized", LOW_SIGNAL_NORMALIZED)
                    .put("near_black_fraction_min", NEAR_BLACK_FRACTION_MIN)
                    .put("low_signal_p90_max", LOW_SIGNAL_P90_MAX),
            )
            .put("can_block_colour_reconstruction_when_near_black", true)
            .put("can_enable_chroma_correction", false)
            .put("local_tile_binding_available", false)
            .put("n2_local_support_bound", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", "D.RAW/BacksideSignalSupport/0.1")
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("authority", "UNKNOWN")
            .put("signal_support_state", "UNKNOWN")
            .put("can_block_colour_reconstruction_when_near_black", false)
            .put("can_enable_chroma_correction", false)
            .put("local_tile_binding_available", false)
            .put("n2_local_support_bound", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    private fun opticalFieldSignalProfile(
        samples: List<FieldSample>,
        activeTop: Int,
        activeLeft: Int,
        activeBottom: Int,
        activeRight: Int,
    ): JSONObject {
        if (samples.isEmpty()) {
            return JSONObject()
                .put(
                    "schema",
                    "D.RAW/ObservationOpticalFieldSignal/0.1",
                )
                .put("status", "UNKNOWN_FAIL_CLOSED")
                .put("reason", "NO_MEASURED_FIELD_SAMPLES")
        }

        val centerX =
            (activeLeft.toDouble() + activeRight.toDouble() - 1.0) * 0.5
        val centerY =
            (activeTop.toDouble() + activeBottom.toDouble() - 1.0) * 0.5

        val corners = arrayOf(
            doubleArrayOf(activeLeft.toDouble(), activeTop.toDouble()),
            doubleArrayOf((activeRight - 1).toDouble(), activeTop.toDouble()),
            doubleArrayOf(activeLeft.toDouble(), (activeBottom - 1).toDouble()),
            doubleArrayOf(
                (activeRight - 1).toDouble(),
                (activeBottom - 1).toDouble(),
            ),
        )
        var maxRadius = 0.0
        for (corner in corners) {
            val dx = corner[0] - centerX
            val dy = corner[1] - centerY
            maxRadius = max(maxRadius, sqrt(dx * dx + dy * dy))
        }
        if (!(maxRadius > 0.0)) {
            return JSONObject()
                .put(
                    "schema",
                    "D.RAW/ObservationOpticalFieldSignal/0.1",
                )
                .put("status", "UNKNOWN_FAIL_CLOSED")
                .put("reason", "ACTIVE_AREA_RADIUS_INVALID")
        }

        val radial =
            Array(OPTICAL_FIELD_RADIAL_BINS) { ArrayList<Double>() }
        val phaseRadial =
            Array(4) {
                Array(OPTICAL_FIELD_RADIAL_BINS) {
                    ArrayList<Double>()
                }
            }
        val sectorRadial =
            Array(OPTICAL_FIELD_RADIAL_BINS) {
                Array(OPTICAL_FIELD_AZIMUTH_BINS) {
                    ArrayList<Double>()
                }
            }

        for (sample in samples) {
            val dx = sample.x.toDouble() - centerX
            val dy = sample.y.toDouble() - centerY
            val r = sqrt(dx * dx + dy * dy)
            val rho = (r / maxRadius).coerceIn(0.0, 1.0)
            val radialBin =
                min(
                    OPTICAL_FIELD_RADIAL_BINS - 1,
                    (rho * OPTICAL_FIELD_RADIAL_BINS.toDouble()).toInt(),
                )

            var angle = atan2(dy, dx)
            if (angle < 0.0) angle += 2.0 * PI
            val sector =
                min(
                    OPTICAL_FIELD_AZIMUTH_BINS - 1,
                    (
                        angle /
                            (2.0 * PI) *
                            OPTICAL_FIELD_AZIMUTH_BINS.toDouble()
                        ).toInt(),
                )

            radial[radialBin] += sample.value
            if (sample.phase in 0..3) {
                phaseRadial[sample.phase][radialBin] += sample.value
            }
            sectorRadial[radialBin][sector] += sample.value
        }

        val annuli = JSONArray()
        val radialP50 = DoubleArray(OPTICAL_FIELD_RADIAL_BINS) {
            Double.NaN
        }

        for (bin in 0 until OPTICAL_FIELD_RADIAL_BINS) {
            val values = radial[bin]
            values.sort()
            val p10 =
                if (values.isNotEmpty()) percentile(values, 0.10) else Double.NaN
            val p50 =
                if (values.isNotEmpty()) percentile(values, 0.50) else Double.NaN
            val p90 =
                if (values.isNotEmpty()) percentile(values, 0.90) else Double.NaN
            radialP50[bin] = p50

            val phaseMedians = JSONArray()
            for (phase in 0..3) {
                val pv = phaseRadial[phase][bin]
                pv.sort()
                phaseMedians.put(
                    if (pv.isNotEmpty()) {
                        percentile(pv, 0.50)
                    } else {
                        JSONObject.NULL
                    },
                )
            }

            val sectorMedians = ArrayList<Double>()
            val sectorJson = JSONArray()
            for (sector in 0 until OPTICAL_FIELD_AZIMUTH_BINS) {
                val sv = sectorRadial[bin][sector]
                sv.sort()
                val median =
                    if (sv.isNotEmpty()) {
                        percentile(sv, 0.50)
                    } else {
                        Double.NaN
                    }
                if (median.isFinite()) sectorMedians += median
                sectorJson.put(
                    JSONObject()
                        .put("sector", sector)
                        .put("sample_count", sv.size)
                        .put(
                            "p50_normalized_above_black",
                            if (median.isFinite()) {
                                median
                            } else {
                                JSONObject.NULL
                            },
                        ),
                )
            }

            sectorMedians.sort()
            val sectorMedian =
                if (sectorMedians.isNotEmpty()) {
                    percentile(sectorMedians, 0.50)
                } else {
                    Double.NaN
                }
            val sectorAbsDeviation =
                sectorMedians
                    .map { kotlin.math.abs(it - sectorMedian) }
                    .sorted()
            val sectorMad =
                if (sectorAbsDeviation.isNotEmpty()) {
                    percentile(sectorAbsDeviation, 0.50)
                } else {
                    Double.NaN
                }
            val relativeSectorMad =
                if (
                    sectorMad.isFinite() &&
                    sectorMedian.isFinite() &&
                    kotlin.math.abs(sectorMedian) > 1.0e-12
                ) {
                    sectorMad / kotlin.math.abs(sectorMedian)
                } else {
                    Double.NaN
                }

            annuli.put(
                JSONObject()
                    .put("bin", bin)
                    .put(
                        "rho_min",
                        bin.toDouble() /
                            OPTICAL_FIELD_RADIAL_BINS.toDouble(),
                    )
                    .put(
                        "rho_max",
                        (bin + 1).toDouble() /
                            OPTICAL_FIELD_RADIAL_BINS.toDouble(),
                    )
                    .put("sample_count", values.size)
                    .put(
                        "p10_normalized_above_black",
                        if (p10.isFinite()) p10 else JSONObject.NULL,
                    )
                    .put(
                        "p50_normalized_above_black",
                        if (p50.isFinite()) p50 else JSONObject.NULL,
                    )
                    .put(
                        "p90_normalized_above_black",
                        if (p90.isFinite()) p90 else JSONObject.NULL,
                    )
                    .put("cfa_phase_p50", phaseMedians)
                    .put("azimuth_sectors", sectorJson)
                    .put(
                        "azimuth_sector_median_mad",
                        if (sectorMad.isFinite()) {
                            sectorMad
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "azimuth_sector_relative_mad",
                        if (relativeSectorMad.isFinite()) {
                            relativeSectorMad
                        } else {
                            JSONObject.NULL
                        },
                    ),
            )
        }

        var validTransitions = 0
        var nonIncreasingTransitions = 0
        for (i in 0 until radialP50.size - 1) {
            val a = radialP50[i]
            val b = radialP50[i + 1]
            if (!a.isFinite() || !b.isFinite()) continue
            validTransitions++
            if (b <= a) nonIncreasingTransitions++
        }

        val inner =
            radialP50.firstOrNull { it.isFinite() } ?: Double.NaN
        val outer =
            radialP50.lastOrNull { it.isFinite() } ?: Double.NaN
        val outerToInner =
            if (
                inner.isFinite() &&
                outer.isFinite() &&
                kotlin.math.abs(inner) > 1.0e-12
            ) {
                outer / inner
            } else {
                Double.NaN
            }

        return JSONObject()
            .put(
                "schema",
                "D.RAW/ObservationOpticalFieldSignal/0.1",
            )
            .put(
                "status",
                "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE",
            )
            .put(
                "authority",
                "MEASURED_SOURCE_SAMPLES_SCENE_LENS_SENSOR_COMPOSITE",
            )
            .put("sample_count", samples.size)
            .put("radial_bin_count", OPTICAL_FIELD_RADIAL_BINS)
            .put("azimuth_bin_count", OPTICAL_FIELD_AZIMUTH_BINS)
            .put(
                "field_origin_source",
                JSONArray().put(centerX).put(centerY),
            )
            .put("max_active_corner_radius_source_px", maxRadius)
            .put("annuli", annuli)
            .put(
                "observed_outer_to_inner_p50_ratio",
                if (outerToInner.isFinite()) {
                    outerToInner
                } else {
                    JSONObject.NULL
                },
            )
            .put(
                "radial_p50_non_increasing_transition_fraction",
                if (validTransitions > 0) {
                    nonIncreasingTransitions.toDouble() /
                        validTransitions.toDouble()
                } else {
                    JSONObject.NULL
                },
            )
            .put("lens_only_vignetting_proven", false)
            .put("scene_illumination_separated", false)
            .put("sensor_angular_response_separated", false)
            .put("optical_axis_proven", false)
            .put("cos4_model_assumed", false)
            .put("correction_gain_allowed", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun findIfdOffset(
        parsed: JSONObject,
        rawPath: String,
    ): Long? {
        val ifds = parsed.optJSONArray("ifds") ?: return null
        for (i in 0 until ifds.length()) {
            val ifd = ifds.optJSONObject(i) ?: continue
            if (ifd.optString("path") == rawPath) {
                return ifd.optLong("offset").takeIf { it > 0L }
            }
        }
        return null
    }

    private fun readIfdEntries(
        channel: FileChannel,
        fileSize: Long,
        order: ByteOrder,
        offset: Long,
    ): Map<Int, Entry>? {
        if (offset <= 0L || offset >= fileSize) return null
        val countBytes = readAt(channel, offset, 2)
        if (countBytes.size != 2) return null
        val count = u16(ByteBuffer.wrap(countBytes).order(order).short)
        if (count !in 1..MAX_IFD_ENTRIES) return null

        val blockSize = 2 + count * 12 + 4
        val block = readAt(channel, offset, blockSize)
        if (block.size != blockSize) return null

        val out = mutableMapOf<Int, Entry>()
        var p = 2
        repeat(count) {
            val eb = ByteBuffer.wrap(block, p, 12).slice().order(order)
            val tag = u16(eb.short)
            val type = u16(eb.short)
            val itemCount = u32(eb.int)
            val valueField = ByteArray(4)
            eb.get(valueField)
            val valueOffset = u32(ByteBuffer.wrap(valueField).order(order).int)
            out[tag] = Entry(tag, type, itemCount, valueOffset, valueField)
            p += 12
        }
        return out
    }

    private fun firstUnsigned(
        channel: FileChannel,
        fileSize: Long,
        order: ByteOrder,
        entry: Entry?,
    ): Long? = unsignedValues(channel, fileSize, order, entry).firstOrNull()

    private fun unsignedValues(
        channel: FileChannel,
        fileSize: Long,
        order: ByteOrder,
        entry: Entry?,
    ): List<Long> {
        if (entry == null) return emptyList()
        val bytes = valueBytes(channel, fileSize, order, entry) ?: return emptyList()
        val b = ByteBuffer.wrap(bytes).order(order)
        val count = entry.count.toInt()
        return when (entry.type) {
            1 -> List(count) { (b.get().toInt() and 0xff).toLong() }
            3 -> List(count) { u16(b.short).toLong() }
            4 -> List(count) { u32(b.int) }
            else -> emptyList()
        }
    }

    private fun numericValues(
        channel: FileChannel,
        fileSize: Long,
        order: ByteOrder,
        entry: Entry?,
    ): List<Double> {
        if (entry == null) return emptyList()
        val bytes = valueBytes(channel, fileSize, order, entry) ?: return emptyList()
        val b = ByteBuffer.wrap(bytes).order(order)
        val count = entry.count.toInt()
        val out = ArrayList<Double>(count)
        repeat(count) {
            when (entry.type) {
                1 -> out += (b.get().toInt() and 0xff).toDouble()
                3 -> out += u16(b.short).toDouble()
                4 -> out += u32(b.int).toDouble()
                5 -> {
                    val n = u32(b.int)
                    val d = u32(b.int)
                    out += if (d != 0L) n.toDouble() / d.toDouble() else Double.NaN
                }
                6 -> out += b.get().toDouble()
                8 -> out += b.short.toDouble()
                9 -> out += b.int.toDouble()
                10 -> {
                    val n = b.int.toLong()
                    val d = b.int.toLong()
                    out += if (d != 0L) n.toDouble() / d.toDouble() else Double.NaN
                }
                11 -> out += b.float.toDouble()
                12 -> out += b.double
                else -> return emptyList()
            }
        }
        return out
    }

    private fun valueBytes(
        channel: FileChannel,
        fileSize: Long,
        order: ByteOrder,
        entry: Entry,
    ): ByteArray? {
        val typeSize = when (entry.type) {
            1, 2, 6, 7 -> 1
            3, 8 -> 2
            4, 9, 11 -> 4
            5, 10, 12 -> 8
            else -> return null
        }
        if (entry.count <= 0L || entry.count > Int.MAX_VALUE) return null
        val byteCount = entry.count * typeSize.toLong()
        if (byteCount <= 0L || byteCount > MAX_VALUE_BYTES) return null

        return if (byteCount <= 4L) {
            entry.inlineBytes.copyOf(byteCount.toInt())
        } else {
            if (
                entry.valueOffset <= 0L ||
                entry.valueOffset + byteCount > fileSize
            ) {
                null
            } else {
                readAt(channel, entry.valueOffset, byteCount.toInt())
            }
        }
    }

    private fun percentile(
        sorted: List<Double>,
        q: Double,
    ): Double {
        if (sorted.isEmpty()) return Double.NaN
        val pos = q.coerceIn(0.0, 1.0) * (sorted.size - 1).toDouble()
        val lo = pos.toInt()
        val hi = min(sorted.size - 1, lo + 1)
        val f = pos - lo.toDouble()
        return sorted[lo] * (1.0 - f) + sorted[hi] * f
    }

    private fun readAt(
        channel: FileChannel,
        offset: Long,
        length: Int,
    ): ByteArray {
        if (offset < 0L || length < 0) return ByteArray(0)
        val buffer = ByteBuffer.allocate(length)
        channel.position(offset)
        while (buffer.hasRemaining()) {
            val n = channel.read(buffer)
            if (n < 0) break
        }
        return buffer.array().copyOf(buffer.position())
    }

    private fun u16(v: Short): Int = v.toInt() and 0xffff
    private fun u32(v: Int): Long = v.toLong() and 0xffffffffL
}
