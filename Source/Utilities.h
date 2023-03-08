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
        auto numChannels = int(reader->numChannels);
        auto numSamples = int(reader->lengthInSamples);

        jassert(numSamples == int(reader->lengthInSamples));

        juce::AudioBuffer<float> data;
        data.setSize(numChannels, numSamples);
        reader->read(data.getArrayOfWritePointers(), numChannels, 0, numSamples);
        
        return { std::move(data), reader->sampleRate };
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
