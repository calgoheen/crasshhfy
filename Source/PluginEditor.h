#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "Components.h"

class CrasshhfyAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    CrasshhfyAudioProcessorEditor (CrasshhfyAudioProcessor&);
    ~CrasshhfyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
	void updateParameterView();
    void setButtonsEnabled(bool enabled);

    CrasshhfyAudioProcessor& _processor;

    CustomLookAndFeel _laf;

    juce::TextButton _generateButton;
    juce::TextButton _drumifyButton;
    juce::TextButton _inpaintButton;
    juce::Label _inpaintText;
    juce::ToggleButton _inpaintSelector;
    
	juce::TextButton _saveButton;
	juce::TextButton _loadButton;

	std::unique_ptr<SampleKeyboard> _keyboard;
	juce::OwnedArray<ParameterView> _parameterViews;
	int _lastNoteIndex{ 0 };

	std::unique_ptr<juce::FileChooser> _chooser;

    juce::Rectangle<int> _logoBounds;
    juce::Image _logo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrasshhfyAudioProcessorEditor)
};
