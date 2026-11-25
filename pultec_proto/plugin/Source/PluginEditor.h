#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>
#include "PluginProcessor.h"

// Design space: 2122 x 505 units (panel of docs/ui_reference.png). Everything scales from it.
namespace ui { constexpr float kW = 2122.0f, kH = 505.0f; }

class PultecLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class DriveLamp : public juce::Component
{
public:
    void setLevel (float l) { if (std::abs (l - level) > 0.004f) { level = l; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    float level = 0.f;
};

class PultecEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PultecEditor (PultecProcessor&);
    ~PultecEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;   // right-click: window size menu

private:
    void timerCallback() override;
    void renderBackground();

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void knob (juce::Slider&, const char* id, std::unique_ptr<SA>&);
    void lever (juce::Slider&, const char* id, float a0, float a1, std::unique_ptr<SA>&);
    void place (juce::Component&, float cx, float cy, float halfW, float halfH);

    PultecProcessor& proc;
    PultecLookAndFeel laf;
    juce::Image background;

    juce::Slider lfBoost, lfCut, hfBoost, hfCut, hfBw, inGain, outGain, sat;
    juce::Slider lfFreq, hfFreq, hfCutFreq, osSel;
    juce::ToggleButton eqIn, autoGain;
    DriveLamp lamp;

    std::unique_ptr<SA> aLfBoost, aLfCut, aHfBoost, aHfCut, aHfBw, aIn, aOut, aSat,
                        aLfFreq, aHfFreq, aHfCutFreq, aOs;
    std::unique_ptr<BA> aEq, aAuto;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PultecEditor)
};
