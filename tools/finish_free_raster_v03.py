from pathlib import Path
import re

main_path = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/MainActivity.kt')
main = main_path.read_text()
old = '''                val dir = File(filesDir, "photo_export/$expectedJob").apply { mkdirs() }
                val rendered = FullResJpegExporter.renderToPrivateJpeg(
                    contentResolver, job, flags, quarterTurns, dir,
                )
                var jpegOutputPreview: UnifiedOutputPreviewResult.Ready? = null
                var status = when (rendered) {
                    is FullResJpegResult.Failed -> rendered.reason
                    is FullResJpegResult.Success -> {
                        val ok = FullResJpegExporter.commit(
                            contentResolver,
                            rendered.file,
                            destination,
                            rendered.metrics.jpegSha256,
                        )
                        val m = rendered.metrics
                        rendered.file.delete()
                        if (!ok) {
                            runCatching { contentResolver.delete(destination, null, null) }
                            "JPG commit/post-write SHA-verify faalde."
                        } else {
                            when (
                                val preview = UnifiedOutputPreviewLoader.loadSavedJpeg(
                                    contentResolver,
                                    destination,
                                    "JPG full-resolution " + route,
                                    384,
                                )
                            ) {
                                is UnifiedOutputPreviewResult.Ready ->
                                    jpegOutputPreview = preview
                                is UnifiedOutputPreviewResult.Failed ->
                                    jpegStatus =
                                        "JPG opgeslagen; uitkomst-preview faalde: " +
                                            preview.reason
                            }
                            "JPG full-resolution gereed · ${m.width}×${m.height} · " +
                                "${formatBytes(m.jpegBytes)} · route=$route · detail=${m.detailApplied} · " +
                                "Light pixels=${m.lightAdjustedPixels} · Scientific Master/Backplane=${m.scientificMasterBound}/${m.backplaneBound} · " +
                                "rotatie=${quarterTurns * 90}° · HDR-front=${m.hdrBakedIntoFront} (APPEARANCE_ONLY) · " +
                                "Restoration-front=${m.restorationBakedIntoFront} (AESTHETIC_REINTEGRATION_ONLY)."
                        }
                    }
                }
'''
new = '''                val dir = File(filesDir, "photo_export/$expectedJob").apply { mkdirs() }
                UnifiedOutputFreeRasterBridge.clear(
                    "full_resolution_render_started:$expectedJob",
                )
                val rendered = FullResJpegExporter.renderToPrivateJpeg(
                    contentResolver, job, flags, quarterTurns, dir,
                )
                var jpegOutputPreview: UnifiedOutputPreviewResult.Ready? = null
                var status = when (rendered) {
                    is FullResJpegResult.Failed -> rendered.reason
                    is FullResJpegResult.Success -> {
                        val m = rendered.metrics
                        val freeRasterPublish =
                            UnifiedOutputFreeRasterPublisherV01.publishFromRenderedJpeg(
                                binding = binding,
                                renderedFile = rendered.file,
                                outputWidth = m.width,
                                outputHeight = m.height,
                                jpegSha256 = m.jpegSha256,
                                artifactDirectory =
                                    File(filesDir, "unified_output_free_raster"),
                            )
                        val freeRasterStatus = when (freeRasterPublish) {
                            is UnifiedOutputFreeRasterPublishAdapterResultV01.Ready ->
                                "Free Raster=${m.width}×${m.height} full-res sibling · PRESENTATION_ONLY"
                            is UnifiedOutputFreeRasterPublishAdapterResultV01.Failed -> {
                                UnifiedOutputFreeRasterBridge.clear(
                                    "full_resolution_publish_failed",
                                )
                                "Free Raster publish geblokkeerd: ${freeRasterPublish.reason}"
                            }
                        }
                        val ok = FullResJpegExporter.commit(
                            contentResolver,
                            rendered.file,
                            destination,
                            m.jpegSha256,
                        )
                        rendered.file.delete()
                        if (!ok) {
                            runCatching { contentResolver.delete(destination, null, null) }
                            "JPG commit/post-write SHA-verify faalde · $freeRasterStatus"
                        } else {
                            when (
                                val preview = UnifiedOutputPreviewLoader.loadSavedJpeg(
                                    contentResolver,
                                    destination,
                                    "JPG full-resolution " + route,
                                    384,
                                )
                            ) {
                                is UnifiedOutputPreviewResult.Ready ->
                                    jpegOutputPreview = preview
                                is UnifiedOutputPreviewResult.Failed ->
                                    jpegStatus =
                                        "JPG opgeslagen; uitkomst-preview faalde: " +
                                            preview.reason
                            }
                            "JPG full-resolution gereed · ${m.width}×${m.height} · " +
                                "${formatBytes(m.jpegBytes)} · route=$route · detail=${m.detailApplied} · " +
                                "Light pixels=${m.lightAdjustedPixels} · Scientific Master/Backplane=${m.scientificMasterBound}/${m.backplaneBound} · " +
                                "rotatie=${quarterTurns * 90}° · HDR-front=${m.hdrBakedIntoFront} (APPEARANCE_ONLY) · " +
                                "Restoration-front=${m.restorationBakedIntoFront} (AESTHETIC_REINTEGRATION_ONLY) · " +
                                freeRasterStatus
                        }
                    }
                }
'''
if new in main:
    pass
elif old in main:
    main = main.replace(old, new, 1)
    main_path.write_text(main)
else:
    raise SystemExit('MainActivity target block not found; refusing non-deterministic patch')

ws_path = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/TruthRawWorkspaceActivity.kt')
ws = ws_path.read_text()
if 'D.RAW_FREE_RASTER_FULL_RES' not in ws:
    pattern = re.compile(r'    private fun consumeUnifiedOutputPresentation\(\) \{.*?\n    \}\n\n    private fun orientUnifiedPresentationBitmap', re.S)
    replacement = '''    private fun consumeUnifiedOutputPresentation() {
        if (presentationMode != PresentationMode.INTERNAL_D_RAW) return
        val generation = ++presentationGeneration
        val presentationSnapshot = UnifiedOutputPresentationBridge.acquire()
        val freeRasterSnapshot = UnifiedOutputFreeRasterBridge.acquire()
        if (presentationSnapshot == null || freeRasterSnapshot == null) {
            presentationSnapshot?.bitmap?.takeUnless { it.isRecycled }?.recycle()
            clearCanvasPresentation(
                "Canvasstatus · EMPTY · geen geldige full-resolution D.RAW Free Raster sibling beschikbaar · " +
                    "geen snapshot = geen afgeleide authority",
            )
            return
        }
        val metadata = presentationSnapshot.metadata.toMap()
        presentationSnapshot.bitmap.takeUnless { it.isRecycled }?.recycle()
        val expectedRoute = currentSelectedRoute()
        val activeSourceJobId = metadata[UnifiedOutputPresentationBridge.META_SOURCE_JOB_ID].orEmpty()
        val activeSourceUri = metadata[UnifiedOutputPresentationBridge.META_SOURCE_URI].orEmpty()
        val publishedRoute = metadata[UnifiedOutputPresentationBridge.META_ROUTE].orEmpty()
        val request = freeRasterSnapshot.request
        val contractValidation = DrawUnifiedOutputRasterContractV01.validate(
            request = request,
            activeJobId = activeSourceJobId,
        )
        val contractFailure =
            (contractValidation as? DrawUnifiedOutputRasterBindResultV01.Failed)?.reason
        val rejection = when {
            presentationSnapshot.contractId != UnifiedOutputPresentationBridge.PRESENTATION_CONTRACT_ID ->
                "presentation contract mismatch"
            metadata[UnifiedOutputPresentationBridge.META_ORIGIN] != UnifiedOutputPresentationBridge.ORIGIN_UNIFIED_OUTPUT_READY ->
                "origin is geen Unified Output Ready"
            metadata[UnifiedOutputPresentationBridge.META_SOURCE_BINDING_KIND] != UnifiedOutputPresentationBridge.SOURCE_BINDING_ACTIVE_JOB ->
                "process-local sourcebinding ontbreekt"
            activeSourceJobId.isBlank() || activeSourceUri.isBlank() ->
                "actieve sourcebinding ontbreekt"
            publishedRoute != expectedRoute ->
                "snapshot-route $publishedRoute != actieve route $expectedRoute"
            metadata[UnifiedOutputPresentationBridge.META_SCIENTIFIC_WRITEBACK_ALLOWED] != "false" ->
                "scientific writeback is niet bewezen dicht"
            metadata[UnifiedOutputPresentationBridge.META_CREATES_NEW_EVIDENCE] != "false" ->
                "creates-new-evidence contract mismatch"
            request.purpose != DrawUnifiedOutputRasterRequestV01.Purpose.FREE_RASTER_VIEW ->
                "full-resolution sibling is geen FREE_RASTER_VIEW"
            request.sourceJobId != activeSourceJobId || request.sourceUri != activeSourceUri ->
                "full-resolution sibling hoort niet bij de actieve sealed observation"
            request.route != expectedRoute || request.route != publishedRoute ->
                "full-resolution sibling-route is stale"
            request.targetWidth != freeRasterSnapshot.rasterWidth || request.targetHeight != freeRasterSnapshot.rasterHeight ->
                "full-resolution rastergeometrie wijkt af van binding"
            contractFailure != null -> contractFailure
            freeRasterSnapshot.viewportAuthority != "PRESENTATION_ONLY" ->
                "Free Raster viewport-authority mismatch"
            freeRasterSnapshot.artifactAuthority != DrawPhotoOutputCableV01.OUTPUT_AUTHORITY ->
                "full-resolution artifact-authority mismatch"
            else -> null
        }
        if (rejection != null) {
            clearCanvasPresentation(
                "Canvasstatus · FREE_RASTER_REJECTED · $rejection · fail-closed · Scientific Master ongewijzigd",
            )
            return
        }
        canvasStatusView.text =
            "Canvasstatus · FREE_RASTER_LOADING · ${freeRasterSnapshot.rasterWidth}×" +
                "${freeRasterSnapshot.rasterHeight} full-resolution sibling · PRESENTATION_ONLY"
        Thread({
            val decoded = UnifiedOutputFreeRasterDisplayLoaderV01.load(freeRasterSnapshot)
            runOnUiThread {
                val latest = UnifiedOutputFreeRasterBridge.acquire()
                val stale =
                    presentationMode != PresentationMode.INTERNAL_D_RAW ||
                    generation != presentationGeneration ||
                    currentSelectedRoute() != request.route ||
                    latest == null ||
                    latest.generation != freeRasterSnapshot.generation ||
                    latest.artifactSha256 != freeRasterSnapshot.artifactSha256
                if (stale) {
                    (decoded as? UnifiedOutputFreeRasterDisplayLoaderV01.Result.Ready)
                        ?.bitmap?.takeUnless { it.isRecycled }?.recycle()
                    return@runOnUiThread
                }
                when (decoded) {
                    is UnifiedOutputFreeRasterDisplayLoaderV01.Result.Failed ->
                        clearCanvasPresentation(
                            "Canvasstatus · FREE_RASTER_REJECTED · ${decoded.reason} · fail-closed · Scientific Master ongewijzigd",
                        )
                    is UnifiedOutputFreeRasterDisplayLoaderV01.Result.Ready -> {
                        presentationBitmap?.takeUnless { it.isRecycled }?.recycle()
                        presentationBitmap = decoded.bitmap
                        presentationUri = null
                        canvasImage.setImageBitmap(decoded.bitmap)
                        canvasPlaceholder.visibility = View.GONE
                        canvasStatusView.text =
                            "Canvasstatus · D.RAW_FREE_RASTER_FULL_RES · route=${request.route} · " +
                                "output ${decoded.sourceWidth}×${decoded.sourceHeight} px · " +
                                "display ${decoded.bitmap.width}×${decoded.bitmap.height} px · " +
                                "sample=${decoded.sampleSize}x · rotation=${request.userQuarterTurns * 90}° · " +
                                "artifactSHA=${freeRasterSnapshot.artifactSha256.take(16)}… · " +
                                "PRESENTATION_ONLY · createsNewEvidence=false · scientificWriteback=false"
                        canvasImage.post { fitCanvasImage(updateStatus = false) }
                    }
                }
            }
        }, "draw-free-raster-display-v03").start()
    }

    private fun orientUnifiedPresentationBitmap'''
    ws, count = pattern.subn(replacement, ws, count=1)
    if count != 1:
        raise SystemExit(f'Workspace consume method replacement count={count}; refusing')
ws = ws.replace(
    '"Canvasstatus · PREVIEW_RASTER_1_TO_1 · 1 display-pixel per decoded preview-pixel · " +',
    '"Canvasstatus · DISPLAY_RASTER_1_TO_1 · 1 display-pixel per decoded viewport-sample · " +',
    1,
)
ws = ws.replace(
    '"Canvasstatus · PREVIEW_RASTER_1_TO_1_PANNED · pan actief · " +',
    '"Canvasstatus · DISPLAY_RASTER_1_TO_1_PANNED · pan actief · " +',
    1,
)
ws_path.write_text(ws)
