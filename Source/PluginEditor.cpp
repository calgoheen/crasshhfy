#include "PluginProcessor.h"
#include "PluginEditor.h"


CrasshhfyAudioProcessorEditor::CrasshhfyAudioProcessorEditor(CrasshhfyAudioProcessor& p)
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

	_stepsSlider.setSliderStyle(juce::Slider::LinearHorizontal);
	_stepsSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 30, 15);
	_stepsSlider.setNormalisableRange({ 5.0, 15.0, 1.0 });
	_stepsSlider.setValue(p.getNumSteps());
	_stepsSlider.onValueChange = [&] { p.setNumSteps(_stepsSlider.getValue()); };
	addAndMakeVisible(_stepsSlider);

	_stepsLabel.setText("Steps", juce::dontSendNotification);
	addAndMakeVisible(_stepsLabel);

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

			auto baseColour = juce::Colours::red.darker();
			static constexpr float shiftToGreen = 0.33f;
			auto col = baseColour.withRotatedHue(shiftToGreen * s->getConfidence());
			_keyboard->setNoteLabel(i, label, col);
		};
		s->drumChanged();
	}
	
	updateParameterView();

    setSize(600, 400);
}

CrasshhfyAudioProcessorEditor::~CrasshhfyAudioProcessorEditor()
{
	setLookAndFeel(nullptr);
}

void CrasshhfyAudioProcessorEditor::paint(juce::Graphics& g)
{
	g.fillAll(CustomLookAndFeel::Palette::background);
	g.drawImageAt(_logo, _logoBounds.getX(), _logoBounds.getY());
}

void CrasshhfyAudioProcessorEditor::resized()
{
	auto bounds = getLocalBounds();

	auto top = bounds.removeFromTop(80);
	auto mid = bounds.removeFromTop(185).reduced(10);
	auto bottom = bounds;

	_logoBounds = top.removeFromLeft(120);
	_logo = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize).rescaled(120, 120);

	auto buttonSectionWidth = top.getWidth() / 3;
	auto generateBounds = top.removeFromLeft(buttonSectionWidth).withSizeKeepingCentre(100, 30);
    _generateButton.setBounds(generateBounds);
	_drumifyButton.setBounds(generateBounds.translated(buttonSectionWidth, 0));
	_inpaintButton.setBounds(generateBounds.translated(2 * buttonSectionWidth, 0));
    _inpaintSelector.setBounds(generateBounds.translated(2 * buttonSectionWidth, 40));
	_stepsSlider.setBounds(generateBounds.translated(buttonSectionWidth, 40));
	_stepsLabel.setBounds(_stepsSlider.getBounds().translated(-45, 0).withSize(45, 20));

	_keyboard->setBounds(mid);

	for (auto view : _parameterViews)
		view->setBounds(bottom);
}

void CrasshhfyAudioProcessorEditor::updateParameterView()
{
	for (int i = 0; i < _parameterViews.size(); i++)
		_parameterViews[i]->setVisible(i == _lastNoteIndex);
}

void CrasshhfyAudioProcessorEditor::setButtonsEnabled(bool enabled)
{
	_generateButton.setEnabled(enabled);
	_drumifyButton.setEnabled(enabled);
	_inpaintButton.setEnabled(enabled);
}
