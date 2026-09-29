#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
base = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

lattice = (base / "RasterIndependentSampleLatticeV01.kt").read_text()
v07 = (base / "DarkChromaStabilityV07Audit.kt").read_text()
profiler = (base / "UniversalSourceProfiler.kt").read_text()
main = (base / "MainActivity.kt").read_text()
state = (ROOT / "state/RASTER_INDEPENDENT_SAMPLE_LATTICE_STATE_2026-09-29.json").read_text()
readme = (ROOT / "docs/research/raster-independent-sample-lattice-v0.1/README.md").read_text()

for needle in [
    '"D.RAW/RasterIndependentSampleLattice/0.1"',
    'FRACTION_BITS: Int = 20',
    'UNITS_PER_SOURCE_PIXEL: Long = 1L shl FRACTION_BITS',
    '"SPARSE_IMPLICIT_FIXED_POINT_SAMPLE_LATTICE"',
    '"unanchored_lattice_positions_authority"',
    '"UNKNOWN"',
    '"unanchored_lattice_positions_have_values",\n                false',
    '"dense_lattice_materialized", false',
    '"source_values_resampled", false',
    '"source_values_interpolated", false',
    '"source_values_modified", false',
    '"source_bytes_modified", false',
    '"new_sensor_measurements_created", false',
    '"optical_resolution_increased", false',
    '"upscaling_performed", false',
    '"coordinate_precision_is_sensor_resolution_claim",\n                false',
    '"finite_source_raster_is_world_boundary",\n                false',
    '"finite_source_raster_is_measurement_sampling",\n                true',
]:
    assert needle in lattice, f"missing lattice invariant: {needle}"

for needle in [
    '"D.RAW/N2RasterIndependentSampleGeometry/0.1"',
    '"new_measured_samples_created", false',
    '"unanchored_positions_inferred", false',
    '"interpolation_performed", false',
    '"upscaling_performed", false',
    '"distance_threshold_admitted", false',
    '"can_reduce_protection", false',
    '"can_enable_correction", false',
    '"chroma_correction_supported", false',
    '"private_ab_delta_allowed", false',
]:
    assert needle in lattice, f"missing N2 lattice binding invariant: {needle}"

for needle in [
    '"D.RAW/Frontside/DarkChromaStability/0.7"',
    '"source_raster_replaced_as_evidence",\n                        false',
    '"source_raster_used_only_as_measurement_sampling",\n                        true',
    '"measured_source_samples_relocated",\n                        false',
    '"measured_source_values_changed",\n                        false',
    '"dense_micro_raster_materialized",\n                        false',
    '"positions_between_measured_anchors_have_measured_values",\n                        false',
    '"unknown_positions_remain_unknown",\n                        true',
    '"coordinate_precision_increases_optical_resolution",\n                        false',
    '"noise_correction_enabled", false',
    '"can_reduce_protection", false',
    '"can_enable_correction", false',
    '"uses_ai_or_learned_model", false',
    '"candidate_applied", false',
    '"scientific_writeback_allowed", false',
]:
    assert needle in v07, f"missing v0.7 invariant: {needle}"

assert 'RasterIndependentSampleLatticeV01.describe' in profiler
assert 'RasterIndependentSampleLatticeV01.bindSupportGeometry' in profiler
assert 'DarkChromaStabilityV07Audit.analyze' in profiler
assert '"raster_independent_sample_lattice"' in profiler
assert '"n2_raster_independent_sample_geometry"' in profiler
assert '"dark_chroma_stability_v0_7"' in profiler
assert '"scientific_coordinate_domain_can_be_raster_independent", true' in profiler
assert '"source_raster_defines_measurement_sampling_not_world_resolution", true' in profiler
assert '"unmeasured_coordinate_positions_remain_unknown", true' in profiler
assert '"coordinate_precision_does_not_create_evidence", true' in profiler

assert 'D.RAW Sample Lattice v0.1 · RASTER-INDEPENDENT' in main
assert 'N2 Raster-Independent Geometry v0.1' in main
assert 'Dark Chroma Stability v0.7 · SAMPLE-LATTICE BINDING' in main
assert 'tussenposities starten UNKNOWN' in main
assert 'upscaling=false' in main

for needle in [
    '"units_per_source_pixel": 1048576',
    '"dense_lattice_materialized": false',
    '"source_samples_remain_exact_measured_anchors": true',
    '"unanchored_positions_authority": "UNKNOWN"',
    '"upscaling_performed": false',
    '"noise_correction_enabled": false',
    '"correction_supported": false',
    '"scientific_writeback_allowed": false',
]:
    assert needle in state, f"missing project-state invariant: {needle}"

assert "The source raster determines where D.RAW measured." in readme
assert "It is not upscaling" in readme
assert "UNKNOWN" in readme

lower = (lattice + "\n" + v07).lower()
for banned in ["tensorflow", "pytorch", "onnx", "neural network", "generative model"]:
    assert banned not in lower, f"banned learned-runtime token: {banned}"

print("Raster-Independent Sample Lattice v0.1 integrity: PASS")
