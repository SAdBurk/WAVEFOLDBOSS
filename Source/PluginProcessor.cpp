#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FoldMath.h"

juce::AudioProcessorValueTreeState::ParameterLayout WaveFoldingBossProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { wfb::ids::drive, 1 }, "Fold",
                                                       NormalisableRange<float> (0.0f, 40.0f, 0.01f), 18.0f));
    layout.add (std::make_unique<AudioParameterInt>   (ParameterID { wfb::ids::stages, 1 }, "Stages", 1, 4, 2));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { wfb::ids::shape, 1 }, "Shape",
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { wfb::ids::symmetry, 1 }, "Symmetry",
                                                       NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { wfb::ids::mix, 1 }, "Mix",
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { wfb::ids::output, 1 }, "Output",
                                                       NormalisableRange<float> (-24.0f, 6.0f, 0.01f), -3.0f));
    return layout;
}

WaveFoldingBossProcessor::WaveFoldingBossProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    driveP    = apvts.getRawParameterValue (wfb::ids::drive);
    stagesP   = apvts.getRawParameterValue (wfb::ids::stages);
    shapeP    = apvts.getRawParameterValue (wfb::ids::shape);
    symmetryP = apvts.getRawParameterValue (wfb::ids::symmetry);
    mixP      = apvts.getRawParameterValue (wfb::ids::mix);
    outputP   = apvts.getRawParameterValue (wfb::ids::output);
}

bool WaveFoldingBossProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void WaveFoldingBossProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    maxBlock = juce::jmax (1, samplesPerBlock);
    const double osRate = sampleRate * (double) (1 << oversamplingOrder);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 2 };

    oversampling.initProcessing ((size_t) maxBlock);
    oversampling.reset();

    mixer.prepare (spec);
    mixer.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    mixer.setWetLatency (oversampling.getLatencyInSamples());
    setLatencySamples (juce::roundToInt (oversampling.getLatencyInSamples()));

    driveSm.reset (osRate, 0.02);
    biasSm.reset  (osRate, 0.02);
    shapeSm.reset (osRate, 0.02);
    outSm.reset   (osRate, 0.02);
    driveSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (driveP->load()));
    biasSm.setCurrentAndTargetValue  (symmetryP->load());
    shapeSm.setCurrentAndTargetValue (shapeP->load());
    outSm.setCurrentAndTargetValue   (juce::Decibels::decibelsToGain (outputP->load()));

    // 10 Hz DC blocker (asymmetric folding creates DC offset)
    dcR = std::exp (-juce::MathConstants<float>::twoPi * 10.0f / (float) osRate);
    dcX1[0] = dcX1[1] = dcY1[0] = dcY1[1] = 0.0f;
}

void WaveFoldingBossProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block (buffer);

    // Hosts may hand us more than the block size announced in prepareToPlay.
    for (size_t start = 0; start < block.getNumSamples(); start += (size_t) maxBlock)
    {
        const auto len = juce::jmin ((size_t) maxBlock, block.getNumSamples() - start);
        processChunk (block.getSubBlock (start, len));
    }
}

void WaveFoldingBossProcessor::processChunk (juce::dsp::AudioBlock<float> block)
{
    mixer.setWetMixProportion (mixP->load());
    mixer.pushDrySamples (block);

    auto os = oversampling.processSamplesUp (block);

    driveSm.setTargetValue (juce::Decibels::decibelsToGain (driveP->load()));
    biasSm.setTargetValue  (symmetryP->load());
    shapeSm.setTargetValue (shapeP->load());
    outSm.setTargetValue   (juce::Decibels::decibelsToGain (outputP->load()));
    const int stages = juce::jlimit (1, 4, (int) std::lround (stagesP->load()));

    const auto numSamples  = os.getNumSamples();
    const auto numChannels = juce::jmin ((size_t) 2, os.getNumChannels());

    for (size_t i = 0; i < numSamples; ++i)
    {
        const float d  = driveSm.getNextValue();
        const float b  = biasSm.getNextValue();
        const float sh = shapeSm.getNextValue();
        const float g  = outSm.getNextValue();

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* x = os.getChannelPointer (ch);
            const float y = wfb::foldChain (x[i], d, b, stages, sh);

            const float o = y - dcX1[ch] + dcR * dcY1[ch];
            dcX1[ch] = y;
            dcY1[ch] = o;

            x[i] = o * g;
        }
    }

    oversampling.processSamplesDown (block);
    mixer.mixWetSamples (block);
}

juce::AudioProcessorEditor* WaveFoldingBossProcessor::createEditor()
{
    return new WaveFoldingBossEditor (*this);
}

void WaveFoldingBossProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void WaveFoldingBossProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveFoldingBossProcessor();
}
