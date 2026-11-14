#pragma once
#include <QDialog>
class QListWidget;class QLabel;
class Album:public QDialog{
 Q_OBJECT
public:explicit Album(const QString& directory,QWidget* parent=nullptr);
signals:void imageSelected(const QString& path);
private:void refresh();void preview();QString directory_;QListWidget* files_;QLabel* image_;QLabel* info_;
};
