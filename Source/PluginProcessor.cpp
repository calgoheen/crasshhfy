#include "PluginProcessor.h"
#include "PluginEditor.h"

CrasshhfyAudioProcessor::CrasshhfyAudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    _parameters(*this, nullptr, "PARAMS", createParameterLayout())
{
    for (int i = 0; i < numSounds; i++)
    {
        auto sound = new DrumSound(baseMidiNote + i);
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

CrasshhfyAudioProcessor::~CrasshhfyAudioProcessor()
{
}

bool CrasshhfyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void CrasshhfyAudioProcessor::prepareToPlay(double sampleRate, int)
{
    // Unused functionality but the Synthesiser class requires a real sample rate value
    _synth.setCurrentPlaybackSampleRate(sampleRate);

    // Sample rates are actually handled by the Sound class
    for (int i = 0; i < _synth.getNumSounds(); i++)
        if (auto s = dynamic_cast<Sound*>(_synth.getSound(i).get()))
            s->setSampleRate(sampleRate);

    _midiState.reset();
}

void CrasshhfyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    _midiState.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);

    buffer.clear();
    _synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    midi.clear();
}

void CrasshhfyAudioProcessor::releaseResources()
{
}

void CrasshhfyAudioProcessor::getStateInformation (juce::MemoryBlock&)
{
}

void CrasshhfyAudioProcessor::setStateInformation (const void*, int)
{
}

bool CrasshhfyAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CrasshhfyAudioProcessor::createEditor()
{
    return new CrasshhfyAudioProcessorEditor (*this);
}

juce::MidiKeyboardState& CrasshhfyAudioProcessor::getMidiKeyboardState()
{
    return _midiState;
}

DrumSound* CrasshhfyAudioProcessor::getSound(int soundIndex)
{
    jassert(juce::isPositiveAndBelow(soundIndex, _sounds.size()));
    return _sounds[soundIndex];
}

void CrasshhfyAudioProcessor::saveSample(int soundIndex, const juce::File& file)
{
    auto sample = getSound(soundIndex)->getSample();

    if (sample != nullptr)
        Utils::writeWavFile(sample->data, sample->sampleRate, file);
}

void CrasshhfyAudioProcessor::generateSample(int soundIndex)
{
    juce::AudioBuffer<float> data{ UnetModelInference::numChannels, UnetModelInference::outputSize };
    size_t classification = 0;
    float confidence = 0;

    unetModelInference.process(data.getWritePointer(0), 10);
    classifierModelInference.process(data.getReadPointer(0), &classification, &confidence);

    Utils::normalize(data);
    data.applyGain(juce::Decibels::decibelsToGain(-3.0f));
    
    // 0 = Kick, 1 = Hat, 2 = Snare
    Drum d;
    d.sample = new Sample{ std::move(data), UnetModelInference::sampleRate };
    d.drumType = static_cast<DrumType>(classification + 1);
    d.confidence = confidence;

    getSound(soundIndex)->loadDrum(d);
}

void CrasshhfyAudioProcessor::drumifySample(int soundIndex, const juce::File& file)
{
    auto [inputData, fs] = Utils::readWavFile(file);

    if (inputData.getNumSamples() == 0)
        return;

    inputData.setSize(UnetModelInference::numChannels, UnetModelInference::outputSize, true, true);

    juce::AudioBuffer<float> outputData{ UnetModelInference::numChannels, UnetModelInference::outputSize };
    size_t classification = 0;
    float confidence = 0;

    Utils::normalize(inputData);

    unetModelInference.processSeeded(outputData.getWritePointer(0), inputData.getReadPointer(0), 10);
    classifierModelInference.process(outputData.getReadPointer(0), &classification, &confidence);

    Utils::normalize(outputData);
    outputData.applyGain(juce::Decibels::decibelsToGain(-3.0f));
    
    // 0 = Kick, 1 = Hat, 2 = Snare
    Drum d;
    d.sample = new Sample{ std::move(outputData), UnetModelInference::sampleRate };
    d.drumType = static_cast<DrumType>(classification + 1);
    d.confidence = confidence;

    getSound(soundIndex)->loadDrum(d);
}

void CrasshhfyAudioProcessor::inpaintSample(int soundIndex, const juce::File& file, bool half)
{
    auto [inputData, fs] = Utils::readWavFile(file);

    if (inputData.getNumSamples() == 0)
        return;

    inputData.setSize(UnetModelInference::numChannels, UnetModelInference::outputSize, true, true);

    juce::AudioBuffer<float> outputData{ UnetModelInference::numChannels, UnetModelInference::outputSize };
    size_t classification = 0;
    float confidence = 0;

    Utils::normalize(inputData);

    unetModelInference.processSeededInpainting(outputData.getWritePointer(0), inputData.getReadPointer(0), half, 10);
    classifierModelInference.process(outputData.getReadPointer(0), &classification, &confidence);

    Utils::normalize(outputData);
    outputData.applyGain(juce::Decibels::decibelsToGain(-3.0f));

    // 0 = Kick, 1 = Hat, 2 = Snare
    Drum d;
    d.sample = new Sample{ std::move(outputData), UnetModelInference::sampleRate };
    d.drumType = static_cast<DrumType>(classification + 1);
    d.confidence = confidence;

    getSound(soundIndex)->loadDrum(d);
}

const juce::String CrasshhfyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CrasshhfyAudioProcessor::acceptsMidi() const
{
    return true;
}

bool CrasshhfyAudioProcessor::producesMidi() const
{
    return false;
}

bool CrasshhfyAudioProcessor::isMidiEffect() const
{
    return false;
}

double CrasshhfyAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CrasshhfyAudioProcessor::getNumPrograms()
{
    return 1;
}

int CrasshhfyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CrasshhfyAudioProcessor::setCurrentProgram(int)
{
}

const juce::String CrasshhfyAudioProcessor::getProgramName(int)
{
    return {};
}

void CrasshhfyAudioProcessor::changeProgramName(int, const juce::String&)
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CrasshhfyAudioProcessor::createParameterLayout()
{
    std::vector<ParameterDefinition> definitions = {
        { "Gain", "Gain", "", { 0.0f, 1.0f }, 0.5f }
    };

    std::vector<std::unique_ptr<juce::AudioParameterFloat>> params;
    for (auto& d : definitions)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ d.id, 1 }, 
                                                                     d.name, 
                                                                     d.range, 
                                                                     d.defaultValue,
                                                                     d.label));

    return { params.begin(), params.end() };
}

Drum CrasshhfyAudioProcessor::renderCRASHSample()
{
    juce::AudioBuffer<float> data{ UnetModelInference::numChannels, UnetModelInference::outputSize };
    size_t classification = 0;
    float confidence = 0;

    unetModelInference.process(data.getWritePointer(0), 10);
    classifierModelInference.process(data.getReadPointer(0), &classification, &confidence);
    
    // 0 = Kick, 1 = Hat, 2 = Snare
    Drum output;
    output.sample = new Sample{ std::move(data), UnetModelInference::sampleRate };
    output.drumType = static_cast<DrumType>(classification);
    output.confidence = confidence;

    return output;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrasshhfyAudioProcessor();
}
