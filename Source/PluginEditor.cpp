#include "PluginProcessor.h"
#include "PluginEditor.h"

Text2SampleAudioProcessorEditor::Text2SampleAudioProcessorEditor(Text2SampleAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), _processor(p)
{
	// Midi note slider
	addAndMakeVisible(_noteSlider);

    // CRASH button
    _crashButton.setButtonText("Generate Sample");
    _crashButton.onClick = [this]
    {
		_crashButton.setEnabled(false);
        juce::Thread::launch([this] { 
			_processor.generateSample(_noteSlider.getValue()); 
			juce::MessageManager::callAsync([this] { _crashButton.setEnabled(true); });
		});
    };
    addAndMakeVisible(_crashButton);

	// Save sample to file
	_saveButton.setButtonText("Save Sample");
	_saveButton.onClick = [this]
	{
        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;
        auto defaultPath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);

        _chooser = std::make_unique<juce::FileChooser>("Save wav file", defaultPath, "*.wav");
        
        _chooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
            auto f = chooser.getResult();
            
            if (f == juce::File())
                return;
            
            auto idx = int(_noteSlider.getValue());
			_processor.saveSample(idx, f);
        });
	};
	addAndMakeVisible(_saveButton);

	// Load sample from file
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
			_processor.loadSampleFromFile(idx, f);
        });
    };
	addAndMakeVisible(_loadButton);

	// On-screen keyboard
	_keyboard.reset(new SampleKeyboard(p.baseMidiNote, p.numSounds, p.getMidiKeyboardState()));
	addAndMakeVisible(*_keyboard);

    setSize(600, 400);
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
	auto bounds = getLocalBounds().reduced(40);

	_noteSlider.setBounds(bounds.removeFromTop(100).withSizeKeepingCentre(120, 60));
    _crashButton.setBounds(bounds.removeFromTop(40));
	_saveButton.setBounds(bounds.removeFromTop(40));
	_loadButton.setBounds(bounds.removeFromTop(40));

	_keyboard->setBounds(bounds);
}
