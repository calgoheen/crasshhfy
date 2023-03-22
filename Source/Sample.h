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
	none = 0,
	kick, 
	snare, 
	hat
};

struct Drum 
{
	Sample::Ptr sample{ nullptr };
	DrumType drumType{ DrumType::none };
	float confidence{ 0.0f };
};
