from pathlib import Path

main_path = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/MainActivity.kt')
main = main_path.read_text()

# Free Raster v0.3 is now integrated in the branch. Historically this helper
# applied the staging block to an older MainActivity shape. Later output work
# added the explicit presentationHeadroomMode argument, so re-applying that old
# text patch is neither necessary nor safe. Treat the integrated current cable
# as the deterministic target and fail closed if any required binding vanishes.
required_main_markers = (
    'UnifiedOutputFreeRasterRuntimeV01.stageRenderedJpeg(',
    'presentationHeadroomMode = binding.presentationHeadroomMode',
    '"jpeg_destination_commit_failed"',
    '"saved_jpeg_preview_failed"',
    '"active_job_changed_before_free_raster_promotion"',
)
missing_main_markers = [marker for marker in required_main_markers if marker not in main]
if missing_main_markers:
    raise SystemExit(
        'MainActivity integrated Free Raster v0.3 contract missing: ' +
        ', '.join(missing_main_markers)
    )

# 1:1 in the current Android viewport means one display pixel per decoded
# viewport sample. Keep this normalization idempotent for historical branches
# while accepting the already-integrated wording on the current branch.
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
if 'DISPLAY_RASTER_1_TO_1' not in ws:
    raise SystemExit('Workspace DISPLAY_RASTER_1_TO_1 contract missing')
ws_path.write_text(ws)

print('FREE_RASTER_V03_INTEGRATED_CONTRACT_PASS')
