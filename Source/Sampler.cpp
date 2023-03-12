#include "Sampler.h"
#include <resample.h>
#include "Utilities.h"

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

void Sound::setSample(juce::AudioBuffer<float>&& data, double sourceFs)
{
    jassert(juce::isPositiveAndBelow(data.getNumChannels(), maxNumChannels));

    _sourceData = std::move(data);
    _sourceSampleRate = sourceFs;
    updateCurrentSample();
}

const juce::AudioBuffer<float>& Sound::getSample() const
{
    return _currentData;
}

void Sound::clearSample() 
{
    _sourceData = juce::AudioBuffer<float>();
    _currentData = juce::AudioBuffer<float>();
}

bool Sound::isEmpty() const
{
    return _currentData.getNumSamples() == 0;
}

void Sound::setFadeLength(double lengthInSeconds)
{
    _fadeLength = lengthInSeconds;
    updateCurrentSample();
}

void Sound::setPitch(float pitch)
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
    if (!(_sampleRate > 0.0 && _sourceSampleRate > 0.0))
        return;

    if (_sourceData.getNumSamples() == 0)
        return;

    _currentData = r8b::resample(_sourceData, _sourceSampleRate, _sampleRate);
    auto numSamples = _currentData.getNumSamples();

    // Apply fade
    auto fadeLengthSamples = juce::jmin(int(_fadeLength * _sampleRate), numSamples / 2);
    auto ptr = _currentData.getArrayOfWritePointers();
    for (int j = 0; j < _currentData.getNumChannels(); j++)
    {
        Utils::applyFade(ptr[j], 0, fadeLengthSamples, true);
        Utils::applyFade(ptr[j], numSamples - fadeLengthSamples, fadeLengthSamples, false);
    }
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

    _adsr.setSampleRate(_sound->getSampleRate());
    _adsr.setParameters(_sound->getEnvelope());
    _adsr.noteOn();

    _pitchRatio = std::pow(2.0, _sound->getPitch() / 12.0);
    _currentIdx = 0.0;
}

void Voice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        _adsr.noteOff();
    }
    else
    {
        if (_adsr.isActive())
        {
            // Fill fifo with tapered output from current note, if part of the sample is still available.
            auto& sample = _sound->getSample();
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

        _adsr.reset();
        clearCurrentNote();
        _sound = nullptr;
    }
}

void Voice::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (_sound == nullptr)
        return;

    // Still have to read from the fifos if the sample is empty
    if (_sound->isEmpty())
    {
        jassert(buffer.getNumChannels() == 2);
        auto outL = buffer.getWritePointer(0);
        auto outR = buffer.getWritePointer(1);

        int endSample = startSample + juce::jmin(numSamples, _fifo.getNumAvailableToRead(0));
        for (int i = startSample; i < endSample; i++)
        {
            outL[i] += _fifo.read(0);
            outR[i] += _fifo.read(1);
        }

        return;
    }
    
    auto& sample = _sound->getSample();
    jassert(buffer.getNumChannels() == 2);
    jassert(sample.getNumChannels() == 1 || sample.getNumChannels() == 2);

    auto outL = buffer.getWritePointer(0);
    auto outR = buffer.getWritePointer(1);

    int endSample = startSample + numSamples;
    for (int i = startSample; i < endSample; i++)
    {
        auto [l, r] = getNextSample();
        bool endOfSample = int(_currentIdx) >= sample.getNumSamples() - 1;

        // Add fifo output if data from previous voice is available
        if (_fifo.getNumAvailableToRead(0) > 0)
        {
            l += _fifo.read(0);
            r += _fifo.read(1);
        }

        // Write to output buffer
        outL[i] += l;
        outR[i] += r;

        if (endOfSample || !_adsr.isActive())
        {
            stopNote(0.0f, false);
            break;
        }
    }
}

Voice::StereoSample Voice::readFromSample(float fractionalIndex) const
{
    // The voice must be active to call this function.
    jassert(_sound != nullptr);

    auto& sample = _sound->getSample();
    auto idx0 = int(fractionalIndex);
    auto idx1 = idx0 + 1;

    // Sample index must be in range
    jassert(idx1 < sample.getNumSamples());

    auto g1 = fractionalIndex - float(idx0);
    auto g0 = 1.0f - g1;

    auto inL = sample.getReadPointer(0);
    auto inR = sample.getNumChannels() > 1 ? sample.getReadPointer(1) : nullptr;

    auto l = g0 * inL[idx0] + g1 * inL[idx1];
    auto r = inR == nullptr ? l : g0 * inR[idx0] + g1 * inR[idx1];

    return { l, r };
}

Voice::StereoSample Voice::getNextSample()
{
    // Get current sample output and increment index & ADSR
    auto g = _adsr.getNextSample();
    auto [l, r] = readFromSample(float(_currentIdx));
    _currentIdx += _pitchRatio;

    return { l * g, r * g };
}
