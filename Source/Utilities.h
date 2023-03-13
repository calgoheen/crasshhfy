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
};
