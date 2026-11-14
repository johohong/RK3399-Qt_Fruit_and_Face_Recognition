#pragma once
#include <QImage>
#include <QStringList>
namespace Photos {
bool validateDirectory(const QString& path, QString& error);
QString save(const QImage& image,const QString& directory,QString& error);
QStringList list(const QString& directory);
}
