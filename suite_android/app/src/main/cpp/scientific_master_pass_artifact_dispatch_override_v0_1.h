#pragma once

// This file is force-included only for truthnegative_pipeline_bridge_common.cpp.
// Include the canonical declaration before defining the token redirect so the
// validated v0.2 header itself is never rewritten by the preprocessor.
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_master_pass_artifact_dispatch_v0_1.h"

// Redirect exactly the Foundation preparation callsite token to the Android
// PassArtifact shim. Namespace qualification remains the established
// scientific_master_streaming_binding::v0_2 namespace. No preview, export or
// appearance callsite is affected.
#define bind_scientific_master_streaming_observed \
    bind_scientific_master_streaming_observed_pass_artifact
