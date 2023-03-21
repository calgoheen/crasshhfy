/*

LICENSE: MIT

*/

#pragma once

#include "onnxruntime_cxx_api.h"
#include "classifier.ort.h"

#include <vector>
#include <array>
#include <random>

class ClassifierModelInference {
public:
    static constexpr int inputSize = 21000;
    static constexpr int numClasses = 3;
    static constexpr int numChannels = 1;

    ClassifierModelInference() {
        Ort::SessionOptions sessionOptions;

        sessionOptions.SetIntraOpNumThreads(1);
        sessionOptions.SetInterOpNumThreads(1);

        mSession = std::make_unique<Ort::Session>(mEnv, (void *) classifier_ort_start, classifier_ort_size,
                                                  sessionOptions);
        info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

        mInputShapes = GetInputShapes();
        mOutputShapes = GetOutputShapes();

        mInputNames = GetInputNames();
        mOutputNames = GetOutputNames();

        // Fix the input/output shapes so that they are (1, inputSize)
        mInputShapes[0] = {1, inputSize};
        mOutputShapes[0] = {1, numClasses};

        mXScratch.resize(inputSize);
        mYScratch.resize(numClasses);

        mInputTensors.push_back(
                Ort::Value::CreateTensor<float>(info, mXScratch.data(), mXScratch.size(), mInputShapes[0].data(),
                                                mInputShapes[0].size()));
        mInputTensors.push_back(
                Ort::Value::CreateTensor<double>(info, sigVal.data(), sigVal.size(), mInputShapes[1].data(),
                                                 mInputShapes[1].size()));
        mOutputTensors.push_back(
                Ort::Value::CreateTensor<float>(info, mYScratch.data(), mYScratch.size(), mOutputShapes[0].data(),
                                                mOutputShapes[0].size()));

        // Prime onnxruntime, so that it doesn't allocate in the RT Thread
        RunInference();
    }

    void process(const float *input, size_t *classification, float *confidence) {
        memcpy(mXScratch.data(), input, inputSize * sizeof(float));
        RunInference();

        memcpy(classification, &argMax, 1 * sizeof(int));
        memcpy(confidence, &confidenceVal, 1 * sizeof(float));
        // Kick,Hat, Snare
    }

private:
    void RunInference() {
        // Initialize variables
        std::fill(mYScratch.begin(), mYScratch.end(), 0.0f);
        const char *inputNamesCstrs[] = {mInputNames[0].c_str(), mInputNames[1].c_str()};
        const char *outputNamesCstrs[] = {mOutputNames[0].c_str()};

        sigVal = {static_cast<double>(0)};

        // Run inference
        mSession->Run(mRunOptions,
                      inputNamesCstrs, mInputTensors.data(), mInputTensors.size(),
                      outputNamesCstrs, mOutputTensors.data(), mOutputTensors.size());

        argMax = static_cast<size_t>(std::distance(mYScratch.begin(), max_element(mYScratch.begin(), mYScratch.end())));
        confidenceVal = mYScratch[argMax];
        // Kick,Snare,Hat
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

    Ort::Env mEnv{};
    Ort::RunOptions mRunOptions{nullptr};
    Ort::MemoryInfo info{nullptr};
    std::unique_ptr<Ort::Session> mSession;

    std::vector<float> mXScratch;       // noise input
    std::vector<float> mYScratch;       // audio output
    std::vector<double> sigVal = {0.0}; // sigma input

    std::vector<Ort::Value> mInputTensors;
    std::vector<std::vector<int64_t>> mInputShapes;

    std::vector<Ort::Value> mOutputTensors;
    std::vector<std::vector<int64_t>> mOutputShapes;

    std::vector<std::string> mInputNames;
    std::vector<std::string> mOutputNames;

    size_t argMax;
    float confidenceVal;
};
