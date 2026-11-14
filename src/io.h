#pragma once
#include "detection_types.h"
#include <QJsonObject>
#include <QCommandLineParser>
cv::Mat readImage(const QString& path);
bool writeImage(const QString& path,const cv::Mat& image);
cv::Mat annotate(const cv::Mat& image,const DetectionResult& result);
QJsonObject resultJson(const DetectionResult& result);
void addDetectorArguments(QCommandLineParser& parser);
bool parseDetectorArguments(const QCommandLineParser& parser,DetectorOptions& options,QString& error);
