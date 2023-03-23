#pragma once

#include <JuceHeader.h>
#include "Sampler.h"
#include "Utilities.h"
#include "UnetModelInference.h"
#include "ClassifierModelInference.h"

class Text2SampleAudioProcessor : public juce::AudioProcessor
{
public:
    const int numSounds = 4;
    const int numVoices = 8;
    const int baseMidiNote = 60;

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
    juce::MidiKeyboardState& getMidiKeyboardState();
    DrumSound* getSound(int soundIndex);

    void saveSample(int soundIndex, const juce::File& file);

    void generateSample(int soundIndex);
    void drumifySample(int soundIndex, const juce::File& file);
    void inpaintSample(int soundIndex, const juce::File& file, bool half);

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
    Drum renderCRASHSample();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState _parameters;

    juce::Synthesiser _synth;
    std::vector<DrumSound*> _sounds;
    std::vector<Voice*> _voices;

    UnetModelInference unetModelInference;
    ClassifierModelInference classifierModelInference;

    juce::MidiKeyboardState _midiState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Text2SampleAudioProcessor)
};
