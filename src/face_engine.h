#pragma once
#include <opencv2/objdetect/face.hpp>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QImage>
#include <memory>
struct FaceResult { QImage image; QString message; bool ok=false; };
class FaceEngine {
public:
    bool load(const QString& models,const QString& library,QString& error);
    FaceResult enroll(const cv::Mat& image,const QString& name);
    FaceResult recognize(const cv::Mat& image,double threshold);
private:
    cv::Mat faces(const cv::Mat& image);
    cv::Mat feature(const cv::Mat& image,const cv::Mat& face);
    cv::Ptr<cv::FaceDetectorYN> detector_;
    cv::Ptr<cv::FaceRecognizerSF> recognizer_;
    std::unique_ptr<QTemporaryDir> temporary_;
    QString library_,hash_;
    QJsonArray samples_;
};
