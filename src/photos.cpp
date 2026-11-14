#include "photos.h"
#include <QDir>
#include <QTemporaryFile>
#include <QSaveFile>
#include <QImageWriter>
#include <QDateTime>
#include <QUuid>
bool Photos::validateDirectory(const QString& path,QString& error){
 error.clear();if(path.isEmpty()||!QDir::isAbsolutePath(path)){error="请选择完整的绝对目录路径";return false;}
 if(!QDir().mkpath(path)){error="无法创建目录："+path;return false;}
 QTemporaryFile probe(QDir(path).filePath(".write-test-XXXXXX"));
 if(!probe.open()){error="目录不可写："+path+"\n"+probe.errorString();return false;}return true;
}
QString Photos::save(const QImage& image,const QString& directory,QString& error){
 if(image.isNull()){error="没有可保存的画面";return {};}
 if(!validateDirectory(directory,error))return {};
 QString name="photo_"+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz")+"_"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".jpg";
 QString path=QDir(directory).filePath(name);QSaveFile file(path);
 if(!file.open(QIODevice::WriteOnly)){error=file.errorString();return {};}
 QImageWriter writer(&file,"jpeg");writer.setQuality(95);
 if(!writer.write(image)){error=writer.errorString();file.cancelWriting();return {};}
 if(!file.commit()){error=file.errorString();return {};}error.clear();return path;
}
QStringList Photos::list(const QString& directory){
 QStringList paths;QDir dir(directory);auto entries=dir.entryInfoList(QDir::Files|QDir::Readable,QDir::Time);
 for(const auto& entry:entries)if(QStringList{"jpg","jpeg","png","bmp"}.contains(entry.suffix().toLower()))paths<<entry.absoluteFilePath();return paths;
}
