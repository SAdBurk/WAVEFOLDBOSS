#pragma once
#include <array>
#include <memory>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// Chunky sticker-style knobs and pill-shaped value boxes
class PinkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PinkLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
};

//==============================================================================
// Live "CRT" display: transfer curve on the left, folded sine on the right.
class FoldScreen : public juce::Component, private juce::Timer
{
public:
    explicit FoldScreen (juce::AudioProcessorValueTreeState&);
    ~FoldScreen() override { stopTimer(); }
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    std::atomic<float>* driveP;
    std::atomic<float>* stagesP;
    std::atomic<float>* shapeP;
    std::atomic<float>* symmetryP;
    float phase = 0.0f;
};

//==============================================================================
class WaveFoldingBossEditor : public juce::AudioProcessorEditor
{
public:
    explicit WaveFoldingBossEditor (WaveFoldingBossProcessor&);
    ~WaveFoldingBossEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    PinkLookAndFeel laf;
    WaveFoldingBossProcessor& proc;
    FoldScreen screen;

    static constexpr int numKnobs = 6;
    std::array<juce::Slider, numKnobs> sliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    juce::Image background;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveFoldingBossEditor)
};
