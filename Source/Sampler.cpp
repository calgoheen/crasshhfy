#include "Sampler.h"
#include <resample.h>
#include "Utilities.h"

int Sound::getMidiNote() const
{
    return _midiNote;
}

void Sound::setSampleRate(double newRate)
{
    if (newRate != _sampleRate)
    {
        _sampleRate = newRate;
        updateCurrentSample();
    }
}

double Sound::getSampleRate() const
{
    return _sampleRate;
}

void Sound::setSample(Sample::Ptr sample)
{
    _source = sample;
    updateCurrentSample();
}

Sample::Ptr Sound::getSample() const
{   
    // Might return nullptr
    return _current;
}

const juce::AudioBuffer<float>& Sound::getSampleData() const
{
    // This function should only be called when a sample is loaded
    jassert(!isEmpty());

    return _current->data;
}

void Sound::clearSample() 
{
    _prev = _current;
    _current.reset();
    _source.reset();
}

bool Sound::isEmpty() const
{
    return _current.get() == nullptr;
}

void Sound::setFadeLength(double lengthInSeconds)
{
    _fadeLength = lengthInSeconds;
    updateCurrentSample();
}

void Sound::setGain(double gain) 
{
    jassert(gain >= 0.0);
    _gain = gain;
}

double Sound::getGain() const 
{ 
    return _gain; 
}

void Sound::setPan(double pan) 
{ 
    jassert(pan >= 0.0 && pan <= 1.0); 
    _pan = pan; 
}

double Sound::getPan() const
{ 
    return _pan; 
}

void Sound::setPitch(double pitch) 
{ 
    _pitchSemitones = pitch; 
}

double Sound::getPitch() const
{
    return _pitchSemitones;
}

void Sound::setEnvelope(const juce::ADSR::Parameters& params) 
{ 
    _envelope = params; 
}

const juce::ADSR::Parameters& Sound::getEnvelope() const
{
    return _envelope;
}

bool Sound::appliesToNote(int midiNoteNumber)
{
    return midiNoteNumber == _midiNote;
}

bool Sound::appliesToChannel(int)
{
    return true;
}

void Sound::updateCurrentSample()
{
    if (_source == nullptr)
    {
        _prev = _current;
        _current = nullptr;
        return;
    }
    
    if (!(_sampleRate > 0.0 && _source->sampleRate > 0.0))
        return;

    if (_source->data.getNumSamples() == 0)
        return;

    auto resampled = r8b::resample(_source->data, _source->sampleRate, _sampleRate);
    auto numSamples = resampled.getNumSamples();

    // Apply fade
    auto fadeLengthSamples = juce::jmin(int(_fadeLength * _sampleRate), numSamples / 2);
    auto ptr = resampled.getArrayOfWritePointers();
    for (int j = 0; j < resampled.getNumChannels(); j++)
    {
        Utils::applyFade(ptr[j], 0, fadeLengthSamples, true);
        Utils::applyFade(ptr[j], numSamples - fadeLengthSamples, fadeLengthSamples, false);
    }

    Sample::Ptr next{ new Sample(std::move(resampled), _sampleRate) };
    _prev = _current;
    _current = next;
}


SoundWithParameters::SoundWithParameters(int midiNote) 
    : Sound(midiNote) 
{ 
    initializeParameters(); 
}

juce::RangedAudioParameter* SoundWithParameters::getParameter(int index)
{
    jassert(juce::isPositiveAndBelow(index, kNumParameters));
    return _parameters[index];
}

void SoundWithParameters::setSample(Sample::Ptr sample)
{
    Sound::setSample(sample);

    if (sampleChanged)
    {
        auto mm = juce::MessageManager::getInstance();

        if (mm->isThisTheMessageThread())
            sampleChanged();
        else
            mm->callAsync([this] { sampleChanged(); });
    }
}

void SoundWithParameters::initializeParameters()
{
    ParameterDefinition defs[kNumParameters] = {
        { "Gain",       "Gain",     "dB",   { -30.0f, 30.0f },              0.0f },
        { "Pan",        "Pan",      "%",    { -100.0f, 100.0f },            0.0f },
        { "Pitch",      "Pitch",    "st",   { -12.0f, 12.0f },              0.0f },
        { "Attack",     "Attack",   "s",    { 0.001f, 1.0f, 0.0f, 0.25f },  0.01f },
        { "Decay",      "Decay",    "s",    { 0.001f, 1.0f, 0.0f, 0.25f },  0.1f },
        { "Sustain",    "Sustain",  "%",    { 0.0f, 100.0f },               100.0f },
        { "Release",    "Release",  "s",    { 0.001f, 10.0f, 0.0f, 0.25f }, 0.1f }
    };

    std::function<void(float)> funcs[kNumParameters] = {
        [this](float x) { setGain(juce::Decibels::decibelsToGain(x)); },
        [this](float x) { setPan(juce::jmap(x, -100.0f, 100.0f, 0.0f, 1.0f)); },
        [this](float x) { setPitch(x); },
        [this](float x) { auto env = getEnvelope(); env.attack = x; setEnvelope(env); },
        [this](float x) { auto env = getEnvelope(); env.decay = x; setEnvelope(env); },
        [this](float x) { auto env = getEnvelope(); env.sustain = 0.01f * x; setEnvelope(env); },
        [this](float x) { auto env = getEnvelope(); env.release = x; setEnvelope(env); }
    };

    for (int i = 0; i < kNumParameters; i++)
    {
        auto& d = defs[i];
        auto p = new juce::AudioParameterFloat({ d.id, 1 }, d.name, d.range, d.defaultValue, d.label);

        _dummyProcessor.addParameter(p);
        _parameters[i] = p;
        _listeners.add(new ParameterListener{ *p, std::move(funcs[i]) });
    }
}


void DrumSound::loadDrum(Drum d)
{
    _drumType = d.drumType;
    _confidence = d.confidence;
    setSample(d.sample);

    if (drumChanged)
    {
        auto mm = juce::MessageManager::getInstance();

        if (mm->isThisTheMessageThread())
            drumChanged();
        else
            mm->callAsync([this] { drumChanged(); });
    }
}

DrumType DrumSound::getDrumType() const
{
    return _drumType;
}

float DrumSound::getConfidence() const
{
    return _confidence;
}


/*template <int k>
struct LagrangeResampleHelper
{
    static forcedinline void calc(float& a, float b) noexcept { a *= b * (1.0f / k); }
};

template <>
struct LagrangeResampleHelper<0>
{
    static forcedinline void calc(float&, float) noexcept {}
};

template <int k>
static float calcCoefficient(float input, float offset) noexcept
{
    LagrangeResampleHelper<0 - k>::calc(input, -2.0f - offset);
    LagrangeResampleHelper<1 - k>::calc(input, -1.0f - offset);
    LagrangeResampleHelper<2 - k>::calc(input, 0.0f - offset);
    LagrangeResampleHelper<3 - k>::calc(input, 1.0f - offset);
    LagrangeResampleHelper<4 - k>::calc(input, 2.0f - offset);
    return input;
}

static float getFractionalSampleFromBuffer(const float* buffer, float readIdx)
{
    int idx0 = std::floor(readIdx);
    int idx1 = idx0 - 1;
    int idx2 = idx0 - 2;
    int idx3 = idx0 - 3;
    int idx4 = idx0 - 4;

    float offset = readIdx - idx0;
    
    float x0 = idx4 < 0 ? 0.0f : buffer[idx4];
    float x1 = idx3 < 0 ? 0.0f : buffer[idx4];
    float x2 = idx2 < 0 ? 0.0f : buffer[idx4];
    float x3 = idx1 < 0 ? 0.0f : buffer[idx4];
    float x4 = idx0 < 0 ? 0.0f : buffer[idx4];

    float result = 0.0f;

    result += calcCoefficient<0>(x0, offset);
    result += calcCoefficient<1>(x1, offset);
    result += calcCoefficient<2>(x2, offset);
    result += calcCoefficient<3>(x3, offset);
    result += calcCoefficient<4>(x4, offset);
}*/

Voice::Voice()
{
    _fifo.prepare(2, _numFifoSamples);
}

bool Voice::canPlaySound(juce::SynthesiserSound* s)
{
    return dynamic_cast<Sound*>(s) != nullptr;
}

void Voice::startNote(int, float, juce::SynthesiserSound* sound, int)
{
    _sound = reinterpret_cast<Sound*>(sound);

    _noteIsOn = true;

    if (!_sound->isEmpty())
    {
        _adsr.setSampleRate(_sound->getSampleRate());
        _adsr.setParameters(_sound->getEnvelope());
        _adsr.noteOn();

        _pitchRatio = std::pow(2.0, _sound->getPitch() / 12.0);
        _currentIdx = 0.0;

        _gain = _sound->getGain();
        _pan = _sound->getPan();
    }
}

void Voice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        // Note off message received
        _noteIsOn = false;
        _adsr.noteOff();
    }
    else
    {
        // If the voice stops while the ADSR is active, then it was stolen and will be reused immediately
        if (_adsr.isActive())
        {
            // Fill fifo with tapered output from current note, if part of the sample is still available.
            auto& sample = _sound->getSampleData();
            auto numSamplesRemaining = sample.getNumSamples() - 1 - int(_currentIdx);
            auto numSamplesScaled = int(numSamplesRemaining / _pitchRatio);
            auto length = juce::jmin(numSamplesScaled, _numFifoSamples);

            if (length > 0)
            {
                for (int i = 0; i < length; i++)
                {
                    auto [l, r] = getNextSample();
                    auto g = float(length - i) / float(length);
                    l *= g;
                    r *= g;

                    // Adds any existing fifo data back in (this probably won't happen very often)
                    if (_fifo.getNumAvailableToRead(0) > i)
                    {
                        l += _fifo.read(0);
                        r += _fifo.read(1);
                    }

                    _fifo.write(0, l);
                    _fifo.write(1, r);
                }
            }
        }
        else
        {
            // Empty the fifo if the voice is not going to be reused
            while(_fifo.getNumAvailableToRead(0) > 0)
            {
                _fifo.read(0);
                _fifo.read(1);
            }
        }

        _adsr.reset();
        clearCurrentNote();
        _sound = nullptr;
    }
}

bool Voice::isNoteOn() const
{
    return _noteIsOn;
}

void Voice::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (_sound == nullptr)
        return;

    // Synth is stereo only
    jassert(buffer.getNumChannels() == 2);
    auto outL = buffer.getWritePointer(0, startSample);
    auto outR = buffer.getWritePointer(1, startSample);

    // Read from fifos if anything is available
    int numSamplesFifo = juce::jmin(numSamples, _fifo.getNumAvailableToRead(0));
    for (int i = 0; i < numSamplesFifo; i++)
    {
        outL[i] += _fifo.read(0);
        outR[i] += _fifo.read(1);
    }

    if (_sound->isEmpty())
        return;
    
    auto& sample = _sound->getSampleData();
    jassert(sample.getNumChannels() == 1 || sample.getNumChannels() == 2);

    for (int i = 0; i < numSamples; i++)
    {
        bool endOfSample = int(_currentIdx) >= sample.getNumSamples() - 1;

        if (endOfSample || !_adsr.isActive())
        {
            stopNote(0.0f, false);
            break;
        }

        // Write to output buffer
        auto [l, r] = getNextSample();

        outL[i] += l;
        outR[i] += r;
    }
}

Voice::StereoSample Voice::readFromSample(const juce::AudioBuffer<float>& data, float fractionalIndex)
{
    auto idx0 = int(fractionalIndex);
    auto idx1 = idx0 + 1;

    // Sample index must be in range
    jassert(idx1 < data.getNumSamples());

    auto g1 = fractionalIndex - float(idx0);
    auto g0 = 1.0f - g1;

    auto inL = data.getReadPointer(0);
    auto inR = data.getNumChannels() > 1 ? data.getReadPointer(1) : nullptr;

    auto l = g0 * inL[idx0] + g1 * inL[idx1];
    auto r = inR == nullptr ? l : g0 * inR[idx0] + g1 * inR[idx1];

    return { l, r };
}

Voice::StereoSample Voice::getNextSample()
{
    jassert(_sound != nullptr && !_sound->isEmpty());

    // Get current sample output and increment index & ADSR
    auto g = _adsr.getNextSample() * _gain;
    auto [l, r] = readFromSample(_sound->getSampleData(), float(_currentIdx));
    _currentIdx += _pitchRatio;

    // Apply pan
    auto panLeft = juce::dsp::FastMathApproximations::cos(juce::MathConstants<float>::halfPi * _pan);
    auto panRight = juce::dsp::FastMathApproximations::sin(juce::MathConstants<float>::halfPi * _pan);

    return { l * g * panLeft, r * g * panRight };
}
