#pragma once

#include <JuceHeader.h>

struct Sample : public juce::ReferenceCountedObject
{
    Sample(juce::AudioBuffer<float>&& sampleData, double sampleFs)
      : data(std::move(sampleData)), sampleRate(sampleFs) {}

    ~Sample() override = default;
    
    juce::AudioBuffer<float> data;
    double sampleRate;

    using Ptr = juce::ReferenceCountedObjectPtr<Sample>;

    JUCE_DECLARE_NON_COPYABLE(Sample)
};

enum class DrumType 
{
    kick = 0, 
    snare, 
    hat
};

struct Drum 
{
	Sample::Ptr sample;
	DrumType drumType;
	float confidence;
};
