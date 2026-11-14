#include "photos.h"
#include "face_engine.h"
#include "io.h"
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <stdexcept>
static void check(bool value,const char* test){if(!value)throw std::runtime_error(test);qInfo()<<"PASS"<<test;}
int main(int argc,char** argv){QGuiApplication app(argc,argv);try{
 if(argc!=3)throw std::runtime_error("Requires absolute model directory and fixture path");
 QTemporaryDir temp(QCoreApplication::applicationDirPath()+"/photo-face-test-XXXXXX");check(temp.isValid(),"temporary directory");QString dir=temp.path()+"/中文照片",error;QImage image(100,80,QImage::Format_RGB888);image.fill(Qt::red);
 auto a=Photos::save(image,dir,error),b=Photos::save(image,dir,error);check(!a.isEmpty()&&!b.isEmpty()&&a!=b,qPrintable("unique photo filenames and Unicode paths: "+error));check(QImage(a).size()==image.size(),"saved photo decodes");check(Photos::list(dir).size()==2,"album lists saved photos");check(Photos::save(QImage(),dir,error).isEmpty(),"empty photo rejected");check(!Photos::validateDirectory("relative",error),"relative directory rejected");
 FaceEngine engine;QString models=app.arguments()[1],library=temp.path()+"/人脸库";check(engine.load(models,library,error),qPrintable("model load: "+error));auto sample=readImage(app.arguments()[2]);check(!sample.empty(),"face fixture decodes");
 auto empty=engine.recognize(sample,.5);check(empty.ok&&empty.message.contains("未登记"),"empty gallery is distinct from unknown person");
 auto enrolled=engine.enroll(sample,"测试人员");check(enrolled.ok,qPrintable("enroll: "+enrolled.message));check(engine.enroll(sample,"测试人员").ok,"multiple samples per name");auto recognized=engine.recognize(sample,.5);check(recognized.ok&&recognized.message.contains("测试人员"),"enrolled fixture matches");
 auto stranger=engine.recognize(readImage(QFileInfo(app.arguments()[2]).absolutePath()+"/messi5.jpg"),.5);check(stranger.ok&&stranger.message.contains("陌生人"),"unregistered fixture is stranger");FaceEngine reloaded;check(reloaded.load(models,library,error),"identity database reload");check(reloaded.recognize(sample,.5).message.contains("测试人员"),"persisted feature matches");
 cv::Mat black(320,320,CV_8UC3,cv::Scalar(0,0,0));check(!engine.enroll(black,"测试人员").ok,"no-face enrollment rejected");auto noface=engine.recognize(black,.5);check(noface.ok&&noface.message.contains("未检测到"),"no face reported explicitly");check(!engine.enroll(sample," ").ok,"empty name rejected");
 FaceEngine bad;check(!bad.load(temp.path()+"/missing",temp.path()+"/bad",error),"missing model rejected");check(!bad.recognize(sample,.5).ok,"unloaded model is an error");QFile db(library+"/identities.json");check(db.open(QIODevice::WriteOnly|QIODevice::Truncate),"open test database");db.write("invalid");db.close();check(!reloaded.load(models,library,error),"corrupt database rejected");qInfo()<<"PHOTO_FACE_TESTS_PASS";return 0;
 }catch(const std::exception& e){qCritical()<<"PHOTO_FACE_TESTS_FAIL"<<e.what();return 1;}}
