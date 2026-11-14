#include "postprocess.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <numeric>
static void require(bool good,const char* message){if(!good)throw std::runtime_error(message);}
QString validateOutputs(const std::vector<TensorData>& outputs){
  for(const auto& t:outputs){size_t count=1;for(int d:t.shape){require(d>0 && d<=100000,"MODEL_SCHEMA_UNSUPPORTED: dimensions");require(count<=10000000/size_t(d),"MODEL_SCHEMA_UNSUPPORTED: oversized tensor");count*=d;}require(count==t.values.size(),"MODEL_SCHEMA_UNSUPPORTED: tensor size");for(float v:t.values)require(std::isfinite(v),"MODEL_SCHEMA_UNSUPPORTED: nonfinite output");}
  if(outputs.size()==1 && outputs[0].shape==std::vector<int>{1,7,8400})return "ultralytics_single_head";
  require(outputs.size()==9,"MODEL_SCHEMA_UNSUPPORTED: output count");
  std::map<std::pair<int,int>,int> seen;
  for(const auto& t:outputs){require(t.shape.size()==4,"MODEL_SCHEMA_UNSUPPORTED: rank");int n=t.shape[0],c=t.shape[1],h=t.shape[2],w=t.shape[3];require(n==1&&(c==64||c==3||c==1)&&(h==80||h==40||h==20)&&w==h,"MODEL_SCHEMA_UNSUPPORTED: legacy shape");require(++seen[{h,c}]==1,"MODEL_SCHEMA_UNSUPPORTED: duplicate head");}
  for(int h:{80,40,20})for(int c:{64,3,1})require(seen[{h,c}]==1,"MODEL_SCHEMA_UNSUPPORTED: missing head");
  return "legacy_nine_head";
}
cv::Mat letterbox(const cv::Mat& image,LetterboxInfo& info){
  require(!image.empty()&&image.type()==CV_8UC3,"INPUT_INVALID: expected nonempty BGR uint8");
  info.original=image.size();info.ratio=std::min(640.f/image.cols,640.f/image.rows);
  int w=int(std::round(image.cols*info.ratio)),h=int(std::round(image.rows*info.ratio));
  info.left=(640-w)/2;info.top=(640-h)/2;
  cv::Mat scaled,padded(640,640,CV_8UC3,cv::Scalar(114,114,114));cv::resize(image,scaled,{w,h});scaled.copyTo(padded(cv::Rect(info.left,info.top,w,h)));return padded;
}
std::vector<Detection> decode(const std::vector<TensorData>& outputs,const LetterboxInfo& info,const DetectorOptions& options){
  QString protocol=validateOutputs(outputs);require(info.ratio>0&&std::isfinite(info.ratio)&&info.original.width>0&&info.original.height>0,"INPUT_INVALID: letterbox metadata");
  std::vector<Detection> candidates;const QString names[]={"apple","banana","orange"};
  auto add=[&](int cls,float score,float x1,float y1,float x2,float y2){
    if(score<options.confidence)return;require(std::isfinite(score)&&std::isfinite(x1)&&std::isfinite(y1)&&std::isfinite(x2)&&std::isfinite(y2),"INFERENCE_INVALID: nonfinite box");
    if(x2<=x1||y2<=y1)return;
    candidates.push_back({cls,names[cls],score,cv::Rect2f(x1,y1,x2-x1,y2-y1)});
  };
  if(protocol=="ultralytics_single_head"){
    const auto& v=outputs[0].values;const int count=8400;
    for(int i=0;i<count;++i){int cls=0;for(int c=1;c<3;++c)if(v[(4+c)*count+i]>v[(4+cls)*count+i])cls=c;float x=v[i],y=v[count+i],w=v[2*count+i],h=v[3*count+i];add(cls,v[(4+cls)*count+i],x-w/2,y-h/2,x+w/2,y+h/2);}
  }else{
    for(int h:{80,40,20}){const TensorData *box=nullptr,*cls=nullptr,*obj=nullptr;for(const auto& t:outputs)if(t.shape[2]==h){if(t.shape[1]==64)box=&t;else if(t.shape[1]==3)cls=&t;else obj=&t;}
      const int count=h*h,stride=640/h;
      for(int i=0;i<count;++i){int cid=0;for(int c=1;c<3;++c)if(cls->values[c*count+i]>cls->values[cid*count+i])cid=c;
        float score=cls->values[cid*count+i]*obj->values[i];if(score<options.confidence)continue;
        float distance[4];for(int side=0;side<4;++side){float maxv=-INFINITY;for(int b=0;b<16;++b)maxv=std::max(maxv,box->values[(side*16+b)*count+i]);double sum=0,weighted=0;for(int b=0;b<16;++b){double e=std::exp(double(box->values[(side*16+b)*count+i]-maxv));sum+=e;weighted+=e*b;}distance[side]=float(weighted/sum);}
        float x=(i%h)+0.5f,y=(i/h)+0.5f;add(cid,score,(x-distance[0])*stride,(y-distance[1])*stride,(x+distance[2])*stride,(y+distance[3])*stride);
      }
    }
  }
  std::stable_sort(candidates.begin(),candidates.end(),[](const Detection&a,const Detection&b){return a.confidence>b.confidence;});
  std::vector<Detection> kept;
  for(const auto& candidate:candidates){bool suppress=false;for(const auto& k:kept){if(k.classId!=candidate.classId)continue;float inter=(k.box&candidate.box).area(),uni=k.box.area()+candidate.box.area()-inter;if(uni>0&&inter/uni>options.nms){suppress=true;break;}}if(!suppress)kept.push_back(candidate);if(kept.size()>=300)break;}
  std::vector<Detection> result;
  for(auto det:kept){float x1=std::clamp((det.box.x-info.left)/info.ratio,0.f,float(info.original.width));float y1=std::clamp((det.box.y-info.top)/info.ratio,0.f,float(info.original.height));float x2=std::clamp((det.box.x+det.box.width-info.left)/info.ratio,0.f,float(info.original.width));float y2=std::clamp((det.box.y+det.box.height-info.top)/info.ratio,0.f,float(info.original.height));if(x2>x1&&y2>y1){det.box={x1,y1,x2-x1,y2-y1};result.push_back(det);}}
  return result;
}
