#include "Components.h"

SamplePad::SamplePad(int midiNoteNumber) : _midiNote(midiNoteNumber)
{
	_noteName = juce::MidiMessage::getMidiNoteName(midiNoteNumber, true, true, 4);
}

int SamplePad::getMidiNote() const
{
	return _midiNote;
}

void SamplePad::paint(juce::Graphics& g)
{
	static constexpr float corner = 2.0f;
	static constexpr float selectedThickness = 5.0f;

	// Pad
	auto col = _noteIsOn ? CustomLookAndFeel::Palette::highlight : CustomLookAndFeel::Palette::light3;
	g.setColour(col);
	g.fillRoundedRectangle(getLocalBounds().toFloat(), corner);

	if (_noteIsSelected)
	{
		g.setColour(CustomLookAndFeel::Palette::highlight);
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(selectedThickness / 2.0f), corner, selectedThickness);
	}

	auto labelHeight = getHeight() / 5;
	auto bounds = getLocalBounds();

	// Note name
	g.setColour(CustomLookAndFeel::Palette::background);
	g.drawText(_noteName, 
			   bounds.removeFromBottom(labelHeight).toFloat(), 
			   juce::Justification::centred, 
			   false);

	if (_label.isNotEmpty())
	{
		g.setColour(CustomLookAndFeel::Palette::background);
		g.drawText(_label, 
				   bounds.withSizeKeepingCentre(getWidth(), labelHeight), 
				   juce::Justification::centred, 
				   false);
	}
}

void SamplePad::setNoteOn(bool noteIsOn)
{
	if (noteIsOn != _noteIsOn)
	{
		_noteIsOn = noteIsOn;
		repaint();
	}
}

void SamplePad::setNoteSelected(bool isSelected)
{
	if (_noteIsSelected != isSelected)
	{
		_noteIsSelected = isSelected;
		repaint();
	}
}

void SamplePad::setLabel(const juce::String& label)
{
	_label = label;
	repaint();
}


SampleKeyboard::SampleKeyboard(int baseNote, int numNotes, juce::MidiKeyboardState& midiKeyboardState, int midiChannel) 
	: _baseMidiNote(baseNote), _midiState(midiKeyboardState), _midiChannel(midiChannel)
{
	for (int i = 0; i < numNotes; i++)
	{
		auto note = _notes.add(new SamplePad(baseNote + i));
		note->addMouseListener(this, false);
		addAndMakeVisible(note);
	}

	updateSelectedNote();

	startTimer(_timerLengthMs);
}

void SampleKeyboard::resized()
{
	auto w = getWidth() / _notes.size();
	auto bounds = getLocalBounds().withSizeKeepingCentre(w * _notes.size(), getHeight());

	for (auto note : _notes)
		note->setBounds(bounds.removeFromLeft(w).withSizeKeepingCentre(70, 55));
}

void SampleKeyboard::setSelectedNote(int idx)
{
	jassert(idx < _notes.size());

	if (idx != _lastNoteIndex)
	{
		_lastNoteIndex = idx;
		updateSelectedNote();

		if (onSelectedNoteChange)
			onSelectedNoteChange(idx);
	}
}

void SampleKeyboard::setNoteLabel(int idx, const juce::String& label)
{
	jassert(juce::isPositiveAndBelow(idx, _notes.size()));
	_notes[idx]->setLabel(label);
}

void SampleKeyboard::timerCallback()
{
	for (int i = 0; i < _notes.size(); i++)
	{
		auto noteState = _midiState.isNoteOn(_midiChannel, _baseMidiNote + i);
		_notes[i]->setNoteOn(noteState);

		if (noteState)
			setSelectedNote(i);
	}
}

void SampleKeyboard::mouseDown(const juce::MouseEvent& e)
{
	for (int i = 0; i < _notes.size(); i++)
	{
		if (_notes[i] == e.eventComponent)
		{
			_midiState.noteOn(_midiChannel, i + _baseMidiNote, 1.0f);
			
			setSelectedNote(i);
			break;
		}
	}
}

void SampleKeyboard::mouseUp(const juce::MouseEvent& e)
{
	for (int i = 0; i < _notes.size(); i++)
		if (_notes[i] == e.eventComponent)
			_midiState.noteOff(_midiChannel, i + _baseMidiNote, 1.0f);
}

void SampleKeyboard::updateSelectedNote()
{
	for (int i = 0; i < _notes.size(); i++)
		_notes[i]->setNoteSelected(i == _lastNoteIndex);
}


ParameterView::ParameterView(SoundWithParameters* sound) 
	: _cache(5), _thumbnail(512, _afm, _cache)
{
	// Set up sliders
	auto initLinearSlider = [this](juce::Slider& s)
	{
		addAndMakeVisible(s);
		s.setSliderStyle(juce::Slider::LinearVertical);
		s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
	};

	auto initRotarySlider = [this](juce::Slider& s)
	{
		addAndMakeVisible(s);
		s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
		s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
	};

	using P = SoundWithParameters::Parameters;

	initLinearSlider(_sliders[P::kAttack]);
	initLinearSlider(_sliders[P::kDecay]);
	initLinearSlider(_sliders[P::kSustain]);
	initLinearSlider(_sliders[P::kRelease]);

	initRotarySlider(_sliders[P::kGain]);
	initRotarySlider(_sliders[P::kPan]);
	initRotarySlider(_sliders[P::kPitch]);

	// Set up labels and parameter attachments
	const juce::StringArray paramNames = {
		"Gain", "Pan", "Pitch",
		"A", "D", "S", "R"
	};

	for (int i = 0; i < numParameters; i++)
	{
		_attachments[i].reset(new juce::SliderParameterAttachment(*(sound->getParameter(i)), _sliders[i]));

		addAndMakeVisible(_labels[i]);
		_labels[i].setText(paramNames[i], juce::dontSendNotification);
		_labels[i].setJustificationType(juce::Justification::centred);
	}

	sound->sampleChanged = [=]
	{
		auto sample = sound->getSample();
		if (sample != nullptr)
		{
			_thumbnail.setSource(&sample->data, sample->sampleRate, 0);
			repaint();
		}
		else
		{
			_thumbnail.clear();
			repaint();
		}
	};

	_saveButton.setButtonText("Save");
	_saveButton.onClick = [=]
	{
        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;
        auto defaultPath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);

        _chooser = std::make_unique<juce::FileChooser>("Save sample", defaultPath, "*.wav");
        
        _chooser->launchAsync(flags, [=](const juce::FileChooser& chooser) {
            auto f = chooser.getResult();
            
            if (f == juce::File())
                return;
            
			auto sample = sound->getSample();
            if (sample != nullptr)
        		Utils::writeWavFile(sample->data, sample->sampleRate, f);
        });
	};
	addAndMakeVisible(_saveButton);

	_clearButton.setButtonText("Clear");
	_clearButton.onClick = [=]
	{
		if (auto drumSound = dynamic_cast<DrumSound*>(sound))
			drumSound->loadDrum({});
	};
	addAndMakeVisible(_clearButton);
}

void ParameterView::paint(juce::Graphics& g)
{
	if (auto laf = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel()))
	{
		laf->drawControlPanel(g, _thumbnailBounds);
		laf->drawControlPanel(g, _adsrBounds);
		laf->drawControlPanel(g, _knobBounds);
	}

	if (_thumbnail.getNumChannels() > 0)
	{
        g.setColour(CustomLookAndFeel::Palette::light2);
        _thumbnail.drawChannels(g,
                                _thumbnailBounds,
                                0.0,
                                _thumbnail.getTotalLength(),
                                1.0f);
	}	
	else
	{
        g.setColour(CustomLookAndFeel::Palette::light1);
        g.drawFittedText("No Sample Loaded", _thumbnailBounds, juce::Justification::centred, 1);
	}
}

void ParameterView::resized()
{
	static constexpr int pad = 4;

	auto bounds = getLocalBounds();
	auto panelWidth = getWidth() / 3;
	_thumbnailBounds = bounds.removeFromLeft(panelWidth).reduced(pad);
	_adsrBounds = bounds.removeFromLeft(panelWidth).reduced(pad);
	_knobBounds = bounds.removeFromLeft(panelWidth).reduced(pad);

	auto thumbnailButtonBounds = _thumbnailBounds;
	thumbnailButtonBounds = thumbnailButtonBounds.removeFromRight(50);
	_saveButton.setBounds(thumbnailButtonBounds.removeFromTop(20));
	_clearButton.setBounds(thumbnailButtonBounds.removeFromTop(20));

	// Envelope sliders
	auto adsrSliderWidth = _adsrBounds.getWidth() / 4;
	auto adsrLabelHeight = _adsrBounds.getHeight() / 8;
	auto adsrSliderBounds = _adsrBounds.withSizeKeepingCentre(adsrSliderWidth * 4, _adsrBounds.getHeight());
	for (int i = SoundWithParameters::kAttack; i <= SoundWithParameters::kRelease; i++)
	{
		auto sliderBounds = adsrSliderBounds.removeFromLeft(adsrSliderWidth);
		sliderBounds.removeFromBottom(5);
		auto labelBounds = sliderBounds.removeFromBottom(adsrLabelHeight);
		_sliders[i].setBounds(sliderBounds);
		_labels[i].setBounds(labelBounds);
	}

	// Gain, pan, pitch sliders
	auto knobSliderWidth = _knobBounds.getWidth() / 3;
	auto knobLabelHeight = _knobBounds.getHeight() / 8;
	auto knobSliderBounds = _knobBounds.withSizeKeepingCentre(knobSliderWidth * 3, int(_knobBounds.getHeight() * 0.67f));
	for (int i = SoundWithParameters::kGain; i <= SoundWithParameters::kPitch; i++)
	{
		auto sliderBounds = knobSliderBounds.removeFromLeft(knobSliderWidth).reduced(12, 0);
		auto labelBounds = sliderBounds.removeFromBottom(knobLabelHeight);
		_sliders[i].setBounds(sliderBounds);
		_labels[i].setBounds(labelBounds);
	}
}
