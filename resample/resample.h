#pragma once

#include <JuceHeader.h>
#include "r8b/CDSPResampler.h"

namespace r8b
{

static juce::AudioBuffer<float> resample(const juce::AudioBuffer<float>& sourceBuffer, double sourceFs, double destFs)
{
    int numChannels = sourceBuffer.getNumChannels();
    int sourceLength = sourceBuffer.getNumSamples();
    int destLength = std::floor(sourceLength * destFs / sourceFs);

    // Output buffer
    juce::AudioBuffer<float> outBuffer;
    outBuffer.setSize(numChannels, destLength);

    for (int j = 0; j < numChannels; j++)
    {
        // Prepare resampler
        auto resampler = std::make_unique<r8b::CDSPResampler>(sourceFs, destFs, sourceLength);
        auto numRequired = resampler->getInputRequiredForOutput(destLength);

        // Convert float source buffer to double to be compatible with resampler
        juce::AudioBuffer<double> sourceBufferDouble;
        sourceBufferDouble.setSize(1, numRequired);
        sourceBufferDouble.clear();

        for (int i = 0; i < sourceLength; i++)
            sourceBufferDouble.setSample(0, i, double(sourceBuffer.getSample(j, i)));

        // Do resample and save to float output buffer
        double* outPtr;
        resampler->process(sourceBufferDouble.getWritePointer(0), numRequired, outPtr);
        for (int i = 0; i < destLength; i++)
            outBuffer.setSample(j, i, float(outPtr[i]));
    }

    return std::move(outBuffer);
}

}
