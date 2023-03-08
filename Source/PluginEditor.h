#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class Text2SampleAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Text2SampleAudioProcessorEditor (Text2SampleAudioProcessor&);
    ~Text2SampleAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Text2SampleAudioProcessor& _processor;
    EditorState* _state;

    class NoteSlider : public juce::Slider
    {
	public:
		NoteSlider()
		{
			setNormalisableRange({ 0.0, double(_noteNames.size() - 1), 1.0 });
			setSliderStyle(juce::Slider::SliderStyle::IncDecButtons);
		}

		~NoteSlider() override = default;

		juce::String getTextFromValue(double value) override
		{
			auto idx = int(value);
			jassert(juce::isPositiveAndBelow(idx, _noteNames.size()));

			return _noteNames[int(value)];
		}

	private:
		juce::StringArray _noteNames = {
			"C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G", "G#4", "A4", "A#4", "B4"
		};
    };

    NoteSlider _noteSlider;
    juce::Label _noteLabel;
    juce::TextButton _loadButton;

    std::unique_ptr<juce::FileChooser> _chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Text2SampleAudioProcessorEditor)
};
