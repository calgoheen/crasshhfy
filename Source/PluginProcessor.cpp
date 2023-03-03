#include "PluginProcessor.h"
#include "PluginEditor.h"

Text2SampleAudioProcessor::Text2SampleAudioProcessor()
  : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    _parameters(*this, nullptr, "PARAMS", createParameterLayout())
{
}

Text2SampleAudioProcessor::~Text2SampleAudioProcessor()
{
}


bool Text2SampleAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void Text2SampleAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
}

void Text2SampleAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto numSamples = buffer.getNumSamples();
    auto bufferPtr = buffer.getArrayOfWritePointers();

    // Copy to other output channels
    for (int i = 1; i < getTotalNumOutputChannels(); i++)
        buffer.copyFrom(i, 0, buffer.getReadPointer(0), numSamples);

    // Process Gain
    float gain = _parameters.getRawParameterValue("Gain")->load();
    buffer.applyGain(gain);
}

void Text2SampleAudioProcessor::releaseResources()
{
}

void Text2SampleAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
}

void Text2SampleAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
}

bool Text2SampleAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* Text2SampleAudioProcessor::createEditor()
{
    return new Text2SampleAudioProcessorEditor (*this);
}

const juce::String Text2SampleAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Text2SampleAudioProcessor::acceptsMidi() const
{
    return false;
}

bool Text2SampleAudioProcessor::producesMidi() const
{
    return false;
}

bool Text2SampleAudioProcessor::isMidiEffect() const
{
    return false;
}

double Text2SampleAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Text2SampleAudioProcessor::getNumPrograms()
{
    return 1;
}

int Text2SampleAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Text2SampleAudioProcessor::setCurrentProgram(int)
{
}

const juce::String Text2SampleAudioProcessor::getProgramName(int)
{
    return {};
}

void Text2SampleAudioProcessor::changeProgramName(int, const juce::String&)
{
}

juce::AudioProcessorValueTreeState::ParameterLayout Text2SampleAudioProcessor::createParameterLayout()
{
    // Makes the list of parameters easier to read/edit
    struct ParameterDefinition
    {
        juce::String id;
        juce::String name;
        juce::NormalisableRange<float> range;
        float defaultValue;
    };

    std::vector<ParameterDefinition> definitions = {
        { "Gain", "Gain", { 0.0f, 1.0f }, 0.5f }
    };

    std::vector<std::unique_ptr<juce::AudioParameterFloat>> params;
    for (auto& d : definitions)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ d.id, 1 }, 
                                                                     d.name, 
                                                                     d.range, 
                                                                     d.defaultValue));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Text2SampleAudioProcessor();
}

#include "model.ort.c"
