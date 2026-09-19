package com.truthraw.adaptiveui

import org.json.JSONObject

object NefMeasurementReportEncoder {
    fun toJson(job: RawJob, result: NefMeasurementResult.Ready): String {
        val m = result.metrics
        return JSONObject()
            .put("schema", "TruthRawNefMeasurementEvidence/0.58")
            .put("authority", "MEASUREMENT_ONLY_CONTAINER_SAMPLE_DECODE")
            .put("source", JSONObject()
                .put("displayName", job.source.displayName)
                .put("declaredSizeBytes", job.source.declaredSizeBytes ?: JSONObject.NULL)
                .put("sealedByteLength", m.sourceBytes)
                .put("sha256", m.sourceSha256)
                .put("ingressRoute", job.source.sourceRoute.name)
                .put("formatId", job.source.format.id)
                .put("vendor", job.source.format.vendorLabel))
            .put("decoder", JSONObject()
                .put("backend", job.source.format.decoderBackend.name)
                .put("id", "truthraw.nikon-nef-uncompressed16-cfa.v0.1")
                .put("measurementAdmissionReady", m.measurementAdmissionReady)
                .put("scientificAdmissionReady", m.scientificAdmissionReady)
                .put("exactCfaSamplesAvailable", m.exactCfaSamplesAvailable)
                .put("directSensorAdcClaimAllowed", m.directSensorAdcClaimAllowed)
                .put("fullRawFrameMaterialized", m.fullRawFrameMaterialized))
            .put("sampleDomain", JSONObject()
                .put("width", m.sourceWidth)
                .put("height", m.sourceHeight)
                .put("cfaCode", m.cfaCode)
                .put("previewMinSampleCode", m.minSampleCodeInPreview)
                .put("previewMaxSampleCode", m.maxSampleCodeInPreview)
                .put("previewSamplesRead", m.previewSamplesRead)
                .put("sourceResidentUpperBoundBytes", m.sourceResidentUpperBoundBytes))
            .put("scientificMasterCreated", false)
            .put("blackSubtractionApplied", false)
            .put("demosaicApplied", false)
            .put("colorCorrectionApplied", false)
            .put("previewRole", "VISIBILITY_PROXY_NOT_PHOTOGRAPHIC_OUTPUT")
            .put("claim",
                "Exact CFA sample codes are exposed for the strict admitted NEF subset; " +
                    "camera black/saturation/noise/color authority and untouched-ADC provenance remain unproven.")
            .toString(2)
    }
}
