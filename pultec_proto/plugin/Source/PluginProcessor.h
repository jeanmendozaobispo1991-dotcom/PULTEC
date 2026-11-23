#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include "PultecDsp.h"

class PultecProcessor : public juce::AudioProcessor
{
public:
    PultecProcessor();
    ~PultecProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> driveLevel { 0.0f };      // 0..1, drives the editor lamp

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float raw (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void redesign (const pultec::Params&, double fsOs);

    static constexpr int kMaxCh = 2;
    double sampleRate = 48000.0;
    int osIndex = 1;                       // 0=2x, 1=4x, 2=8x
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;

    // block-rate smoothing of the 0..10 knobs and gains
    juce::SmoothedValue<float> smLfBoost, smLfCut, smHfBoost, smHfBw, smHfCut,
                               smInGain, smSat, smOutGain;

    pultec::Params lastParams;
    bool haveParams = false;
    std::array<pultec::Biquad, 5> coeffs {};
    std::array<bool, 5> active {};
    double compGain = 1.0;
    double lastSat = -1.0;
    std::array<std::array<pultec::BiquadState, 5>, kMaxCh> eqState {};
    std::array<pultec::Transformer, kMaxCh> xfmr {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PultecProcessor)
};
