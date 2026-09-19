#!/usr/bin/env python3
import json
import sys
from pathlib import Path

EXPECTED = {
    "source": "7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67",
    "cfa": "883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c",
    "master": "a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640",
    "authority": "7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098",
}

def validate(data):
    assert data["schema"] == "TruthNegativeRealSourceMasterBinding/1.0"
    assert data["sourceEvidence"]["sha256"] == EXPECTED["source"]
    assert data["sourceEvidence"]["decodedCfa"]["sha256"] == EXPECTED["cfa"]
    assert data["reconstructedHouse"]["scientificMaster"]["sha256"] == EXPECTED["master"]
    assert data["reconstructedHouse"]["dynamicAuthority"]["sha256"] == EXPECTED["authority"]
    assert data["truthNegativeCore"]["bindsExistingHouse"] is True
    assert data["truthNegativeCore"]["createsSecondScientificWorld"] is False
    assert data["truthNegativeCore"]["createsNewEvidence"] is False
    assert data["sensorNegativeProjection"]["generated"] is False
    assert data["sensorNegativeProjection"]["impliesPhysicalSensorGeometry"] is False
    assert data["evidenceCounts"] == {
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
    }
    assert data["reconstructedHouse"]["scientificMaster"]["sampleEncoding"] == "IEEE754_BINARY32"
    assert data["precision"]["branchSensitiveReconstructionCompute"] == "F64_WHERE_REQUIRED"

if __name__ == "__main__":
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(
        "state/TRUTHNEGATIVE_TN1_REAL_SOURCE_MASTER_BINDING_2026-09-19.json"
    )
    validate(json.loads(path.read_text()))
    print("TruthNegative TN-1 frozen source/master binding PASS")
