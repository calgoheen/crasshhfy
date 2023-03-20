#pragma once

#include <JuceHeader.h>
#include "Sampler.h"

class SamplePad : public juce::Component
{
public:
	SamplePad(int midiNoteNumber);
	~SamplePad() override = default;

	void paint(juce::Graphics& g) override;

	int getMidiNote() const;
	void setNoteOn(bool noteIsOn);
	void setNoteSelected(bool isSelected);

private:
	const int _midiNote;

	juce::String _label;
	bool _noteIsOn{ false };
	bool _noteIsSelected{ false };

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
	void setSelectedNote(int idx);

	std::function<void(int)> onSelectedNoteChange{ nullptr };

private:
	void timerCallback() override;
	void mouseDown(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;
	void updateSelectedNote();

	static constexpr int _timerLengthMs{ 30 };

	const int _midiChannel;
	const int _baseMidiNote;
	juce::MidiKeyboardState& _midiState;

	juce::OwnedArray<SamplePad> _notes;
	int _lastNoteIndex{ 0 };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleKeyboard)
};

class ParameterView : public juce::Component
{
public:
	static constexpr int numParameters = SoundWithParameters::kNumParameters;

	ParameterView(SoundWithParameters* sound);
	~ParameterView() override = default;

	void resized() override;

private:
	std::array<juce::Slider, numParameters> _sliders;
	std::array<std::unique_ptr<juce::SliderParameterAttachment>, numParameters> _attachments;
	std::array<juce::Label, numParameters> _labels;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterView)
};
