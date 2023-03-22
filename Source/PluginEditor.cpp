#include "PluginProcessor.h"
#include "PluginEditor.h"


Text2SampleAudioProcessorEditor::Text2SampleAudioProcessorEditor(Text2SampleAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), _processor(p)
{
	setLookAndFeel(&_laf);

    // Generate sample
    _generateButton.setButtonText("Generate");
    _generateButton.onClick = [this]
    {
		setButtonsEnabled(false);
		auto idx = _lastNoteIndex;
        juce::Thread::launch([this, idx] { 
			_processor.generateSample(idx); 
			juce::MessageManager::callAsync([this] { setButtonsEnabled(true); });
		});
    };
    addAndMakeVisible(_generateButton);

	// Drumify sample
	_drumifyButton.setButtonText("Drumify");
	_drumifyButton.onClick = [this]
	{
		auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
		auto defaultPath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
		auto idx = _lastNoteIndex;

        _chooser = std::make_unique<juce::FileChooser>("Load wav file", defaultPath, "*.wav");
        _chooser->launchAsync(flags, [this, idx](const juce::FileChooser& chooser) {
            auto f = chooser.getResult();

            if (f == juce::File())
                return;

			if (idx < 0)
				return;

			setButtonsEnabled(false);
			juce::Thread::launch([this, idx, f] {
				_processor.drumifySample(idx, f);
				juce::MessageManager::callAsync([this] { setButtonsEnabled(true); });
			});
        });
	};
	addAndMakeVisible(_drumifyButton);

	// Inpaint sample
	_inpaintButton.setButtonText("Variation");
    _inpaintSelector.setButtonText("Start/End");
    _inpaintButton.onClick = [this]
    {
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        auto defaultPath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
        auto idx = _lastNoteIndex;

        _chooser = std::make_unique<juce::FileChooser>("Load wav file", defaultPath, "*.wav");
        _chooser->launchAsync(flags, [this, idx](const juce::FileChooser& chooser) {
            auto f = chooser.getResult();

            if (f == juce::File())
                return;

            if (idx < 0)
                return;

            setButtonsEnabled(false);
            juce::Thread::launch([this, idx, f] {
                _processor.inpaintSample(idx, f, _inpaintSelector.getToggleState());
                juce::MessageManager::callAsync([this] { setButtonsEnabled(true); });
            });
        });

    };
	addAndMakeVisible(_inpaintButton);
    addAndMakeVisible(_inpaintSelector);

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

	// On-screen keyboard
	_keyboard.reset(new SampleKeyboard(p.baseMidiNote, p.numSounds, p.getMidiKeyboardState()));
	_keyboard->onSelectedNoteChange = [this](int noteIndex) 
	{ 
		_lastNoteIndex = noteIndex; 
		updateParameterView();
	};
	addAndMakeVisible(*_keyboard);

	for (int i = 0; i < p.numSounds; i++)
	{
		auto s = p.getSound(i);

		// Set up parameter controls for the sample
		addChildComponent(_parameterViews.add(new ParameterView(s)));

		// Change note label depending on classifier output
		s->drumChanged = [this, i, s] 
		{
			juce::String label = "";
			switch(s->getDrumType())
			{
				case DrumType::kick: 	label = "Kick"; 	break;
				case DrumType::snare: 	label = "Snare"; 	break;
				case DrumType::hat: 	label = "Hat"; 		break;
			};

			_keyboard->setNoteLabel(i, label);
		};
	}
	
	updateParameterView();

    setSize(600, 400);
}

Text2SampleAudioProcessorEditor::~Text2SampleAudioProcessorEditor()
{
	setLookAndFeel(nullptr);

}

void Text2SampleAudioProcessorEditor::paint(juce::Graphics& g)
{
	g.fillAll(CustomLookAndFeel::Palette::background);
}

void Text2SampleAudioProcessorEditor::resized()
{
	auto bounds = getLocalBounds();

	auto top = bounds.removeFromTop(80);
	auto mid = bounds.removeFromTop(160).reduced(10);
	auto bottom = bounds;

	auto buttonSectionWidth = top.getWidth() / 3;
	auto generateBounds = top.removeFromLeft(buttonSectionWidth).withSizeKeepingCentre(100, 30);
    _generateButton.setBounds(generateBounds);
	_drumifyButton.setBounds(generateBounds.translated(buttonSectionWidth, 0));
	_inpaintButton.setBounds(generateBounds.translated(2 * buttonSectionWidth, 0));
    _inpaintSelector.setBounds(generateBounds.translated(2 * buttonSectionWidth, 40));

	_keyboard->setBounds(mid);

	for (auto view : _parameterViews)
		view->setBounds(bottom);
}

void Text2SampleAudioProcessorEditor::updateParameterView()
{
	for (int i = 0; i < _parameterViews.size(); i++)
		_parameterViews[i]->setVisible(i == _lastNoteIndex);
}

void Text2SampleAudioProcessorEditor::setButtonsEnabled(bool enabled)
{
	_generateButton.setEnabled(enabled);
	_drumifyButton.setEnabled(enabled);
	_inpaintButton.setEnabled(enabled);
}
