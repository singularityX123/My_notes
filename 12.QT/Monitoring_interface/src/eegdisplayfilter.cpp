#include "eegdisplayfilter.h"
#include <cmath>

namespace {
constexpr double kQ = 1.0 / std::sqrt(2.0);

struct Coeffs { double b0,b1,b2, a1,a2; };

Coeffs makeLowPass(double fc, double fs) {
    double w=2.0*M_PI*fc/fs, sw=std::sin(w), cw=std::cos(w), alpha=sw/(2.0*kQ), a0=1.0+alpha;
    return {((1.0-cw)*0.5)/a0, (1.0-cw)/a0, ((1.0-cw)*0.5)/a0, (-2.0*cw)/a0, (1.0-alpha)/a0};
}
Coeffs makeHighPass(double fc, double fs) {
    double w=2.0*M_PI*fc/fs, sw=std::sin(w), cw=std::cos(w), alpha=sw/(2.0*kQ), a0=1.0+alpha;
    return {((1.0+cw)*0.5)/a0, (-(1.0+cw))/a0, ((1.0+cw)*0.5)/a0, (-2.0*cw)/a0, (1.0-alpha)/a0};
}

void applyStage(double in, double &out, EEGDisplayFilter::Stage &s) {
    out = s.b0*in + s.z1;
    s.z1 = s.b1*in - s.a1*out + s.z2;
    s.z2 = s.b2*in - s.a2*out;
}
}

void EEGDisplayFilter::reset() {
    m_hp = {}; m_lp = {}; m_sampleRate = 0;
}

void EEGDisplayFilter::configure(int sr) {
    if (sr <= 0 || sr == m_sampleRate) return;
    m_sampleRate = sr;
    double fs = static_cast<double>(sr);

    auto hp = makeHighPass(8.0, fs);
    m_hp.b0=hp.b0; m_hp.b1=hp.b1; m_hp.b2=hp.b2;
    m_hp.a1=hp.a1; m_hp.a2=hp.a2;
    m_hp.z1=0; m_hp.z2=0;

    auto lp = makeLowPass(30.0, fs);
    m_lp.b0=lp.b0; m_lp.b1=lp.b1; m_lp.b2=lp.b2;
    m_lp.a1=lp.a1; m_lp.a2=lp.a2;
    m_lp.z1=0; m_lp.z2=0;
}

double EEGDisplayFilter::filter(double sample) {
    double hpOut, lpOut;
    applyStage(sample, hpOut, m_hp);
    applyStage(hpOut,  lpOut, m_lp);
    return lpOut;
}

