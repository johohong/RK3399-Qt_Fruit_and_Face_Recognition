#pragma once
#include <QString>
#include <opencv2/core.hpp>
#include <vector>
struct Detection { int classId; QString className; float confidence; cv::Rect2f box; };
struct DetectorOptions { QString backend="mnn"; QString modelPath; int threads=4; float confidence=0.25f; float nms=0.45f; };
struct DetectionResult { bool ok=false; QString error; QString protocol; std::vector<Detection> detections; double preprocessMs=0,inferenceMs=0,postprocessMs=0,totalMs=0; };
struct TensorData { std::vector<int> shape; std::vector<float> values; };
struct LetterboxInfo { float ratio=1; int left=0,top=0; cv::Size original; };
