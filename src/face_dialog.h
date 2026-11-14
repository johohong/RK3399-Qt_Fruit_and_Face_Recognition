#pragma once
#include "face_engine.h"
#include <QDialog>
#include <QFutureWatcher>
#include <opencv2/videoio.hpp>
class QLabel;class QLineEdit;class QDoubleSpinBox;class QTimer;
class FaceDialog:public QDialog {
public:
 FaceDialog(const QString& camera,const QString& photos,bool administrator=false,QWidget* parent=nullptr);
 ~FaceDialog() override;
private:
 void stopCamera();void submit(bool enrollment);void display(const QImage& image);
 FaceEngine engine_;QFutureWatcher<FaceResult> watcher_;cv::VideoCapture camera_;cv::Mat current_;
 QLabel *view_,*status_;QLineEdit* name_;QDoubleSpinBox* threshold_;QTimer* timer_;
 bool administrator_=false;bool loaded_=false,recognizing_=false;quint64 generation_=0,jobGeneration_=0;
};
