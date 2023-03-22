#pragma once

#include <JuceHeader.h>
#include "Fifo.h"
#include "Sample.h"
#include "Utilities.h"

class Sound : public juce::SynthesiserSound
{
public:
    static constexpr int maxNumChannels{ 2 };

    Sound(int midiNote) : _midiNote(midiNote) {}
    ~Sound() override = default;

    int getMidiNote() const;

    void setSampleRate(double newRate);
    double getSampleRate() const;

    void setSample(Sample::Ptr sample);
    Sample::Ptr getSample() const;
    const juce::AudioBuffer<float>& getSampleData() const;
    void clearSample();
    bool isEmpty() const;
    void setFadeLength(double lengthInSeconds);

    void setGain(double gain);
    double getGain() const;

    void setPan(double pan);
    double getPan() const;

    void setPitch(double pitch);
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

    double _gain{ 1.0f };
    double _pan{ 0.5f };
    double _pitchSemitones{ 0.0f };
    juce::ADSR::Parameters _envelope{ 0.002f, 0.1f, 1.0f, 1.0f };

    Sample::Ptr _source, _current, _prev;

    double _fadeLength{ 3e-3 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sound)
};

class SoundWithParameters : public Sound
{
public:
    enum Parameters
    {
        kGain = 0,
        kPan,
        kPitch,
        kAttack,
        kDecay,
        kSustain,
        kRelease,
        kNumParameters
    };

    SoundWithParameters(int midiNote);
    ~SoundWithParameters() override = default;

    juce::RangedAudioParameter* getParameter(int index);

private:
    void initializeParameters();

    DummyProcessor _dummyProcessor;
    juce::RangedAudioParameter* _parameters[kNumParameters];
    juce::OwnedArray<ParameterListener> _listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundWithParameters)
};

class DrumSound : public SoundWithParameters
{
public:
    DrumSound(int midiNote) : SoundWithParameters(midiNote) {}
    ~DrumSound() override = default;

    std::function<void()> drumChanged = nullptr;

    void loadDrum(Drum d);
    DrumType getDrumType() const;
    float getConfidence() const;

private:
    DrumType _drumType{ DrumType::kick };
    float _confidence{ 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSound)
};

class Voice : public juce::SynthesiserVoice
{
public:
    Voice();
    ~Voice() override = default;

    bool canPlaySound(juce::SynthesiserSound* s) override;

    void startNote(int, float velocity, juce::SynthesiserSound* sound, int) override;
    void stopNote(float velocity, bool allowTailOff) override;
    bool isNoteOn() const;

    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override;

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

private:
    typedef std::array<float, 2> StereoSample;
    static StereoSample readFromSample(const juce::AudioBuffer<float>& data, float fractionalIndex);
    StereoSample getNextSample();
    
    juce::ADSR _adsr;
    bool _noteIsOn{ false };

    const Sound* _sound{ nullptr };
    double _pitchRatio{ 1.0 };
    double _currentIdx{ 0.0 };

    float _gain{ 1.0f };
    float _pan{ 0.5f };

    const int _numFifoSamples{ 512 };
    Fifo _fifo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Voice)
};
