#include "worker.h"
#include "io.h"
#include <QMutexLocker>
#include <opencv2/imgproc.hpp>
DetectorWorker::DetectorWorker(DetectorOptions options,QObject* parent):QThread(parent),options_(std::move(options)){}
DetectorWorker::~DetectorWorker(){stop();wait();}
void DetectorWorker::submit(const cv::Mat& image,quint64 generation){QMutexLocker lock(&mutex_);if(stopping_)return;pending_=image.clone();generation_=generation;wake_.wakeOne();}
void DetectorWorker::stop(){QMutexLocker lock(&mutex_);stopping_=true;pending_.release();wake_.wakeOne();}
void DetectorWorker::run(){auto detector=createDetector();QString error;bool ok=detector->load(options_,error);emit loaded(ok,error);if(!ok)return;for(;;){cv::Mat image;quint64 generation;{QMutexLocker lock(&mutex_);while(!stopping_&&pending_.empty())wake_.wait(&mutex_);if(stopping_)break;image=pending_;pending_.release();generation=generation_;}auto result=detector->detect(image);QImage display;if(result.ok){cv::Mat drawn=annotate(image,result),rgb;cv::cvtColor(drawn,rgb,cv::COLOR_BGR2RGB);display=QImage(rgb.data,rgb.cols,rgb.rows,int(rgb.step),QImage::Format_RGB888).copy();}emit completed(display,resultJson(result),generation);}detector->release();}
