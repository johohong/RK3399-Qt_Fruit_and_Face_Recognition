#pragma once
#include "detection_types.h"
QString validateOutputs(const std::vector<TensorData>& outputs);
cv::Mat letterbox(const cv::Mat& image,LetterboxInfo& info);
std::vector<Detection> decode(const std::vector<TensorData>& outputs,const LetterboxInfo& info,const DetectorOptions& options);
