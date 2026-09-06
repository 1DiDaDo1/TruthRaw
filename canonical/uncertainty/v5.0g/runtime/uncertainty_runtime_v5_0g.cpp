#include "uncertainty_runtime_v5_0g.h"
#include <algorithm>
#include <cmath>
namespace truthraw {
static constexpr std::array<float,18> C={0.395601829f, 1.01616829f, -0.14277533f, 0.735921985f, -0.230914241f, 0.0217581422f, -0.100941628f, -0.00342952311f, 0.0824666634f, 0.0266935733f, 0.485900593f, 0.0192627418f, 0.281847627f, 0.519084282f, 0.0425594193f, -0.0499333677f, -0.0365210334f, 0.0438949818f};
static constexpr float I=-9.85952204f;
static constexpr float E=2e-05f;
static constexpr float Q50[4][5]={{1.23186568f, 1.29081367f, 1.250442f, 1.23057982f, 1.25909627f}, {1.64072784f, 1.40598901f, 1.35229132f, 1.32266775f, 1.28092143f}, {1.64228642f, 1.39439067f, 1.35299496f, 1.32419466f, 1.23851723f}, {1.29139635f, 1.27813684f, 1.25189713f, 1.23929701f, 1.20947405f}};
static constexpr float Q95[4][5]={{3.54679684f, 3.79396531f, 3.62211433f, 3.64434517f, 3.62210633f}, {4.76397741f, 4.27615232f, 3.96592525f, 3.83541287f, 3.83764344f}, {4.73872316f, 4.22170938f, 3.92232166f, 3.81420021f, 3.76353938f}, {3.71949947f, 3.82415852f, 3.62994847f, 3.56435722f, 3.56799556f}};
static int sb(float s){return s<2?0:s<4?1:s<8?2:s<16?3:4;}
UncertaintyBands predict_uncertainty_v5_0g(const std::array<float,18>& x,int role,float snr){float z=I;for(size_t i=0;i<C.size();++i)z+=C[i]*x[i];float mu=std::max(std::exp(z)-E,1e-6f);int r=std::max(0,std::min(3,role)),b=sb(snr);float p50=std::max(mu*Q50[r][b],1e-8f);float p95=std::max(mu*Q95[r][b],p50+1e-8f);return {p50,p95};}
}
