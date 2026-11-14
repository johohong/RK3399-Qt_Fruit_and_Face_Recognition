#include "mainwindow.h"
#include "io.h"
#include "photos.h"
#include "admin_dialog.h"
#include "album.h"
#include "face_dialog.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QJsonArray>
#include <opencv2/imgproc.hpp>
MainWindow::MainWindow(const DetectorOptions& options,const QString& camera):cameraId_(camera){setWindowTitle("FruitDetect2 - 水果检测");resize(1000,720);auto central=new QWidget(this);setCentralWidget(central);auto layout=new QVBoxLayout(central);view_=new QLabel("请选择图片或打开摄像头");view_->setMinimumSize(640,420);view_->setAlignment(Qt::AlignCenter);layout->addWidget(view_,1);status_=new QLabel("正在加载模型："+options.modelPath);status_->setWordWrap(true);layout->addWidget(status_);auto row=new QHBoxLayout;layout->addLayout(row);load_=new QPushButton("打开图片");open_=new QPushButton("打开摄像头");close_=new QPushButton("关闭摄像头");detect_=new QPushButton("开始检测");for(auto b:{load_,open_,close_,detect_})row->addWidget(b);close_->setEnabled(false);detect_->setEnabled(false);worker_=new DetectorWorker(options,this);
connect(load_,&QPushButton::clicked,this,[this]{QString path=QFileDialog::getOpenFileName(this,"选择图片",QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),"Images (*.jpg *.jpeg *.png *.bmp)");if(!path.isEmpty()&&!openImage(path))QMessageBox::warning(this,"图片错误",failure_);});connect(open_,&QPushButton::clicked,this,[this]{if(!openCamera())QMessageBox::warning(this,"摄像头错误",failure_);});connect(close_,&QPushButton::clicked,this,&MainWindow::closeCamera);connect(detect_,&QPushButton::clicked,this,&MainWindow::detectImage);connect(&timer_,&QTimer::timeout,this,&MainWindow::updateFrame);
connect(worker_,&DetectorWorker::loaded,this,[this](bool ok,const QString& error){ready_=ok;failure_=error;detect_->setEnabled(ok);status_->setText(ok?"模型加载成功":("模型加载失败："+error));});
connect(worker_,&DetectorWorker::completed,this,[this](const QImage& image,const QJsonObject& result,quint64 generation){if(generation!=generation_)return;if(!result["ok"].toBool()){failure_=result["error"].toString();status_->setText("推理失败："+failure_);detecting_=false;return;}++completed_;view_->setPixmap(QPixmap::fromImage(image).scaled(view_->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));status_->setText(QString("检测到 %1 个对象，耗时 %2 ms").arg(result["detections"].toArray().size()).arg(result["timing_ms"].toObject()["total"].toDouble(),0,'f',1));});setupPhotos();worker_->start();}

void MainWindow::setupPhotos(){
 auto layout=qobject_cast<QVBoxLayout*>(centralWidget()->layout());auto row=new QHBoxLayout;layout->addLayout(row);
 capture_=new QPushButton("拍照保存");auto choose=new QPushButton("设置照片保存目录");auto album=new QPushButton("浏览相册");auto faces=new QPushButton("人脸登记与识别");for(auto b:{capture_,choose,album,faces})row->addWidget(b);
 auto admin=new QPushButton("管理员模式");admin->setObjectName("adminMode");admin->setCheckable(true);admin->setStyleSheet("QPushButton:checked {background-color:#237b45;color:white;}");row->addWidget(admin);choose->setEnabled(false);choose->setToolTip("请先登录管理员模式");
 connect(admin,&QPushButton::clicked,this,[this,admin,choose](bool checked){if(checked){AdminDialog dialog(this);if(dialog.exec()!=QDialog::Accepted){admin->setChecked(false);return;}admin->setText("退出管理员模式");choose->setEnabled(true);}else{admin->setText("管理员模式");choose->setEnabled(false);}});
 auto exitButton=new QPushButton("退出");exitButton->setObjectName("exitMain");row->addWidget(exitButton);connect(exitButton,&QPushButton::clicked,this,&MainWindow::close);
 photoStatus_=new QLabel;photoStatus_->setWordWrap(true);photoStatus_->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(photoStatus_);
 const QString settingsPath=QCoreApplication::applicationDirPath()+"/photo-settings.ini";
 QSettings settings(settingsPath,QSettings::IniFormat);photoDirectory_=settings.value("photos/directory",QCoreApplication::applicationDirPath()+"/photos").toString();photoStatus_->setText("照片目录："+photoDirectory_);
 connect(choose,&QPushButton::clicked,this,[this,settingsPath,admin]{if(!admin->isChecked())return;auto path=QFileDialog::getExistingDirectory(this,"选择照片保存目录",photoDirectory_);if(path.isEmpty())return;QString error;if(!Photos::validateDirectory(path,error)){QMessageBox::warning(this,"目录错误",error);return;}QSettings settings(settingsPath,QSettings::IniFormat);settings.setValue("photos/directory",path);settings.sync();if(settings.status()!=QSettings::NoError){QMessageBox::warning(this,"设置保存失败","无法写入 "+settingsPath);return;}photoDirectory_=path;photoStatus_->setText("照片目录："+path);});
 connect(capture_,&QPushButton::clicked,this,[this]{if(!camera_.isOpened()||current_.empty()){QMessageBox::information(this,"拍照","请先打开摄像头并等待画面出现");return;}cv::Mat rgb;cv::cvtColor(current_,rgb,cv::COLOR_BGR2RGB);QImage image(rgb.data,rgb.cols,rgb.rows,int(rgb.step),QImage::Format_RGB888);QString error;auto path=Photos::save(image,photoDirectory_,error);if(path.isEmpty())QMessageBox::warning(this,"照片保存失败",error);else photoStatus_->setText("照片已保存："+path);});
 connect(album,&QPushButton::clicked,this,[this]{Album dialog(photoDirectory_,this);connect(&dialog,&Album::imageSelected,this,[this](const QString& path){if(!openImage(path))QMessageBox::warning(this,"图片错误",failure_);});dialog.exec();});
 connect(faces,&QPushButton::clicked,this,[this,admin]{closeCamera();FaceDialog dialog(cameraId_,photoDirectory_,admin->isChecked(),this);dialog.exec();});
}
MainWindow::~MainWindow(){timer_.stop();camera_.release();worker_->stop();worker_->wait();}
void MainWindow::closeEvent(QCloseEvent* event){timer_.stop();detecting_=false;++generation_;worker_->stop();worker_->wait();camera_.release();event->accept();}
void MainWindow::showImage(const cv::Mat& image){cv::Mat rgb;cv::cvtColor(image,rgb,cv::COLOR_BGR2RGB);QImage display(rgb.data,rgb.cols,rgb.rows,int(rgb.step),QImage::Format_RGB888);view_->setPixmap(QPixmap::fromImage(display.copy()).scaled(view_->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));}
bool MainWindow::openImage(const QString& path){closeCamera();try{current_=readImage(path);}catch(const std::exception& e){failure_=e.what();return false;}if(current_.empty()){failure_="IMAGE_UNREADABLE: "+path;status_->setText(failure_);return false;}failure_.clear();showImage(current_);status_->setText(path);return true;}
bool MainWindow::openCamera(){closeCamera();bool opened=false;try{
#ifdef _WIN32
bool numeric=false;int index=cameraId_.toInt(&numeric);if(numeric&&index>=0)opened=camera_.open(index,cv::CAP_DSHOW);
#else
opened=camera_.open(cameraId_.toStdString(),cv::CAP_V4L2);
#endif
}catch(const std::exception& e){failure_=e.what();return false;}if(!opened){failure_="CAMERA_OPEN_FAILED: "+cameraId_;status_->setText(failure_);return false;}failure_.clear();load_->setEnabled(false);open_->setEnabled(false);close_->setEnabled(true);timer_.start(33);status_->setText("摄像头已开启");return true;}
void MainWindow::closeCamera(){timer_.stop();camera_.release();detecting_=false;current_.release();++generation_;load_->setEnabled(true);open_->setEnabled(true);close_->setEnabled(false);view_->clear();status_->setText("摄像头已关闭");}
void MainWindow::detectImage(){if(!ready_||current_.empty())return;detecting_=camera_.isOpened();worker_->submit(current_,generation_);}
void MainWindow::updateFrame(){cv::Mat frame;try{if(!camera_.read(frame)||frame.empty()){closeCamera();failure_="CAMERA_FRAME_FAILED";status_->setText("摄像头已断开或无法采集");return;}current_=frame;if(detecting_&&ready_)worker_->submit(current_,generation_);else showImage(current_);}catch(const std::exception& e){closeCamera();failure_=e.what();status_->setText(failure_);}}
