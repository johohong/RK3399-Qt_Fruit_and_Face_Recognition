# FruitDetect2 验收记录

日期：2026-11-08。

- PASS：Windows Qt 5.12.12 MSVC2017 x64 Release qmake 构建和依赖部署。
- PASS：CMake 3.12 Windows 构建 FruitDetect2、FruitDetect2Cli、AdminTests、PhotoFaceTests。初次配置OpenCV顶层目录自动推断为vc14，改为实际x64/vc16/lib配置目录后通过。该现象不涉及Linux配置。
- PASS：默认MNN水果图片检测；缺失模型退出码3。
- PASS：照片保存、相册列表、姓名多样本登记、陌生人、数据库重载及错误分支离线测试。
- PASS：默认密码、旧密码验证、长度边界、二次确认、持久保存与退出登录测试。
- PASS：GUI初始化及图片推理offscreen冒烟；此检查不是桌面视觉/摄像头验收。
- PASS：Linux三份Bash脚本语法检查。Git Bash在沙箱内因系统对象权限失败，正常授权环境下检查通过。
- PASS：原工程源码根目录文件SHA-256未变，详见original-integrity.json。
- NOT EXECUTED：RK3399编译、libMNN兼容性、真实桌面、人脸/摄像头实测、CPU/NEON性能对比。当前未连接开发板。

证据位于本reports目录：windows-build.log、cmake-windows-check.log、admin-tests.log、photo-face-tests.log、cli-result.json、qmake-image-smoke.log、invalid-model.log。

本项目仍是Qt+OpenCV+MNN。没有删除MNN，也没有把默认水果推理切回OpenCV ONNX。人脸继续使用OpenCV DNN的预训练ONNX；不存在已执行的人脸重新训练。

原工程与新工程的照片、人脸库、密码隔离；测试只写独立临时数据。新源码中保留默认密码123，未复制用户改过的密码。
