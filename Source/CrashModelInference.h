/*

iPlug2 ONNX Runtime inference example
LICENSE: MIT

*/

#pragma once

#include "onnxruntime_cxx_api.h"
#include "model.ort.h"
#include "classifier_model.ort.h"

#include <vector>
#include <array>
#include <random>

class CrashModelInference
{
public:
  static constexpr int outputSize = 21000;
  static constexpr int numChannels = 1;
  static constexpr double sampleRate = 44.1e3;
  static constexpr int classifierOut = 3;

  CrashModelInference()
  {
    Ort::SessionOptions sessionOptions;

    sessionOptions.SetIntraOpNumThreads(1);
    sessionOptions.SetInterOpNumThreads(1);

    mSession1 = std::make_unique<Ort::Session>(mEnv, (void *)model_ort_start, model_ort_size, sessionOptions);
    mSession2 = std::make_unique<Ort::Session>(mEnv, (void *)classifier_model_ort_start, classifier_model_ort_size, sessionOptions);
    info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

    mInputShapes1 = GetInputShapes(mSession1);
    mOutputShapes1 = GetOutputShapes(mSession1);

    mInputNames1 = GetInputNames(mSession1);
    mOutputNames1 = GetOutputNames(mSession1);

    mInputShapes2 = GetInputShapes(mSession2);
    mOutputShapes2 = GetOutputShapes(mSession2);

    mInputNames2 = GetInputNames(mSession2);
    mOutputNames2 = GetOutputNames(mSession2);

    // Fix the input/output shapes so that they are (1, outputSize)
    mInputShapes1[0] = {1, outputSize};
    mOutputShapes1[0] = {1, outputSize};

    mInputShapes2[0] = {1, outputSize};
    mOutputShapes2[0] = {1, classifierOut};

    mXScratch.resize(outputSize);
    mYScratch.resize(outputSize);
    mNoise.resize(outputSize);
    mSig.resize(nbSteps);
    mClassificationOut.resize(classifierOut);


    mInputTensors.push_back(Ort::Value::CreateTensor<float>(info, mXScratch.data(), mXScratch.size(), mInputShapes1[0].data(), mInputShapes1[0].size()));
    mInputTensors.push_back(Ort::Value::CreateTensor<double>(info, sigVal.data(), sigVal.size(), mInputShapes1[1].data(), mInputShapes1[1].size()));
    mOutputTensors.push_back(Ort::Value::CreateTensor<float>(info, mYScratch.data(), mYScratch.size(), mOutputShapes1[0].data(), mOutputShapes1[0].size()));

    mClassificationOutputTensors.push_back(Ort::Value::CreateTensor<float>(info, mClassificationOut.data(), mClassificationOut.size(), mOutputShapes2[0].data(), mOutputShapes2[0].size()));

    // Prime onnxruntime, so that it doesn't allocate in the RT Thread
    RunInference();
  }
  void process(float *output, int* classification, float* confidence)
  {
    RunInference();

    memcpy(output, mYScratch.data(), outputSize * sizeof(float));
    memcpy(classification, &argMax, 1 * sizeof(int));
    memcpy(confidence, &confidenceVal, 1 * sizeof(float));
    // Kick,Hat, Snare

  }

private:
  void RunInference()
  {
    // Initialize variables
    auto [s, m] = create_schedules();
    mSig = s;
    mMean = m;
    std::fill(mYScratch.begin(), mYScratch.end(), 0.0f);
    std::fill(mClassificationOut.begin(), mClassificationOut.end(), 0.0f);
    for (size_t i = 0; i < outputSize; i++)
      mNoise[i] = d(mersenne_engine);
    mXScratch = mNoise;
    const char *inputNames1Cstrs[] = {mInputNames1[0].c_str(), mInputNames1[1].c_str()};
    const char *outputNames1Cstrs[] = {mOutputNames1[0].c_str()};

    const char *inputNames2Cstrs[] = {mInputNames2[0].c_str(), mInputNames2[1].c_str()};
    const char *outputNames2Cstrs[] = {mOutputNames2[0].c_str()};

    float alphaMix[3] = {0.0, 1.0, 0.0};

    // Begin diffusion
    for (size_t n = nbSteps - 1; n > 0; n--)
    {
      // Run classifier


//      std::vector<float> gradClassifier;
//      gradClassifier.resize(classifierOut);


      sigVal = {static_cast<double>(mSig[n])};
      mSession1->Run(mRunOptions, inputNames1Cstrs, mInputTensors.data(), mInputTensors.size(), outputNames1Cstrs, mOutputTensors.data(), mOutputTensors.size());

      // Create gaussian noise based on noise schedule
      for (size_t i = 0; i < outputSize; i++)
      {
        float newNoise = d(mersenne_engine);
        mNoise[i] =
            mSig[n - 1] * powf(1.0f - powf(mSig[n - 1] * mMean[n] / (mSig[n] * mMean[n - 1]), 2.0f), 0.5f) * newNoise;
      }
      float scale = mMean[n] / mMean[n - 1] * powf(mSig[n - 1], 2.0f) / mSig[n] - mMean[n - 1] / mMean[n] * mSig[n];
      // Next input is current input + scaled output + new noise
      for (size_t i = 0; i < outputSize; i++)
        mXScratch[i] = mMean[n - 1] / mMean[n] * mXScratch[i] + scale * mYScratch[i] + mNoise[i];
    }

    // Final run, output is subtraction of previous output and scaled final output
    std::vector<float> diffuseOut = mXScratch;
    sigVal = {static_cast<double>(mSig[0])};
    float scale = mSig[0];
    mSession1->Run(mRunOptions, inputNames1Cstrs, mInputTensors.data(), mInputTensors.size(), outputNames1Cstrs, mOutputTensors.data(), mOutputTensors.size());
    for (size_t i = 0; i < outputSize; i++)
      mYScratch[i] = (diffuseOut[i] - scale * mYScratch[i]) / mMean[0];

    sigVal = {static_cast<double>(0)};
    memcpy(mXScratch.data(), mYScratch.data(), outputSize);
    mSession2->Run(mRunOptions, inputNames2Cstrs, mInputTensors.data(), mInputTensors.size(), outputNames2Cstrs, mClassificationOutputTensors.data(), mClassificationOutputTensors.size());
//    auto it = max_element(std::begin(mClassificationOutputTensors), std::end(mClassificationOutputTensors)); // C++11
    argMax = std::distance(mClassificationOut.begin(), max_element(mClassificationOut.begin(), mClassificationOut.end()));
    confidenceVal = mClassificationOut[argMax];
  }

  inline std::vector<std::vector<int64_t>> GetInputShapes(std::unique_ptr<Ort::Session> &mSession) const
  {
    size_t node_count = mSession->GetInputCount();
    std::vector<std::vector<int64_t>> out(node_count);
    for (size_t i = 0; i < node_count; i++)
      out[i] = mSession->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
    return out;
  }

  inline std::vector<std::vector<int64_t>> GetOutputShapes(std::unique_ptr<Ort::Session> &mSession) const
  {
    size_t node_count = mSession->GetOutputCount();
    std::vector<std::vector<int64_t>> out(node_count);
    for (size_t i = 0; i < node_count; i++)
      out[i] = mSession->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
    return out;
  }

  inline std::vector<std::string> GetInputNames(std::unique_ptr<Ort::Session> &mSession) const
  {
    Ort::AllocatorWithDefaultOptions allocator;
    size_t node_count = mSession->GetInputCount();
    std::vector<std::string> out(node_count);
    for (size_t i = 0; i < node_count; i++)
    {
      auto tmp = mSession->GetInputNameAllocated(i, allocator);
      out[i] = tmp.get();
    }
    return out;
  }

  inline std::vector<std::string> GetOutputNames(std::unique_ptr<Ort::Session> &mSession) const
  {
    Ort::AllocatorWithDefaultOptions allocator;
    size_t node_count = mSession->GetOutputCount();
    std::vector<std::string> out(node_count);
    for (size_t i = 0; i < node_count; i++)
    {
      auto tmp = mSession->GetOutputNameAllocated(i, allocator);
      out[i] = tmp.get();
    }
    return out;
  }

  inline float sigma(float t) const
  {
    return 0.5f * (1.0f - juce::dsp::FastMathApproximations::cos(MathConstants<float>::pi * t));
  }

  inline float mean(float t) const
  {
    return powf(powf(1.0f - sigma(t), 2.0f), 0.5f);
  }

  std::tuple<std::vector<float>, std::vector<float>> create_schedules()
  {
    std::vector<float> tSchedule(nbSteps + 1);
    std::vector<float> sigmaSchedule(nbSteps + 1);
    std::vector<float> mSchedule(nbSteps + 1);
    for (std::size_t i = 0; i < nbSteps + 1; i++)
    {
      tSchedule[i] = (t_max - t_min) * float(i) / float(nbSteps) + t_min;
    }
    for (std::size_t i = 0; i < nbSteps + 1; i++)
    {
      sigmaSchedule[i] = sigma(tSchedule[i]);
    }
    for (std::size_t i = 0; i < nbSteps + 1; i++)
    {
      mSchedule[i] = mean(tSchedule[i]);
    }
    return {sigmaSchedule, mSchedule};
  }

  Ort::Env mEnv{};
  Ort::RunOptions mRunOptions{nullptr};
  Ort::MemoryInfo info{nullptr};
  std::unique_ptr<Ort::Session> mSession1;
  std::unique_ptr<Ort::Session> mSession2;

  std::vector<float> mXScratch;       // noise input
  std::vector<float> mSig;            // sigma
  std::vector<float> mMean;           // mean
  std::vector<float> mYScratch;       // audio output
  std::vector<float> mClassificationOut;       // classification output
  std::vector<float> mNoise;          // noise temp
  std::vector<double> sigVal = {0.0}; // sigma input

  std::vector<Ort::Value> mInputTensors;
  std::vector<std::vector<int64_t>> mInputShapes1;
  std::vector<std::vector<int64_t>> mInputShapes2;

  std::vector<Ort::Value> mOutputTensors;
  std::vector<Ort::Value> mClassificationOutputTensors;
  std::vector<std::vector<int64_t>> mOutputShapes1;
  std::vector<std::vector<int64_t>> mOutputShapes2;

  std::vector<std::string> mInputNames1;
  std::vector<std::string> mInputNames2;
  std::vector<std::string> mOutputNames1;
  std::vector<std::string> mOutputNames2;

  std::random_device rnd_device;              // random generator
  std::mt19937 mersenne_engine{rnd_device()}; // Generates random integers
  std::normal_distribution<float> d{0, 1};

  float t_min = 0.007f;
  float t_max = 1.0f - 0.007f;
  size_t nbSteps = 10;
  size_t argMax = 0;
  float confidenceVal = 0;
};
