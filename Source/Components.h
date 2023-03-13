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

class SampleKeyboard : public juce::Component
{
public:
	SampleKeyboard(int baseNote, int numNotes);
	~SampleKeyboard() override = default;

	void resized() override;

	void setNoteOn(int noteIndex, bool noteIsOn);

private:
	const int _baseMidiNote;

	std::vector<bool> _noteOnStates;
	juce::OwnedArray<SamplePad> _notes;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleKeyboard)
};
