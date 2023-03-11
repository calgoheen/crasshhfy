#include "PluginProcessor.h"
#include "PluginEditor.h"


Text2SampleAudioProcessor::Text2SampleAudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    _parameters(*this, nullptr, "PARAMS", createParameterLayout())
{
    const int c4 = 60;
    for (int i = 0; i < numSounds; i++)
        _synth.addSound(new Sound(c4 + i));

    for (int i = 0; i < numVoices; i++)
        _synth.addVoice(new Voice());
}

Text2SampleAudioProcessor::~Text2SampleAudioProcessor()
{
}

bool Text2SampleAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void Text2SampleAudioProcessor::prepareToPlay(double sampleRate, int)
{
    // Unused functionality but the Synthesiser class requires a real sample rate value
    _synth.setCurrentPlaybackSampleRate(sampleRate);

    // Sample rates are actually handled by the Sound class
    for (int i = 0; i < _synth.getNumSounds(); i++)
        if (auto s = dynamic_cast<Sound*>(_synth.getSound(i).get()))
            s->setSampleRate(sampleRate);
}

void Text2SampleAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();
    _synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}

void Text2SampleAudioProcessor::releaseResources()
{
}

void Text2SampleAudioProcessor::getStateInformation (juce::MemoryBlock&)
{
}

void Text2SampleAudioProcessor::setStateInformation (const void*, int)
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

EditorState* Text2SampleAudioProcessor::getEditorState()
{
    return &_editorState;
}

void Text2SampleAudioProcessor::loadSample(int soundIndex, const juce::File& file)
{
    jassert(juce::isPositiveAndBelow(soundIndex, _synth.getNumSounds()));

    auto [data, fs] = Utils::readWavFile(file);
    if (auto sound = dynamic_cast<Sound*>(_synth.getSound(soundIndex).get()))
    {
        suspendProcessing(true);
        sound->setSampleData(std::move(data), fs);
        suspendProcessing(false);
    }

}

void Text2SampleAudioProcessor::renderCRASHSample()
{
    float output[21000];
    modelInference.process(output);
    Utils::writeWavFile("~/Desktop/output.wav", output, 1, 21000);
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