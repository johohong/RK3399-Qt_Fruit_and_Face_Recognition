#include "admin_dialog.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QDebug>
#include <stdexcept>
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);qInfo()<<"PASS"<<message;}
int main(int argc,char** argv){QApplication app(argc,argv);try{
 QTemporaryDir tmp(QCoreApplication::applicationDirPath()+"/admin-test-XXXXXX");check(tmp.isValid(),"temporary test directory");QString path=tmp.path()+"/password.json",error;AdminStore store(path);
 check(store.verify("123",error),"default password");check(!store.verify("wrong",error),"wrong password rejected");check(!AdminStore::valid("12")&&!AdminStore::valid("1234567890")&&AdminStore::valid("123")&&AdminStore::valid("123456789"),"length boundaries 3-9");
 AdminDialog dialog(nullptr,path);auto input=dialog.findChild<QLineEdit*>("adminPassword");auto change=dialog.findChild<QPushButton*>("changePassword");auto submit=dialog.findChild<QPushButton*>("adminSubmit");auto status=dialog.findChild<QLabel*>("adminStatus");
 change->click();check(change->isChecked(),"change password stays selected for color feedback");input->setText("wrong");submit->click();check(status->text().contains("错误"),"old password must be verified");input->setText("123");submit->click();input->setText("12");submit->click();check(status->text().contains("3位"),"UI rejects short password");input->setText("abc456");submit->click();input->setText("abc457");submit->click();check(status->text().contains("不一致")&&store.verify("123",error),"mismatch keeps old password");input->setText("abc456");submit->click();check(status->text().contains("修改成功"),"successful change confirmation");
 AdminStore reloaded(path);check(reloaded.verify("abc456",error)&&!reloaded.verify("123",error),"password survives reload and invalidates old password");input->setText("abc456");submit->click();check(dialog.result()==QDialog::Accepted,"new password login");
 AdminDialog cancel(nullptr,path);cancel.findChild<QPushButton*>("adminExit")->click();check(cancel.result()==QDialog::Rejected,"exit cancels login");
 QFile file(path);check(file.open(QIODevice::ReadOnly),"read saved password file");auto bytes=file.readAll();file.close();check(!bytes.contains("abc456"),"password not stored in plaintext");check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"write corrupt test file");file.write("broken");file.close();check(!store.verify("123",error)&&!error.isEmpty(),"corrupt file does not restore default password");
 qInfo()<<"ADMIN_TESTS_PASS";return 0;
 }catch(const std::exception& e){qCritical()<<"ADMIN_TESTS_FAIL"<<e.what();return 1;}}
