/* 

iPlug2 ONNX Runtime inference example
LICENSE: MIT

*/

#pragma once

#include "onnxruntime_cxx_api.h"
#include "model.ort.h"

#include <vector>
#include <array>

class LSTMModelInference
{
public:
  static constexpr int kMaxBufferSize = 128;

  LSTMModelInference()
  {
    Ort::SessionOptions sessionOptions;

    sessionOptions.SetIntraOpNumThreads(1);
    sessionOptions.SetInterOpNumThreads(1);

    mSession = std::make_unique<Ort::Session>(mEnv, (void*) model_ort_start, model_ort_size, sessionOptions);
    auto info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    
    mInputShapes = GetInputShapes();
    mOutputShapes = GetOutputShapes();
    
    mInputNames = GetInputNames();
    mOutputNames = GetOutputNames();
    
    // override the -1
    mInputShapes[0] = {kMaxBufferSize};
    mOutputShapes[0] = {kMaxBufferSize};

    mXScratch.resize(kMaxBufferSize);
    mYScratch.resize(kMaxBufferSize);
    std::fill(mXScratch.begin(), mXScratch.end(), 0.0f);
    std::fill(mYScratch.begin(), mYScratch.end(), 0.0f);

    mHScratch.resize(mInputShapes[1][0] * mInputShapes[1][1]);
    std::fill(mHScratch.begin(), mHScratch.end(), 0.0f);

    mCScratch.resize(mInputShapes[2][0] * mInputShapes[2][1]);
    std::fill(mCScratch.begin(), mCScratch.end(), 0.0f);
    
    mInputTensors.push_back(Ort::Value::CreateTensor<float>(info, mXScratch.data(), mXScratch.size(), mInputShapes[0].data(), mInputShapes[0].size()));
    mInputTensors.push_back(Ort::Value::CreateTensor<float>(info, mHScratch.data(), mHScratch.size(), mInputShapes[1].data(), mInputShapes[1].size()));
    mInputTensors.push_back(Ort::Value::CreateTensor<float>(info, mCScratch.data(), mCScratch.size(), mInputShapes[2].data(), mInputShapes[2].size()));
    
    mOutputTensors.push_back(Ort::Value::CreateTensor<float>(info, mYScratch.data(), mYScratch.size(), mOutputShapes[0].data(), mOutputShapes[0].size()));
    mOutputTensors.push_back(Ort::Value::CreateTensor<float>(info, mHScratch.data(), mHScratch.size(), mOutputShapes[1].data(), mOutputShapes[1].size()));
    mOutputTensors.push_back(Ort::Value::CreateTensor<float>(info, mCScratch.data(), mCScratch.size(), mOutputShapes[2].data(), mOutputShapes[2].size()));
    
    // Prime onnxruntime, so that it doesn't allocate in the RT Thread
    RunInference();
  }
  
  void ProcessBlock(float* input, float* output, int nFrames)
  {
    if (nFrames <= kMaxBufferSize)
    {
      memcpy(mXScratch.data(), input, nFrames * sizeof(float));
    }
    else
    {
      assert("Buffer size cannot be > kMaxBufferSize frames");
      return;
    }

    RunInference();
    
    memcpy(output, mYScratch.data(), nFrames * sizeof(float));
  }
  
private:
  void RunInference()
  {
    const char* inputNamesCstrs[] = {mInputNames[0].c_str(), mInputNames[1].c_str(), mInputNames[2].c_str()};
    const char* outputNamesCstrs[] = {mOutputNames[0].c_str(), mOutputNames[1].c_str(), mOutputNames[2].c_str()};

    mSession->Run(mRunOptions, inputNamesCstrs, mInputTensors.data(), mInputTensors.size(), outputNamesCstrs, mOutputTensors.data(), mOutputTensors.size());
  }
  
  inline std::vector<std::vector<int64_t>> GetInputShapes() const {
    size_t node_count = mSession->GetInputCount();
    std::vector<std::vector<int64_t>> out(node_count);
    for (size_t i = 0; i < node_count; i++) out[i] = mSession->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
    return out;
  }

  inline std::vector<std::vector<int64_t>> GetOutputShapes() const {
    size_t node_count = mSession->GetOutputCount();
    std::vector<std::vector<int64_t>> out(node_count);
    for (size_t i = 0; i < node_count; i++) out[i] = mSession->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
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
  
  Ort::Env mEnv {};
  Ort::RunOptions mRunOptions {nullptr};
  std::unique_ptr<Ort::Session> mSession;

  std::vector<float> mXScratch;
  std::vector<float> mYScratch;
  std::vector<float> mHScratch;
  std::vector<float> mCScratch;

  std::vector<Ort::Value> mInputTensors;
  std::vector<std::vector<int64_t>> mInputShapes;

  std::vector<Ort::Value> mOutputTensors;
  std::vector<std::vector<int64_t>> mOutputShapes;
  std::vector<std::string> mInputNames;
  std::vector<std::string> mOutputNames;
};
