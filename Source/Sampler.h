#pragma once

#include <JuceHeader.h>
#include "Fifo.h"

class Sound : public juce::SynthesiserSound
{
public:
    static constexpr int maxNumChannels{ 2 };

    Sound(int midiNote) : _midiNote(midiNote) {}
    ~Sound() override = default;

    void setSampleRate(double newRate);
    double getSampleRate() const;

    void setSample(juce::AudioBuffer<float>&& data, double sourceFs);
    const juce::AudioBuffer<float>& getSample() const;
    void clearSample();
    bool isEmpty() const;
    void setFadeLength(double lengthInSeconds);

    void setPitch(float pitchInSemitones);
    double getPitch() const;

    void setEnvelope(const juce::ADSR::Parameters& params);
    const juce::ADSR::Parameters& getEnvelope() const;

    bool appliesToNote(int midiNoteNumber) override;
    bool appliesToChannel(int midiChannel) override;

private:
    void updateCurrentSample();

    const int _midiNote;
    double _sampleRate{ 0.0 };
    double _sourceSampleRate{ 0.0 };

    double _pitchSemitones{ 0.0f };
    juce::ADSR::Parameters _envelope{ 0.002f, 0.1f, 1.0f, 1.0f };

    juce::AudioBuffer<float> _sourceData;
    juce::AudioBuffer<float> _currentData;

    double _fadeLength{ 3e-3 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sound)
};

class Voice : public juce::SynthesiserVoice
{
public:
    Voice();
    ~Voice() override = default;

    bool canPlaySound(juce::SynthesiserSound* s) override;

    void startNote(int, float velocity, juce::SynthesiserSound* sound, int) override;
    void stopNote(float velocity, bool allowTailOff) override;

    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override;

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

private:
    typedef std::array<float, 2> StereoSample;
    StereoSample readFromSample(float fractionalIndex) const;
    StereoSample getNextSample();
    
    juce::ADSR _adsr;

    const Sound* _sound{ nullptr };
    double _pitchRatio{ 1.0f };
    double _currentIdx{ 0.0f };

    const int _numFifoSamples{ 512 };
    Fifo _fifo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Voice)
};
