#!/usr/bin/env python3
"""TruthRaw conservation/restoration authority contract v0.1.

This is a provenance/authority guard, not an image-restoration algorithm.
It encodes the conservation principle that loss compensation or aesthetic
reintegration may improve a derivative while never becoming original measured
material/evidence.
"""
from __future__ import annotations

from dataclasses import dataclass, asdict
import argparse
import json

CORE = {"MEASURED", "RECONSTRUCTED", "CENSORED", "UNKNOWN", "COUNTERFACTUAL"}
ROLES = {
    "PRESERVE_ORIGINAL",
    "STABILIZED_DERIVED",
    "LOSS_COMPENSATION_RECONSTRUCTED",
    "AESTHETIC_REINTEGRATION_ONLY",
    "UNRESOLVED_LOSS",
}


@dataclass(frozen=True)
class Decision:
    allowed: bool
    classification: str
    scientific_authority_out: str
    restoration_role_out: str
    scientific_writeback_allowed: bool
    preserves_source_identity: bool
    creates_new_evidence: bool
    reason: str


def evaluate_site(
    *,
    source_authority: str,
    source_valid: bool,
    requested_role: str,
    support_present: bool,
    provenance_bound: bool,
    retreatable: bool,
) -> dict:
    if source_authority not in CORE:
        raise ValueError(f"unknown source authority: {source_authority}")
    if requested_role not in ROLES:
        raise ValueError(f"unknown restoration role: {requested_role}")

    def out(
        allowed: bool,
        classification: str,
        authority: str,
        role: str,
        writeback: bool,
        reason: str,
    ) -> dict:
        return asdict(
            Decision(
                allowed=allowed,
                classification=classification,
                scientific_authority_out=authority,
                restoration_role_out=role,
                scientific_writeback_allowed=writeback,
                preserves_source_identity=True,
                creates_new_evidence=False,
                reason=reason,
            )
        )

    # Surviving valid measured support is the digital equivalent of original
    # material. It cannot be replaced by an inferred/pretty value in science.
    if source_authority == "MEASURED" and source_valid:
        if requested_role == "PRESERVE_ORIGINAL":
            return out(
                True,
                "PASS_ORIGINAL_MEASURED_PRESERVED",
                "MEASURED",
                "ORIGINAL_MEASURED_SUPPORT",
                False,
                "Valid measured support is preserved unchanged.",
            )
        if requested_role == "STABILIZED_DERIVED":
            if not provenance_bound:
                return out(
                    False,
                    "BLOCKED_STABILIZATION_WITHOUT_PROVENANCE",
                    "MEASURED",
                    "ORIGINAL_MEASURED_SUPPORT",
                    False,
                    "A derived stabilization must remain bound to source provenance.",
                )
            return out(
                True,
                "PASS_STABILIZED_DERIVED_MEASURED_SUPPORT_RETAINED",
                "MEASURED",
                "STABILIZED_DERIVED",
                False,
                "Derived stabilization may coexist with measured support but cannot replace its evidence identity.",
            )
        if requested_role == "AESTHETIC_REINTEGRATION_ONLY":
            if not (provenance_bound and retreatable):
                return out(
                    False,
                    "BLOCKED_AESTHETIC_OVERLAY_NOT_RETREATABLE_OR_BOUND",
                    "MEASURED",
                    "ORIGINAL_MEASURED_SUPPORT",
                    False,
                    "Appearance overlay requires provenance binding and retreatability.",
                )
            return out(
                True,
                "PASS_AESTHETIC_OVERLAY_NO_WRITEBACK",
                "MEASURED",
                "AESTHETIC_REINTEGRATION_ONLY",
                False,
                "Appearance may overlay presentation while measured scientific support remains unchanged.",
            )
        return out(
            False,
            "BLOCKED_OVERPAINTING_VALID_MEASURED_SUPPORT",
            "MEASURED",
            "ORIGINAL_MEASURED_SUPPORT",
            False,
            "Valid measured support cannot be replaced by loss compensation or unresolved-state relabelling.",
        )

    # Censored support is evidence of a bound. It cannot be converted into an
    # exact scientific reconstruction merely to complete a highlight.
    if source_authority == "CENSORED":
        if requested_role == "AESTHETIC_REINTEGRATION_ONLY":
            if provenance_bound and retreatable:
                return out(
                    True,
                    "PASS_CENSORED_PRESENTATION_REINTEGRATION_ONLY",
                    "CENSORED",
                    "AESTHETIC_REINTEGRATION_ONLY",
                    False,
                    "A presentation estimate is allowed while the scientific state remains censored.",
                )
            return out(
                False,
                "BLOCKED_CENSORED_PRESENTATION_NOT_BOUND",
                "CENSORED",
                "UNRESOLVED_LOSS",
                False,
                "Presentation compensation must be provenance-bound and retreatable.",
            )
        if requested_role in {"PRESERVE_ORIGINAL", "UNRESOLVED_LOSS"}:
            return out(
                True,
                "PASS_CENSORED_BOUND_PRESERVED",
                "CENSORED",
                "UNRESOLVED_LOSS",
                False,
                "Censored evidence remains a bound; no exact latent value is invented.",
            )
        return out(
            False,
            "BLOCKED_EXACT_COMPENSATION_FROM_CENSORED_BOUND",
            "CENSORED",
            "UNRESOLVED_LOSS",
            False,
            "A censored bound is insufficient to promote an exact scientific loss compensation.",
        )

    # Counterfactual state never becomes evidence of the captured world.
    if source_authority == "COUNTERFACTUAL":
        if requested_role == "AESTHETIC_REINTEGRATION_ONLY" and provenance_bound:
            return out(
                True,
                "PASS_COUNTERFACTUAL_PRESENTATION_REMAINS_COUNTERFACTUAL",
                "COUNTERFACTUAL",
                "AESTHETIC_REINTEGRATION_ONLY",
                False,
                "Counterfactual content may be presented but remains counterfactual.",
            )
        return out(
            False,
            "BLOCKED_COUNTERFACTUAL_AUTHORITY_UPGRADE",
            "COUNTERFACTUAL",
            "UNRESOLVED_LOSS",
            False,
            "Counterfactual state cannot be restored into captured scientific evidence.",
        )

    # Unknown/invalid support may be reconstructed only if there is actual
    # bounded support and exact provenance. Otherwise it remains unresolved.
    effectively_missing = source_authority == "UNKNOWN" or not source_valid
    if effectively_missing:
        if requested_role == "LOSS_COMPENSATION_RECONSTRUCTED":
            if not support_present:
                return out(
                    False,
                    "BLOCKED_LOSS_COMPENSATION_WITHOUT_SUPPORT",
                    "UNKNOWN",
                    "UNRESOLVED_LOSS",
                    False,
                    "Missing support must remain unresolved rather than guessed.",
                )
            if not (provenance_bound and retreatable):
                return out(
                    False,
                    "BLOCKED_LOSS_COMPENSATION_NOT_BOUND_OR_RETREATABLE",
                    "UNKNOWN",
                    "UNRESOLVED_LOSS",
                    False,
                    "Loss compensation requires provenance binding and retreatability.",
                )
            return out(
                True,
                "PASS_LOSS_COMPENSATION_RECONSTRUCTED",
                "RECONSTRUCTED",
                "LOSS_COMPENSATION_RECONSTRUCTED",
                False,
                "A supported compensation is admitted as reconstruction, never measurement.",
            )
        if requested_role == "AESTHETIC_REINTEGRATION_ONLY":
            if provenance_bound and retreatable:
                return out(
                    True,
                    "PASS_AESTHETIC_REINTEGRATION_UNKNOWN_NO_WRITEBACK",
                    "UNKNOWN",
                    "AESTHETIC_REINTEGRATION_ONLY",
                    False,
                    "Appearance may fill the display while the scientific state remains unknown.",
                )
            return out(
                False,
                "BLOCKED_AESTHETIC_REINTEGRATION_NOT_BOUND",
                "UNKNOWN",
                "UNRESOLVED_LOSS",
                False,
                "Appearance compensation requires provenance binding and retreatability.",
            )
        return out(
            True,
            "PASS_UNRESOLVED_LOSS_RETAINED",
            "UNKNOWN",
            "UNRESOLVED_LOSS",
            False,
            "Insufficiently supported loss remains unresolved.",
        )

    # Existing reconstruction can remain reconstruction or get a non-writing
    # appearance overlay. It cannot be promoted to measured by restoration.
    if source_authority == "RECONSTRUCTED":
        if requested_role in {"PRESERVE_ORIGINAL", "STABILIZED_DERIVED"}:
            if requested_role == "STABILIZED_DERIVED" and not provenance_bound:
                return out(
                    False,
                    "BLOCKED_RECONSTRUCTION_STABILIZATION_WITHOUT_PROVENANCE",
                    "RECONSTRUCTED",
                    "LOSS_COMPENSATION_RECONSTRUCTED",
                    False,
                    "Derived reconstruction stabilization must remain provenance-bound.",
                )
            return out(
                True,
                "PASS_RECONSTRUCTED_AUTHORITY_PRESERVED",
                "RECONSTRUCTED",
                "LOSS_COMPENSATION_RECONSTRUCTED",
                False,
                "Existing reconstruction remains reconstruction.",
            )
        if requested_role == "AESTHETIC_REINTEGRATION_ONLY" and provenance_bound and retreatable:
            return out(
                True,
                "PASS_RECONSTRUCTED_WITH_AESTHETIC_OVERLAY",
                "RECONSTRUCTED",
                "AESTHETIC_REINTEGRATION_ONLY",
                False,
                "Appearance overlay does not upgrade reconstructed authority.",
            )
        return out(
            False,
            "BLOCKED_RECONSTRUCTION_AUTHORITY_UPGRADE",
            "RECONSTRUCTED",
            "LOSS_COMPENSATION_RECONSTRUCTED",
            False,
            "Restoration cannot promote reconstructed state to measured evidence.",
        )

    raise AssertionError("unreachable")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-authority", required=True, choices=sorted(CORE))
    ap.add_argument("--source-valid", action=argparse.BooleanOptionalAction, default=True)
    ap.add_argument("--requested-role", required=True, choices=sorted(ROLES))
    ap.add_argument("--support-present", action=argparse.BooleanOptionalAction, default=False)
    ap.add_argument("--provenance-bound", action=argparse.BooleanOptionalAction, default=False)
    ap.add_argument("--retreatable", action=argparse.BooleanOptionalAction, default=False)
    ns = ap.parse_args()
    print(json.dumps(evaluate_site(**vars(ns)), indent=2))


if __name__ == "__main__":
    main()
