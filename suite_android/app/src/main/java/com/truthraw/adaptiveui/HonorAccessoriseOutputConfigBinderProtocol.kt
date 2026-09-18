package com.truthraw.adaptiveui

import android.os.Binder
import android.os.IBinder
import android.os.IInterface
import android.os.Parcel
import org.json.JSONObject

/**
 * v0.41 exact Binder protocol reconstructed from the supplied Honor Camera.apk.
 *
 * Scope is intentionally narrow:
 * - only registerOutputConfigCallback (transaction 3)
 * - only unregisterOutputConfigCallback (transaction 4)
 * - never setPreviewSurface (transaction 1)
 * - never exitPreview (transaction 2)
 * - never register capture/preview-state callbacks in this build
 *
 * This class does not assign semantics to callback strings/integers. It only preserves
 * the exact values delivered by the Honor Binder interface.
 */
object HonorAccessoriseOutputConfigBinderProtocol {
    const val SERVICE_DESCRIPTOR =
        "com.hihonor.camera.accessorise.aidl.ICameraAccessoriseService"
    const val OUTPUT_CALLBACK_DESCRIPTOR =
        "com.hihonor.camera.accessorise.aidl.IOutputConfigCallback"

    const val SERVICE_TX_REGISTER_OUTPUT_CONFIG_CALLBACK = 3
    const val SERVICE_TX_UNREGISTER_OUTPUT_CONFIG_CALLBACK = 4

    const val OUTPUT_CALLBACK_TX_PREVIEW_CONFIG_CHANGED = 1
    const val OUTPUT_CALLBACK_TX_CURRENT_MODE_CHANGED = 2

    data class TransactionResult(
        val transactReturned: Boolean,
        val replyReadPass: Boolean,
        val exceptionClass: String?,
        val exceptionMessage: String?,
    ) {
        val success: Boolean
            get() = transactReturned && replyReadPass && exceptionClass == null

        fun toJson(): JSONObject =
            JSONObject()
                .put("transactReturned", transactReturned)
                .put("replyReadPass", replyReadPass)
                .put("exceptionClass", exceptionClass ?: JSONObject.NULL)
                .put("exceptionMessage", exceptionMessage ?: JSONObject.NULL)
                .put("success", success)
    }

    class OutputConfigCallbackBinder(
        private val sink: (type: String, payload: JSONObject) -> Unit,
    ) : Binder(), IInterface {
        init {
            // We deliberately attach only the descriptor. The remote Honor service will
            // see this as a remote binder and use its proxy path; no Honor classes are
            // linked into TruthRaw.
            attachInterface(this, OUTPUT_CALLBACK_DESCRIPTOR)
        }

        override fun asBinder(): IBinder = this

        override fun onTransact(code: Int, data: Parcel, reply: Parcel?, flags: Int): Boolean {
            if (code == INTERFACE_TRANSACTION) {
                reply?.writeString(OUTPUT_CALLBACK_DESCRIPTOR)
                return true
            }

            return when (code) {
                OUTPUT_CALLBACK_TX_PREVIEW_CONFIG_CHANGED -> {
                    data.enforceInterface(OUTPUT_CALLBACK_DESCRIPTOR)
                    val width = data.readInt()
                    val height = data.readInt()
                    runCatching { data.enforceNoDataAvail() }
                    sink(
                        "HONOR_OUTPUT_PREVIEW_CONFIG_CHANGED",
                        JSONObject()
                            .put("transactionCode", code)
                            .put("width", width)
                            .put("height", height)
                            .put("semanticMeaningAssumed", false),
                    )
                    reply?.writeNoException()
                    true
                }

                OUTPUT_CALLBACK_TX_CURRENT_MODE_CHANGED -> {
                    data.enforceInterface(OUTPUT_CALLBACK_DESCRIPTOR)
                    val mode = data.readString()
                    runCatching { data.enforceNoDataAvail() }
                    sink(
                        "HONOR_OUTPUT_CURRENT_MODE_CHANGED",
                        JSONObject()
                            .put("transactionCode", code)
                            .put("modeString", mode ?: JSONObject.NULL)
                            .put("semanticMeaningAssumed", false),
                    )
                    reply?.writeNoException()
                    true
                }

                else -> {
                    sink(
                        "HONOR_OUTPUT_CALLBACK_UNKNOWN_TRANSACTION",
                        JSONObject()
                            .put("transactionCode", code)
                            .put("flags", flags)
                            .put("handled", false),
                    )
                    super.onTransact(code, data, reply, flags)
                }
            }
        }
    }

    fun registerOutputConfigCallback(
        service: IBinder,
        callback: IBinder,
    ): TransactionResult =
        transactOneBinderArg(
            service = service,
            transactionCode = SERVICE_TX_REGISTER_OUTPUT_CONFIG_CALLBACK,
            callback = callback,
        )

    fun unregisterOutputConfigCallback(
        service: IBinder,
        callback: IBinder,
    ): TransactionResult =
        transactOneBinderArg(
            service = service,
            transactionCode = SERVICE_TX_UNREGISTER_OUTPUT_CONFIG_CALLBACK,
            callback = callback,
        )

    private fun transactOneBinderArg(
        service: IBinder,
        transactionCode: Int,
        callback: IBinder,
    ): TransactionResult {
        val data = Parcel.obtain()
        val reply = Parcel.obtain()
        return try {
            data.writeInterfaceToken(SERVICE_DESCRIPTOR)
            data.writeStrongBinder(callback)

            val transactReturned = try {
                service.transact(transactionCode, data, reply, 0)
            } catch (t: Throwable) {
                return TransactionResult(
                    transactReturned = false,
                    replyReadPass = false,
                    exceptionClass = t.javaClass.name,
                    exceptionMessage = t.message,
                )
            }

            try {
                reply.readException()
                TransactionResult(
                    transactReturned = transactReturned,
                    replyReadPass = true,
                    exceptionClass = null,
                    exceptionMessage = null,
                )
            } catch (t: Throwable) {
                TransactionResult(
                    transactReturned = transactReturned,
                    replyReadPass = false,
                    exceptionClass = t.javaClass.name,
                    exceptionMessage = t.message,
                )
            }
        } finally {
            reply.recycle()
            data.recycle()
        }
    }
}
