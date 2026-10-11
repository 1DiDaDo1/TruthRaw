package com.truthraw.adaptiveui

/**
 * D.RAW non-destructive workbench state v0.1.
 *
 * The source and Scientific Master are inputs, never edit targets. User edits
 * are reversible downstream instructions that can be re-rendered or discarded.
 * Export is a separate derived-output action and is never represented here as
 * source mutation.
 *
 * The state deliberately uses open operation/parameter identifiers so the
 * cable can evolve internally without turning today's UI controls into a
 * closed-world architecture.
 */
internal object NonDestructiveWorkbenchStateV01 {
    const val CONTRACT_ID = "draw.workbench.non_destructive.v0.1"
    const val APPEARANCE_ONLY = "APPEARANCE_ONLY"
    const val OUTPUT_TRANSFORM_ONLY = "OUTPUT_TRANSFORM_ONLY"
    const val VIEW_ONLY = "VIEW_ONLY"

    const val SOURCE_MUTATION_ALLOWED = false
    const val SCIENTIFIC_MASTER_WRITEBACK_ALLOWED = false
    const val OVERWRITE_SOURCE_ON_EXPORT_ALLOWED = false

    /** Stable binding to the immutable/read-only image input for this edit set. */
    data class SourceBinding(
        val bindingId: String,
        val sourceKindId: String,
        val sourceSha256: String? = null,
        val observationId: String? = null,
        val scientificMasterBindingId: String? = null,
        val extensions: Map<String, String> = emptyMap(),
    ) {
        init {
            require(bindingId.isNotBlank()) { "bindingId must not be blank" }
            require(sourceKindId.isNotBlank()) { "sourceKindId must not be blank" }
            require(sourceSha256 == null || SHA256.matches(sourceSha256)) {
                "sourceSha256 must be null or lowercase/uppercase 64-hex SHA-256"
            }
        }
    }

    /**
     * One reversible downstream instruction.
     *
     * operationId identifies the semantic operation (for example
     * `appearance.black_point` or `output.free_raster.transform`). instanceId
     * keeps multiple instances addressable without requiring a closed enum.
     */
    data class EditOperation(
        val instanceId: String,
        val operationId: String,
        val domainId: String,
        val enabled: Boolean = true,
        val parameters: Map<String, Double> = emptyMap(),
        val extensions: Map<String, String> = emptyMap(),
    ) {
        init {
            require(instanceId.isNotBlank()) { "instanceId must not be blank" }
            require(operationId.isNotBlank()) { "operationId must not be blank" }
            require(domainId in SAFE_DOWNSTREAM_DOMAINS) {
                "Workbench edits must remain downstream: $domainId"
            }
            require(parameters.keys.none { it.isBlank() }) { "parameter key must not be blank" }
            require(parameters.values.all { it.isFinite() }) { "parameters must be finite" }
            require(extensions.keys.none { it.isBlank() }) { "extension key must not be blank" }
        }
    }

    /**
     * Current edit recipe. The immutable source binding is not replaced by
     * render output; render output is merely a view/result of applying this
     * recipe downstream.
     */
    data class State(
        val source: SourceBinding,
        val revision: Long = 0L,
        val operations: List<EditOperation> = emptyList(),
        val extensions: Map<String, String> = emptyMap(),
    ) {
        init {
            require(revision >= 0L) { "revision must be non-negative" }
            require(operations.map { it.instanceId }.distinct().size == operations.size) {
                "edit instanceId values must be unique"
            }
            require(extensions.keys.none { it.isBlank() }) { "extension key must not be blank" }
        }

        val sourceMutationAllowed: Boolean
            get() = SOURCE_MUTATION_ALLOWED

        val scientificMasterWritebackAllowed: Boolean
            get() = SCIENTIFIC_MASTER_WRITEBACK_ALLOWED

        val overwriteSourceOnExportAllowed: Boolean
            get() = OVERWRITE_SOURCE_ON_EXPORT_ALLOWED
    }

    sealed interface ChangeResult {
        data class Ready(val state: State) : ChangeResult
        data class Rejected(val reason: String) : ChangeResult
    }

    fun upsert(state: State, operation: EditOperation): ChangeResult {
        val nextOperations = state.operations.toMutableList()
        val index = nextOperations.indexOfFirst { it.instanceId == operation.instanceId }
        if (index >= 0) {
            nextOperations[index] = operation
        } else {
            nextOperations += operation
        }
        return increment(state, nextOperations)
    }

    fun setEnabled(state: State, instanceId: String, enabled: Boolean): ChangeResult {
        if (instanceId.isBlank()) return ChangeResult.Rejected("instanceId must not be blank")
        val index = state.operations.indexOfFirst { it.instanceId == instanceId }
        if (index < 0) return ChangeResult.Rejected("edit operation not found: $instanceId")

        val nextOperations = state.operations.toMutableList()
        nextOperations[index] = nextOperations[index].copy(enabled = enabled)
        return increment(state, nextOperations)
    }

    fun remove(state: State, instanceId: String): ChangeResult {
        if (instanceId.isBlank()) return ChangeResult.Rejected("instanceId must not be blank")
        if (state.operations.none { it.instanceId == instanceId }) {
            return ChangeResult.Rejected("edit operation not found: $instanceId")
        }
        return increment(state, state.operations.filterNot { it.instanceId == instanceId })
    }

    /** Reset returns to the same immutable source binding with no edits. */
    fun reset(state: State): ChangeResult = increment(state, emptyList())

    private fun increment(state: State, operations: List<EditOperation>): ChangeResult {
        if (state.revision == Long.MAX_VALUE) {
            return ChangeResult.Rejected("revision overflow")
        }
        return ChangeResult.Ready(
            state.copy(
                revision = state.revision + 1L,
                operations = operations,
            ),
        )
    }

    private val SAFE_DOWNSTREAM_DOMAINS = setOf(
        APPEARANCE_ONLY,
        OUTPUT_TRANSFORM_ONLY,
        VIEW_ONLY,
    )

    private val SHA256 = Regex("^[0-9a-fA-F]{64}$")
}
