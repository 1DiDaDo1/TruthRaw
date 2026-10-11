package com.truthraw.adaptiveui

/**
 * D.RAW flexible-inside cable contract v0.1.
 *
 * A cable keeps a stable outer evidence/provenance contract while allowing its
 * internal stages, adapters and capability routing to evolve. This descriptor
 * is deliberately open-ended: stage IDs, role IDs and capability IDs are
 * strings rather than closed enums, and every stage has an extension map.
 *
 * IMPORTANT: describing a cable never grants scientific authority. Authority,
 * provenance and writeback rules remain owned by the concrete scientific or
 * presentation contract that the stage declares.
 */
internal object DrawFlexibleCableContractV01 {
    const val CONTRACT_ID = "draw.flexible_cable.v0.1"

    data class StageDescriptor(
        val stageId: String,
        val roleId: String,
        val implementationId: String,
        val inputCapabilities: Set<String> = emptySet(),
        val outputCapabilities: Set<String> = emptySet(),
        val authorityContractId: String,
        val provenanceContractId: String,
        val extensions: Map<String, String> = emptyMap(),
    ) {
        init {
            require(stageId.isNotBlank()) { "stageId must not be blank" }
            require(roleId.isNotBlank()) { "roleId must not be blank" }
            require(implementationId.isNotBlank()) { "implementationId must not be blank" }
            require(authorityContractId.isNotBlank()) { "authorityContractId must not be blank" }
            require(provenanceContractId.isNotBlank()) { "provenanceContractId must not be blank" }
        }
    }

    data class CableDescriptor(
        val cableId: String,
        val version: String,
        val stages: List<StageDescriptor>,
        val outerInputContractId: String,
        val outerOutputContractId: String,
        val extensions: Map<String, String> = emptyMap(),
    ) {
        init {
            require(cableId.isNotBlank()) { "cableId must not be blank" }
            require(version.isNotBlank()) { "version must not be blank" }
            require(outerInputContractId.isNotBlank()) { "outerInputContractId must not be blank" }
            require(outerOutputContractId.isNotBlank()) { "outerOutputContractId must not be blank" }
        }
    }

    data class Validation(
        val valid: Boolean,
        val violations: List<String>,
    )

    /**
     * Structural check only. It intentionally does not infer authority or
     * decide which scientific transformations are allowed.
     */
    fun validate(descriptor: CableDescriptor): Validation {
        val violations = mutableListOf<String>()
        val duplicateStageIds = descriptor.stages
            .groupBy { it.stageId }
            .filterValues { it.size > 1 }
            .keys

        if (duplicateStageIds.isNotEmpty()) {
            violations += "duplicate stageId(s): ${duplicateStageIds.sorted().joinToString()}"
        }

        descriptor.stages.forEachIndexed { index, stage ->
            if (stage.inputCapabilities.any { it.isBlank() }) {
                violations += "stage[$index] ${stage.stageId}: blank input capability"
            }
            if (stage.outputCapabilities.any { it.isBlank() }) {
                violations += "stage[$index] ${stage.stageId}: blank output capability"
            }
            if (stage.extensions.keys.any { it.isBlank() }) {
                violations += "stage[$index] ${stage.stageId}: blank extension key"
            }
        }

        if (descriptor.extensions.keys.any { it.isBlank() }) {
            violations += "cable ${descriptor.cableId}: blank extension key"
        }

        return Validation(
            valid = violations.isEmpty(),
            violations = violations,
        )
    }
}
