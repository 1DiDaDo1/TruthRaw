#include "truthraw/ptc.h"
#include <iostream>
using namespace truthraw::ptc;
int main(){Evidence e; e.scope=Scope::FullPhysical;e.sourceHash=e.sourceClass=e.cfa=e.black=e.white=e.colorTransform=e.sourceIdentity=true;e.gainMapPresent=true;e.gainMapApplicationCount=1;e.backendName=e.backendVersion=e.backendHash=e.backendBound=true;e.masterHash=true;e.exportFormat=e.mediaPayloadHash=e.metadataEmbedded=e.copyrightEmbedded=true;e.noiseModelBound=e.backendBoundUncertainty=e.perLensColorCalibrated=e.illuminantCalibrated=e.electronCalibrationBound=e.opticsCalibrationBound=true;auto r=evaluate(e);std::cout<<statusName(r.status)<<"\n";return r.status==Status::PureTruthCertified?0:1;}
