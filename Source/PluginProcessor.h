#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class WaveFoldingBossProcessor : public juce::AudioProcessor
{
public:
    WaveFoldingBossProcessor();
    ~WaveFoldingBossProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    void processChunk (juce::dsp::AudioBlock<float> block);

    static constexpr int oversamplingOrder = 2; // 2^2 = 4x oversampling
    juce::dsp::Oversampling<float> oversampling { 2, (size_t) oversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };
    juce::dsp::DryWetMixer<float> mixer;

    std::atomic<float>* driveP    = nullptr;
    std::atomic<float>* stagesP   = nullptr;
    std::atomic<float>* shapeP    = nullptr;
    std::atomic<float>* symmetryP = nullptr;
    std::atomic<float>* mixP      = nullptr;
    std::atomic<float>* outputP   = nullptr;

    juce::SmoothedValue<float> driveSm, biasSm, shapeSm, outSm;

    float dcR = 0.999f;
    float dcX1[2] { 0.f, 0.f };
    float dcY1[2] { 0.f, 0.f };
    int maxBlock = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveFoldingBossProcessor)
};
