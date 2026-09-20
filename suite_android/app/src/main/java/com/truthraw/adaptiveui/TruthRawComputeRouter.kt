package com.truthraw.adaptiveui

enum class TruthRawComputeClass {
    EXACT_SCIENTIFIC,
    BOUNDED_SCIENTIFIC,
    APPEARANCE,
    PRESENTATION,
}

enum class TruthRawComputeBackend {
    CPU_REFERENCE,
    CPU_ARM64_OPTIMIZED,
    VULKAN_GENERIC,
    QUALCOMM_FASTCV,
    QUALCOMM_QNN,
    MEDIATEK_NEUROPILOT,
}

data class TruthRawComputePlan(
    val computeClass: TruthRawComputeClass,
    val selected: TruthRawComputeBackend,
    val candidates: List<TruthRawComputeBackend>,
    val reason: String,
    val authorityChanged: Boolean = false,
)

object TruthRawComputeRouterV01 {
    /**
     * v0.1 is discovery-only. Candidate enumeration may expose hardware, but
     * pixel work stays on CPU_REFERENCE until a kernel-specific correctness
     * self-test + benchmark has been sealed.
     */
    fun plan(
        computeClass: TruthRawComputeClass,
        capabilities: TruthRawComputeCapabilities,
    ): TruthRawComputePlan {
        val candidates = buildList {
            add(TruthRawComputeBackend.CPU_REFERENCE)
            if (capabilities.arm64 && capabilities.neon) {
                add(TruthRawComputeBackend.CPU_ARM64_OPTIMIZED)
            }
            if (capabilities.genericVulkanCandidate &&
                computeClass in setOf(
                    TruthRawComputeClass.APPEARANCE,
                    TruthRawComputeClass.PRESENTATION,
                )
            ) {
                add(TruthRawComputeBackend.VULKAN_GENERIC)
            }
        }
        return TruthRawComputePlan(
            computeClass = computeClass,
            selected = TruthRawComputeBackend.CPU_REFERENCE,
            candidates = candidates,
            reason =
                "v0.1 discovery-only: accelerated candidates are not selectable " +
                    "until kernel-specific correctness + benchmark validation passes.",
            authorityChanged = false,
        )
    }
}
