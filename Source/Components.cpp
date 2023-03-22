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
	auto col = _noteIsOn ? juce::Colours::lightpink : juce::Colours::lightgrey;
	g.setColour(col);
	g.fillRoundedRectangle(getLocalBounds().toFloat(), corner);

	if (_noteIsSelected)
	{
		g.setColour(juce::Colours::lightpink);
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(selectedThickness / 2.0f), corner, selectedThickness);
	}

	auto labelHeight = getHeight() / 5;
	auto bounds = getLocalBounds();

	// Note name
	g.setColour(juce::Colours::black);
	g.drawText(_noteName, 
			   bounds.removeFromBottom(labelHeight).toFloat(), 
			   juce::Justification::centred, 
			   false);

	if (_label.isNotEmpty())
	{
		g.setColour(juce::Colours::black);
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
{
	const juce::StringArray paramNames = {
		"Gain", "Pan", "Pitch",
		"Attack", "Decay", "Sustain", "Release"
	};

	for (int i = 0; i < numParameters; i++)
	{
		addAndMakeVisible(_sliders[i]);
		_sliders[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
		_attachments[i].reset(new juce::SliderParameterAttachment(*(sound->getParameter(i)), _sliders[i]));

		addAndMakeVisible(_labels[i]);
		_labels[i].setText(paramNames[i], juce::dontSendNotification);
	}
}

void ParameterView::resized()
{
	auto w = getWidth() / 4;
	auto h = getHeight() / 2;

	auto top = getLocalBounds();
	auto bottom = top.removeFromBottom(h);

	top = top.withSizeKeepingCentre(3 * w, h);
	bottom = bottom.withSizeKeepingCentre(4 * w, h);

	const auto setSliderBounds = [this](int index, juce::Rectangle<int> bounds)
	{
		auto sliderHeight = bounds.getHeight() * 6 / 7;
		_sliders[index].setBounds(bounds.removeFromTop(sliderHeight));
		_sliders[index].setTextBoxStyle(juce::Slider::TextBoxBelow, true, bounds.getWidth(), bounds.getHeight());
		_labels[index].setBounds(bounds);
	};

	for (int i = 0; i < 3; i++)
		setSliderBounds(i, top.removeFromLeft(w));

	for (int i = 3; i < 7; i++)
		setSliderBounds(i, bottom.removeFromLeft(w));
}
