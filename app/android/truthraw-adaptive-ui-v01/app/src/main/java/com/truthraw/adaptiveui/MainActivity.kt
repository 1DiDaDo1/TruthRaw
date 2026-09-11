package com.truthraw.adaptiveui

import android.app.Activity
import android.content.ClipData
import android.content.Intent
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.Space
import android.widget.TextView

class MainActivity : Activity() {
    private var session = BatchSession()

    private enum class LayoutTier { COMPACT, MEDIUM, EXPANDED }

    private data class Palette(
        val background: Int,
        val surface: Int,
        val surfaceAlt: Int,
        val text: Int,
        val textMuted: Int,
        val accent: Int,
    )

    private val palette: Palette
        get() {
            val dark = resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK ==
                Configuration.UI_MODE_NIGHT_YES
            return if (dark) {
                Palette(
                    background = Color.rgb(15, 17, 20),
                    surface = Color.rgb(27, 30, 35),
                    surfaceAlt = Color.rgb(36, 40, 46),
                    text = Color.rgb(245, 246, 248),
                    textMuted = Color.rgb(172, 178, 187),
                    accent = Color.rgb(128, 157, 255),
                )
            } else {
                Palette(
                    background = Color.rgb(246, 247, 249),
                    surface = Color.WHITE,
                    surfaceAlt = Color.rgb(235, 238, 243),
                    text = Color.rgb(24, 27, 31),
                    textMuted = Color.rgb(92, 99, 109),
                    accent = Color.rgb(54, 88, 200),
                )
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        render()
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        render()
    }

    @Suppress("DEPRECATION")
    private fun launchRawPicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
            putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(intent, REQUEST_OPEN_RAW)
    }

    @Deprecated("Platform result bridge is intentionally dependency-light in this v0.1 prototype")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != REQUEST_OPEN_RAW || resultCode != RESULT_OK || data == null) return

        val uris = buildList {
            data.data?.let(::add)
            val clip: ClipData? = data.clipData
            if (clip != null) {
                for (index in 0 until clip.itemCount) add(clip.getItemAt(index).uri)
            }
        }

        val jobs = RawIngress.readHandlesOnly(contentResolver, uris, data.flags)
        session = session.withJobs(jobs)
        render()
    }

    private fun render() {
        val tier = currentLayoutTier()
        val root = vertical().apply {
            setBackgroundColor(palette.background)
            setPadding(dp(12), 0, dp(12), dp(12))
        }

        root.addView(topBar(tier))
        root.addView(
            when (tier) {
                LayoutTier.COMPACT -> compactLayout()
                LayoutTier.MEDIUM -> mediumLayout()
                LayoutTier.EXPANDED -> expandedLayout()
            },
            LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f),
        )

        root.setOnApplyWindowInsetsListener { view, insets ->
            val bars = insets.getInsets(WindowInsets.Type.systemBars())
            view.setPadding(dp(12) + bars.left, bars.top, dp(12) + bars.right, dp(12) + bars.bottom)
            insets
        }

        setContentView(root)
    }

    private fun topBar(tier: LayoutTier): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        setPadding(dp(4), dp(8), dp(4), dp(8))

        addView(vertical().apply {
            addView(label("TruthRaw", 22f, bold = true))
            addView(label("${tier.name.lowercase().replaceFirstChar { it.uppercase() }} layout · ${session.selectedCount} RAW geselecteerd", 12f, muted = true))
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))

        addView(actionButton("RAW kiezen") { launchRawPicker() })
    }

    private fun compactLayout(): View = vertical().apply {
        addView(previewPane(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
        addView(routePane(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        if (session.jobs.isNotEmpty()) {
            addView(jobStrip(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(132)))
        }
    }

    private fun mediumLayout(): View = horizontal().apply {
        addView(vertical().apply {
            addView(jobListPane(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
            addView(routePane())
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.38f).apply { marginEnd = dp(8) })

        addView(previewPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.62f))
    }

    private fun expandedLayout(): View = horizontal().apply {
        addView(jobListPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.25f).apply { marginEnd = dp(8) })
        addView(previewPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.50f).apply { marginEnd = dp(8) })
        addView(toolsPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.25f))
    }

    private fun previewPane(): View = card().apply {
        gravity = Gravity.CENTER
        val title = if (session.jobs.isEmpty()) "Selecteer één of meerdere RAW-bestanden" else "TruthRaw Scene Preview"
        addView(label(title, 20f, bold = true).apply { gravity = Gravity.CENTER })
        addView(space(10))
        addView(label(
            if (session.jobs.isEmpty()) {
                "De ingang bewaart alleen documenthandles en metadata. RAW-sensorwaarden worden later tile-voor-tile opgevraagd."
            } else {
                "${session.selectedCount} bronhandle(s) · 0 volledige RAW-bytearrays in de UI-laag"
            },
            13f,
            muted = true,
        ).apply { gravity = Gravity.CENTER })
        addView(space(14))
        addView(label("Preview pipeline: proxy → tiled preview → final", 12f, muted = true).apply { gravity = Gravity.CENTER })
    }

    private fun routePane(): View = card().apply {
        addView(label("Uitkomst", 16f, bold = true))
        addView(space(6))

        when (session.jobs.size) {
            0 -> addView(label("Kies eerst RAW-bestanden.", 13f, muted = true))
            1 -> {
                addView(routeButton("1 upload → 1 uitkomst", InputRoute.SINGLE_ONE_OUTPUT))
                addView(routeButton("1 upload → meerdere uitkomsten", InputRoute.SINGLE_MULTIPLE_OUTPUTS))
            }
            else -> {
                addView(routeButton("Afzonderlijk verwerken", InputRoute.BATCH_INDEPENDENT))
                addView(routeButton("Verbeterde foto", InputRoute.MULTI_CAPTURE_ENHANCED))
                addView(routeButton("HDR", InputRoute.MULTI_CAPTURE_HDR))
                addView(label(
                    "Fusion/HDR is in v0.1 alleen een expliciete kandidaatroute. Frames worden niet automatisch als gezamenlijk bewijs behandeld.",
                    11f,
                    muted = true,
                ))
            }
        }
    }

    private fun toolsPane(): View = vertical().apply {
        addView(routePane())
        addView(space(8))
        addView(card().apply {
            addView(label("Kamers", 16f, bold = true))
            addView(label("Natural", 13f))
            addView(label("Detail", 13f))
            addView(label("Illumination", 13f))
            addView(label("Export", 13f))
            addView(space(8))
            addView(label("UI-keuzes veranderen geen scientific authority, zero-line of scene-ISO-contract.", 11f, muted = true))
        })
    }

    private fun jobListPane(): View = card().apply {
        addView(label("Ingang", 16f, bold = true))
        addView(label("${session.selectedCount} onafhankelijke bronhandle(s)", 12f, muted = true))
        addView(space(6))
        val scroll = ScrollView(this@MainActivity)
        scroll.addView(vertical().apply {
            if (session.jobs.isEmpty()) {
                addView(label("Nog geen RAW geselecteerd.", 13f, muted = true))
            } else {
                session.jobs.forEachIndexed { index, job -> addView(jobRow(index, job)) }
            }
        })
        addView(scroll, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
    }

    private fun jobStrip(): View = ScrollView(this).apply {
        addView(vertical().apply {
            session.jobs.forEachIndexed { index, job -> addView(jobRow(index, job)) }
        })
    }

    private fun jobRow(index: Int, job: RawJob): View = vertical().apply {
        setPadding(dp(10), dp(8), dp(10), dp(8))
        background = rounded(palette.surfaceAlt, 12f)
        addView(label("${index + 1}. ${job.source.displayName}", 13f, bold = true))
        val size = job.source.declaredSizeBytes?.let { " · ${formatBytes(it)}" } ?: ""
        addView(label("${job.state.name.lowercase()}$size", 11f, muted = true))
        addView(label("lineage: afzonderlijk totdat expliciete fusion-validatie bestaat", 10f, muted = true))
    }.also {
        it.layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            bottomMargin = dp(6)
        }
    }

    private fun routeButton(text: String, route: InputRoute): View = actionButton(
        if (session.route == route) "✓ $text" else text,
    ) {
        session = session.copy(route = route)
        render()
    }.also {
        it.layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            bottomMargin = dp(5)
        }
    }

    private fun card(): LinearLayout = vertical().apply {
        setPadding(dp(16), dp(16), dp(16), dp(16))
        background = rounded(palette.surface, 18f)
    }

    private fun vertical(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
    private fun horizontal(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }

    private fun label(text: String, sizeSp: Float, bold: Boolean = false, muted: Boolean = false): TextView =
        TextView(this).apply {
            this.text = text
            textSize = sizeSp
            setTextColor(if (muted) palette.textMuted else palette.text)
            if (bold) setTypeface(typeface, Typeface.BOLD)
        }

    private fun actionButton(text: String, action: () -> Unit): Button = Button(this).apply {
        this.text = text
        isAllCaps = false
        setTextColor(palette.text)
        background = rounded(palette.surfaceAlt, 14f)
        setOnClickListener { action() }
    }

    private fun rounded(color: Int, radiusDp: Float): GradientDrawable = GradientDrawable().apply {
        setColor(color)
        cornerRadius = dp(radiusDp).toFloat()
    }

    private fun space(heightDp: Int): View = Space(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(heightDp))
    }

    private fun currentLayoutTier(): LayoutTier {
        val widthPx = windowManager.currentWindowMetrics.bounds.width()
        val widthDp = widthPx / resources.displayMetrics.density
        return when {
            widthDp >= 840f -> LayoutTier.EXPANDED
            widthDp >= 600f -> LayoutTier.MEDIUM
            else -> LayoutTier.COMPACT
        }
    }

    private fun formatBytes(bytes: Long): String = when {
        bytes >= 1024L * 1024L * 1024L -> "%.1f GB".format(bytes / (1024.0 * 1024.0 * 1024.0))
        bytes >= 1024L * 1024L -> "%.1f MB".format(bytes / (1024.0 * 1024.0))
        bytes >= 1024L -> "%.1f KB".format(bytes / 1024.0)
        else -> "$bytes B"
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + 0.5f).toInt()
    private fun dp(value: Float): Int = (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        private const val REQUEST_OPEN_RAW = 4101
    }
}
