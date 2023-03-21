#include "PluginProcessor.h"
#include "PluginEditor.h"

Text2SampleAudioProcessor::Text2SampleAudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    _parameters(*this, nullptr, "PARAMS", createParameterLayout())
{
    for (int i = 0; i < numSounds; i++)
    {
        auto sound = new SoundWithParameters(baseMidiNote + i);
        _sounds.push_back(sound);
        _synth.addSound(sound);
    }

    for (int i = 0; i < numVoices; i++)
    {
        auto voice = new Voice();
        _voices.push_back(voice);
        _synth.addVoice(voice);
    }
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

    _midiState.reset();
}

void Text2SampleAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    _midiState.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);

    buffer.clear();
    _synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    Limiter limit{ buffer };

    midi.clear();
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

juce::MidiKeyboardState& Text2SampleAudioProcessor::getMidiKeyboardState()
{
    return _midiState;
}

SoundWithParameters* Text2SampleAudioProcessor::getSound(int soundIndex)
{
    jassert(juce::isPositiveAndBelow(soundIndex, _sounds.size()));
    return _sounds[soundIndex];
}

void Text2SampleAudioProcessor::loadSample(int soundIndex, Sample::Ptr sample)
{
    jassert(juce::isPositiveAndBelow(soundIndex, _synth.getNumSounds()));

    if (auto sound = dynamic_cast<Sound*>(_synth.getSound(soundIndex).get()))
        sound->setSample(sample);
}

void Text2SampleAudioProcessor::saveSample(int soundIndex, const juce::File& file)
{
    jassert(juce::isPositiveAndBelow(soundIndex, _synth.getNumSounds()));

    if (auto sound = dynamic_cast<Sound*>(_synth.getSound(soundIndex).get()))
    {
        auto sample = sound->getSample();

        if (sample != nullptr)
            Utils::writeWavFile(sample->data, sample->sampleRate, file);
    }
}

void Text2SampleAudioProcessor::loadSampleFromFile(int soundIndex, const juce::File& file)
{
    auto [data, fs] = Utils::readWavFile(file);
    loadSample(soundIndex, new Sample{ std::move(data), fs });
}

void Text2SampleAudioProcessor::generateSample(int soundIndex) {
    Drum drum;
    renderCRASHSample(&drum);
    loadSample(soundIndex, drum.sample);
}
const juce::String Text2SampleAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Text2SampleAudioProcessor::acceptsMidi() const
{
    return true;
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

void Text2SampleAudioProcessor::renderCRASHSample(Drum *d)
{
    juce::AudioBuffer<float> data;
    size_t classification = 0;
    float confidence = 0;
    data.setSize(UnetModelInference::numChannels, UnetModelInference::outputSize);
    unetModelInference.process(data.getWritePointer(0));
    classifierModelInference.process(data.getReadPointer(0), &classification, &confidence);
    // 0 = Kick, 1 = Hat, 2 = Snare
    auto drumType = static_cast<DrumClass>(classification);
    d->sample = new Sample{std::move(data), UnetModelInference::sampleRate};
    d->drumType = drumType;
    d->confidence = confidence;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Text2SampleAudioProcessor();
}
