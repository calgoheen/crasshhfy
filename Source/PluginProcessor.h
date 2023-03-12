#pragma once

#include <JuceHeader.h>
#include "Sampler.h"
#include "Utilities.h"
#include "CrashModelInference.h"

struct Limiter
{
    Limiter(juce::AudioBuffer<float>& data)
    {
        for (int i = 0; i < data.getNumSamples(); i++)
            for (int j = 0; j < data.getNumChannels(); j++)
                data.setSample(j, i, juce::jlimit(-1.0f, 1.0f, data.getSample(j, i)));
    }
};

class Text2SampleAudioProcessor : public juce::AudioProcessor
{
public:
    const int numSounds = 12;
    const int numVoices = 8;

    Text2SampleAudioProcessor();
    ~Text2SampleAudioProcessor() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void releaseResources() override;
    
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    void loadSample(int soundIndex, Sample::Ptr sample);
    void loadSampleFromFile(int soundIndex, const juce::File& file);
    void generateSample(int soundIndex);

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;

private:
    Sample::Ptr renderCRASHSample();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState _parameters;
    juce::Synthesiser _synth;

    CrashModelInference modelInference;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Text2SampleAudioProcessor)
};
