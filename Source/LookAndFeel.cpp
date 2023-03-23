#include "LookAndFeel.h"

const juce::Colour CustomLookAndFeel::Palette::background 	= { 37, 38, 41 };
const juce::Colour CustomLookAndFeel::Palette::light1 		= juce::Colours::white;
const juce::Colour CustomLookAndFeel::Palette::light2 		= { 174, 173, 174 };
const juce::Colour CustomLookAndFeel::Palette::light3 		= juce::Colours::lightgrey;
const juce::Colour CustomLookAndFeel::Palette::dark 		= Palette::light2.darker(0.8f);
const juce::Colour CustomLookAndFeel::Palette::highlight1 	= juce::Colours::pink;
const juce::Colour CustomLookAndFeel::Palette::highlight2 	= { 95, 143, 190 };


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
	auto textColour = Palette::light1;
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

void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          				 float sliderPos, float, float,
                          				 const juce::Slider::SliderStyle style, juce::Slider&)
{
    static constexpr float stroke = 2.0f;
	static constexpr float diameter = 8.0f;

	auto pathColour = Palette::light2;
	auto dotColour = Palette::light1;

    auto bounds = juce::Rectangle<int>{ x, y, w, h }.toFloat();

	bool isVertical = (style == juce::Slider::SliderStyle::LinearVertical);

	auto startPoint = isVertical ? juce::Point<float>{ bounds.getCentreX(), bounds.getBottom() } 
                                 : juce::Point<float>{ bounds.getX(), bounds.getCentreY() };

	auto endPoint = isVertical ? juce::Point<float>{ bounds.getCentreX(), bounds.getY() }
                               : juce::Point<float>{ bounds.getRight(), bounds.getCentreY() };

	auto currentValuePoint = isVertical ? juce::Point<float>{ bounds.getCentreX(), sliderPos }
                                        : juce::Point<float>{ sliderPos, bounds.getCentreY() };

	g.setColour(pathColour);
	g.drawLine({ startPoint, endPoint }, stroke);

	g.setColour(dotColour);
	g.fillEllipse(juce::Rectangle<float>().withCentre(currentValuePoint).withSizeKeepingCentre(diameter, diameter));
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
										 float sliderPos, float rotaryStartAngle,
										 float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>{ x, y, w, h }.toFloat();
    auto arcRadius = juce::jmin(w, h) / 2.0f - 2.0f;

	auto bottomArcColour = Palette::light2;
	auto topArcColour = Palette::light1;
	static constexpr float strokeSize = 2.0f;

    juce::Path bottomArc, topArc;

    bottomArc.addCentredArc(bounds.getCentreX(),
        					bounds.getCentreY(),
        					arcRadius,
        					arcRadius,
        					0.0f,
        					rotaryStartAngle,
        					rotaryEndAngle,
        					true);

    auto topArcEndAngle = sliderPos * (rotaryEndAngle - rotaryStartAngle) + rotaryStartAngle;

    topArc.addCentredArc(bounds.getCentreX(),
        				 bounds.getCentreY(),
        				 arcRadius,
        				 arcRadius,
        				 0.0f,
        				 rotaryStartAngle,
        				 topArcEndAngle,
        				 true);
	
    topArc.lineTo(bounds.getCentreX(), bounds.getCentreY());

	auto stroke = juce::PathStrokeType(strokeSize, juce::PathStrokeType::JointStyle::curved);

    g.setColour(bottomArcColour);
    g.strokePath(bottomArc, stroke);

    g.setColour(topArcColour);
    g.strokePath(topArc, stroke);
}

void CustomLookAndFeel::drawControlPanel(juce::Graphics& g, juce::Rectangle<int> bounds)
{
	static constexpr float corner = 5.0f;
	static constexpr float stroke = 2.0f;

	auto floatBounds = bounds.toFloat();
	auto backgroundColour = Palette::light2.darker(0.8f);

	g.setColour(backgroundColour);
	g.fillRoundedRectangle(floatBounds, corner);
}
