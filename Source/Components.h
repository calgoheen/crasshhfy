#pragma once

#include <JuceHeader.h>

class SamplePad : public juce::Component
{
public:
	SamplePad(int midiNoteNumber);
	~SamplePad() override = default;

	void paint(juce::Graphics& g) override;

	int getMidiNote() const;
	void setNoteOn(bool noteIsOn);

private:
	const int _midiNote;

	juce::String _label;
	bool _noteIsOn{ false };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePad)
};

class SampleKeyboard : public juce::Component, public juce::Timer
{
public:
	SampleKeyboard(int baseNote, 
				   int numNotes, 
			       juce::MidiKeyboardState& midiKeyboardState, 
				   int midiChannel = 1);

	~SampleKeyboard() override = default;

	void resized() override;

private:
	void timerCallback() override;
	void mouseDown(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;

	static constexpr int _timerLengthMs{ 30 };

	const int _midiChannel;
	const int _baseMidiNote;
	juce::MidiKeyboardState& _midiState;

	juce::OwnedArray<SamplePad> _notes;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleKeyboard)
};
