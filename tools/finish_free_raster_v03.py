from pathlib import Path

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
                val rendered = FullResJpegExporter.renderToPrivateJpeg(
                    contentResolver, job, flags, quarterTurns, dir,
                )
                var jpegOutputPreview: UnifiedOutputPreviewResult.Ready? = null
                var status = when (rendered) {
                    is FullResJpegResult.Failed -> rendered.reason
                    is FullResJpegResult.Success -> {
                        val m = rendered.metrics
                        val freeRasterStage =
                            UnifiedOutputFreeRasterRuntimeV01.stageRenderedJpeg(
                                binding = binding,
                                renderedFile = rendered.file,
                                outputWidth = m.width,
                                outputHeight = m.height,
                                jpegSha256 = m.jpegSha256,
                            )
                        val freeRasterStageStatus = when (freeRasterStage) {
                            is UnifiedOutputFreeRasterRuntimeV01.StageResult.Ready ->
                                "Free Raster full-res sibling staged · promotion wacht op commit/current-output bevestiging"
                            is UnifiedOutputFreeRasterRuntimeV01.StageResult.Failed ->
                                "Free Raster staging geblokkeerd: ${freeRasterStage.reason}"
                        }
                        val ok = FullResJpegExporter.commit(
                            contentResolver,
                            rendered.file,
                            destination,
                            m.jpegSha256,
                        )
                        rendered.file.delete()
                        if (!ok) {
                            UnifiedOutputFreeRasterRuntimeV01.discardStaged(
                                expectedJob,
                                "jpeg_destination_commit_failed",
                            )
                            runCatching { contentResolver.delete(destination, null, null) }
                            "JPG commit/post-write SHA-verify faalde · $freeRasterStageStatus"
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
                                is UnifiedOutputPreviewResult.Failed -> {
                                    UnifiedOutputFreeRasterRuntimeV01.discardStaged(
                                        expectedJob,
                                        "saved_jpeg_preview_failed",
                                    )
                                    jpegStatus =
                                        "JPG opgeslagen; uitkomst-preview faalde: " +
                                            preview.reason
                                }
                            }
                            "JPG full-resolution gereed · ${m.width}×${m.height} · " +
                                "${formatBytes(m.jpegBytes)} · route=$route · detail=${m.detailApplied} · " +
                                "Light pixels=${m.lightAdjustedPixels} · Scientific Master/Backplane=${m.scientificMasterBound}/${m.backplaneBound} · " +
                                "rotatie=${quarterTurns * 90}° · HDR-front=${m.hdrBakedIntoFront} (APPEARANCE_ONLY) · " +
                                "Restoration-front=${m.restorationBakedIntoFront} (AESTHETIC_REINTEGRATION_ONLY) · " +
                                freeRasterStageStatus
                        }
                    }
                }
'''
if new in main:
    pass
elif old in main:
    main = main.replace(old, new, 1)
else:
    raise SystemExit('MainActivity target block not found; refusing non-deterministic patch')

# If the source/job changed between worker completion and UI publication, no
# saved-JPEG preview is published and therefore the staged artifact is not
# promotable. Discard it explicitly instead of waiting for timeout cleanup.
old_ui_else = '''                    } else {
                        jpegOutputPreview?.bitmap?.recycle()
                    }
'''
new_ui_else = '''                    } else {
                        UnifiedOutputFreeRasterRuntimeV01.discardStaged(
                            expectedJob,
                            "active_job_changed_before_free_raster_promotion",
                        )
                        jpegOutputPreview?.bitmap?.recycle()
                    }
'''
if new_ui_else in main:
    pass
elif old_ui_else in main:
    main = main.replace(old_ui_else, new_ui_else, 1)
else:
    raise SystemExit('MainActivity UI result block not found; refusing non-deterministic patch')
main_path.write_text(main)

# 1:1 in the current Android viewport means one display pixel per decoded
# viewport sample. A sampled display Bitmap must never be labelled as one
# full-resolution output-raster pixel per display pixel.
ws_path = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/TruthRawWorkspaceActivity.kt')
ws = ws_path.read_text()
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
