#include "face_dialog.h"
#include "io.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QFileDialog>
#include <QCoreApplication>
#include <QDir>
#include <opencv2/imgproc.hpp>
FaceDialog::FaceDialog(const QString& camera,const QString& photos,bool administrator,QWidget* parent):QDialog(parent),administrator_(administrator){
 setWindowTitle("本地人脸登记与识别");resize(1050,760);auto layout=new QVBoxLayout(this);
 auto info=new QLabel("同一姓名可登记多张不同角度照片。识别分数为余弦相似度，不是概率；默认阈值需现场验证。",this);info->setWordWrap(true);layout->addWidget(info);auto exitButton=new QPushButton("退出");exitButton->setObjectName("exitFace");layout->addWidget(exitButton);connect(exitButton,&QPushButton::clicked,this,&QDialog::reject);
 QString models=QCoreApplication::applicationDirPath()+"/face-models",library=QDir(photos).filePath("face_library");
 auto paths=new QLabel("模型："+models+"\n人脸库："+library,this);paths->setWordWrap(true);paths->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(paths);
 view_=new QLabel("摄像头尚未打开",this);view_->setMinimumSize(640,400);view_->setAlignment(Qt::AlignCenter);layout->addWidget(view_,1);
 auto row=new QHBoxLayout;layout->addLayout(row);auto open=new QPushButton("打开摄像头",this);auto close=new QPushButton("关闭摄像头",this);auto start=new QPushButton("开始识别",this);auto stop=new QPushButton("停止识别",this);for(auto b:{open,close,start,stop})row->addWidget(b);
 threshold_=new QDoubleSpinBox(this);threshold_->setRange(0.0,1.0);threshold_->setSingleStep(.01);threshold_->setDecimals(2);threshold_->setValue(.5);threshold_->setEnabled(administrator_);threshold_->setToolTip("阈值越高匹配越严格；仅管理员可调整");row->addWidget(new QLabel("匹配阈值",this));row->addWidget(threshold_);
 auto enrollment=new QHBoxLayout;layout->addLayout(enrollment);name_=new QLineEdit(this);name_->setPlaceholderText("登记姓名，同名照片归入同一个人");name_->setMaxLength(80);enrollment->addWidget(name_);auto capture=new QPushButton("拍照登记一张",this);auto import=new QPushButton("导入多张照片登记",this);enrollment->addWidget(capture);enrollment->addWidget(import);capture->setEnabled(administrator_);import->setEnabled(administrator_);name_->setEnabled(administrator_);if(!administrator_)info->setText(info->text()+" 普通模式：登记人脸和调整阈值需要管理员登录。");
 status_=new QLabel(this);status_->setWordWrap(true);status_->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(status_);timer_=new QTimer(this);timer_->setInterval(50);
 connect(&watcher_,&QFutureWatcher<FaceResult>::finished,this,[this]{auto r=watcher_.result();if(jobGeneration_!=generation_)return;status_->setText((r.ok?QString():QString("错误："))+r.message);if(!r.image.isNull())display(r.image);});
 // Loading and all engine operations run serially off the GUI thread.
 watcher_.setFuture(QtConcurrent::run([this,models,library]{FaceResult r;QString error;loaded_=engine_.load(models,library,error);r.ok=loaded_;r.message=loaded_?QString("模型已加载；可导入照片或打开摄像头登记"):error;return r;}));
 connect(open,&QPushButton::clicked,this,[this,camera]{stopCamera();bool ok=false;
#ifdef Q_OS_WIN
 bool numeric=false;int index=camera.toInt(&numeric);if(numeric&&index>=0)ok=camera_.open(index,cv::CAP_DSHOW);
#else
 ok=camera_.open(camera.toStdString(),cv::CAP_V4L2);
#endif
 if(!ok){status_->setText("摄像头打开失败：检查设备、占用和权限");return;}timer_->start();status_->setText("预览已开启");});
 connect(close,&QPushButton::clicked,this,[this]{stopCamera();});connect(start,&QPushButton::clicked,this,[this]{if(watcher_.isRunning()||!loaded_){status_->setText("模型未就绪或当前任务尚未结束");return;}if(!camera_.isOpened()){status_->setText("请先打开摄像头");return;}recognizing_=true;});connect(stop,&QPushButton::clicked,this,[this]{recognizing_=false;++generation_;status_->setText("识别已停止");});
 connect(timer_,&QTimer::timeout,this,[this]{cv::Mat frame;if(!camera_.read(frame)||frame.empty()){stopCamera();status_->setText("摄像头采集失败或已断开");return;}current_=frame;if(recognizing_)submit(false);else {cv::Mat color;cv::cvtColor(frame,color,cv::COLOR_BGR2RGB);display(QImage(color.data,color.cols,color.rows,int(color.step),QImage::Format_RGB888).copy());}});
 connect(capture,&QPushButton::clicked,this,[this]{if(!administrator_)return;recognizing_=false;submit(true);});
 connect(import,&QPushButton::clicked,this,[this]{if(!administrator_)return;if(watcher_.isRunning()||!loaded_){status_->setText("模型未就绪或任务正在执行");return;}QString person=name_->text().trimmed();if(person.isEmpty()){status_->setText("请先填写姓名");return;}recognizing_=false;auto files=QFileDialog::getOpenFileNames(this,"选择登记照片",QString(),"Images (*.jpg *.jpeg *.png *.bmp)");if(files.isEmpty())return;jobGeneration_=generation_;watcher_.setFuture(QtConcurrent::run([this,files,person]{FaceResult total;QStringList messages;int passed=0;for(const QString& path:files){FaceResult r;try{r=engine_.enroll(readImage(path),person);}catch(const std::exception& e){r.message=QString::fromUtf8(e.what());}if(r.ok)++passed;messages<<path+"："+r.message;}total.ok=passed==files.size();total.message=QString("登记成功 %1/%2\n").arg(passed).arg(files.size())+messages.join("\n");return total;}));});
}
void FaceDialog::display(const QImage& image){view_->setPixmap(QPixmap::fromImage(image).scaled(view_->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));}
void FaceDialog::stopCamera(){timer_->stop();recognizing_=false;++generation_;camera_.release();current_.release();view_->clear();view_->setText("摄像头已关闭");}
void FaceDialog::submit(bool enrollment){if(enrollment&&!administrator_){status_->setText("需要管理员权限");return;}if(watcher_.isRunning())return;if(!loaded_){status_->setText("人脸模型未加载");return;}if(current_.empty()){status_->setText("没有摄像头画面");return;}auto image=current_.clone();QString person=name_->text();double threshold=threshold_->value();jobGeneration_=generation_;watcher_.setFuture(QtConcurrent::run([this,image,person,threshold,enrollment]{return enrollment?engine_.enroll(image,person):engine_.recognize(image,threshold);}));}
FaceDialog::~FaceDialog(){timer_->stop();camera_.release();watcher_.waitForFinished();}
