import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from adobe_hdr_tiff_audit_v10 import (  # noqa: E402
    AdobeHdrTiffObservationV10,
    REAL_MOBILE_TIFF_OBSERVATION_V10,
    TiffHdrTransportClass,
    assess_adobe_hdr_tiff_v10,
)


class AdobeHdrTiffAuditV10Tests(unittest.TestCase):
    def test_real_mobile_tiff_is_adobe_hdr_interchange_not_self_describing_pq(self):
        a = assess_adobe_hdr_tiff_v10(REAL_MOBILE_TIFF_OBSERVATION_V10)
        self.assertTrue(a.adobe_hdr_edit_state_present)
        self.assertEqual(a.classification, TiffHdrTransportClass.ADOBE_ECOSYSTEM_HDR_INTERCHANGE)
        self.assertFalse(a.generic_decoder_self_describing_hdr_transport_proven)
        self.assertFalse(a.standard_pixel_payload_can_be_treated_as_pq_without_extra_contract)

    def test_adobe_tiff_never_writes_scientific_authority_back(self):
        a = assess_adobe_hdr_tiff_v10(REAL_MOBILE_TIFF_OBSERVATION_V10)
        self.assertFalse(a.scientific_master_writeback_allowed)
        self.assertFalse(a.creates_new_measured_dynamic_range)

    def test_code_ceiling_is_explicitly_flagged(self):
        a = assess_adobe_hdr_tiff_v10(REAL_MOBILE_TIFF_OBSERVATION_V10)
        self.assertTrue(a.code_ceiling_requires_caution)

    def test_independent_transport_proof_can_upgrade_transport_only(self):
        o = AdobeHdrTiffObservationV10(
            **{**REAL_MOBILE_TIFF_OBSERVATION_V10.__dict__, "independent_decoder_hdr_roundtrip_proven": True}
        )
        a = assess_adobe_hdr_tiff_v10(o)
        self.assertEqual(a.classification, TiffHdrTransportClass.SELF_DESCRIBING_HDR_TRANSPORT)
        self.assertTrue(a.generic_decoder_self_describing_hdr_transport_proven)
        self.assertFalse(a.standard_pixel_payload_can_be_treated_as_pq_without_extra_contract)
        self.assertFalse(a.scientific_master_writeback_allowed)

    def test_explicit_standard_transfer_is_required_before_raw_codes_are_called_pq(self):
        o = AdobeHdrTiffObservationV10(
            **{**REAL_MOBILE_TIFF_OBSERVATION_V10.__dict__, "explicit_standard_hdr_transfer_in_tiff_tags": True}
        )
        a = assess_adobe_hdr_tiff_v10(o)
        self.assertEqual(a.classification, TiffHdrTransportClass.SELF_DESCRIBING_HDR_TRANSPORT)
        self.assertTrue(a.standard_pixel_payload_can_be_treated_as_pq_without_extra_contract)


if __name__ == "__main__":
    unittest.main()
