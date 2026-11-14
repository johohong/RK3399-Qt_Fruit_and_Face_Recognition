#pragma once
#include "worker.h"
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <opencv2/videoio.hpp>
class MainWindow:public QMainWindow{
 Q_OBJECT
public:MainWindow(const DetectorOptions& options,const QString& camera);~MainWindow()override;
 bool openImage(const QString& path);bool openCamera();void closeCamera();void detectImage();bool ready()const{return ready_;}QString failure()const{return failure_;}int completedCount()const{return completed_;}
protected:void closeEvent(QCloseEvent* event)override;
private:void setupPhotos();void updateFrame();void showImage(const cv::Mat& image);DetectorWorker* worker_;QLabel *view_,*status_,*photoStatus_;QPushButton *load_,*open_,*close_,*detect_,*capture_;QString photoDirectory_;QTimer timer_;cv::VideoCapture camera_;cv::Mat current_;QString cameraId_,failure_;bool ready_=false,detecting_=false;quint64 generation_=0;int completed_=0;
};
