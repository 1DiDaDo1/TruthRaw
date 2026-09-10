#include "streaming_test_support_v0_1.h"

int main(){
    const int w=66,h=50;auto f=make_frame(w,h);
    auto recon=std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance=std::make_shared<SkinSafeDetailedCrispAppearance>();

    ProcessOptions co;co.tile={16,7};co.threads=1;co.hdrEnabled=true;co.appearance=AppearanceProfile::SkinSafeDetailedCrisp;co.keepScientificDiagnostics=true;co.sdrLutSize=4096;
    ProcessResult canonical;TruthRawProcessor cp(recon,appearance);auto cs=cp.processFrame(f,co,canonical);REQUIRE(cs);

    FrameSource source(f);CollectSink sink(w,h,true);
    StreamingOptions so;so.tile={16,7};so.workers=1;so.hdrEnabled=true;so.streamScientificDiagnostics=true;so.sdrLutSize=4096;so.memoryBudgetBytes=0;
    StreamingResult sr;StreamingTruthRawProcessor sp(recon,appearance);auto ss=sp.process(source,sink,so,sr);REQUIRE(ss);REQUIRE(sink.finished());

    compare_exposure(canonical.exposure,sr.exposure);
    const float sdrDiff=max_abs_diff(canonical.sdrRgb,sink.sdr());
    const float gainDiff=max_abs_diff(canonical.halfLogGain,sink.gain());
    const float diagDiff=max_abs_diff(canonical.stage2Diagnostic,sink.diagnostic());
    REQUIRE(sdrDiff<=2e-6f);REQUIRE(gainDiff<=2e-6f);REQUIRE(diagDiff<=1e-7f);
    REQUIRE(sr.stage2Over1Count==canonical.exposure.stage2Over1Count);
    REQUIRE(sr.provenance.physicalFrameCount==1&&sr.provenance.independentEvidenceCount==1);
    REQUIRE(!sr.memory.adapterOwnsFullRawFrame&&!sr.memory.adapterOwnsFullSdrFrame&&!sr.memory.adapterOwnsFullHalfGainFrame&&!sr.memory.adapterOwnsFullDiagnosticFrame);
    REQUIRE(sr.tilesProcessedPass1==sr.tilesProcessedPass2&&sr.tilesProcessedPass1>1);

    DngMetadata big=f.meta;big.width=16320;big.height=12288;big.hasResidualBlack=false;big.hasGainField=true;
    StreamingOptions low=so;low.tile={64,7};low.streamScientificDiagnostics=false;low.memoryBudgetBytes=8u*1024u*1024u;
    StreamingPlan pBig,pSmall;auto ps=plan_streaming_frame(big,low,*recon,*appearance,256u*1024u,256u*1024u,pBig);REQUIRE(ps);
    DngMetadata small=big;small.width=4000;small.height=3000;ps=plan_streaming_frame(small,low,*recon,*appearance,256u*1024u,256u*1024u,pSmall);REQUIRE(ps);
    REQUIRE(pBig.logicalWorkspaceUpperBound==pSmall.logicalWorkspaceUpperBound);
    REQUIRE(pBig.logicalResidentUpperBound<8u*1024u*1024u);
    REQUIRE(pBig.tileCount>pSmall.tileCount);

    auto bad=low;bad.tile.core=63;StreamingPlan pb;auto bs=plan_streaming_frame(big,bad,*recon,*appearance,0,0,pb);REQUIRE(!bs&&bs.code==StreamStatusCode::InvalidArgument);
    bad=low;bad.workers=2;bs=plan_streaming_frame(big,bad,*recon,*appearance,0,0,pb);REQUIRE(!bs&&bs.code==StreamStatusCode::InvalidArgument);
    bad=low;bad.memoryBudgetBytes=1024;bs=plan_streaming_frame(big,bad,*recon,*appearance,0,0,pb);REQUIRE(!bs&&bs.code==StreamStatusCode::BudgetExceeded);

    std::cout<<"FULL_FRAME_STREAMING_V0_1_PASS\n";
    std::cout<<"equivalence_sdr_max_abs="<<sdrDiff<<"\n";
    std::cout<<"equivalence_half_gain_max_abs="<<gainDiff<<"\n";
    std::cout<<"equivalence_stage2_diag_max_abs="<<diagDiff<<"\n";
    std::cout<<"synthetic_tiles_pass1="<<sr.tilesProcessedPass1<<"\n";
    std::cout<<"big_frame_tiles="<<pBig.tileCount<<"\n";
    std::cout<<"small_frame_tiles="<<pSmall.tileCount<<"\n";
    std::cout<<"low_end_workspace_bytes="<<pBig.logicalWorkspaceUpperBound<<"\n";
    std::cout<<"low_end_resident_bound_bytes="<<pBig.logicalResidentUpperBound<<"\n";
    std::cout<<"adapter_full_frame_buffers=0\n";
}
