#include "PluginProcessor.h"
#include "PluginEditor.h"

Text2SampleAudioProcessorEditor::Text2SampleAudioProcessorEditor(Text2SampleAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), _processor(p)
{
    // CRASH button
    _crashButton.setButtonText("Generate Sample");
    _crashButton.onClick = [this]
    {
		_crashButton.setEnabled(false);
		auto idx = _lastNoteIndex;
        juce::Thread::launch([this, idx] { 
			_processor.generateSample(idx); 
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
            
            if (_lastNoteIndex >= 0)
				_processor.saveSample(_lastNoteIndex, f);
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
            
			if (_lastNoteIndex >= 0)
				_processor.loadSampleFromFile(_lastNoteIndex, f);
        });
    };
	addAndMakeVisible(_loadButton);

	// On-screen keyboard
	_keyboard.reset(new SampleKeyboard(p.baseMidiNote, p.numSounds, p.getMidiKeyboardState()));
	_keyboard->onSelectedNoteChange = [this](int noteIndex) 
	{ 
		_lastNoteIndex = noteIndex; 
		updateParameterView();
	};
	addAndMakeVisible(*_keyboard);

	// Sample parameter section
	for (int i = 0; i < p.numSounds; i++)
		addChildComponent(_parameterViews.add(new ParameterView(p.getSound(i))));
	
	updateParameterView();

    setSize(400, 500);
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

    _crashButton.setBounds(bounds.removeFromTop(40));
	_saveButton.setBounds(bounds.removeFromTop(40));
	_loadButton.setBounds(bounds.removeFromTop(40));

	_keyboard->setBounds(bounds.removeFromTop(120));

	for (auto view : _parameterViews)
		view->setBounds(bounds);
}

void Text2SampleAudioProcessorEditor::updateParameterView()
{
	for (int i = 0; i < _parameterViews.size(); i++)
		_parameterViews[i]->setVisible(i == _lastNoteIndex);
}
