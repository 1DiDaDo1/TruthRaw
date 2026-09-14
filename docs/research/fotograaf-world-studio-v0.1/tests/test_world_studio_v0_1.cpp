#include "../native/world_studio_v0_1.h"

#include <cassert>

using namespace truthraw::fotograaf::world_studio_v0_1;

int main() {
    WorldState state;
    assert(valid_evidence_invariants(state));

    state.independent_evidence_count = 2;
    assert(!valid_evidence_invariants(state));

    assert(authority_may_claim_physical_snr(
        WorldAuthority::WS2_CALIBRATED_NEUTRAL_PHOTOMETRIC));
    assert(!authority_may_claim_physical_snr(
        WorldAuthority::WS1_GEOMETRY_AWARE_RELATIVE));

    return 0;
}
