#include "face_engine.h"
#include "photos.h"
#include <opencv2/imgproc.hpp>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QPainter>
#include <QMap>
#include <cmath>
#include <stdexcept>
static QImage rgb(const cv::Mat& bgr){cv::Mat m;cv::cvtColor(bgr,m,cv::COLOR_BGR2RGB);return QImage(m.data,m.cols,m.rows,int(m.step),QImage::Format_RGB888).copy();}
bool FaceEngine::load(const QString& models,const QString& library,QString& error){
 try {
  error.clear();detector_.release();recognizer_.release();samples_=QJsonArray();
  if(!QDir::isAbsolutePath(models)||!QDir::isAbsolutePath(library))throw std::runtime_error("Model/library paths must be absolute");
  library_=library;if(!Photos::validateDirectory(library_,error))return false;
  temporary_.reset(new QTemporaryDir(QDir(models).filePath("cache-XXXXXX")));
  if(!temporary_->isValid())throw std::runtime_error("Cannot create model cache");
  QStringList names={"face_detection_yunet_2023mar.onnx","face_recognition_sface_2021dec.onnx"};
  for(const QString& name:names){QFile f(QDir(models).filePath(name));if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("Face model missing/unreadable");QByteArray bytes=f.readAll();if(name==names.last())hash_=QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());QFile dst(temporary_->filePath(name));if(!dst.open(QIODevice::WriteOnly)||dst.write(bytes)!=bytes.size())throw std::runtime_error("Model cache write failed");}
  detector_=cv::FaceDetectorYN::create(temporary_->filePath(names[0]).toLocal8Bit().constData(),"",cv::Size(320,320),0.9f);
  recognizer_=cv::FaceRecognizerSF::create(temporary_->filePath(names[1]).toLocal8Bit().constData(),"");
  QFile db(QDir(library_).filePath("identities.json"));
  if(db.exists()){
   if(!db.open(QIODevice::ReadOnly))throw std::runtime_error("Cannot read identity database");
   QJsonParseError pe;auto doc=QJsonDocument::fromJson(db.readAll(),&pe);auto obj=doc.object();
   if(pe.error!=QJsonParseError::NoError||obj.value("schema").toInt()!=1||obj.value("model_sha256").toString()!=hash_||!obj.value("samples").isArray())throw std::runtime_error("Invalid/incompatible identity database");
   samples_=obj.value("samples").toArray();
   for(auto item:samples_){auto s=item.toObject();auto v=s.value("feature").toArray();if(s.value("name").toString().trimmed().isEmpty()||v.size()!=128)throw std::runtime_error("Invalid identity sample");double norm=0;for(auto n:v){double x=n.toDouble(NAN);if(!std::isfinite(x))throw std::runtime_error("Invalid feature value");norm+=x*x;}if(std::abs(norm-1)>0.02)throw std::runtime_error("Invalid feature norm");}
  }
  return true;
 }catch(const std::exception& e){error=QString::fromUtf8(e.what());detector_.release();recognizer_.release();return false;}
}
cv::Mat FaceEngine::faces(const cv::Mat& image){if(!detector_||!recognizer_)throw std::runtime_error("Face model not loaded");if(image.empty()||image.type()!=CV_8UC3)throw std::runtime_error("Invalid input image");detector_->setInputSize(image.size());cv::Mat f;detector_->detect(image,f);return f;}
cv::Mat FaceEngine::feature(const cv::Mat& image,const cv::Mat& face){cv::Mat aligned,f;recognizer_->alignCrop(image,face,aligned);recognizer_->feature(aligned,f);f=f.reshape(1,1).clone();double n=cv::norm(f);if(f.total()!=128||!cv::checkRange(f)||n<1e-8)throw std::runtime_error("Invalid embedding");return f/n;}
FaceResult FaceEngine::enroll(const cv::Mat& image,const QString& name){
 FaceResult r;
 try{
  QString person=name.trimmed();if(person.isEmpty()||person.size()>80)throw std::runtime_error("Enter a name (1-80 characters)");
  auto f=faces(image);if(f.rows!=1)throw std::runtime_error("Enrollment requires exactly one face");
  if(f.at<float>(0,2)<64||f.at<float>(0,3)<64)throw std::runtime_error("Face too small; move closer");
  auto vec=feature(image,f.row(0));QJsonArray a;for(int i=0;i<128;++i)a.append(double(vec.at<float>(0,i)));
  QString error;QString photo=Photos::save(rgb(image),QDir(library_).filePath("photos"),error);if(photo.isEmpty())throw std::runtime_error(error.toStdString());
  QJsonArray next=samples_;next.append(QJsonObject{{"name",person},{"photo",QDir(library_).relativeFilePath(photo)},{"feature",a}});
  QSaveFile out(QDir(library_).filePath("identities.json"));QByteArray bytes=QJsonDocument(QJsonObject{{"schema",1},{"model_sha256",hash_},{"samples",next}}).toJson();
  if(!out.open(QIODevice::WriteOnly)||out.write(bytes)!=bytes.size()||!out.commit()){QFile::remove(photo);throw std::runtime_error("Identity database save failed");}
  samples_=next;int count=0;for(auto s:samples_)if(s.toObject().value("name").toString()==person)++count;
  r.ok=true;r.image=rgb(image);r.message=QString("已登记 %1：%2 张照片；本地库共 %3 张").arg(person).arg(count).arg(samples_.size());
 }catch(const std::exception& e){r.message=QString::fromUtf8(e.what());}return r;
}
FaceResult FaceEngine::recognize(const cv::Mat& image,double threshold){
 FaceResult r;try{
  if(!std::isfinite(threshold)||threshold<0||threshold>1)throw std::runtime_error("Invalid threshold");
  auto f=faces(image);r.image=rgb(image);QPainter painter(&r.image);painter.setPen(QPen(Qt::green,2));QFont font=painter.font();font.setPixelSize(22);painter.setFont(font);QStringList labels;
  for(int i=0;i<f.rows;++i){auto v=feature(image,f.row(i));QMap<QString,double> scores;
   for(auto item:samples_){auto s=item.toObject();auto a=s.value("feature").toArray();double score=0;for(int j=0;j<128;++j)score+=v.at<float>(0,j)*a[j].toDouble();QString name=s.value("name").toString();scores[name]=qMax(scores.value(name,-1.0),score);}
   double best=-1,second=-1;QString name;for(auto it=scores.cbegin();it!=scores.cend();++it){if(it.value()>best){second=best;best=it.value();name=it.key();}else second=qMax(second,it.value());}
   QString label=samples_.isEmpty()?QString("未登记人脸"):((best>=threshold&&best-second>=0.05)?name:QString("陌生人"));
   if(!samples_.isEmpty())label+=QString(" (%1)").arg(best,0,'f',3);labels<<label;
   const float* b=f.ptr<float>(i);QRect rect(qRound(b[0]),qRound(b[1]),qRound(b[2]),qRound(b[3]));painter.drawRect(rect);painter.drawText(qMax(0,rect.x()),qMax(25,rect.y()-5),label);
  }r.ok=true;r.message=labels.isEmpty()?QString("未检测到人脸"):labels.join("；");
 }catch(const std::exception& e){r.message=QString::fromUtf8(e.what());}return r;
}
