import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from restoration_authority_v01 import evaluate_site  # noqa: E402


class RestorationAuthorityV01Tests(unittest.TestCase):
    def test_valid_measured_support_cannot_be_overpainted(self):
        r = evaluate_site(
            source_authority="MEASURED",
            source_valid=True,
            requested_role="LOSS_COMPENSATION_RECONSTRUCTED",
            support_present=True,
            provenance_bound=True,
            retreatable=True,
        )
        self.assertFalse(r["allowed"])
        self.assertEqual(r["classification"], "BLOCKED_OVERPAINTING_VALID_MEASURED_SUPPORT")
        self.assertEqual(r["scientific_authority_out"], "MEASURED")
        self.assertFalse(r["scientific_writeback_allowed"])

    def test_unknown_with_support_can_be_reconstructed_but_not_measured(self):
        r = evaluate_site(
            source_authority="UNKNOWN",
            source_valid=False,
            requested_role="LOSS_COMPENSATION_RECONSTRUCTED",
            support_present=True,
            provenance_bound=True,
            retreatable=True,
        )
        self.assertTrue(r["allowed"])
        self.assertEqual(r["scientific_authority_out"], "RECONSTRUCTED")
        self.assertEqual(r["restoration_role_out"], "LOSS_COMPENSATION_RECONSTRUCTED")
        self.assertFalse(r["creates_new_evidence"])

    def test_unknown_without_support_stays_unresolved(self):
        r = evaluate_site(
            source_authority="UNKNOWN",
            source_valid=False,
            requested_role="LOSS_COMPENSATION_RECONSTRUCTED",
            support_present=False,
            provenance_bound=True,
            retreatable=True,
        )
        self.assertFalse(r["allowed"])
        self.assertEqual(r["scientific_authority_out"], "UNKNOWN")
        self.assertEqual(r["restoration_role_out"], "UNRESOLVED_LOSS")

    def test_censored_highlight_cannot_be_turned_into_exact_scientific_value(self):
        r = evaluate_site(
            source_authority="CENSORED",
            source_valid=True,
            requested_role="LOSS_COMPENSATION_RECONSTRUCTED",
            support_present=True,
            provenance_bound=True,
            retreatable=True,
        )
        self.assertFalse(r["allowed"])
        self.assertEqual(r["scientific_authority_out"], "CENSORED")
        self.assertEqual(r["classification"], "BLOCKED_EXACT_COMPENSATION_FROM_CENSORED_BOUND")

    def test_censored_highlight_may_have_presentation_only_reintegration(self):
        r = evaluate_site(
            source_authority="CENSORED",
            source_valid=True,
            requested_role="AESTHETIC_REINTEGRATION_ONLY",
            support_present=False,
            provenance_bound=True,
            retreatable=True,
        )
        self.assertTrue(r["allowed"])
        self.assertEqual(r["scientific_authority_out"], "CENSORED")
        self.assertFalse(r["scientific_writeback_allowed"])

    def test_counterfactual_never_becomes_capture_evidence(self):
        r = evaluate_site(
            source_authority="COUNTERFACTUAL",
            source_valid=True,
            requested_role="LOSS_COMPENSATION_RECONSTRUCTED",
            support_present=True,
            provenance_bound=True,
            retreatable=True,
        )
        self.assertFalse(r["allowed"])
        self.assertEqual(r["scientific_authority_out"], "COUNTERFACTUAL")

    def test_appearance_overlay_on_measured_support_is_non_writing(self):
        r = evaluate_site(
            source_authority="MEASURED",
            source_valid=True,
            requested_role="AESTHETIC_REINTEGRATION_ONLY",
            support_present=False,
            provenance_bound=True,
            retreatable=True,
        )
        self.assertTrue(r["allowed"])
        self.assertEqual(r["scientific_authority_out"], "MEASURED")
        self.assertEqual(r["restoration_role_out"], "AESTHETIC_REINTEGRATION_ONLY")
        self.assertFalse(r["scientific_writeback_allowed"])

    def test_appearance_overlay_must_be_bound_and_retreatable(self):
        r = evaluate_site(
            source_authority="MEASURED",
            source_valid=True,
            requested_role="AESTHETIC_REINTEGRATION_ONLY",
            support_present=False,
            provenance_bound=True,
            retreatable=False,
        )
        self.assertFalse(r["allowed"])
        self.assertEqual(r["scientific_authority_out"], "MEASURED")


if __name__ == "__main__":
    unittest.main()
