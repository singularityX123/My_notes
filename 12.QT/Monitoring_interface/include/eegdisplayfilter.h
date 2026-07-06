#ifndef EEGDISPLAYFILTER_H
#define EEGDISPLAYFILTER_H

// ============================================================================
// EEGDisplayFilter — 8-30 Hz Butterworth 双二阶带通 (HP+LP级联)
// ============================================================================

class EEGDisplayFilter {
public:
    struct Stage { double b0=1,b1=0,b2=0, a1=0,a2=0, z1=0,z2=0; };

    void reset();
    void configure(int sampleRateHz);
    double filter(double sample);

private:
    Stage m_hp, m_lp;
    int m_sampleRate = 0;
};

#endif

