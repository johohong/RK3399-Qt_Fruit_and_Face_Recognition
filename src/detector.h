#pragma once
#include "detection_types.h"
#include <memory>
class IDetector { public: virtual ~IDetector()=default; virtual bool load(const DetectorOptions&,QString& error)=0; virtual DetectionResult detect(const cv::Mat&)=0; virtual void release()=0; virtual QString protocol() const=0; };
std::unique_ptr<IDetector> createDetector();
