#include "Components.h"

SamplePad::SamplePad(int midiNoteNumber) : _midiNote(midiNoteNumber)
{
	_label = juce::MidiMessage::getMidiNoteName(midiNoteNumber, true, false, 0);
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

	// Label
	g.setColour(juce::Colours::black);
	g.drawText(_label, 
				getLocalBounds().removeFromBottom(getHeight() / 5).toFloat(), 
				juce::Justification::centred, 
				false);
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
	// Quick method of arranging the notes, probably has some flaws
	// For example the first note and last note cannot be sharp
	auto numNotes = _notes.size();
	int numWhiteNotes = 0;
	for (int i = 0; i < numNotes; i++)
		if (!juce::MidiMessage::isMidiNoteBlack(_notes[i]->getMidiNote()))
			++numWhiteNotes;

	auto noteWidth = getWidth() / numWhiteNotes;
	auto noteHeight = getHeight() / 2;
	auto bounds = getLocalBounds().withSizeKeepingCentre(noteWidth * numWhiteNotes, getHeight());
	
	auto blackNoteBounds = bounds.removeFromTop(noteHeight);
	auto whiteNoteBounds = bounds.removeFromBottom(noteHeight);

	for (int i = 0; i < numNotes; i++)
	{
		auto isWhite = !juce::MidiMessage::isMidiNoteBlack(_notes[i]->getMidiNote());
		juce::Rectangle<int> noteBounds;

		if (isWhite)
		{
			noteBounds = whiteNoteBounds.removeFromLeft(noteWidth);
		}
		else if (i != 0)
		{
			auto lastWhiteNoteX = _notes[i - 1]->getBounds().getX();
			auto pos = juce::Point<int>{ lastWhiteNoteX + noteWidth / 2, blackNoteBounds.getY() };
			noteBounds = juce::Rectangle<int>().withSize(noteWidth, noteHeight).withPosition(pos);
		}

		_notes[i]->setBounds(noteBounds);
	}
}

void SampleKeyboard::setSelectedNote(int idx)
{
	if (idx != _lastNoteIndex)
	{
		_lastNoteIndex = idx;
		updateSelectedNote();

		if (onSelectedNoteChange)
			onSelectedNoteChange(idx);
	}
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
