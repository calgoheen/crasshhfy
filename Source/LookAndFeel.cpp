#include "LookAndFeel.h"

const juce::Colour CustomLookAndFeel::Palette::text 		= juce::Colours::white;
const juce::Colour CustomLookAndFeel::Palette::background 	= juce::Colour(37, 38, 41);
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
    auto backgroundColor = juce::Colour(59, 58, 56);
    auto selectedColor = juce::Colour(81, 145, 194);
    auto j = juce::Justification::centred;

    auto b = button.getLocalBounds().toFloat().reduced (4, 4);

    auto cornerSize = 2;
    g.setColour (Colours::white);
    float lineThickness = 1.0f;
    g.drawRoundedRectangle(b, cornerSize, lineThickness);

    auto leftHalf = b;
    auto rightHalf = b;
    leftHalf.setWidth(b.getWidth() / 2 );
    rightHalf.setWidth(b.getWidth() / 2);
    leftHalf.setX( b.getX());
    rightHalf.setX(b.getWidth() / 2 + 3);
    leftHalf.setY(b.getY() + lineThickness);
    rightHalf.setY(b.getY() + lineThickness);
    leftHalf.setHeight(b.getHeight() - 2 * lineThickness);
    rightHalf.setHeight(b.getHeight() - 2 * lineThickness);
    g.setColour (button.getToggleState() ? backgroundColor : selectedColor);
    g.fillRoundedRectangle(leftHalf, cornerSize);
    g.setColour (button.getToggleState() ?  selectedColor : backgroundColor);
    g.fillRoundedRectangle(rightHalf, cornerSize);

    g.setColour(textColour);
    if (!button.getToggleState())
        g.drawText("Start", leftHalf, j);
    else
        g.drawText("End", rightHalf, j);




}

juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
{
	return juce::Font();
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
											 bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{

    auto flatOnLeft   = button.isConnectedOnLeft();
    auto flatOnRight  = button.isConnectedOnRight();
    auto flatOnTop    = button.isConnectedOnTop();
    auto flatOnBottom = button.isConnectedOnBottom();

    auto width  = (float) button.getWidth()  - 1.0f;
    auto height = (float) button.getHeight() - 1.0f;

    if (width > 0 && height > 0)
    {
        auto cornerSize = jmin (15.0f, jmin (width, height) * 0.45f);
        auto lineThickness = cornerSize    * 0.1f;
        auto halfThickness = lineThickness * 0.5f;

        Path outline;
        outline.addRoundedRectangle (0.5f + halfThickness, 0.5f + halfThickness, width - lineThickness, height - lineThickness,
                                     cornerSize, cornerSize,
                                     ! (flatOnLeft  || flatOnTop),
                                     ! (flatOnRight || flatOnTop),
                                     ! (flatOnLeft  || flatOnBottom),
                                     ! (flatOnRight || flatOnBottom));

        auto outlineColour = button.findColour (button.getToggleState() ? TextButton::textColourOnId
                                                                        : TextButton::textColourOffId);

        juce::ColourGradient gradient;
        if(button.getButtonText() == "Generate") {
            gradient = juce::ColourGradient(juce::Colour(178, 31, 97), 0, 0, juce::Colours::pink, width, height,true);
        } else if (button.getButtonText() == "Drumify") {
            gradient = juce::ColourGradient(juce::Colours::seagreen, 0, 0, juce::Colour(28, 124, 156), width, height,true);
        } else if (button.getButtonText() == "Variation") {
            gradient = juce::ColourGradient(juce::Colour(63, 93, 181), 0, 0, juce::Colour(108, 189, 185), width, height,false);
        }

        g.setGradientFill(gradient);
        g.fillPath (outline);

        if (! button.getToggleState())
        {
            g.setColour (outlineColour);
            g.strokePath (outline, PathStrokeType (lineThickness));
        }
        g.setColour(juce::Colours::grey);
        g.setOpacity(0.5);
        if (!button.isEnabled())
            g.fillPath (outline);
    }
}

