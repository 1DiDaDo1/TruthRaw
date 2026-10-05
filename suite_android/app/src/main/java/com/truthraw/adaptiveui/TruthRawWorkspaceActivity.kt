package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
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
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import java.util.Locale
import kotlin.math.max
import kotlin.math.min

/**
 * D.RAW Workspace / Vrije Raster-weergave v0.1.
 *
 * Product/UI shell around the existing proven D.RAW routes. This activity does
 * not decode RAW/DNG, does not build a second Scientific Master and does not
 * infer authority. RAW/DNG stays routed through MainActivity / Universal
 * Intake. The local canvas can inspect only an already-rendered presentation
 * raster and its pan/zoom matrix is APPEARANCE/PRESENTATION_ONLY.
 *
 * Free-raster rule: representation may move/scale independently; evidence and
 * authority remain bound to the upstream observation.
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
    private lateinit var canvasImage: ImageView
    private lateinit var canvasPlaceholder: TextView
    private lateinit var canvasStatusView: TextView
    private lateinit var canvasTelemetryView: TextView

    private var presentationBitmap: Bitmap? = null
    private var presentationUri: Uri? = null
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
                    return true
                }
            },
        )

        setContentView(buildUi())

        getSharedPreferences(PREF_WORKSPACE, MODE_PRIVATE)
            .getString(KEY_PRESENTATION_URI, null)
            ?.takeIf { it.isNotBlank() }
            ?.let { saved ->
                canvasImage.post { loadPresentationRaster(Uri.parse(saved), restored = true) }
            }
    }

    override fun onResume() {
        super.onResume()
        DrawVisualTheme.applyWindow(this)
        updateRouteStatus()
    }

    override fun onDestroy() {
        presentationBitmap?.recycle()
        presentationBitmap = null
        super.onDestroy()
    }

    private fun buildUi(): ScrollView {
        val root = vertical().apply {
            setBackgroundColor(bg)
            setPadding(dp(18), dp(12), dp(18), dp(28))
        }

        root.addView(header())
        root.addView(space(12))
        root.addView(
            body(
                "One Free World. Many sealed observations. One evidence law. Deze Workspace brengt bron, route, vrije raster-weergave en authority-uitleg bij elkaar zonder een tweede scientific pipeline te maken.",
                12.5f,
            ),
        )

        root.addView(space(14))
        root.addView(card("Actieve route").apply {
            routeStatusView = title("", 14f)
            addView(routeStatusView)
            addView(space(7))
            addView(body(
                "PURE, ADVANCED en PRO delen dezelfde sealed source en Scientific Master. Routekeuze verandert alleen downstream gereedschap/view.",
                11.5f,
            ))
            addView(space(9))
            addView(routeAction("PURE · Scientific View", teal) {
                selectRoute(TruthRawSuiteLauncherActivity.OUTPUT_PURE)
            })
            addView(space(7))
            addView(routeAction("ADVANCED · Appearance / Restoration", orange) {
                selectRoute(TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED)
            })
            addView(space(7))
            addView(routeAction("PRO · Open Scene / Research", purple) {
                selectRoute(TruthRawSuiteLauncherActivity.OUTPUT_PRO)
            })
        })
        updateRouteStatus()

        root.addView(space(14))
        root.addView(card("Bron / Observation").apply {
            addView(body(
                "RAW/DNG wordt bewust niet in deze Activity gedecodeerd. Daardoor blijft er één Universele Ingang en één evidence-keten.",
                11.7f,
            ))
            addView(space(9))
            addView(action("Bestand · Open RAW / DNG in D.RAW werkbank", blue) {
                startActivity(
                    Intent(this@TruthRawWorkspaceActivity, MainActivity::class.java).apply {
                        flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                        putExtra(MainActivity.EXTRA_AUTO_OPEN_RAW_PICKER, true)
                    },
                )
            })
            addView(space(7))
            addView(action("Camera · Universele fysieke RAW", teal) {
                startActivity(
                    Intent(
                        this@TruthRawWorkspaceActivity,
                        UniversalPhysicalCaptureActivity::class.java,
                    ),
                )
            })
            addView(space(7))
            addView(action("Open bestaande D.RAW werkbank / actieve sessie", blue) {
                startActivity(
                    Intent(this@TruthRawWorkspaceActivity, MainActivity::class.java).apply {
                        flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                    },
                )
            })
        })

        root.addView(space(14))
        root.addView(card("Vrije Raster-weergave · v0.1").apply {
            addView(body(
                "Het canvas hieronder is een vrije VIEW-projectie: pan en zoom veranderen alleen de zichtbare rasterpositie. Voor veiligheid accepteert deze lokale canvaslaag alleen een reeds gerenderd presentatiebeeld; RAW/DNG hoort altijd via de D.RAW werkbank. Geen canvasbewerking creëert MEASURED pixels of schrijft terug naar Scientific Master.",
                11.7f,
            ))
            addView(space(10))
            addView(buildCanvas())
            addView(space(8))
            addView(horizontal().apply {
                addView(
                    smallAction("Fit", teal) { fitCanvasImage() },
                    LinearLayout.LayoutParams(0, dp(48), 1f).apply { marginEnd = dp(4) },
                )
                addView(
                    smallAction("Preview 1:1", blue) { setCanvasOneToOne() },
                    LinearLayout.LayoutParams(0, dp(48), 1f).apply { marginStart = dp(4); marginEnd = dp(4) },
                )
                addView(
                    smallAction("Reset", orange) { fitCanvasImage() },
                    LinearLayout.LayoutParams(0, dp(48), 1f).apply { marginStart = dp(4) },
                )
            })
            addView(space(8))
            addView(action("Open gerenderd presentatie-raster", teal) { launchPresentationRasterPicker() })
            addView(space(7))
            addView(action("Projectie / export uitvoeren in bestaande D.RAW werkbank", blue) {
                startActivity(
                    Intent(this@TruthRawWorkspaceActivity, MainActivity::class.java).apply {
                        flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                    },
                )
            })
            addView(space(7))
            addView(body(
                "v0.1 productiseert de vrije x/y-weergave zonder de bestaande full-resolution projectieruntime te dupliceren. De volgende runtime-bridge mag uitsluitend bestaande Unified Output / projectie-state consumeren.",
                10.8f,
            ))
        })

        root.addView(space(14))
        root.addView(card("Pipeline · authority-neutrale kaart").apply {
            addView(body(
                "Deze kaart is navigatie, geen meetresultaat. Alleen de brongebonden runtime mag een stap als werkelijk beschikbaar/bewezen markeren.",
                11.5f,
            ))
            addView(space(8))
            addView(pipelineRow("1 · SOURCE", "selectie / camera-ingress · runtime-bound"))
            addView(pipelineRow("2 · SEALED", "alleen seal/provenance-record kan dit bewijzen"))
            addView(pipelineRow("3 · SCIENTIFIC", "Scientific Master · nooit afgeleid uit UI-status"))
            addView(pipelineRow("4 · CONTINUOUS", "TruthNegative / reconstruction · authority blijft expliciet"))
            addView(pipelineRow("5 · FREE RASTER", "vrije view/projectie · creëert geen evidence"))
            addView(pipelineRow("6 · APPEARANCE", "downstream display/view · nooit scientific writeback"))
        })

        root.addView(space(14))
        root.addView(card("Evidence / Authority Inspector · betekenis").apply {
            addView(body(
                "Nog zonder actieve bronbinding toont v0.1 uitsluitend de betekenis van authority-klassen. Het canvas mag deze labels nooit zelf toekennen.",
                11.5f,
            ))
            addView(space(8))
            addView(authorityRow("MEASURED", "direct gemeten / sealed support"))
            addView(authorityRow("CALIBRATED_ESTIMATE", "gekalibreerde schatting met expliciete basis"))
            addView(authorityRow("RECONSTRUCTED", "afgeleid uit toegelaten support; niet gemeten"))
            addView(authorityRow("CENSORED", "bronverlies/clipping is aantoonbaar aanwezig"))
            addView(authorityRow("UNKNOWN", "onvoldoende bewijs; blijft zichtbaar/fail-closed"))
            addView(authorityRow("APPEARANCE", "presentatie/view; geen scientific authority"))
        })

        root.addView(space(14))
        root.addView(card("Werkbanken").apply {
            addView(action("Appearance / Restoration instellingen", orange) {
                startActivity(Intent(this@TruthRawWorkspaceActivity, TruthRawAdvancedActivity::class.java))
            })
            addView(space(7))
            addView(action("PRO · Open Scene / Light Transport", purple) {
                startActivity(Intent(this@TruthRawWorkspaceActivity, TruthRawProActivity::class.java))
            })
            addView(space(7))
            addView(action("Research & JSON · Observation Graph basis", blue) {
                startActivity(Intent(this@TruthRawWorkspaceActivity, TruthRawResearchHubActivity::class.java))
            })
            addView(space(7))
            addView(action("Wat is geïmplementeerd?", teal) {
                startActivity(Intent(this@TruthRawWorkspaceActivity, TruthRawImplementationGuideActivity::class.java))
            })
            addView(space(7))
            addView(action("Klassiek route-overzicht", blue) {
                startActivity(Intent(this@TruthRawWorkspaceActivity, TruthRawSuiteLauncherActivity::class.java))
            })
        })

        root.addView(space(16))
        root.addView(
            body(
                "Workspace v0.1 · UI/presentation layer. No new evidence · no candidate promotion · no Scientific Master writeback · no AI/ML scientific inference.",
                10.5f,
            ).apply { gravity = Gravity.CENTER },
        )
        root.addView(space(10))
        root.addView(DrawVisualTheme.brandFooter(this, 82))

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

    private fun buildCanvas(): View {
        canvasImage = ImageView(this).apply {
            setBackgroundColor(DrawVisualTheme.INK)
            scaleType = ImageView.ScaleType.MATRIX
            imageMatrix = canvasMatrix
            contentDescription = "Vrije Raster presentatie-canvas"
            setOnTouchListener { view, event -> handleCanvasTouch(view, event) }
        }

        canvasPlaceholder = TextView(this).apply {
            text = "VRIJE RASTER\n\nGeen presentatie-raster geladen\n\nRAW / DNG → D.RAW werkbank"
            textSize = 15f
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

        canvasStatusView = body("Canvasstatus · EMPTY · authority niet afgeleid", 10.8f)
        canvasTelemetryView = body("View transform · geen raster", 10.3f)

        return vertical().apply {
            addView(
                canvas,
                LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    dp(430),
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
            MotionEvent.ACTION_MOVE -> {
                if (dragging && !scaleDetector.isInProgress && event.pointerCount == 1) {
                    val dx = event.x - lastTouchX
                    val dy = event.y - lastTouchY
                    canvasMatrix.postTranslate(dx, dy)
                    lastTouchX = event.x
                    lastTouchY = event.y
                    applyCanvasMatrix()
                }
            }
            MotionEvent.ACTION_UP,
            MotionEvent.ACTION_CANCEL -> {
                dragging = false
                view.parent?.requestDisallowInterceptTouchEvent(false)
            }
        }
        return true
    }

    private fun fitCanvasImage() {
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
            "Canvasstatus · PREVIEW_RASTER_1_TO_1 · 1 display-pixel per decoded preview-pixel · " +
                "source/scientific sampling ongewijzigd · PRESENTATION_ONLY"
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
            "View transform · preview %d×%d px · schaal %.3fx · x %.1f px · y %.1f px · PRESENTATION_ONLY",
            bitmap.width,
            bitmap.height,
            scale,
            x,
            y,
        )
    }

    private fun launchPresentationRasterPicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/*"
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

        runCatching {
            contentResolver.takePersistableUriPermission(
                uri,
                Intent.FLAG_GRANT_READ_URI_PERMISSION,
            )
        }

        if (looksLikeRawSource(uri)) {
            canvasStatusView.text =
                "RAW/DNG geweigerd door lokale canvasdecoder · gebruik D.RAW werkbank / Universal Intake"
            return
        }

        getSharedPreferences(PREF_WORKSPACE, MODE_PRIVATE)
            .edit()
            .putString(KEY_PRESENTATION_URI, uri.toString())
            .apply()
        loadPresentationRaster(uri, restored = false)
    }

    private fun loadPresentationRaster(uri: Uri, restored: Boolean) {
        canvasStatusView.text = if (restored) {
            "Presentatie-raster herstellen…"
        } else {
            "Presentatie-raster laden…"
        }
        Thread({
            val decoded = runCatching { decodePresentationBitmap(uri) }
            runOnUiThread {
                decoded.fold(
                    onSuccess = { bitmap ->
                        presentationBitmap?.recycle()
                        presentationBitmap = bitmap
                        presentationUri = uri
                        canvasImage.setImageBitmap(bitmap)
                        canvasPlaceholder.visibility = View.GONE
                        canvasStatusView.text =
                            "Canvasstatus · PRESENTATION_ONLY · " +
                                bitmap.width + "×" + bitmap.height +
                                " preview · bron-authority niet gewijzigd"
                        canvasImage.post { fitCanvasImage() }
                    },
                    onFailure = { error ->
                        canvasStatusView.text =
                            "Presentatie-raster kon niet worden gelezen: " +
                                (error.message ?: error.javaClass.simpleName)
                    },
                )
            }
        }, "draw-workspace-presentation-decode").start()
    }

    private fun decodePresentationBitmap(uri: Uri): Bitmap {
        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        contentResolver.openInputStream(uri)?.use { stream ->
            BitmapFactory.decodeStream(stream, null, bounds)
        } ?: error("Geen leesbare inputstream")

        require(bounds.outWidth > 0 && bounds.outHeight > 0) {
            "Geen ondersteund gerenderd raster"
        }

        var sample = 1
        val largest = max(bounds.outWidth, bounds.outHeight)
        while (largest / sample > MAX_PRESENTATION_EDGE) sample *= 2

        val options = BitmapFactory.Options().apply {
            inSampleSize = sample
            inPreferredConfig = Bitmap.Config.ARGB_8888
        }
        return contentResolver.openInputStream(uri)?.use { stream ->
            BitmapFactory.decodeStream(stream, null, options)
        } ?: error("Rasterdecode gaf geen bitmap")
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
    }

    private fun updateRouteStatus() {
        if (!::routeStatusView.isInitialized) return
        val route = getSharedPreferences(TruthRawSuiteLauncherActivity.PREFS, MODE_PRIVATE)
            .getString(
                TruthRawSuiteLauncherActivity.KEY_OUTPUT,
                TruthRawSuiteLauncherActivity.OUTPUT_PURE,
            ) ?: TruthRawSuiteLauncherActivity.OUTPUT_PURE
        routeStatusView.text = "Geselecteerd · " + when (route) {
            TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED -> "D.RAW ADVANCED"
            TruthRawSuiteLauncherActivity.OUTPUT_PRO -> "D.RAW PRO"
            else -> "D.RAW PURE"
        }
    }

    private fun header(): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        addView(
            vertical().apply {
                addView(title("D.RAW Workspace", 27f))
                addView(body("Vrije Raster-weergave · evidence-bound UI v0.1", 11.5f))
            },
            LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f),
        )
        addView(
            TextView(this@TruthRawWorkspaceActivity).apply {
                text = "⚙"
                textSize = 26f
                gravity = Gravity.CENTER
                setTextColor(ink)
                background = rounded(DrawVisualTheme.PAPER_WHITE, DrawVisualTheme.PENCIL_YELLOW, 14f)
                contentDescription = "Instellingen"
                setOnClickListener {
                    startActivity(Intent(this@TruthRawWorkspaceActivity, TruthRawSettingsActivity::class.java))
                }
            },
            LinearLayout.LayoutParams(dp(48), dp(48)),
        )
    }

    private fun pipelineRow(label: String, detail: String): View = vertical().apply {
        setPadding(dp(11), dp(9), dp(11), dp(9))
        background = rounded(DrawVisualTheme.PAPER_BLUE, DrawVisualTheme.BORDER, 12f)
        addView(title(label, 12.8f))
        addView(body(detail, 10.7f))
        addView(space(5))
    }

    private fun authorityRow(label: String, detail: String): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        setPadding(dp(10), dp(8), dp(10), dp(8))
        addView(
            title(label, 11.5f),
            LinearLayout.LayoutParams(dp(138), ViewGroup.LayoutParams.WRAP_CONTENT),
        )
        addView(
            body(detail, 10.7f),
            LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f),
        )
    }

    private fun card(heading: String): LinearLayout = vertical().apply {
        setPadding(dp(15), dp(15), dp(15), dp(15))
        background = rounded(surface, DrawVisualTheme.BORDER, 18f)
        addView(title(heading, 17f))
        addView(space(7))
    }

    private fun action(label: String, accent: Int, onClick: () -> Unit): View =
        TextView(this).apply {
            text = label
            textSize = 14f
            setTextColor(ink)
            gravity = Gravity.CENTER
            setPadding(dp(13), dp(13), dp(13), dp(13))
            background = rounded(surface, accent, 14f)
            setOnClickListener { onClick() }
        }

    private fun smallAction(label: String, accent: Int, onClick: () -> Unit): View =
        TextView(this).apply {
            text = label
            textSize = 13f
            setTextColor(ink)
            gravity = Gravity.CENTER
            background = rounded(surface, accent, 12f)
            setOnClickListener { onClick() }
        }

    private fun routeAction(label: String, accent: Int, onClick: () -> Unit): View =
        action(label, accent, onClick)

    private fun rounded(fill: Int, stroke: Int, radius: Float): GradientDrawable =
        GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = radius * resources.displayMetrics.density
            setColor(fill)
            setStroke(dp(1), stroke)
        }

    private fun vertical() = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
    private fun horizontal() = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
    private fun space(height: Int) = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
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
        private const val MAX_PRESENTATION_EDGE = 2048

        private val RAW_EXTENSIONS = listOf(
            ".dng", ".raw", ".nef", ".cr3", ".cr2", ".arw", ".raf", ".rw2",
            ".orf", ".pef", ".rwl", ".3fr", ".fff", ".iiq",
        )
    }
}