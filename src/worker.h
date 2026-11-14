#pragma once
#include "detector.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>
#include <QJsonObject>
class DetectorWorker:public QThread{
 Q_OBJECT
public: explicit DetectorWorker(DetectorOptions options,QObject* parent=nullptr);~DetectorWorker()override;void submit(const cv::Mat& image,quint64 generation);void stop();
signals:void loaded(bool ok,const QString& error);void completed(const QImage& image,const QJsonObject& result,quint64 generation);
protected:void run()override;
private:DetectorOptions options_;QMutex mutex_;QWaitCondition wake_;bool stopping_=false;cv::Mat pending_;quint64 generation_=0;
};
