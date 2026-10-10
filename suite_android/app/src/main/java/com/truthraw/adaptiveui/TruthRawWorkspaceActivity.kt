package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.Matrix
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import android.view.Gravity
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.FrameLayout
import android.widget.HorizontalScrollView
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import java.util.Locale
import kotlin.math.abs
import kotlin.math.min

/**
 * D.RAW Workspace / Output / Vrije Raster v0.3 vision shell.
 *
 * This Activity is intentionally downstream from the sealed source and Scientific Master.
 * It reorganises the product UI around one central image/workspace and one shared
 * PURE / ADVANCED / PRO state. It does not decode RAW/DNG, create scientific evidence,
 * mutate sealed source data or write Appearance/View state back to Scientific Master.
 *
 * Non-destructive rule:
 * read-only source + reversible workbench/view/output state -> live presentation / new export.
 */
class TruthRawWorkspaceActivity : Activity() {
    private val bg = DrawVisualTheme.PAPER_YELLOW
    private val surface = DrawVisualTheme.PAPER_WHITE
    private val ink = DrawVisualTheme.INK
    private val muted = DrawVisualTheme.MUTED
    private val blue = DrawVisualTheme.BLUE
    private val teal = DrawVisualTheme.TEAL
    private val orange = DrawVisualTheme.ORANGE
    private val purple = DrawVisualTheme.PURPLE

    private lateinit var routeStatusView: TextView
    private lateinit var pureRouteTab: TextView
    private lateinit var advancedRouteTab: TextView
    private lateinit var proRouteTab: TextView

    private lateinit var canvasImage: ImageView
    private lateinit var canvasPlaceholder: TextView
    private lateinit var canvasStatusView: TextView
    private lateinit var canvasTelemetryView: TextView

    private enum class PresentationMode {
        INTERNAL_D_RAW,
        EXTERNAL_RASTER,
    }

    private var presentationMode = PresentationMode.INTERNAL_D_RAW
    private var presentationBitmap: Bitmap? = null
    private var presentationUri: Uri? = null
    private var presentationGeneration: Long = 0L
    private val canvasMatrix = Matrix()
    private var fitScale = 1f
    private var viewZoom = 1f
    private var lastTouchX = 0f
    private var lastTouchY = 0f
    private var dragging = false

    private lateinit var scaleDetector: ScaleGestureDetector

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        DrawVisualTheme.applyWindow(this)

        scaleDetector = ScaleGestureDetector(
            this,
            object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
                override fun onScale(detector: ScaleGestureDetector): Boolean {
                    if (presentationBitmap == null) return false
                    val desired = (viewZoom * detector.scaleFactor).coerceIn(0.25f, 8f)
                    val incremental = desired / viewZoom
                    canvasMatrix.postScale(
                        incremental,
                        incremental,
                        detector.focusX,
                        detector.focusY,
                    )
                    viewZoom = desired
                    applyCanvasMatrix()
                    markManualCanvasTransform()
                    return true
                }
            },
        )

        setContentView(buildUi())

        val prefs = getSharedPreferences(PREF_WORKSPACE, MODE_PRIVATE)
        val savedUri = prefs.getString(KEY_PRESENTATION_URI, null)?.takeIf { it.isNotBlank() }
        presentationMode = when (prefs.getString(KEY_PRESENTATION_MODE, null)) {
            MODE_EXTERNAL -> PresentationMode.EXTERNAL_RASTER
            MODE_INTERNAL -> PresentationMode.INTERNAL_D_RAW
            else -> if (savedUri != null) {
                PresentationMode.EXTERNAL_RASTER
            } else {
                PresentationMode.INTERNAL_D_RAW
            }
        }

        if (presentationMode == PresentationMode.EXTERNAL_RASTER && savedUri != null) {
            canvasImage.post {
                loadPresentationRaster(Uri.parse(savedUri), restored = true)
            }
        }
    }

    override fun onResume() {
        super.onResume()
        DrawVisualTheme.applyWindow(this)
        updateRouteStatus()
        if (
            ::canvasImage.isInitialized &&
            presentationMode == PresentationMode.INTERNAL_D_RAW
        ) {
            canvasImage.post { consumeUnifiedOutputPresentation() }
        }
    }

    override fun onDestroy() {
        ++presentationGeneration
        presentationBitmap?.takeUnless { it.isRecycled }?.recycle()
        presentationBitmap = null
        super.onDestroy()
    }

    private fun buildUi(): ScrollView {
        val root = vertical().apply {
            setBackgroundColor(bg)
            setPadding(dp(16), dp(10), dp(16), dp(28))
        }

        root.addView(header())
        root.addView(space(8))
        root.addView(
            body(
                "One Free World. Many sealed observations. One evidence law. " +
                    "Eén centrale werkruimte, één bronbinding en één gedeelde outputkabel.",
                11.8f,
            ),
        )

        root.addView(space(12))
        root.addView(buildRouteStrip())

        root.addView(space(10))
        root.addView(buildSourceStrip())

        root.addView(space(12))
        root.addView(buildVisionWorkspace())

        root.addView(space(12))
        root.addView(buildInspectorDeck())

        root.addView(space(12))
        root.addView(buildWorkbenchDock())

        root.addView(space(14))
        root.addView(
            body(
                "Workspace v0.3 vision shell · bron read-only · Scientific Master immutable · " +
                    "edits reversibel · Free Raster/Appearance/View downstream · export maakt een nieuw derivaat · " +
                    "geen nieuwe evidence · geen scientific writeback.",
                10.4f,
            ).apply { gravity = Gravity.CENTER },
        )
        root.addView(space(10))
        root.addView(DrawVisualTheme.brandFooter(this, 82))

        updateRouteStatus()

        return ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(bg)
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
            addView(root)
        }
    }

    private fun buildRouteStrip(): View = card("PURE / ADVANCED / PRO · één upstream kern").apply {
        routeStatusView = title("", 13.5f)
        addView(routeStatusView)
        addView(space(7))
        addView(
            body(
                "De route verandert downstream gereedschap en view-state; sealed source, " +
                    "Scientific Master en authority worden niet herschreven.",
                10.8f,
            ),
        )
        addView(space(9))

        addView(horizontal().apply {
            pureRouteTab = routeTab("PURE", teal) {
                selectRoute(TruthRawSuiteLauncherActivity.OUTPUT_PURE)
            }
            advancedRouteTab = routeTab("ADVANCED", orange) {
                selectRoute(TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED)
            }
            proRouteTab = routeTab("PRO", purple) {
                selectRoute(TruthRawSuiteLauncherActivity.OUTPUT_PRO)
            }

            addView(
                pureRouteTab,
                LinearLayout.LayoutParams(0, dp(52), 1f).apply { marginEnd = dp(4) },
            )
            addView(
                advancedRouteTab,
                LinearLayout.LayoutParams(0, dp(52), 1f).apply {
                    marginStart = dp(4)
                    marginEnd = dp(4)
                },
            )
            addView(
                proRouteTab,
                LinearLayout.LayoutParams(0, dp(52), 1f).apply { marginStart = dp(4) },
            )
        })
    }

    private fun buildSourceStrip(): View = card("Bron / Observation · universele ingang").apply {
        addView(
            body(
                "RAW/DNG blijft via Universal Intake. Externe JPG/PNG/WebP blijft read-only PRESENTATION_ONLY.",
                10.8f,
            ),
        )
        addView(space(8))
        addView(horizontal().apply {
            addView(
                smallAction("Bestand", blue) {
                    enterInternalPresentationMode()
                    startActivity(
                        Intent(this@TruthRawWorkspaceActivity, MainActivity::class.java).apply {
                            flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                            putExtra(MainActivity.EXTRA_AUTO_OPEN_RAW_PICKER, true)
                        },
                    )
                },
                LinearLayout.LayoutParams(0, dp(48), 1f).apply { marginEnd = dp(4) },
            )
            addView(
                smallAction("Camera", teal) {
                    enterInternalPresentationMode()
                    startActivity(
                        Intent(
                            this@TruthRawWorkspaceActivity,
                            UniversalPhysicalCaptureActivity::class.java,
                        ),
                    )
                },
                LinearLayout.LayoutParams(0, dp(48), 1f).apply {
                    marginStart = dp(4)
                    marginEnd = dp(4)
                },
            )
            addView(
                smallAction("Sessie", blue) {
                    enterInternalPresentationMode()
                    startActivity(
                        Intent(this@TruthRawWorkspaceActivity, MainActivity::class.java).apply {
                            flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                        },
                    )
                },
                LinearLayout.LayoutParams(0, dp(48), 1f).apply { marginStart = dp(4) },
            )
        })
    }

    private fun buildVisionWorkspace(): View = card("Centrale Werkruimte · Unified Output").apply {
        addView(
            body(
                "De afbeelding staat centraal. Evidence/Authority, Pipeline, Atlas/Observation Graph, " +
                    "Appearance, Vrije Raster en Export zijn gereedschappen rond dezelfde actieve output-state.",
                11.2f,
            ),
        )
        addView(space(9))
        addView(buildWorkspaceRail())
        addView(space(9))
        addView(buildCanvas())
        addView(space(8))

        addView(horizontal().apply {
            addView(
                smallAction("Fit", teal) { fitCanvasImage() },
                LinearLayout.LayoutParams(0, dp(46), 1f).apply { marginEnd = dp(4) },
            )
            addView(
                smallAction("Preview 1:1", blue) { setCanvasOneToOne() },
                LinearLayout.LayoutParams(0, dp(46), 1f).apply {
                    marginStart = dp(4)
                    marginEnd = dp(4)
                },
            )
            addView(
                smallAction("Reset view", orange) { fitCanvasImage() },
                LinearLayout.LayoutParams(0, dp(46), 1f).apply { marginStart = dp(4) },
            )
        })

        addView(space(8))
        addView(horizontal().apply {
            addView(
                smallAction("Extern raster", teal) { launchPresentationRasterPicker() },
                LinearLayout.LayoutParams(0, dp(48), 1f).apply { marginEnd = dp(4) },
            )
            addView(
                smallAction("Output / Export", blue) {
                    enterInternalPresentationMode()
                    startActivity(
                        Intent(this@TruthRawWorkspaceActivity, MainActivity::class.java).apply {
                            flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                        },
                    )
                },
                LinearLayout.LayoutParams(0, dp(48), 1f).apply { marginStart = dp(4) },
            )
        })

        addView(space(8))
        addView(
            body(
                "Fit/pan/zoom/Preview 1:1 zijn alleen VIEW-state. Crop, rotatie, resolutie en Appearance " +
                    "horen als reversibele edit/output-state bovenop dezelfde bron; nooit als nieuwe bronpixels.",
                10.5f,
            ),
        )
    }

    private fun buildWorkspaceRail(): View {
        val rail = horizontal().apply {
            gravity = Gravity.CENTER_VERTICAL
            addView(toolLabel("EVIDENCE", teal))
            addView(toolLabel("PIPELINE", blue))
            addView(toolLabel("ATLAS / GRAPH", purple))
            addView(toolLabel("APPEARANCE", orange))
            addView(toolLabel("FREE RASTER", teal))
            addView(toolLabel("EXPORT", blue))
        }
        return HorizontalScrollView(this).apply {
            isHorizontalScrollBarEnabled = false
            addView(rail)
        }
    }

    private fun buildInspectorDeck(): View = vertical().apply {
        addView(card("Evidence / Authority · brongebonden").apply {
            addView(
                body(
                    "UI-status kent nooit zelf authority toe. Zonder runtimebinding blijft bewijs UNKNOWN/fail-closed.",
                    10.7f,
                ),
            )
            addView(space(7))
            addView(authorityRow("MEASURED", "direct gemeten / sealed support"))
            addView(authorityRow("CALIBRATED_ESTIMATE", "gekalibreerde schatting"))
            addView(authorityRow("RECONSTRUCTED", "afgeleid; niet gemeten"))
            addView(authorityRow("CENSORED", "aantoonbaar bronverlies/clipping"))
            addView(authorityRow("UNKNOWN", "onvoldoende bewijs; zichtbaar"))
            addView(authorityRow("APPEARANCE", "view/presentatie; geen scientific authority"))
        })

        addView(space(10))
        addView(card("Pipeline · authority-neutrale kaart").apply {
            addView(pipelineRow("1 · SOURCE", "selectie / camera-ingress"))
            addView(pipelineRow("2 · SEALED", "provenance / immutable observation"))
            addView(pipelineRow("3 · SCIENTIFIC", "Scientific Master"))
            addView(pipelineRow("4 · CONTINUOUS", "reconstruction / TruthNegative"))
            addView(pipelineRow("5 · FREE RASTER", "downstream projectie/output"))
            addView(pipelineRow("6 · APPEARANCE", "view/display; geen writeback"))
        })
    }

    private fun buildWorkbenchDock(): View = card("Werkbank · gereedschappen rond dezelfde afbeelding").apply {
        addView(action("Appearance / Restoration", orange) {
            startActivity(Intent(this@TruthRawWorkspaceActivity, TruthRawAdvancedActivity::class.java))
        })
        addView(space(7))
        addView(action("PRO · Open Scene / Light Transport", purple) {
            startActivity(Intent(this@TruthRawWorkspaceActivity, TruthRawProActivity::class.java))
        })
        addView(space(7))
        addView(action("Atlas / Observation Graph / Research", blue) {
            startActivity(Intent(this@TruthRawWorkspaceActivity, TruthRawResearchHubActivity::class.java))
        })
        addView(space(7))
        addView(action("Output / Vrije Raster / Export", teal) {
            enterInternalPresentationMode()
            startActivity(
                Intent(this@TruthRawWorkspaceActivity, MainActivity::class.java).apply {
                    flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                },
            )
        })
        addView(space(7))
        addView(action("Implementatiestatus", blue) {
            startActivity(
                Intent(
                    this@TruthRawWorkspaceActivity,
                    TruthRawImplementationGuideActivity::class.java,
                ),
            )
        })
    }

    private fun buildCanvas(): View {
        canvasImage = ImageView(this).apply {
            setBackgroundColor(DrawVisualTheme.INK)
            scaleType = ImageView.ScaleType.MATRIX
            imageMatrix = canvasMatrix
            contentDescription = "Centrale D.RAW Output / Vrije Raster werkruimte"
            setOnTouchListener { view, event -> handleCanvasTouch(view, event) }
        }

        canvasPlaceholder = TextView(this).apply {
            text =
                "D.RAW CENTRALE WERKRUIMTE\n\nGeen gebonden output-raster\n\n" +
                    "PURE / ADVANCED / PRO delen dezelfde bron\n" +
                    "Extern JPG / PNG / WebP = PRESENTATION_ONLY"
            textSize = 14.5f
            setTextColor(DrawVisualTheme.PAPER_WHITE)
            gravity = Gravity.CENTER
            setPadding(dp(20), dp(20), dp(20), dp(20))
            isClickable = false
            isFocusable = false
        }

        val canvas = FrameLayout(this).apply {
            clipChildren = true
            clipToPadding = true
            background = rounded(DrawVisualTheme.INK, DrawVisualTheme.BORDER, 16f)
            addView(
                canvasImage,
                FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT,
                ),
            )
            addView(
                canvasPlaceholder,
                FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT,
                ),
            )
        }

        canvasStatusView = body(
            "Canvasstatus · EMPTY · geen output-raster · authority niet afgeleid",
            10.6f,
        )
        canvasTelemetryView = body("View transform · geen raster", 10.1f)

        return vertical().apply {
            addView(
                canvas,
                LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    dp(500),
                ),
            )
            addView(space(7))
            addView(canvasStatusView)
            addView(space(3))
            addView(canvasTelemetryView)
        }
    }

    private fun handleCanvasTouch(view: View, event: MotionEvent): Boolean {
        if (presentationBitmap == null) return false
        view.parent?.requestDisallowInterceptTouchEvent(true)
        scaleDetector.onTouchEvent(event)

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                lastTouchX = event.x
                lastTouchY = event.y
                dragging = true
            }
            MotionEvent.ACTION_POINTER_DOWN -> {
                dragging = false
            }
            MotionEvent.ACTION_MOVE -> {
                if (dragging && !scaleDetector.isInProgress && event.pointerCount == 1) {
                    val dx = event.x - lastTouchX
                    val dy = event.y - lastTouchY
                    canvasMatrix.postTranslate(dx, dy)
                    lastTouchX = event.x
                    lastTouchY = event.y
                    applyCanvasMatrix()
                    markManualCanvasTransform()
                }
            }
            MotionEvent.ACTION_POINTER_UP -> {
                val remainingIndex = if (event.actionIndex == 0) 1 else 0
                if (remainingIndex < event.pointerCount) {
                    lastTouchX = event.getX(remainingIndex)
                    lastTouchY = event.getY(remainingIndex)
                }
                dragging = event.pointerCount - 1 == 1
            }
            MotionEvent.ACTION_UP,
            MotionEvent.ACTION_CANCEL -> {
                dragging = false
                view.parent?.requestDisallowInterceptTouchEvent(false)
            }
        }
        return true
    }

    private fun fitCanvasImage(updateStatus: Boolean = true) {
        val bitmap = presentationBitmap ?: return
        val width = canvasImage.width.toFloat()
        val height = canvasImage.height.toFloat()
        if (width <= 0f || height <= 0f || bitmap.width <= 0 || bitmap.height <= 0) return

        fitScale = min(width / bitmap.width.toFloat(), height / bitmap.height.toFloat())
            .coerceAtLeast(0.0001f)
        viewZoom = 1f
        canvasMatrix.reset()
        canvasMatrix.postScale(fitScale, fitScale)
        canvasMatrix.postTranslate(
            (width - bitmap.width * fitScale) / 2f,
            (height - bitmap.height * fitScale) / 2f,
        )
        applyCanvasMatrix()
        if (updateStatus) {
            canvasStatusView.text =
                "Canvasstatus · FIT_VIEW · presentation passend in centrale werkruimte · " +
                    "source/scientific sampling ongewijzigd"
        }
    }

    private fun setCanvasOneToOne() {
        val bitmap = presentationBitmap ?: return
        val width = canvasImage.width.toFloat()
        val height = canvasImage.height.toFloat()
        if (width <= 0f || height <= 0f) return

        canvasMatrix.reset()
        canvasMatrix.postTranslate(
            (width - bitmap.width.toFloat()) / 2f,
            (height - bitmap.height.toFloat()) / 2f,
        )
        viewZoom = if (fitScale > 0f) 1f / fitScale else 1f
        applyCanvasMatrix()
        canvasStatusView.text =
            "Canvasstatus · DISPLAY_RASTER_1_TO_1 · 1 display-pixel per decoded viewport-sample · " +
                "niet sensor/master 1:1 · PRESENTATION_ONLY"
    }

    private fun markManualCanvasTransform() {
        if (presentationBitmap == null) return
        val values = FloatArray(9)
        canvasMatrix.getValues(values)
        val scale = values[Matrix.MSCALE_X]
        canvasStatusView.text = if (abs(scale - 1f) <= 0.001f) {
            "Canvasstatus · DISPLAY_RASTER_1_TO_1_PANNED · pan actief · " +
                "scientific sampling ongewijzigd · PRESENTATION_ONLY"
        } else {
            String.format(
                Locale.US,
                "Canvasstatus · VIEW_TRANSFORM · handmatige pan/zoom · schaal %.3fx · PRESENTATION_ONLY",
                scale,
            )
        }
    }

    private fun applyCanvasMatrix() {
        canvasImage.imageMatrix = canvasMatrix
        updateCanvasTelemetry()
    }

    private fun updateCanvasTelemetry() {
        val bitmap = presentationBitmap
        if (bitmap == null) {
            canvasTelemetryView.text = "View transform · geen raster"
            return
        }
        val values = FloatArray(9)
        canvasMatrix.getValues(values)
        val scale = values[Matrix.MSCALE_X]
        val x = values[Matrix.MTRANS_X]
        val y = values[Matrix.MTRANS_Y]
        canvasTelemetryView.text = String.format(
            Locale.US,
            "View transform · presentation %d×%d px · schaal %.3fx · x %.1f px · y %.1f px",
            bitmap.width,
            bitmap.height,
            scale,
            x,
            y,
        )
    }

    private fun currentSelectedRoute(): String =
        getSharedPreferences(TruthRawSuiteLauncherActivity.PREFS, MODE_PRIVATE)
            .getString(
                TruthRawSuiteLauncherActivity.KEY_OUTPUT,
                TruthRawSuiteLauncherActivity.OUTPUT_PURE,
            ) ?: TruthRawSuiteLauncherActivity.OUTPUT_PURE

    private fun enterInternalPresentationMode() {
        presentationMode = PresentationMode.INTERNAL_D_RAW
        ++presentationGeneration
        getSharedPreferences(PREF_WORKSPACE, MODE_PRIVATE)
            .edit()
            .putString(KEY_PRESENTATION_MODE, MODE_INTERNAL)
            .apply()
        clearCanvasPresentation(
            "Canvasstatus · INTERNAL_D_RAW · wacht op gebonden Unified Output Ready-state · " +
                "authority niet afgeleid",
        )
    }

    private fun clearCanvasPresentation(status: String) {
        presentationBitmap?.takeUnless { it.isRecycled }?.recycle()
        presentationBitmap = null
        presentationUri = null
        canvasImage.setImageDrawable(null)
        canvasPlaceholder.visibility = View.VISIBLE
        canvasMatrix.reset()
        fitScale = 1f
        viewZoom = 1f
        canvasImage.imageMatrix = canvasMatrix
        canvasStatusView.text = status
        updateCanvasTelemetry()
    }

    private fun consumeUnifiedOutputPresentation() {
        if (presentationMode != PresentationMode.INTERNAL_D_RAW) return
        ++presentationGeneration

        val snapshot = UnifiedOutputPresentationBridge.acquire()
        if (snapshot == null) {
            clearCanvasPresentation(
                "Canvasstatus · EMPTY · geen D.RAW-output-raster beschikbaar · " +
                    "geen snapshot = geen afgeleide authority",
            )
            return
        }

        val metadata = snapshot.metadata
        val expectedRoute = currentSelectedRoute()
        val sourceJobId = metadata[UnifiedOutputPresentationBridge.META_SOURCE_JOB_ID].orEmpty()
        val sourceUri = metadata[UnifiedOutputPresentationBridge.META_SOURCE_URI].orEmpty()
        val outputLabel = metadata[UnifiedOutputPresentationBridge.META_OUTPUT_LABEL].orEmpty()
        val publishedRoute = metadata[UnifiedOutputPresentationBridge.META_ROUTE].orEmpty()
        val quarterTurns = metadata[UnifiedOutputPresentationBridge.META_DISPLAY_QUARTER_TURNS]
            ?.toIntOrNull()
        val expectedPreviewWidth = metadata[UnifiedOutputPresentationBridge.META_PREVIEW_WIDTH]
            ?.toIntOrNull()
        val expectedPreviewHeight = metadata[UnifiedOutputPresentationBridge.META_PREVIEW_HEIGHT]
            ?.toIntOrNull()
        val sourceWidth = metadata[UnifiedOutputPresentationBridge.META_SOURCE_WIDTH]
            ?.toIntOrNull()
        val sourceHeight = metadata[UnifiedOutputPresentationBridge.META_SOURCE_HEIGHT]
            ?.toIntOrNull()

        val rejection = when {
            snapshot.contractId != UnifiedOutputPresentationBridge.PRESENTATION_CONTRACT_ID ->
                "presentation contract mismatch"
            metadata[UnifiedOutputPresentationBridge.META_ORIGIN] !=
                UnifiedOutputPresentationBridge.ORIGIN_UNIFIED_OUTPUT_READY ->
                "origin is geen Unified Output Ready"
            metadata[UnifiedOutputPresentationBridge.META_SOURCE_BINDING_KIND] !=
                UnifiedOutputPresentationBridge.SOURCE_BINDING_ACTIVE_JOB ->
                "process-local sourcebinding ontbreekt"
            sourceJobId.isBlank() || sourceUri.isBlank() ->
                "actieve sourcebinding ontbreekt"
            outputLabel.isBlank() ->
                "outputlabel ontbreekt"
            publishedRoute != expectedRoute ->
                "snapshot-route $publishedRoute != actieve route $expectedRoute"
            metadata[UnifiedOutputPresentationBridge.META_SCIENTIFIC_WRITEBACK_ALLOWED] != "false" ->
                "scientific writeback is niet bewezen dicht"
            metadata[UnifiedOutputPresentationBridge.META_CREATES_NEW_EVIDENCE] != "false" ->
                "creates-new-evidence contract mismatch"
            metadata[UnifiedOutputPresentationBridge.META_PRESENTATION_LAYER] !=
                UnifiedOutputPresentationBridge.PRESENTATION_LAYER_VIEW_ONLY ->
                "snapshot is niet VIEW_ONLY_COPY"
            quarterTurns !in 0..3 ->
                "display orientation ontbreekt of is ongeldig"
            expectedPreviewWidth != snapshot.bitmap.width ||
                expectedPreviewHeight != snapshot.bitmap.height ->
                "previewdimensies komen niet overeen met snapshot"
            sourceWidth == null || sourceWidth <= 0 || sourceHeight == null || sourceHeight <= 0 ->
                "upstream sourcedimensies ontbreken"
            snapshot.bitmap.width <= 0 || snapshot.bitmap.height <= 0 ->
                "lege snapshotbitmap"
            else -> null
        }

        if (rejection != null) {
            snapshot.bitmap.takeUnless { it.isRecycled }?.recycle()
            clearCanvasPresentation(
                "Canvasstatus · UNIFIED_OUTPUT_REJECTED · $rejection · fail-closed · " +
                    "Scientific Master ongewijzigd",
            )
            return
        }

        val oriented = orientUnifiedPresentationBitmap(snapshot.bitmap, quarterTurns!!)
        if (oriented == null) {
            clearCanvasPresentation(
                "Canvasstatus · UNIFIED_OUTPUT_REJECTED · preview-oriëntatie kon niet veilig " +
                    "worden gekopieerd · fail-closed",
            )
            return
        }

        presentationBitmap?.takeUnless { it.isRecycled }?.recycle()
        presentationBitmap = oriented
        presentationUri = null
        canvasImage.setImageBitmap(oriented)
        canvasPlaceholder.visibility = View.GONE

        val sourceName = metadata[UnifiedOutputPresentationBridge.META_SOURCE_DISPLAY_NAME]
            ?.takeIf { it.isNotBlank() }
            ?: "onbekende bronnaam"
        val sourceSha = metadata[UnifiedOutputPresentationBridge.META_SOURCE_SHA256]
            ?.takeIf { it.isNotBlank() }
            ?: UnifiedOutputPresentationBridge.UNKNOWN_SOURCE_SHA256
        val shaText = if (sourceSha == UnifiedOutputPresentationBridge.UNKNOWN_SOURCE_SHA256) {
            "sourceSHA=UNKNOWN"
        } else {
            "sourceSHA=${sourceSha.take(16)}…"
        }

        canvasStatusView.text =
            "Canvasstatus · D.RAW_UNIFIED_OUTPUT_PRESENTATION · $outputLabel · route=$publishedRoute · " +
                "bron=$sourceName · source ${sourceWidth}×${sourceHeight} px · " +
                "preview ${oriented.width}×${oriented.height} px · rotation=${quarterTurns * 90}° · " +
                "$shaText · VIEW_ONLY_COPY · createsNewEvidence=false · scientificWriteback=false"
        canvasImage.post { fitCanvasImage(updateStatus = false) }
    }

    private fun orientUnifiedPresentationBitmap(
        source: Bitmap,
        quarterTurns: Int,
    ): Bitmap? {
        if (source.isRecycled) return null
        val turns = ((quarterTurns % 4) + 4) % 4
        if (turns == 0) return source

        val rotated = try {
            Bitmap.createBitmap(
                source,
                0,
                0,
                source.width,
                source.height,
                Matrix().apply { postRotate(turns * 90f) },
                false,
            )
        } catch (_: OutOfMemoryError) {
            null
        } catch (_: Exception) {
            null
        }

        if (rotated == null) {
            source.takeUnless { it.isRecycled }?.recycle()
            return null
        }
        if (rotated !== source) {
            source.takeUnless { it.isRecycled }?.recycle()
        }
        return rotated
    }

    private fun launchPresentationRasterPicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/*"
            putExtra(
                Intent.EXTRA_MIME_TYPES,
                arrayOf("image/jpeg", "image/png", "image/webp"),
            )
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        @Suppress("DEPRECATION")
        startActivityForResult(intent, REQUEST_PRESENTATION_RASTER)
    }

    @Deprecated("Legacy activity result retained for platform compatibility.")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != REQUEST_PRESENTATION_RASTER || resultCode != RESULT_OK) return
        val uri = data?.data ?: return

        if (looksLikeRawSource(uri)) {
            canvasStatusView.text =
                "RAW/DNG geweigerd door lokale canvasdecoder · gebruik D.RAW werkbank / Universal Intake"
            return
        }

        runCatching {
            contentResolver.takePersistableUriPermission(
                uri,
                data.flags and Intent.FLAG_GRANT_READ_URI_PERMISSION,
            )
        }

        presentationMode = PresentationMode.EXTERNAL_RASTER
        getSharedPreferences(PREF_WORKSPACE, MODE_PRIVATE)
            .edit()
            .putString(KEY_PRESENTATION_MODE, MODE_EXTERNAL)
            .putString(KEY_PRESENTATION_URI, uri.toString())
            .apply()
        loadPresentationRaster(uri, restored = false)
    }

    private fun loadPresentationRaster(uri: Uri, restored: Boolean) {
        presentationMode = PresentationMode.EXTERNAL_RASTER
        val generation = ++presentationGeneration
        clearCanvasPresentation(
            if (restored) {
                "Extern presentatie-raster herstellen… · read-only · PRESENTATION_ONLY"
            } else {
                "Extern presentatie-raster laden… · read-only · PRESENTATION_ONLY"
            },
        )

        Thread({
            val result = PresentationRasterLoader.load(this, uri)
            runOnUiThread {
                if (
                    generation != presentationGeneration ||
                    presentationMode != PresentationMode.EXTERNAL_RASTER
                ) {
                    (result as? PresentationRasterLoader.Result.Ready)
                        ?.bitmap
                        ?.takeUnless { it.isRecycled }
                        ?.recycle()
                    return@runOnUiThread
                }

                when (result) {
                    is PresentationRasterLoader.Result.Ready -> {
                        presentationBitmap?.takeUnless { it.isRecycled }?.recycle()
                        presentationBitmap = result.bitmap
                        presentationUri = uri
                        canvasImage.setImageBitmap(result.bitmap)
                        canvasPlaceholder.visibility = View.GONE
                        canvasStatusView.text =
                            "Canvasstatus · EXTERNAL_PRESENTATION_RASTER · read-only bron " +
                                result.sourceWidth + "×" + result.sourceHeight +
                                " px · preview " + result.bitmap.width + "×" + result.bitmap.height +
                                " px · sample " + result.sampleSize + "× · PRESENTATION_ONLY · " +
                                "geen source/scientific writeback"
                        canvasImage.post { fitCanvasImage(updateStatus = false) }
                    }
                    is PresentationRasterLoader.Result.Failure -> {
                        clearCanvasPresentation(
                            "Rasterfout · " + result.kind.name + " · " + result.detail +
                                " · bron en scientific state ongewijzigd",
                        )
                    }
                }
            }
        }, "draw-workspace-presentation-decode-v03").start()
    }

    private fun looksLikeRawSource(uri: Uri): Boolean {
        val name = runCatching {
            contentResolver.query(
                uri,
                arrayOf(OpenableColumns.DISPLAY_NAME),
                null,
                null,
                null,
            )?.use { cursor ->
                if (cursor.moveToFirst()) cursor.getString(0) else null
            }
        }.getOrNull().orEmpty().lowercase(Locale.ROOT)

        return RAW_EXTENSIONS.any { name.endsWith(it) }
    }

    private fun selectRoute(route: String) {
        getSharedPreferences(TruthRawSuiteLauncherActivity.PREFS, MODE_PRIVATE)
            .edit()
            .putString(TruthRawSuiteLauncherActivity.KEY_OUTPUT, route)
            .apply()
        updateRouteStatus()
        if (presentationMode == PresentationMode.INTERNAL_D_RAW) {
            consumeUnifiedOutputPresentation()
        }
    }

    private fun updateRouteStatus() {
        if (!::routeStatusView.isInitialized) return
        val route = currentSelectedRoute()
        routeStatusView.text = "Actief · " + when (route) {
            TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED -> "D.RAW ADVANCED · Appearance / Restoration"
            TruthRawSuiteLauncherActivity.OUTPUT_PRO -> "D.RAW PRO · Open Scene / Research"
            else -> "D.RAW PURE · Scientific View"
        }

        if (::pureRouteTab.isInitialized) {
            styleRouteTab(
                pureRouteTab,
                route == TruthRawSuiteLauncherActivity.OUTPUT_PURE,
                teal,
            )
        }
        if (::advancedRouteTab.isInitialized) {
            styleRouteTab(
                advancedRouteTab,
                route == TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED,
                orange,
            )
        }
        if (::proRouteTab.isInitialized) {
            styleRouteTab(
                proRouteTab,
                route == TruthRawSuiteLauncherActivity.OUTPUT_PRO,
                purple,
            )
        }
    }

    private fun header(): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        addView(
            vertical().apply {
                addView(title("D.RAW Workspace", 27f))
                addView(body("Vision UI · centrale afbeelding · non-destructive v0.3", 11.2f))
            },
            LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f),
        )
        addView(
            TextView(this@TruthRawWorkspaceActivity).apply {
                text = "⚙"
                textSize = 26f
                gravity = Gravity.CENTER
                setTextColor(ink)
                background = rounded(surface, DrawVisualTheme.PENCIL_YELLOW, 14f)
                contentDescription = "Instellingen"
                setOnClickListener {
                    startActivity(
                        Intent(
                            this@TruthRawWorkspaceActivity,
                            TruthRawSettingsActivity::class.java,
                        ),
                    )
                }
            },
            LinearLayout.LayoutParams(dp(48), dp(48)),
        )
    }

    private fun routeTab(
        label: String,
        accent: Int,
        onClick: () -> Unit,
    ): TextView = TextView(this).apply {
        text = label
        textSize = 12f
        setTypeface(typeface, Typeface.BOLD)
        gravity = Gravity.CENTER
        setTextColor(ink)
        setPadding(dp(8), 0, dp(8), 0)
        background = rounded(surface, accent, 14f)
        setOnClickListener { onClick() }
    }

    private fun styleRouteTab(view: TextView, selected: Boolean, accent: Int) {
        view.setTextColor(if (selected) DrawVisualTheme.PAPER_WHITE else ink)
        view.background = rounded(
            if (selected) accent else surface,
            accent,
            14f,
        )
        view.alpha = if (selected) 1f else 0.82f
    }

    private fun toolLabel(label: String, accent: Int): View =
        TextView(this).apply {
            text = label
            textSize = 10.5f
            setTypeface(typeface, Typeface.BOLD)
            setTextColor(ink)
            gravity = Gravity.CENTER
            setPadding(dp(13), dp(10), dp(13), dp(10))
            background = rounded(surface, accent, 14f)
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
            ).apply { marginEnd = dp(7) }
        }

    private fun pipelineRow(label: String, detail: String): View = vertical().apply {
        setPadding(dp(10), dp(7), dp(10), dp(7))
        background = rounded(surface, DrawVisualTheme.BORDER, 11f)
        addView(title(label, 11.2f))
        addView(space(2))
        addView(body(detail, 10.1f))
        layoutParams = LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
        ).apply { bottomMargin = dp(5) }
    }

    private fun authorityRow(label: String, detail: String): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        addView(
            TextView(this@TruthRawWorkspaceActivity).apply {
                text = label
                textSize = 10.2f
                setTypeface(typeface, Typeface.BOLD)
                setTextColor(ink)
                setPadding(dp(8), dp(6), dp(8), dp(6))
                background = rounded(surface, DrawVisualTheme.BORDER, 10f)
            },
            LinearLayout.LayoutParams(dp(150), ViewGroup.LayoutParams.WRAP_CONTENT),
        )
        addView(spaceHorizontal(8))
        addView(
            body(detail, 10.2f),
            LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f),
        )
    }

    private fun card(label: String): LinearLayout = vertical().apply {
        setPadding(dp(14), dp(13), dp(14), dp(14))
        background = rounded(surface, DrawVisualTheme.BORDER, 18f)
        addView(title(label, 15f))
        addView(space(8))
    }

    private fun action(label: String, accent: Int, onClick: () -> Unit): TextView =
        TextView(this).apply {
            text = label
            textSize = 12f
            setTypeface(typeface, Typeface.BOLD)
            setTextColor(ink)
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(14), 0, dp(14), 0)
            background = rounded(surface, accent, 14f)
            setOnClickListener { onClick() }
            minHeight = dp(50)
        }

    private fun smallAction(label: String, accent: Int, onClick: () -> Unit): TextView =
        TextView(this).apply {
            text = label
            textSize = 10.7f
            setTypeface(typeface, Typeface.BOLD)
            setTextColor(ink)
            gravity = Gravity.CENTER
            setPadding(dp(8), 0, dp(8), 0)
            background = rounded(surface, accent, 12f)
            setOnClickListener { onClick() }
        }

    private fun rounded(fill: Int, stroke: Int, radius: Float): GradientDrawable =
        GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(radius.toInt()).toFloat()
            setColor(fill)
            setStroke(dp(1), stroke)
        }

    private fun vertical() = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
    private fun horizontal() = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }

    private fun space(height: Int) = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
    }

    private fun spaceHorizontal(width: Int) = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(dp(width), 1)
    }

    private fun title(value: String, size: Float) = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(ink)
        setTypeface(typeface, Typeface.BOLD)
    }

    private fun body(value: String, size: Float) = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(muted)
        setLineSpacing(0f, 1.12f)
    }

    private fun dp(value: Int): Int =
        (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        private const val REQUEST_PRESENTATION_RASTER = 4810
        private const val PREF_WORKSPACE = "draw_workspace_v0_1"
        private const val KEY_PRESENTATION_URI = "presentation_uri"
        private const val KEY_PRESENTATION_MODE = "presentation_mode"
        private const val MODE_INTERNAL = "INTERNAL_D_RAW"
        private const val MODE_EXTERNAL = "EXTERNAL_RASTER"

        private val RAW_EXTENSIONS = listOf(
            ".dng", ".raw", ".nef", ".cr3", ".cr2", ".arw", ".raf", ".rw2",
            ".orf", ".pef", ".rwl", ".3fr", ".fff", ".iiq",
        )
    }
}
