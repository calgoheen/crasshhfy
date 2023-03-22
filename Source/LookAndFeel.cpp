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

void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          				 float sliderPos, float, float,
                          				 const juce::Slider::SliderStyle style, juce::Slider&)
{
	if (style != juce::Slider::SliderStyle::LinearVertical)
		return;

	static constexpr float stroke = 2.0f;
	static constexpr float diameter = 8.0f;

	auto pathColour = juce::Colours::lightgrey;
	auto dotColour = juce::Colours::white;

	auto bounds = juce::Rectangle<int>{ x, y, w, h }.toFloat();
	auto startPoint = juce::Point<float>{ bounds.getCentreX(), bounds.getBottom() };
	auto endPoint = juce::Point<float>{ bounds.getCentreX(), bounds.getY() };
	auto currentValuePoint = juce::Point<float>{ float(bounds.getCentreX()), sliderPos };

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

	auto bottomArcColour = juce::Colours::grey;
	auto topArcColour = juce::Colours::white;
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
	static constexpr float stroke = 1.0f;

	auto floatBounds = bounds.toFloat();
	auto backgroundColour = juce::Colours::darkgrey;
	auto strokeColour = juce::Colours::lightgrey;

	g.setColour(backgroundColour);
	g.fillRoundedRectangle(floatBounds, corner);

	g.setColour(strokeColour);
	g.drawRoundedRectangle(floatBounds, corner, stroke);
}
