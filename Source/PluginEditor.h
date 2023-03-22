#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Components.h"

class Text2SampleAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Text2SampleAudioProcessorEditor (Text2SampleAudioProcessor&);
    ~Text2SampleAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
	void updateParameterView();
    void setButtonsEnabled(bool enabled);

    Text2SampleAudioProcessor& _processor;

    juce::TextButton _generateButton;
    juce::TextButton _drumifyButton;
    juce::TextButton _inpaintButton;
    
	juce::TextButton _saveButton;
	juce::TextButton _loadButton;

	std::unique_ptr<SampleKeyboard> _keyboard;
	juce::OwnedArray<ParameterView> _parameterViews;
	int _lastNoteIndex{ 0 };

	std::unique_ptr<juce::FileChooser> _chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Text2SampleAudioProcessorEditor)
};
