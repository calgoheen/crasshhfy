#include "PluginProcessor.h"
#include "PluginEditor.h"

Text2SampleAudioProcessorEditor::Text2SampleAudioProcessorEditor(Text2SampleAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), _processor(p), _state(p.getEditorState())
{
	// Midi note slider
	addAndMakeVisible(_noteSlider);

	// Slider label
	_noteLabel.setText("Note Index", juce::dontSendNotification);
	_noteLabel.setJustificationType(juce::Justification::centred);
	_noteLabel.setFont(juce::Font{ 14.0f });
	addAndMakeVisible(_noteLabel);

	// Button
	_loadButton.setButtonText("Load Sample");
	_loadButton.onClick = [this] 
	{
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
		auto defaultPath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
        _chooser = std::make_unique<juce::FileChooser>("Load wav file", defaultPath, "*.wav");
        
        _chooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
            auto f = chooser.getResult();
            
            if (f == juce::File())
                return;
            
			auto idx = int(_noteSlider.getValue());
			_processor.loadSample(idx, f);
        });
    };
	addAndMakeVisible(_loadButton);

    setSize(400, 300);
}

Text2SampleAudioProcessorEditor::~Text2SampleAudioProcessorEditor()
{
}

void Text2SampleAudioProcessorEditor::paint(juce::Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}


void Text2SampleAudioProcessorEditor::resized()
{
	auto bounds = getLocalBounds().reduced(30);

	_noteLabel.setBounds(bounds.removeFromTop(30));
	_noteSlider.setBounds(bounds.removeFromTop(100).withSizeKeepingCentre(120, 60));
	_loadButton.setBounds(bounds.removeFromTop(50));
}
