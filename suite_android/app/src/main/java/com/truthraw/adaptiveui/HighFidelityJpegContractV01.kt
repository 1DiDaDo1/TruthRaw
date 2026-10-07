package com.truthraw.adaptiveui

import java.io.BufferedInputStream
import java.io.DataInputStream
import java.io.EOFException
import java.io.File
import java.io.FileInputStream

/**
 * Fail-closed verifier for D.RAW High-Fidelity JPEG v0.1.
 *
 * JPEG has no standardized integer "quality" metadata. D.RAW therefore verifies
 * the admitted Q100 encoder's actual output structure: baseline SOF0, exact
 * geometry, 1x1 sampling for all three components, and all-one 8-bit DQT tables.
 */
internal object HighFidelityJpegContractV01 {
    const val CONTRACT_VERSION = "HighFidelityJpegContract/0.1"
    const val REQUIRED_QUALITY = 100
    const val REQUIRED_SAMPLING = "1x1,1x1,1x1"

    sealed interface Result {
        data class Ready(
            val width: Int,
            val height: Int,
            val requestedQuality: Int = REQUIRED_QUALITY,
            val sampling: String = REQUIRED_SAMPLING,
            val baselineSof0: Boolean = true,
            val quantizationTablesAllOnes: Boolean = true,
        ) : Result

        data class Failed(val reason: String) : Result
    }

    fun verify(file: File, expectedWidth: Int, expectedHeight: Int): Result {
        if (!file.isFile || file.length() <= 4L) {
            return Result.Failed("High-Fidelity JPEG ontbreekt of is leeg.")
        }
        if (expectedWidth !in 1..65535 || expectedHeight !in 1..65535) {
            return Result.Failed("High-Fidelity JPEG heeft ongeldige verwachte geometrie.")
        }

        return try {
            DataInputStream(BufferedInputStream(FileInputStream(file), 64 * 1024)).use { input ->
                if (input.readUnsignedByte() != 0xff || input.readUnsignedByte() != 0xd8) {
                    return Result.Failed("High-Fidelity JPEG mist SOI-marker.")
                }

                var dqt0AllOnes = false
                var dqt1AllOnes = false
                var sof0Valid = false

                while (true) {
                    val marker = nextMarker(input)
                        ?: return Result.Failed("High-Fidelity JPEG eindigde vóór SOS.")
                    when (marker) {
                        0xd8 -> Unit
                        0xd9 -> return Result.Failed("High-Fidelity JPEG bereikte EOI vóór SOS.")
                        in 0xd0..0xd7, 0x01 -> Unit
                        else -> {
                            val segmentLength = input.readUnsignedShort()
                            if (segmentLength < 2) {
                                return Result.Failed("High-Fidelity JPEG heeft ongeldige segmentlengte.")
                            }
                            val payloadLength = segmentLength - 2
                            when (marker) {
                                0xdb -> {
                                    val payload = ByteArray(payloadLength)
                                    input.readFully(payload)
                                    var offset = 0
                                    while (offset < payload.size) {
                                        val spec = payload[offset].toInt() and 0xff
                                        offset += 1
                                        val precision = spec ushr 4
                                        val tableId = spec and 0x0f
                                        if (precision != 0) {
                                            return Result.Failed("High-Fidelity JPEG DQT is niet 8-bit.")
                                        }
                                        if (offset + 64 > payload.size) {
                                            return Result.Failed("High-Fidelity JPEG DQT is afgekapt.")
                                        }
                                        var allOnes = true
                                        repeat(64) { index ->
                                            if ((payload[offset + index].toInt() and 0xff) != 1) {
                                                allOnes = false
                                            }
                                        }
                                        when (tableId) {
                                            0 -> dqt0AllOnes = allOnes
                                            1 -> dqt1AllOnes = allOnes
                                        }
                                        offset += 64
                                    }
                                    if (offset != payload.size) {
                                        return Result.Failed("High-Fidelity JPEG DQT payload mismatch.")
                                    }
                                }
                                0xc0 -> {
                                    val payload = ByteArray(payloadLength)
                                    input.readFully(payload)
                                    if (payload.size != 15) {
                                        return Result.Failed("High-Fidelity JPEG SOF0-lengte wijkt af.")
                                    }
                                    val precision = payload[0].toInt() and 0xff
                                    val height = u16(payload, 1)
                                    val width = u16(payload, 3)
                                    val componentCount = payload[5].toInt() and 0xff
                                    if (precision != 8 || width != expectedWidth || height != expectedHeight || componentCount != 3) {
                                        return Result.Failed(
                                            "High-Fidelity JPEG SOF0 geometrie/componenten voldoen niet aan contract.",
                                        )
                                    }
                                    val expectedIds = intArrayOf(1, 2, 3)
                                    val expectedTables = intArrayOf(0, 1, 1)
                                    for (component in 0 until 3) {
                                        val base = 6 + component * 3
                                        val id = payload[base].toInt() and 0xff
                                        val sampling = payload[base + 1].toInt() and 0xff
                                        val qTable = payload[base + 2].toInt() and 0xff
                                        if (id != expectedIds[component] || sampling != 0x11 || qTable != expectedTables[component]) {
                                            return Result.Failed(
                                                "High-Fidelity JPEG is niet bewezen 4:4:4 (1x1/1x1/1x1).",
                                            )
                                        }
                                    }
                                    sof0Valid = true
                                }
                                0xda -> {
                                    if (!sof0Valid) {
                                        return Result.Failed("High-Fidelity JPEG mist geldige baseline SOF0.")
                                    }
                                    if (!dqt0AllOnes || !dqt1AllOnes) {
                                        return Result.Failed(
                                            "High-Fidelity JPEG Q100-contract faalde: DQT0/DQT1 zijn niet volledig 1.",
                                        )
                                    }
                                    return Result.Ready(expectedWidth, expectedHeight)
                                }
                                in 0xc1..0xcf -> {
                                    if (marker != 0xc4 && marker != 0xc8 && marker != 0xcc) {
                                        return Result.Failed(
                                            "High-Fidelity JPEG gebruikt een niet-toegelaten SOF-marker 0x${marker.toString(16)}.",
                                        )
                                    }
                                    skipFully(input, payloadLength)
                                }
                                else -> skipFully(input, payloadLength)
                            }
                        }
                    }
                }
            }
        } catch (_: EOFException) {
            Result.Failed("High-Fidelity JPEG header is afgekapt.")
        } catch (error: Throwable) {
            Result.Failed(
                "High-Fidelity JPEG verificatie faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }
    }

    private fun nextMarker(input: DataInputStream): Int? {
        var value: Int
        do {
            value = try { input.readUnsignedByte() } catch (_: EOFException) { return null }
        } while (value != 0xff)
        do {
            value = input.readUnsignedByte()
        } while (value == 0xff)
        return if (value == 0x00) nextMarker(input) else value
    }

    private fun skipFully(input: DataInputStream, count: Int) {
        var remaining = count
        while (remaining > 0) {
            val skipped = input.skipBytes(remaining)
            if (skipped <= 0) {
                input.readUnsignedByte()
                remaining -= 1
            } else {
                remaining -= skipped
            }
        }
    }

    private fun u16(bytes: ByteArray, offset: Int): Int =
        ((bytes[offset].toInt() and 0xff) shl 8) or
            (bytes[offset + 1].toInt() and 0xff)
}
