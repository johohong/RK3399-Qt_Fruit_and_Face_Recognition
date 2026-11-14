#include "detector.h"
#include "postprocess.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <opencv2/dnn.hpp>
#ifdef FRUIT_WITH_MNN
#include <MNN/Interpreter.hpp>
#include <MNN/Tensor.hpp>
#endif
#include <chrono>
#include <cstring>
#include <stdexcept>
using Clock=std::chrono::steady_clock;
static double elapsed(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
class Detector final:public IDetector{
  DetectorOptions options_;cv::dnn::Net cvnet_;QString protocol_;bool loaded_=false;
#ifdef FRUIT_WITH_MNN
  std::shared_ptr<MNN::Interpreter> mnn_;MNN::Session* session_=nullptr;
#endif
  std::vector<TensorData> forward(const cv::Mat& blob){
    std::vector<TensorData> out;
    if(options_.backend=="opencv"){
      cvnet_.setInput(blob);std::vector<cv::Mat> mats;cvnet_.forward(mats,cvnet_.getUnconnectedOutLayersNames());
      for(auto& mat:mats){if(mat.type()!=CV_32F)throw std::runtime_error("MODEL_SCHEMA_UNSUPPORTED: output dtype");if(!mat.isContinuous())mat=mat.clone();TensorData t;for(int d=0;d<mat.dims;++d)t.shape.push_back(mat.size[d]);t.values.assign(mat.ptr<float>(),mat.ptr<float>()+mat.total());out.push_back(std::move(t));}
    }else{
#ifdef FRUIT_WITH_MNN
      auto* input=mnn_->getSessionInput(session_,nullptr);MNN::Tensor host(input,MNN::Tensor::CAFFE);
      if(host.shape()!=std::vector<int>{1,3,640,640}||!host.host<float>())throw std::runtime_error("MODEL_SCHEMA_UNSUPPORTED: input shape");
      std::memcpy(host.host<float>(),blob.ptr<float>(),blob.total()*sizeof(float));
      if(!input->copyFromHostTensor(&host))throw std::runtime_error("INFERENCE_FAILED: input copy");
      if(mnn_->runSession(session_)!=MNN::NO_ERROR)throw std::runtime_error("INFERENCE_FAILED: MNN runSession");
      for(const auto& item:mnn_->getSessionOutputAll(session_)){auto* tensor=item.second;auto type=tensor->getType();if(type.code!=halide_type_float||type.bits!=32)throw std::runtime_error("MODEL_SCHEMA_UNSUPPORTED: output dtype");MNN::Tensor hostOut(tensor,MNN::Tensor::CAFFE);if(!tensor->copyToHostTensor(&hostOut)||!hostOut.host<float>())throw std::runtime_error("INFERENCE_FAILED: output copy");TensorData t;t.shape=hostOut.shape();size_t count=1;for(int d:t.shape){if(d<=0||count>10000000/size_t(d))throw std::runtime_error("MODEL_SCHEMA_UNSUPPORTED: output size");count*=d;}t.values.assign(hostOut.host<float>(),hostOut.host<float>()+count);out.push_back(std::move(t));}
#else
      throw std::runtime_error("BACKEND_UNAVAILABLE: MNN was not built");
#endif
    }return out;
  }
public:
  ~Detector()override{release();}
  QString protocol()const override{return protocol_;}
  void release()override{loaded_=false;
#ifdef FRUIT_WITH_MNN
if(mnn_&&session_)mnn_->releaseSession(session_);session_=nullptr;mnn_.reset();
#endif
cvnet_=cv::dnn::Net();protocol_.clear();}
  bool load(const DetectorOptions& opt,QString& error)override{
    release();options_=opt;
    try{
      if(!QFileInfo(opt.modelPath).isAbsolute()||opt.threads<1||opt.threads>64||!(opt.confidence>=0&&opt.confidence<=1)||!(opt.nms>=0&&opt.nms<=1))throw std::runtime_error("MODEL_OPTIONS_INVALID");
      QFile file(opt.modelPath);if(!file.open(QIODevice::ReadOnly))throw std::runtime_error("MODEL_FILE_UNREADABLE");QByteArray bytes=file.readAll();if(bytes.isEmpty())throw std::runtime_error("MODEL_FILE_EMPTY");
      if(opt.backend=="opencv"){std::vector<uchar> buffer(bytes.begin(),bytes.end());cvnet_=cv::dnn::readNetFromONNX(buffer);cvnet_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);cvnet_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);cv::setNumThreads(opt.threads);}
      else if(opt.backend=="mnn"){
#ifdef FRUIT_WITH_MNN
        mnn_.reset(MNN::Interpreter::createFromBuffer(bytes.constData(),bytes.size()));if(!mnn_)throw std::runtime_error("MODEL_MNN_CREATE_FAILED");
        MNN::ScheduleConfig cfg;cfg.type=MNN_FORWARD_CPU;cfg.numThread=opt.threads;MNN::BackendConfig backend;backend.precision=MNN::BackendConfig::Precision_Normal;cfg.backendConfig=&backend;session_=mnn_->createSession(cfg);if(!session_)throw std::runtime_error("MODEL_MNN_SESSION_FAILED");
        auto inputs=mnn_->getSessionInputAll(session_);if(inputs.size()!=1)throw std::runtime_error("MODEL_SCHEMA_UNSUPPORTED: input count");auto* input=inputs.begin()->second;auto type=input->getType();if(type.code!=halide_type_float||type.bits!=32)throw std::runtime_error("MODEL_SCHEMA_UNSUPPORTED: input dtype");
#else
        throw std::runtime_error("BACKEND_UNAVAILABLE: MNN was not built");
#endif
      }else throw std::runtime_error("BACKEND_UNSUPPORTED");
      cv::Mat dummy(640,640,CV_8UC3,cv::Scalar(114,114,114));auto outputs=forward(cv::dnn::blobFromImage(dummy,1./255.,{640,640},{},true,false));protocol_=validateOutputs(outputs);loaded_=true;qInfo().noquote()<<"MODEL_LOADED"<<opt.modelPath<<"protocol="<<protocol_<<"input=[1,3,640,640]";for(const auto& t:outputs){QStringList dims;for(int d:t.shape)dims<<QString::number(d);qInfo().noquote()<<"output=["+dims.join(',')+"]";}return true;
    }catch(const std::exception& e){error=QString::fromUtf8(e.what());release();return false;}
  }
  DetectionResult detect(const cv::Mat& image)override{
    DetectionResult result;result.protocol=protocol_;auto t0=Clock::now();
    if(!loaded_){result.error="MODEL_NOT_LOADED";return result;}
    try{LetterboxInfo info;cv::Mat padded=letterbox(image,info);cv::Mat blob=cv::dnn::blobFromImage(padded,1./255.,{640,640},{},true,false);auto t1=Clock::now();auto outputs=forward(blob);auto t2=Clock::now();result.detections=decode(outputs,info,options_);auto t3=Clock::now();result.preprocessMs=elapsed(t0,t1);result.inferenceMs=elapsed(t1,t2);result.postprocessMs=elapsed(t2,t3);result.totalMs=elapsed(t0,t3);result.ok=true;}catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;
  }
};
std::unique_ptr<IDetector> createDetector(){return std::make_unique<Detector>();}
