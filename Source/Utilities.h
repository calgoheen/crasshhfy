#pragma once

#include <JuceHeader.h>

struct Utils
{
    static void saveBufferToFile(const juce::String& file, const float* x, int numSamples)
    {
        juce::File f{ file };
        f.deleteFile();
        f.create();

        for (int i = 0; i < numSamples; i++)
            f.appendText(juce::String(x[i]) + "\n");
    }

    static void makeSine(float* x, int numSamples, float f, double fs)
    {
        // Sine wave input signal
        float w = juce::MathConstants<float>::twoPi * f / float(fs);
        
        for (int i = 0; i < numSamples; i++)
            x[i] = std::sin(w * i);
    }

    static std::pair<juce::AudioBuffer<float>, double> readWavFile(const juce::File& file)
    {
        jassert(file.hasFileExtension(".wav"));

        juce::WavAudioFormat format;
        auto reader = std::unique_ptr<juce::AudioFormatReader>(format.createReaderFor(new juce::FileInputStream(file), true));

        if (reader == nullptr)
            return { {}, 0.0 };

        auto numChannels = int(reader->numChannels);
        auto numSamples = int(reader->lengthInSamples);

        jassert(numSamples == int(reader->lengthInSamples));

        juce::AudioBuffer<float> data;
        data.setSize(numChannels, numSamples);
        reader->read(data.getArrayOfWritePointers(), numChannels, 0, numSamples);
        
        return { std::move(data), reader->sampleRate };
    }

    static void writeWavFile(const juce::AudioBuffer<float>& data, double sampleRate, const juce::File& file)
    {
        jassert(file.hasFileExtension(".wav"));
        file.deleteFile();
        file.create();

        static constexpr juce::uint32 numBits = 16;
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatWriter> writer;
        writer.reset(format.createWriterFor(new juce::FileOutputStream{ file },
                                            sampleRate,
                                            data.getNumChannels(),
                                            numBits,
                                            {},
                                            0));

        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer(data, 0, data.getNumSamples());
    }
    
    static void applyFade(float* data, int startSample, int numSamples, bool fadeIn = true)
    {
        if (fadeIn)
        {
            for (int i = 0; i < numSamples; i++)
                data[startSample + i] *= std::sqrt(float(i) / numSamples);
        }
        else
        {
            for (int i = 0; i < numSamples; i++)
                data[startSample + i] *= std::sqrt(float(numSamples - i) / numSamples);
        }
    }

    static void normalize(juce::AudioBuffer<float>& buffer)
    {
        auto ptr = buffer.getArrayOfWritePointers();
        
        float max = 0.0f;
        for (int j = 0; j < buffer.getNumChannels(); j++)
        {
            for (int i = 0; i < buffer.getNumSamples(); i++)
            {
                auto val = std::abs(ptr[j][i]);
                if (val > max) max = val;
            }
        }

        for (int j = 0; j < buffer.getNumChannels(); j++)
            for (int i = 0; i < buffer.getNumSamples(); i++)
                ptr[j][i] /= max;
    }
};

struct ParameterDefinition
{
    juce::String id;
    juce::String name;
    juce::String label;
    juce::NormalisableRange<float> range;
    float defaultValue;
};

class DummyProcessor : public juce::AudioProcessor
{
public:
    DummyProcessor() = default;
    ~DummyProcessor() override = default;

private:
    bool isBusesLayoutSupported (const BusesLayout&) const override { return false; }
    void prepareToPlay(double, int) override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    void releaseResources() override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    const juce::String getName() const override { return ""; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return ""; }
    void changeProgramName(int, const juce::String&) override {}

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DummyProcessor)
};

class ParameterListener : public juce::AudioProcessorParameter::Listener
{
public:
    ParameterListener(juce::RangedAudioParameter& param, std::function<void(float)> func)
        : param(param), _func(std::move(func))
    {
        jassert(_func != nullptr);
        param.addListener(this);
    }

private:
    void parameterValueChanged(int, float newValue) override
    {
        _func(param.convertFrom0to1(newValue));
    }

    void parameterGestureChanged(int, bool) override {}

    juce::RangedAudioParameter& param;
    std::function<void(float)> _func;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterListener)
};
