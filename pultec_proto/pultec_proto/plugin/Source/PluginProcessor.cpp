#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr std::array<double, 4> kLfFreqs {20.0, 30.0, 60.0, 100.0};
    constexpr std::array<double, 7> kHfFreqs {3000.0, 4000.0, 5000.0, 8000.0, 10000.0, 12000.0, 16000.0};
    constexpr std::array<double, 3> kHfCutFreqs {5000.0, 10000.0, 20000.0};
    constexpr int kOsFactor[3] {2, 4, 8};
}

juce::AudioProcessorValueTreeState::ParameterLayout PultecProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto knob = [&] (const char* id, const char* name, float def)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID {id, 1}, name,
                     NormalisableRange<float> (0.f, 10.f, 0.1f), def));
    };
    auto choice = [&] (const char* id, const char* name, StringArray items, int def)
    {
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID {id, 1}, name, items, def));
    };
    auto gain = [&] (const char* id, const char* name)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID {id, 1}, name,
                     NormalisableRange<float> (-12.f, 12.f, 0.1f), 0.f));
    };

    choice ("lf_freq", "LF Frequency", {"20 Hz", "30 Hz", "60 Hz", "100 Hz"}, 2);
    knob   ("lf_boost", "LF Boost", 0.f);
    knob   ("lf_cut", "LF Atten", 0.f);
    choice ("hf_freq", "HF Boost Freq", {"3 kHz", "4 kHz", "5 kHz", "8 kHz", "10 kHz", "12 kHz", "16 kHz"}, 4);
    knob   ("hf_boost", "HF Boost", 0.f);
    knob   ("hf_bw", "HF Bandwidth (0 sharp)", 5.f);
    choice ("hf_cut_freq", "HF Atten Freq", {"5 kHz", "10 kHz", "20 kHz"}, 1);
    knob   ("hf_cut", "HF Atten", 0.f);
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID {"eq_in", 1}, "EQ In", true));
    gain ("in_gain", "Input Gain (dB)");
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID {"sat", 1}, "Saturation",
                 NormalisableRange<float> (0.f, 6.f, 0.01f, 0.5f), 2.f));
    gain ("out_gain", "Output Gain (dB)");
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID {"auto_gain", 1}, "Auto Gain", false));
    choice ("os", "Oversampling", {"2x", "4x", "8x"}, 1);
    return {p.begin(), p.end()};
}

PultecProcessor::PultecProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool PultecProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void PultecProcessor::prepareToPlay (double sr, int blockSize)
{
    sampleRate = sr;
    for (int i = 0; i < 3; ++i)
    {
        oversamplers[(size_t) i] = std::make_unique<juce::dsp::Oversampling<float>> (
            (size_t) kMaxCh, (size_t) (i + 1),
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversamplers[(size_t) i]->initProcessing ((size_t) blockSize);
    }
    osIndex = juce::jlimit (0, 2, (int) raw ("os"));
    setLatencySamples (juce::roundToInt (oversamplers[(size_t) osIndex]->getLatencyInSamples()));

    for (auto* s : {&smLfBoost, &smLfCut, &smHfBoost, &smHfBw, &smHfCut, &smInGain, &smSat, &smOutGain})
        s->reset (sr, 0.02);
    smLfBoost.setCurrentAndTargetValue (raw ("lf_boost"));
    smLfCut.setCurrentAndTargetValue (raw ("lf_cut"));
    smHfBoost.setCurrentAndTargetValue (raw ("hf_boost"));
    smHfBw.setCurrentAndTargetValue (raw ("hf_bw"));
    smHfCut.setCurrentAndTargetValue (raw ("hf_cut"));
    smInGain.setCurrentAndTargetValue (raw ("in_gain"));
    smSat.setCurrentAndTargetValue (raw ("sat"));
    smOutGain.setCurrentAndTargetValue (raw ("out_gain"));

    haveParams = false;
    lastSat = -1.0;
    for (auto& ch : eqState) for (auto& s : ch) s = {};
    for (auto& x : xfmr) x = {};
}

void PultecProcessor::redesign (const pultec::Params& p, double fsOs)
{
    auto set = pultec::design (p);
    for (size_t i = 0; i < 5; ++i)
    {
        active[i] = set[i].active;
        coeffs[i] = pultec::toDigital (set[i], fsOs);
    }
    compGain = std::pow (10.0, pultec::loudnessCompDb (p) / 20.0);
}

void PultecProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = juce::jmin (kMaxCh, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (nCh == 0 || n == 0 || oversamplers[0] == nullptr) return;

    // ---- parameters (block rate) ----
    const int newOs = juce::jlimit (0, 2, (int) raw ("os"));
    if (newOs != osIndex)
    {
        osIndex = newOs;
        oversamplers[(size_t) osIndex]->reset();
        setLatencySamples (juce::roundToInt (oversamplers[(size_t) osIndex]->getLatencyInSamples()));
        haveParams = false;
        for (auto& ch : eqState) for (auto& s : ch) s = {};
    }
    smLfBoost.setTargetValue (raw ("lf_boost")); smLfBoost.skip (n);
    smLfCut.setTargetValue (raw ("lf_cut"));     smLfCut.skip (n);
    smHfBoost.setTargetValue (raw ("hf_boost")); smHfBoost.skip (n);
    smHfBw.setTargetValue (raw ("hf_bw"));       smHfBw.skip (n);
    smHfCut.setTargetValue (raw ("hf_cut"));     smHfCut.skip (n);
    smInGain.setTargetValue (raw ("in_gain"));   smInGain.skip (n);
    smSat.setTargetValue (raw ("sat"));          smSat.skip (n);
    smOutGain.setTargetValue (raw ("out_gain")); smOutGain.skip (n);

    pultec::Params p;
    p.lfFreq = kLfFreqs[(size_t) juce::jlimit (0, 3, (int) raw ("lf_freq"))];
    p.hfFreq = kHfFreqs[(size_t) juce::jlimit (0, 6, (int) raw ("hf_freq"))];
    p.hfCutFreq = kHfCutFreqs[(size_t) juce::jlimit (0, 2, (int) raw ("hf_cut_freq"))];
    p.lfBoost = smLfBoost.getCurrentValue();
    p.lfCut = smLfCut.getCurrentValue();
    p.hfBoost = smHfBoost.getCurrentValue();
    p.hfBw = smHfBw.getCurrentValue();
    p.hfCut = smHfCut.getCurrentValue();

    const double fsOs = sampleRate * kOsFactor[osIndex];
    auto differs = [] (const pultec::Params& a, const pultec::Params& b)
    {
        return a.lfFreq != b.lfFreq || a.lfBoost != b.lfBoost || a.lfCut != b.lfCut
            || a.hfFreq != b.hfFreq || a.hfBoost != b.hfBoost || a.hfBw != b.hfBw
            || a.hfCutFreq != b.hfCutFreq || a.hfCut != b.hfCut;
    };
    if (! haveParams || differs (p, lastParams))
    {
        redesign (p, fsOs);
        lastParams = p;
        haveParams = true;
    }

    const bool eqIn = raw ("eq_in") > 0.5f;
    const double sat = smSat.getCurrentValue();
    if (sat != lastSat)
    {
        for (auto& x : xfmr) x.prepare (fsOs, sat);
        lastSat = sat;
    }
    const float inGain = juce::Decibels::decibelsToGain (smInGain.getCurrentValue());
    float outGain = juce::Decibels::decibelsToGain (smOutGain.getCurrentValue());
    if (raw ("auto_gain") > 0.5f && eqIn) outGain *= (float) compGain;

    // ---- processing ----
    buffer.applyGain (inGain);
    {
        const float db = juce::Decibels::gainToDecibels (buffer.getMagnitude (0, n), -80.0f);
        const float target = juce::jlimit (0.0f, 1.0f, (db + 30.0f) / 30.0f);
        driveLevel.store (juce::jmax (target, driveLevel.load() * 0.85f));
    }
    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) nCh);
    auto& os = *oversamplers[(size_t) osIndex];
    auto up = os.processSamplesUp (sub);
    const size_t nUp = up.getNumSamples();
    const double k = pultec::kTubeKCal * sat;

    for (int ch = 0; ch < nCh; ++ch)
    {
        float* d = up.getChannelPointer ((size_t) ch);
        auto& st = eqState[(size_t) ch];
        for (size_t i = 0; i < nUp; ++i)
        {
            double v = d[i];
            if (eqIn)
                for (size_t s = 0; s < 5; ++s)
                    if (active[s]) v = st[s].process (coeffs[s], v);
            if (sat > 0.0)
            {
                v = pultec::tube (v, k);
                v = xfmr[(size_t) ch].process (v);
            }
            d[i] = (float) v;
        }
    }
    os.processSamplesDown (sub);
    buffer.applyGain (outGain);
}

juce::AudioProcessorEditor* PultecProcessor::createEditor()
{
    return new PultecEditor (*this);
}

void PultecProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void PultecProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PultecProcessor();
}
