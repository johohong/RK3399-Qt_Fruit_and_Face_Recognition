# RK3399 实机部署

日期：2026-11-09。主机192.168.1.8，用户pi，Debian 11 ARM64，NanoPC-T4。

## 实际路径

- 源码与CMake入口：/home/pi/work/qt/FruitDetect2/FruitDetect2/CMakeLists.txt
- Release构建：/home/pi/work/qt/FruitDetect2/build-agent-release
- 程序：/home/pi/work/qt/FruitDetect2/build-agent-release/bin/FruitDetect2
- CLI：/home/pi/work/qt/FruitDetect2/build-agent-release/bin/FruitDetect2Cli
- 基础工程备份：/home/pi/work/qt/FruitDetect2/basic-backup-20261109-000204
- OpenCV：/usr/local/lib/cmake/opencv4，版本4.8.0；源码在/home/pi/work/opencv-4.8.0。
- MNN：/usr/local/lib/libMNN.so，头文件/usr/local/include/MNN，版本3.6.2。
- Qt：5.15.2；GCC：10.2.1；CMake：3.18.4。

## 启动

```bash
bash /home/pi/work/qt/FruitDetect2/start-FruitDetect2.sh
```

脚本选择已验证USB摄像头的稳定路径/dev/v4l/by-id/usb-ARKMICRO_USB2.0_PC_CAMERA-video-index0（本次对应/dev/video10），不是板载/dev/video0。程序启动后仍需点击打开摄像头；管理员初始密码123。

若通过Qt Creator运行，打开上述CMakeLists.txt，选择Release，并在运行参数中加入：

```text
--backend mnn --threads 4 --camera /dev/v4l/by-id/usb-ARKMICRO_USB2.0_PC_CAMERA-video-index0
```

Qt Creator原有Debug/Release目录保留，可能重新生成程序；本次验证的可执行文件在build-agent-release/bin。不同构建目录的照片、密码文件互相独立，不要误以为切换程序后数据丢失。

## 重新构建已验证版本

```bash
cmake -S /home/pi/work/qt/FruitDetect2/FruitDetect2 -B /home/pi/work/qt/FruitDetect2/build-agent-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DOpenCV_DIR=/usr/local/lib/cmake/opencv4 -DMNN_ROOT=/usr/local -DFRUIT_BUILD_TESTS=ON -DFRUIT_RK3399_TUNING=ON
cmake --build /home/pi/work/qt/FruitDetect2/build-agent-release --parallel 2
```

保留原OpenCV和MNN库，没有重新安装或覆盖。用户给定的-mcpu/-mtune参数已通过当前GCC检查并用于业务目标。板端MNN与转换器版本不同，本次已通过实际水果推理验证，未据此声称所有模型都兼容。

## 验证结果

- ARM64 Release GUI、CLI、AdminTests、PhotoFaceTests均编译成功。
- ldd未发现缺失库，链接到既有/usr/local OpenCV和MNN。
- 默认水果模型加载及图片推理成功；标准输出[1,7,8400]。
- 人脸加载、单人登记、多样本、持久重载、陌生人及错误分支离线测试通过。
- 中文路径照片保存和相册列表测试通过。
- 管理员旧密码校验、新密码长度、二次确认、持久化及退出测试通过。
- 真实桌面DISPLAY=:0、1280x800；USB摄像头三次开启关闭及约20秒连续检测通过，共完成49次推理。
- 尚未执行10分钟稳定性测试、全面跨平台精度评估或真实人员识别准确率评估。不得把本次短测当成完整性能认证。

GUI测试输出了rockchip OpenGL驱动警告，但窗口与摄像头检测仍成功；本程序使用CPU推理，本次没有改系统图形驱动。若出现视觉异常，需另行排查，而不是宣称警告已修复。

照片默认位于/home/pi/work/qt/FruitDetect2/build-agent-release/bin/photos。人脸库随照片目录保存；管理员密码在同一bin目录的admin-credentials.json。测试使用临时库，没有登记用户身份。

源码包SHA256：52de42e58b9a7187cd4c4665d77b20818031996f2a8ea482020a157f7f9657d9。SFTP传输约13.53秒，接收端哈希检查通过。
