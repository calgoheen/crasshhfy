/*

LICENSE: MIT

*/

#pragma once

#include "onnxruntime_cxx_api.h"
#include "crash.ort.h"

#include <vector>
#include <array>
#include <random>

class UnetModelInference {
public:
    static constexpr int outputSize = 21000;
    static constexpr int numChannels = 1;
    static constexpr double sampleRate = 44.1e3;

    UnetModelInference() {
        Ort::SessionOptions sessionOptions;

        sessionOptions.SetIntraOpNumThreads(1);
        sessionOptions.SetInterOpNumThreads(1);

        mSession = std::make_unique<Ort::Session>(mEnv, (void *) crash_ort_start, crash_ort_size, sessionOptions);
        info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

        mInputShapes = GetInputShapes();
        mOutputShapes = GetOutputShapes();

        mInputNames = GetInputNames();
        mOutputNames = GetOutputNames();

        // Fix the input/output shapes so that they are (1, outputSize)
        mInputShapes[0] = {1, outputSize};
        mOutputShapes[0] = {1, outputSize};

        mXScratch.resize(outputSize);
        mYScratch.resize(outputSize);
        mNoise.resize(outputSize);

        mInpaintScratch.resize(outputSize);


        mOutputTensors.push_back(
                Ort::Value::CreateTensor<float>(info, mYScratch.data(), mYScratch.size(), mOutputShapes[0].data(),
                                                mOutputShapes[0].size()));

    }

    void process(float *output, size_t numSteps) {
        // Noise Input
        for (size_t i = 0; i < outputSize; i++)
            mXScratch[i] = d(mersenne_engine);
        RunInference(numSteps);
        memcpy(output, mYScratch.data(), outputSize * sizeof(float));
    }

    void processSeeded(float *output, const float* seedAudio, size_t numSteps) {
        // Audio Input
        memcpy(mXScratch.data(), seedAudio, outputSize * sizeof (float));
        RunInference(numSteps);
        memcpy(output, mYScratch.data(), outputSize * sizeof(float));
    }

    void processSeededInpainting(float *output, const float* seedAudio, bool paintHalf, size_t numSteps) {
        // Noise Input
        for (size_t i = 0; i < outputSize; i++)
            mXScratch[i] = d(mersenne_engine);
        // Save seed to inpaint buffer
        memcpy(mInpaintScratch.data(), seedAudio, outputSize);
        RunInference(numSteps,true, paintHalf);
        memcpy(output, mYScratch.data(), outputSize * sizeof(float));
    }


private:
    void RunInference(size_t numSteps, bool inpainting = false, bool paintHalf = 0) {
        // Initialize variables
        auto [s, m] = create_schedules(numSteps);
        mSig = s;
        mMean = m;
        std::fill(mYScratch.begin(), mYScratch.end(), 0.0f);

        mInputTensors.clear();
        mInputTensors.push_back(
                Ort::Value::CreateTensor<float>(info, mXScratch.data(), mXScratch.size(), mInputShapes[0].data(),
                                                mInputShapes[0].size()));
        mInputTensors.push_back(
                Ort::Value::CreateTensor<double>(info, sigVal.data(), sigVal.size(), mInputShapes[1].data(),
                                                 mInputShapes[1].size()));

        const char *inputNamesCstrs[] = {mInputNames[0].c_str(), mInputNames[1].c_str()};
        const char *outputNamesCstrs[] = {mOutputNames[0].c_str()};

        // Begin diffusion
        for (size_t n = numSteps - 1; n > 0; n--) {
            sigVal = {static_cast<double>(mSig[n])};
            mSession->Run(mRunOptions, inputNamesCstrs, mInputTensors.data(), mInputTensors.size(), outputNamesCstrs,
                          mOutputTensors.data(), mOutputTensors.size());

            // Create gaussian noise based on noise schedule
            for (size_t i = 0; i < outputSize; i++) {
                float newNoise = d(mersenne_engine);
                mNoise[i] =
                        mSig[n - 1] * powf(1.0f - powf(mSig[n - 1] * mMean[n] / (mSig[n] * mMean[n - 1]), 2.0f), 0.5f) *
                        newNoise;
            }
            float scale =
                    (mMean[n] / mMean[n - 1]) * powf(mSig[n - 1], 2.0f) / mSig[n] - mMean[n - 1] / mMean[n] * mSig[n];

            // mYScratch contains noise
            // Next input is current input + scaled output + new noise
            for (size_t i = 0; i < outputSize; i++)
                mXScratch[i] = (mMean[n - 1] / mMean[n]) * mXScratch[i] + scale * mYScratch[i] + mNoise[i];

            // Inpainting
            if (inpainting) {
                size_t midPoint = outputSize / 2;
                // Create noise
                std::vector<float> noise;
                noise.resize(outputSize);
                for (size_t i = 0; i < outputSize; i++)
                    noise[i] = d(mersenne_engine);
                size_t start, end;
                if (paintHalf) {
                    // Replace first half
                    start = 0;
                    end = midPoint;
                }
                else {
                    // Replace second half
                    start = midPoint;
                    end = outputSize;
                }
                for (size_t i = start; i<end; i++)
                    mXScratch[i] = mMean[n] * mInpaintScratch[i] + mSig[n] * noise[i];
            }
        }

        // Final run, output is subtraction of previous output and scaled final output
        std::vector<float> diffuseOut = mXScratch;
        sigVal = {static_cast<double>(mSig[0])};
        float scale = mSig[0];
        mSession->Run(mRunOptions, inputNamesCstrs, mInputTensors.data(), mInputTensors.size(), outputNamesCstrs,
                      mOutputTensors.data(), mOutputTensors.size());
        for (size_t i = 0; i < outputSize; i++)
            mYScratch[i] = (diffuseOut[i] - scale * mYScratch[i]) / mMean[0];
    }

    inline std::vector<std::vector<int64_t>> GetInputShapes() const {
        size_t node_count = mSession->GetInputCount();
        std::vector<std::vector<int64_t>> out(node_count);
        for (size_t i = 0; i < node_count; i++)
            out[i] = mSession->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
        return out;
    }

    inline std::vector<std::vector<int64_t>> GetOutputShapes() const {
        size_t node_count = mSession->GetOutputCount();
        std::vector<std::vector<int64_t>> out(node_count);
        for (size_t i = 0; i < node_count; i++)
            out[i] = mSession->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
        return out;
    }

    inline std::vector<std::string> GetInputNames() const {
        Ort::AllocatorWithDefaultOptions allocator;
        size_t node_count = mSession->GetInputCount();
        std::vector<std::string> out(node_count);
        for (size_t i = 0; i < node_count; i++) {
            auto tmp = mSession->GetInputNameAllocated(i, allocator);
            out[i] = tmp.get();
        }
        return out;
    }

    inline std::vector<std::string> GetOutputNames() const {
        Ort::AllocatorWithDefaultOptions allocator;
        size_t node_count = mSession->GetOutputCount();
        std::vector<std::string> out(node_count);
        for (size_t i = 0; i < node_count; i++) {
            auto tmp = mSession->GetOutputNameAllocated(i, allocator);
            out[i] = tmp.get();
        }
        return out;
    }

    inline float sigma(float t) const {
        return 0.5f * (1.0f - juce::dsp::FastMathApproximations::cos(MathConstants<float>::pi * t));
    }

    inline float mean(float t) const {
        return powf(powf(1.0f - sigma(t), 2.0f), 0.5f);
    }

    std::tuple<std::vector<float>, std::vector<float>> create_schedules(size_t numSteps) {
        std::vector<float> tSchedule(numSteps + 1);
        std::vector<float> sigmaSchedule(numSteps + 1);
        std::vector<float> mSchedule(numSteps + 1);
        for (std::size_t i = 0; i < numSteps + 1; i++) {
            tSchedule[i] = (t_max - t_min) * float(i) / float(numSteps) + t_min;
        }
        for (std::size_t i = 0; i < numSteps + 1; i++) {
            sigmaSchedule[i] = sigma(tSchedule[i]);
        }
        for (std::size_t i = 0; i < numSteps + 1; i++) {
            mSchedule[i] = mean(tSchedule[i]);
        }
        return {sigmaSchedule, mSchedule};
    }

    Ort::Env mEnv{};
    Ort::RunOptions mRunOptions{nullptr};
    Ort::MemoryInfo info{nullptr};
    std::unique_ptr<Ort::Session> mSession;

    std::vector<float> mXScratch;       // noise input
    std::vector<float> mSig;            // sigma
    std::vector<float> mMean;           // mean
    std::vector<float> mYScratch;       // audio output
    std::vector<float> mNoise;          // noise temp
    std::vector<double> sigVal = {0.0}; // sigma input
    std::vector<float> mInpaintScratch;

    std::vector<Ort::Value> mInputTensors;
    std::vector<std::vector<int64_t>> mInputShapes;

    std::vector<Ort::Value> mOutputTensors;
    std::vector<std::vector<int64_t>> mOutputShapes;

    std::vector<std::string> mInputNames;
    std::vector<std::string> mOutputNames;

    std::random_device rnd_device;              // random generator
    std::mt19937 mersenne_engine{rnd_device()}; // Generates random integers
    std::normal_distribution<float> d{0, 1};

    float t_min = 0.007f;
    float t_max = 1.0f - 0.007f;
};
