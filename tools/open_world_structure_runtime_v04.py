#!/usr/bin/env python3
"""TruthRaw evidence-bound Structure Evidence runtime v0.4.

This module closes a specific gap between the open-world contracts and the
actual canonical reconstruction/uncertainty evidence chain.

It binds Structure Evidence to:
- exact v4.7i-family measured-channel reinjection,
- the canonical v5.0g-p1 backend-bound uncertainty evidence,
- the v0.8 held-out CFA topology study as a *proxy ceiling only*,
- explicit censoring,
- optional independently hold-out-validated optical MTF support.

Critical domain rule: the prospective v5.0g-p1 uncertainty certification was
performed on the HONOR BKQ-N49 22.48 mm tele vendor-DNG source class whose
prospective source is 4080x3072 BGGR. It must not silently authorize a
16320x12288 maximum-resolution Camera-5 RAW route. That larger route needs its
own uncertainty validation before this runtime can admit it.

No rendered RGB contrast and no semantic classifier can create scientific
support. Appearance output never writes back into the Scientific Master.
"""
from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from enum import Enum
from math import isfinite, sqrt
from pathlib import Path
from statistics import median
from typing import Optional, Sequence, Tuple

from open_world_foundations_v01 import ContractError, StructureEvidenceRecord
from open_world_integration_v02 import DetailAuthorityDecision, gate_detail_and_acutance
from open_world_integration_v03 import StructureMeasurementInputs, derive_structure_evidence


_SHA256_HEX = set("0123456789abcdef")


def _sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _check_sha256(value: str, field_name: str) -> None:
    v = value.lower()
    if len(v) != 64 or any(ch not in _SHA256_HEX for ch in v):
        raise ContractError(f"{field_name} must be a 64-character hexadecimal SHA-256")


def _unit(value: float, field_name: str) -> None:
    if not isfinite(value) or not (0.0 <= value <= 1.0):
        raise ContractError(f"{field_name} must be finite and within [0, 1]")


def _canonical_hash(payload: dict) -> str:
    blob = json.dumps(payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    return hashlib.sha256(blob).hexdigest()


class RuntimeStructureStatus(str, Enum):
    ADMITTED_APPEARANCE_ONLY = "ADMITTED_APPEARANCE_ONLY"
    NEUTRAL_WEAK_STRUCTURE = "NEUTRAL_WEAK_STRUCTURE"
    BLOCKED_OPTICS_CALIBRATION_MISSING = "BLOCKED_OPTICS_CALIBRATION_MISSING"
    BLOCKED_UNCERTAINTY_OUT_OF_DOMAIN = "BLOCKED_UNCERTAINTY_OUT_OF_DOMAIN"
    BLOCKED_BACKEND_BINDING_MISMATCH = "BLOCKED_BACKEND_BINDING_MISMATCH"
    BLOCKED_MEASURED_REINJECTION_MISMATCH = "BLOCKED_MEASURED_REINJECTION_MISMATCH"


@dataclass(frozen=True)
class CanonicalStructureBindings:
    reconstruction_backend_name: str
    production_backend_combined_sha256: str
    uncertainty_binding_sha256: str
    uncertainty_make: str
    uncertainty_model: str
    uncertainty_cfa: str
    uncertainty_focal_length_mm: float
    uncertainty_source_width: int
    uncertainty_source_height: int
    prospective_p95_coverage: float
    topology_proxy_by_channel: Tuple[float, float, float]  # R, G, B
    topology_certified: bool
    backend_binding_file_sha256: str
    prospective_result_file_sha256: str
    topology_summary_file_sha256: str
    binding_sha256: str


def load_canonical_structure_bindings(repo_root: Path) -> CanonicalStructureBindings:
    """Load and cross-check the real canonical evidence files from the repo."""
    backend_path = repo_root / "canonical/uncertainty/v5.0g/BACKEND_BINDING_v5_0g.json"
    frozen_path = repo_root / "canonical/uncertainty/v5.0g/FROZEN_PROTOCOL_v5_0g.json"
    readiness_path = repo_root / "canonical/uncertainty/v5.0g-p1/PTC_TELE_READINESS_v5_0g_p1.json"
    prospective_path = repo_root / "canonical/uncertainty/v5.0g-p1/PROSPECTIVE_TELE_HOLDOUT_RESULT_v5_0g_p1.json"
    topology_path = repo_root / "docs/research/missing-channel-topology-v0.8/TOPOLOGY_HELDOUT_SUMMARY_v0_8.json"

    backend = json.loads(backend_path.read_text(encoding="utf-8"))
    frozen = json.loads(frozen_path.read_text(encoding="utf-8"))
    readiness = json.loads(readiness_path.read_text(encoding="utf-8"))
    prospective = json.loads(prospective_path.read_text(encoding="utf-8"))
    topology = json.loads(topology_path.read_text(encoding="utf-8"))

    if readiness.get("backend_bound_uncertainty_pass") is not True:
        raise ContractError("canonical v5.0g-p1 backend-bound uncertainty is not certified")
    if readiness.get("full_physical_pass") is not False:
        raise ContractError("v0.4 expects full physical calibration to remain unresolved")
    if prospective.get("source_admission", {}).get("pass") is not True:
        raise ContractError("prospective tele uncertainty source admission did not pass")
    if prospective.get("prospective_checks", {}).get("p95_coverage") is not True:
        raise ContractError("prospective tele uncertainty p95 coverage gate did not pass")
    if topology.get("deterministic_rerun_byte_exact") is not True:
        raise ContractError("topology proxy study is not deterministic-byte-exact")

    source_class = frozen.get("source_class") or {}
    metadata = prospective.get("source_admission", {}).get("metadata") or {}
    shape = metadata.get("shape") or []
    if len(shape) != 2:
        raise ContractError("prospective uncertainty source shape is missing")
    source_h, source_w = int(shape[0]), int(shape[1])
    if source_w <= 0 or source_h <= 0:
        raise ContractError("prospective uncertainty source shape is invalid")

    expected_binding = backend.get("uncertainty_binding_sha256")
    if prospective.get("uncertainty_binding_sha256") != expected_binding:
        raise ContractError("prospective uncertainty binding does not match backend binding")

    summary = topology.get("summary") or {}
    proxy = []
    for ch in ("R", "G", "B"):
        s = summary.get(ch) or {}
        ordering = float(s.get("worst_fold_ordering", -1.0))
        curvature = float(s.get("worst_fold_curvature", -1.0))
        _unit(ordering, f"{ch}.worst_fold_ordering")
        _unit(curvature, f"{ch}.worst_fold_curvature")
        # Conservative proxy ceiling. It is explicitly not co-sited certification.
        proxy.append(min(ordering, curvature))

    p95_coverage = float(prospective.get("score", {}).get("coverage_p95", -1.0))
    _unit(p95_coverage, "prospective coverage_p95")

    backend_file_sha = _sha256_file(backend_path)
    prospective_file_sha = _sha256_file(prospective_path)
    topology_file_sha = _sha256_file(topology_path)
    payload = {
        "schema": "TruthRawCanonicalStructureBindings/0.4",
        "reconstruction_backend_name": backend.get("production_reconstruction_backend"),
        "production_backend_combined_sha256": backend.get("production_backend_combined_sha256"),
        "uncertainty_binding_sha256": expected_binding,
        "source_class": {
            "make": source_class.get("make"),
            "model": source_class.get("model"),
            "cfa": source_class.get("cfa"),
            "focal_length_mm": source_class.get("focal_length_mm"),
            "width": source_w,
            "height": source_h,
        },
        "prospective_p95_coverage": p95_coverage,
        "topology_proxy_by_channel": proxy,
        "topology_certified": False,
        "evidence_file_sha256": [backend_file_sha, prospective_file_sha, topology_file_sha],
    }
    for key in ("production_backend_combined_sha256", "uncertainty_binding_sha256"):
        _check_sha256(str(payload[key]), key)

    return CanonicalStructureBindings(
        reconstruction_backend_name=str(payload["reconstruction_backend_name"]),
        production_backend_combined_sha256=str(payload["production_backend_combined_sha256"]).lower(),
        uncertainty_binding_sha256=str(payload["uncertainty_binding_sha256"]).lower(),
        uncertainty_make=str(source_class.get("make")),
        uncertainty_model=str(source_class.get("model")),
        uncertainty_cfa=str(source_class.get("cfa")),
        uncertainty_focal_length_mm=float(source_class.get("focal_length_mm")),
        uncertainty_source_width=source_w,
        uncertainty_source_height=source_h,
        prospective_p95_coverage=p95_coverage,
        topology_proxy_by_channel=(proxy[0], proxy[1], proxy[2]),
        topology_certified=False,
        backend_binding_file_sha256=backend_file_sha,
        prospective_result_file_sha256=prospective_file_sha,
        topology_summary_file_sha256=topology_file_sha,
        binding_sha256=_canonical_hash(payload),
    )


@dataclass(frozen=True)
class RuntimeStructureTile:
    region_id: str
    device_make: str
    device_model: str
    physical_camera_id: str
    focal_length_mm: float
    source_width: int
    source_height: int
    cfa: str
    tile_x: int
    tile_y: int
    tile_width: int
    tile_height: int
    source_evidence_sha256: str
    scientific_master_sha256: str
    reconstruction_backend_name: str
    production_backend_combined_sha256: str
    uncertainty_binding_sha256: str
    stage2_cfa: Sequence[float]
    source_sigma_cfa: Sequence[float]
    reconstructed_rgb: Sequence[float]
    uncertainty_p95_rgb: Sequence[float]
    censored_cfa: Sequence[bool]

    def validate_shape(self) -> int:
        if not self.region_id.strip():
            raise ContractError("region_id may not be empty")
        if self.tile_width <= 0 or self.tile_height <= 0:
            raise ContractError("tile dimensions must be positive")
        n = self.tile_width * self.tile_height
        if len(self.stage2_cfa) != n or len(self.source_sigma_cfa) != n or len(self.censored_cfa) != n:
            raise ContractError("CFA/sigma/censor arrays must match tile sample count")
        if len(self.reconstructed_rgb) != 3 * n or len(self.uncertainty_p95_rgb) != 3 * n:
            raise ContractError("RGB/uncertainty arrays must contain three values per tile sample")
        _check_sha256(self.source_evidence_sha256, "source_evidence_sha256")
        _check_sha256(self.scientific_master_sha256, "scientific_master_sha256")
        _check_sha256(self.production_backend_combined_sha256, "production_backend_combined_sha256")
        _check_sha256(self.uncertainty_binding_sha256, "uncertainty_binding_sha256")
        for x in self.stage2_cfa:
            if not isfinite(float(x)):
                raise ContractError("stage2 CFA must be finite")
        for x in self.source_sigma_cfa:
            if not isfinite(float(x)) or float(x) < 0.0:
                raise ContractError("source sigma must be finite and nonnegative")
        for x in self.reconstructed_rgb:
            if not isfinite(float(x)):
                raise ContractError("reconstructed RGB must be finite")
        for x in self.uncertainty_p95_rgb:
            if not isfinite(float(x)) or float(x) < 0.0:
                raise ContractError("p95 uncertainty must be finite and nonnegative")
        return n


@dataclass(frozen=True)
class OpticsMtfSupport:
    calibration_id: str
    source_width: int
    source_height: int
    cfa: str
    focal_length_mm: float
    support: float
    calibration_evidence_sha256: str
    holdout_report_sha256: str

    def validate_for(self, tile: RuntimeStructureTile) -> None:
        if not self.calibration_id.strip():
            raise ContractError("optics calibration_id may not be empty")
        _unit(self.support, "optics MTF support")
        _check_sha256(self.calibration_evidence_sha256, "optics calibration_evidence_sha256")
        _check_sha256(self.holdout_report_sha256, "optics holdout_report_sha256")
        if (
            self.source_width != tile.source_width
            or self.source_height != tile.source_height
            or self.cfa != tile.cfa
            or abs(self.focal_length_mm - tile.focal_length_mm) > 1.0e-6
        ):
            raise ContractError("optics support is outside the runtime source domain")


@dataclass(frozen=True)
class RuntimeStructureDiagnostics:
    exact_measured_reinjection: bool
    measured_pair_count: int
    significant_measured_pair_count: int
    measured_cfa_structure_support: float
    reconstructed_topology_proxy_support: float
    uncertainty_confidence: float
    censoring_risk: float
    local_measured_contrast: float
    missing_channel_p95_median: float
    topology_certified: bool
    uncertainty_domain_certified: bool
    optics_calibrated: bool


@dataclass(frozen=True)
class RuntimeStructureResult:
    status: RuntimeStructureStatus
    record: Optional[StructureEvidenceRecord]
    detail_decision: Optional[DetailAuthorityDecision]
    diagnostics: RuntimeStructureDiagnostics
    binding_sha256: str
    claim_boundary: str


_CFA = {
    "BGGR": (2, 1, 1, 0),
    "RGGB": (0, 1, 1, 2),
    "GRBG": (1, 0, 2, 1),
    "GBRG": (1, 2, 0, 1),
}


def _color_at(cfa: str, x: int, y: int) -> int:
    pattern = _CFA.get(cfa)
    if pattern is None:
        raise ContractError(f"unsupported CFA: {cfa}")
    return pattern[(y & 1) * 2 + (x & 1)]


def _empty_diag(*, uncertainty_domain: bool = False) -> RuntimeStructureDiagnostics:
    return RuntimeStructureDiagnostics(False, 0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, False, uncertainty_domain, False)


def _domain_matches(tile: RuntimeStructureTile, bindings: CanonicalStructureBindings) -> bool:
    return (
        tile.device_make == bindings.uncertainty_make
        and tile.device_model == bindings.uncertainty_model
        and tile.physical_camera_id == "5"
        and tile.cfa == bindings.uncertainty_cfa
        and tile.source_width == bindings.uncertainty_source_width
        and tile.source_height == bindings.uncertainty_source_height
        and abs(tile.focal_length_mm - bindings.uncertainty_focal_length_mm) <= 1.0e-6
    )


def _measured_reinjection_exact(tile: RuntimeStructureTile) -> bool:
    for yy in range(tile.tile_height):
        for xx in range(tile.tile_width):
            i = yy * tile.tile_width + xx
            c = _color_at(tile.cfa, tile.tile_x + xx, tile.tile_y + yy)
            if float(tile.reconstructed_rgb[3 * i + c]) != float(tile.stage2_cfa[i]):
                return False
    return True


def _measured_structure_support(tile: RuntimeStructureTile) -> Tuple[int, int, float, float]:
    values = [float(v) for i, v in enumerate(tile.stage2_cfa) if not bool(tile.censored_cfa[i])]
    local_contrast = (max(values) - min(values)) if values else 0.0
    candidate = 0
    significant = 0
    # Same-colour Bayer samples are two pixels apart along cardinal axes.
    for yy in range(tile.tile_height):
        for xx in range(tile.tile_width):
            i = yy * tile.tile_width + xx
            if bool(tile.censored_cfa[i]):
                continue
            for dx, dy in ((2, 0), (0, 2)):
                x2, y2 = xx + dx, yy + dy
                if x2 >= tile.tile_width or y2 >= tile.tile_height:
                    continue
                j = y2 * tile.tile_width + x2
                if bool(tile.censored_cfa[j]):
                    continue
                if _color_at(tile.cfa, tile.tile_x + xx, tile.tile_y + yy) != _color_at(
                    tile.cfa, tile.tile_x + x2, tile.tile_y + y2
                ):
                    continue
                candidate += 1
                sigma = sqrt(float(tile.source_sigma_cfa[i]) ** 2 + float(tile.source_sigma_cfa[j]) ** 2)
                if abs(float(tile.stage2_cfa[i]) - float(tile.stage2_cfa[j])) > 2.0 * sigma:
                    significant += 1
    support = (significant / candidate) if candidate else 0.0
    return candidate, significant, support, local_contrast


def _topology_and_uncertainty_support(
    tile: RuntimeStructureTile,
    bindings: CanonicalStructureBindings,
    local_contrast: float,
) -> Tuple[float, float, float]:
    proxy_sum = 0.0
    proxy_n = 0
    missing_p95 = []
    for yy in range(tile.tile_height):
        for xx in range(tile.tile_width):
            i = yy * tile.tile_width + xx
            measured = _color_at(tile.cfa, tile.tile_x + xx, tile.tile_y + yy)
            if bool(tile.censored_cfa[i]):
                continue
            for c in (0, 1, 2):
                if c == measured:
                    continue
                p95 = float(tile.uncertainty_p95_rgb[3 * i + c])
                recon = float(tile.reconstructed_rgb[3 * i + c])
                if not isfinite(p95) or p95 < 0.0 or not isfinite(recon):
                    continue
                proxy_sum += bindings.topology_proxy_by_channel[c]
                proxy_n += 1
                missing_p95.append(p95)
    topology_support = proxy_sum / proxy_n if proxy_n else 0.0
    p95_med = median(missing_p95) if missing_p95 else 0.0
    if local_contrast <= 0.0 or not missing_p95:
        uncertainty_confidence = 0.0
    else:
        # Dimensionless contrast-to-error support. This is not a probability;
        # cap it by the empirically validated prospective p95 coverage.
        uncertainty_confidence = min(
            bindings.prospective_p95_coverage,
            local_contrast / (local_contrast + p95_med),
        )
    return topology_support, uncertainty_confidence, p95_med


def derive_runtime_structure_evidence(
    tile: RuntimeStructureTile,
    bindings: CanonicalStructureBindings,
    *,
    optics: Optional[OpticsMtfSupport] = None,
) -> RuntimeStructureResult:
    """Derive Structure Evidence from bound runtime artifacts, fail closed."""
    n = tile.validate_shape()

    if not _domain_matches(tile, bindings):
        return RuntimeStructureResult(
            RuntimeStructureStatus.BLOCKED_UNCERTAINTY_OUT_OF_DOMAIN,
            None,
            None,
            _empty_diag(uncertainty_domain=False),
            bindings.binding_sha256,
            "The v5.0g-p1 uncertainty certification is source-class/domain bound and may not be transferred to this geometry or route.",
        )

    if (
        tile.reconstruction_backend_name != bindings.reconstruction_backend_name
        or tile.production_backend_combined_sha256.lower() != bindings.production_backend_combined_sha256
        or tile.uncertainty_binding_sha256.lower() != bindings.uncertainty_binding_sha256
    ):
        return RuntimeStructureResult(
            RuntimeStructureStatus.BLOCKED_BACKEND_BINDING_MISMATCH,
            None,
            None,
            _empty_diag(uncertainty_domain=True),
            bindings.binding_sha256,
            "Structure Evidence requires the exact reconstruction/uncertainty binding that passed prospective validation.",
        )

    exact_reinjection = _measured_reinjection_exact(tile)
    if not exact_reinjection:
        d = _empty_diag(uncertainty_domain=True)
        d = RuntimeStructureDiagnostics(
            False, d.measured_pair_count, d.significant_measured_pair_count,
            d.measured_cfa_structure_support, d.reconstructed_topology_proxy_support,
            d.uncertainty_confidence, d.censoring_risk, d.local_measured_contrast,
            d.missing_channel_p95_median, False, True, False,
        )
        return RuntimeStructureResult(
            RuntimeStructureStatus.BLOCKED_MEASURED_REINJECTION_MISMATCH,
            None,
            None,
            d,
            bindings.binding_sha256,
            "The physically measured CFA component was not preserved exactly in reconstructed RGB; no structure authority is emitted.",
        )

    pair_count, significant_pairs, measured_support, local_contrast = _measured_structure_support(tile)
    topology_support, uncertainty_confidence, p95_med = _topology_and_uncertainty_support(
        tile, bindings, local_contrast
    )
    censoring_risk = sum(1 for x in tile.censored_cfa if bool(x)) / n

    optics_support = 0.0
    optics_calibrated = False
    evidence = {
        tile.source_evidence_sha256.lower(),
        tile.scientific_master_sha256.lower(),
        bindings.backend_binding_file_sha256,
        bindings.prospective_result_file_sha256,
        bindings.topology_summary_file_sha256,
    }
    if optics is not None:
        optics.validate_for(tile)
        optics_support = optics.support
        optics_calibrated = True
        evidence.add(optics.calibration_evidence_sha256.lower())
        evidence.add(optics.holdout_report_sha256.lower())

    record = derive_structure_evidence(
        StructureMeasurementInputs(
            region_id=tile.region_id,
            measured_cfa_support=measured_support,
            reconstructed_topology_support=topology_support,
            optical_mtf_support=optics_support,
            uncertainty_confidence=uncertainty_confidence,
            censoring_risk=censoring_risk,
            evidence_sha256=tuple(sorted(evidence)),
        )
    )
    decision = gate_detail_and_acutance(record)
    diagnostics = RuntimeStructureDiagnostics(
        exact_measured_reinjection=True,
        measured_pair_count=pair_count,
        significant_measured_pair_count=significant_pairs,
        measured_cfa_structure_support=measured_support,
        reconstructed_topology_proxy_support=topology_support,
        uncertainty_confidence=uncertainty_confidence,
        censoring_risk=censoring_risk,
        local_measured_contrast=local_contrast,
        missing_channel_p95_median=p95_med,
        topology_certified=False,
        uncertainty_domain_certified=True,
        optics_calibrated=optics_calibrated,
    )

    if not optics_calibrated:
        status = RuntimeStructureStatus.BLOCKED_OPTICS_CALIBRATION_MISSING
        boundary = (
            "Measured CFA/topology-proxy/uncertainty/censoring support was derived, but adaptive detail stays neutral because no independently hold-out-validated optical MTF support is bound."
        )
    elif record.measured_support <= 0.0 and record.reconstructed_support <= 0.0:
        status = RuntimeStructureStatus.NEUTRAL_WEAK_STRUCTURE
        boundary = "Bound evidence is insufficient for adaptive detail; neutral appearance is retained."
    else:
        status = RuntimeStructureStatus.ADMITTED_APPEARANCE_ONLY
        boundary = (
            "Bound structure evidence may admit appearance-domain detail/acutance only. Topology remains proxy-only and transformed pixels never become measured Scientific Master data."
        )

    result_payload = {
        "schema": "TruthRawRuntimeStructureResult/0.4",
        "canonical_binding_sha256": bindings.binding_sha256,
        "region_id": tile.region_id,
        "source_evidence_sha256": tile.source_evidence_sha256.lower(),
        "scientific_master_sha256": tile.scientific_master_sha256.lower(),
        "status": status.value,
        "diagnostics": {
            "exact_measured_reinjection": diagnostics.exact_measured_reinjection,
            "measured_pair_count": diagnostics.measured_pair_count,
            "significant_measured_pair_count": diagnostics.significant_measured_pair_count,
            "measured_cfa_structure_support": diagnostics.measured_cfa_structure_support,
            "reconstructed_topology_proxy_support": diagnostics.reconstructed_topology_proxy_support,
            "uncertainty_confidence": diagnostics.uncertainty_confidence,
            "censoring_risk": diagnostics.censoring_risk,
            "local_measured_contrast": diagnostics.local_measured_contrast,
            "missing_channel_p95_median": diagnostics.missing_channel_p95_median,
            "topology_certified": False,
            "uncertainty_domain_certified": True,
            "optics_calibrated": diagnostics.optics_calibrated,
        },
    }
    return RuntimeStructureResult(
        status,
        record,
        decision,
        diagnostics,
        _canonical_hash(result_payload),
        boundary,
    )
