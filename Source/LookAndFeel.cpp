#include "LookAndFeel.h"

const juce::Colour CustomLookAndFeel::Palette::text 		= juce::Colours::white;
const juce::Colour CustomLookAndFeel::Palette::background 	= juce::Colours::black;
const juce::Colour CustomLookAndFeel::Palette::highlight 	= juce::Colours::pink;

const juce::Font& CustomLookAndFeel::getFont()
{
    static juce::Font font = juce::Font(
		// We can load in a font from BinaryData here
        //juce::Typeface::createSystemTypefaceFor(BinaryData::GalanoGrotesqueRegular_otf, 
        //                                        BinaryData::GalanoGrotesqueRegular_otfSize)
    );

    return font;
}

CustomLookAndFeel::CustomLookAndFeel()
{
}

void CustomLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          				 bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	auto textColour = Palette::text;
}

juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
{
	return juce::Font();
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
											 bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
}
