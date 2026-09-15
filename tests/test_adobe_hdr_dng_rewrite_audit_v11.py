import pathlib,sys,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from adobe_hdr_dng_rewrite_audit_v11 import FROZEN_ADOBE_HDR_DNG_REWRITE_V11,AdobeHdrDngRewriteAuditV11

class AdobeHdrDngRewriteAuditV11Tests(unittest.TestCase):
    def test_frozen_roundtrip_preserves_all_cfa_samples(self):
        a=FROZEN_ADOBE_HDR_DNG_REWRITE_V11.validate()
        self.assertTrue(a.cfa_sample_exact)
        self.assertEqual(a.raw_mismatch_count,0)
        self.assertEqual(a.raw_max_abs_difference,0)
        self.assertEqual(a.width*a.height,12533760)
        self.assertEqual(a.source_white_count,217)

    def test_container_is_not_source_exact(self):
        a=FROZEN_ADOBE_HDR_DNG_REWRITE_V11
        self.assertNotEqual(a.original_file_sha256,a.rewritten_file_sha256)
        self.assertFalse(a.container_exact)
        self.assertFalse(a.metadata_source_exact)

    def test_hdr_xmp_never_becomes_sensor_evidence(self):
        a=FROZEN_ADOBE_HDR_DNG_REWRITE_V11
        self.assertTrue(a.hdr_edit_mode)
        self.assertEqual(a.hdr_max_value_ev,8.0)
        self.assertEqual(a.whites_2012,2.0)
        self.assertFalse(a.adobe_hdr_xmp_is_scientific_evidence)
        self.assertFalse(a.scientific_master_writeback_allowed)

    def test_preserved_fields_do_not_authorize_changed_color_matrices(self):
        a=FROZEN_ADOBE_HDR_DNG_REWRITE_V11
        self.assertTrue(a.noise_profile_exact)
        self.assertTrue(a.opcode_list2_exact)
        self.assertTrue(a.opcode_list3_exact)
        self.assertGreater(a.color_matrix1_max_abs_delta,0.02)
        self.assertGreater(a.color_matrix2_max_abs_delta,0.2)
        self.assertFalse(a.rewritten_metadata_may_replace_original_calibration_authority)

    def test_classification_is_sample_exact_but_metadata_not_source_exact(self):
        self.assertEqual(FROZEN_ADOBE_HDR_DNG_REWRITE_V11.classification,
            'ADOBE_HDR_DNG_REWRITE_CFA_SAMPLE_EXACT_METADATA_NOT_SOURCE_EXACT')

    def test_any_cfa_change_revokes_measured_authority_transfer(self):
        a=FROZEN_ADOBE_HDR_DNG_REWRITE_V11
        b=AdobeHdrDngRewriteAuditV11(**{**a.__dict__,'raw_mismatch_count':1,'raw_max_abs_difference':1})
        self.assertFalse(b.cfa_sample_exact)
        self.assertFalse(b.measured_cfa_authority_may_be_preserved)
        self.assertIn('CFA_CHANGED',b.classification)

if __name__=='__main__': unittest.main()
