#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
java = ROOT / 'suite_android/app/src/main/java/com/truthraw/adaptiveui'

chart = (java / 'ObservationOpticalFieldChartV01.kt').read_text()
signal = (java / 'BacksideSignalSupportAudit.kt').read_text()
profiler = (java / 'UniversalSourceProfiler.kt').read_text()
readme = (ROOT / 'docs/research/observation-optical-field-chart-v0.1/README.md').read_text()
state = json.loads((ROOT / 'state/OBSERVATION_OPTICAL_FIELD_CHART_STATE_2026-09-29.json').read_text())

for needle in [
    'D.RAW/ObservationOpticalFieldChart/0.1',
    'ACTIVE_AREA_GEOMETRIC_CENTER_ONLY',
    'FLAT_SOURCE_FIELD_COORDINATE_CHART_NOT_PHYSICAL_LENS_FLATTENING',
    'field_angle_from_rho_proven',
    'radial_unit',
    'tangential_unit',
    'lens_only_vignetting_proven',
    'scene_illumination_separated',
    'sensor_angular_response_separated',
    'cos4_model_assumed',
    'correction_gain_allowed',
    'scientific_writeback_allowed',
]:
    assert needle in chart, f'missing field-chart invariant: {needle}'

for needle in [
    'OPTICAL_FIELD_RADIAL_BINS = 12',
    'OPTICAL_FIELD_AZIMUTH_BINS = 12',
    'D.RAW/ObservationOpticalFieldSignal/0.1',
    'MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE',
    'MEASURED_SOURCE_SAMPLES_SCENE_LENS_SENSOR_COMPOSITE',
    'cfa_phase_p50',
    'azimuth_sector_relative_mad',
    'observed_outer_to_inner_p50_ratio',
    'radial_p50_non_increasing_transition_fraction',
    'lens_only_vignetting_proven',
    'correction_gain_allowed',
]:
    assert needle in signal, f'missing measured field-signal invariant: {needle}'

assert 'ObservationOpticalFieldChartV01.describe' in profiler
assert 'observation_optical_field_chart' in profiler
assert 'observation_optical_field_signal_v0_1' in profiler

for banned in [
    'lens correction applied',
    'vignetting corrected',
    'optical_axis_proven", true',
    'correction_gain_allowed", true',
]:
    assert banned.lower() not in (chart + signal).lower(), f'forbidden claim: {banned}'

assert state['geometry']['origin_is_physical_optical_axis'] is False
assert state['geometry']['field_angle_degrees_proven'] is False
assert state['measured_signal']['radial_bins'] == 12
assert state['measured_signal']['azimuth_bins'] == 12
assert state['interpretation']['lens_only_vignetting_proven'] is False
assert state['interpretation']['correction_gain_allowed'] is False
assert state['invariants']['v03_selector_modified'] is False
assert state['invariants']['measured_anchors_modified'] is False
assert state['invariants']['new_measured_samples_created'] is False
assert state['invariants']['scientific_writeback_allowed'] is False

lower = (chart + '\n' + signal).lower()
for banned in ['tensorflow', 'pytorch', 'onnxruntime', 'neural network', 'generative model']:
    assert banned not in lower, f'banned runtime/model token: {banned}'

assert 'flat lens' in readme.lower()
assert 'not automatically lens vignetting' in readme.lower()

print('Observation Optical Field Chart v0.1 integrity: PASS')
