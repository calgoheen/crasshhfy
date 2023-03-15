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
	// Pad
	auto col = _noteIsOn ? juce::Colours::lightpink : juce::Colours::lightgrey;
	g.setColour(col);
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);

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


SampleKeyboard::SampleKeyboard(int baseNote, int numNotes, juce::MidiKeyboardState& midiKeyboardState, int midiChannel) 
	: _baseMidiNote(baseNote), _midiState(midiKeyboardState), _midiChannel(midiChannel)
{
	for (int i = 0; i < numNotes; i++)
	{
		auto note = _notes.add(new SamplePad(baseNote + i));
		note->addMouseListener(this, false);
		addAndMakeVisible(note);
	}

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

void SampleKeyboard::timerCallback()
{
	for (int i = 0; i < _notes.size(); i++)
	{
		auto noteState = _midiState.isNoteOn(_midiChannel, _baseMidiNote + i);
		_notes[i]->setNoteOn(noteState);
	}
}

void SampleKeyboard::mouseDown(const juce::MouseEvent& e)
{
	for (int i = 0; i < _notes.size(); i++)
		if (_notes[i] == e.eventComponent)
			_midiState.noteOn(_midiChannel, i + _baseMidiNote, 1.0f);
}

void SampleKeyboard::mouseUp(const juce::MouseEvent& e)
{
	for (int i = 0; i < _notes.size(); i++)
		if (_notes[i] == e.eventComponent)
			_midiState.noteOff(_midiChannel, i + _baseMidiNote, 1.0f);
}
