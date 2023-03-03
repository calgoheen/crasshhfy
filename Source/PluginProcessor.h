#pragma once

#include <JuceHeader.h>

class Sound : public juce::SynthesiserSound
{
public:
    Sound() = default;
    ~Sound() override = default;

    void setSampleData(juce::AudioBuffer<float>&& sampleData)
    {
        _data = std::move(sampleData);
    }

    const juce::AudioBuffer<float>* getSampleData() const
    {
        return &_data;
    }

    void setMidiNote(int midiNoteNumber)
    {
        _midiNote = midiNoteNumber;
    }

    int getMidiNote() const
    {
        return _midiNote;
    }

    void setEnvelopeParameters(const juce::ADSR::Parameters& params)
    {
        _envelopeParameters = params;
    }

    const juce::ADSR::Parameters& getEnvelopeParameters() const
    {
        return _envelopeParameters;
    }

    bool appliesToNote(int midiNoteNumber) override
    {
        return midiNoteNumber == _midiNote;
    }

    bool appliesToChannel(int midiChannel) override
    {
        return true;
    }

private:
    juce::AudioBuffer<float> _data;

    int _midiNote{ -1 };
    juce::ADSR::Parameters _envelopeParameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sound)
};

class Voice : public juce::SynthesiserVoice
{
public:
    Voice() = default;
    ~Voice() override = default;

    bool canPlaySound(SynthesiserSound* s) override
    {
        return dynamic_cast<Sound*>(s) != nullptr;
    }

    void startNote(int, float velocity, SynthesiserSound* sound, int) override
    {
        auto s = reinterpret_cast<Sound*>(sound);
        _sample = s->getSampleData();

        _adsr.setParameters(s->getEnvelopeParameters());
        _adsr.reset();
    }

    void stopNote(float velocity, bool allowTailOff) override
    {
        
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

private:
    const juce::AudioBuffer<float>* _sample{ nullptr };
    juce::ADSR _adsr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Voice)
};

class Text2SampleAudioProcessor : public juce::AudioProcessor
{
public:
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
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState _parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Text2SampleAudioProcessor)
};
