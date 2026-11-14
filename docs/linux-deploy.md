# RK3399 部署说明

## 1. 用户提供的命令属于 OpenCV

`ENABLE_NEON`、`CPU_BASELINE`、`CPU_DISPATCH`、`WITH_OPENMP`、`WITH_GSTREAMER`、`WITH_FFMPEG`、`WITH_V4L`、`BUILD_opencv_*` 和 `OPENCV_EXTRA_MODULES_PATH` 是OpenCV源码构建配置，不应复制到应用的CMake配置里期待生效。

已有OpenCV运行正常时优先复用，不自动重编译或覆盖/usr/local。构建选项ON并不保证依赖已找到，需查看OpenCV构建摘要或cv::getBuildInformation()确认实际启用状态。

当前人脸依赖OpenCV dnn和objdetect，不依赖contrib的face模块。因此BUILD_opencv_face=OFF可以保留，必须确认BUILD_opencv_objdetect=ON。关闭WITH_GTK/WITH_OPENGL不等于关闭Qt Widgets，本应用不使用OpenCV imshow显示窗口。

OpenCV的WITH_OPENMP不会自动开启MNN OpenMP。MNN的线程池、CPU算子和ARM指令集配置需单独确认。模型转换记录见config/mnn-source.json：MNN 3.3.0，提交5047919a1ae594d090ee54c7ea3403a9dee8cad4。板端其他版本可先验证，不能仅凭同名libMNN.so判定兼容。

## 2. 待核实的板端条件

- 操作系统/库/编译器均为AArch64；Qt至少5.12，包含Widgets和Concurrent。
- OpenCV 4.8，包含core、imgproc、imgcodecs、videoio、dnn、objdetect。
- MNN头文件与libMNN.so匹配，CPU后端可执行，动态依赖完整。
- GCC支持C++17。摄像头采集节点、DISPLAY与用户权限仍需实测。

旧构建文件声明OpenCV_DIR=/usr/local/lib/cmake/opencv4、MNN安装前缀/usr/local；这些是待板端验证的默认路径，不是本次远程检查结果。

## 3. 源码传输

在Windows运行scripts/export_linux_bundle.ps1生成源码ZIP。归档包含src、models、face-models、scripts、config和构建说明，不包含Windows MNN二进制、构建缓存、用户密码或登记照片。

下面采用明确的板端部署约定 `/home/pi/FruitDetect2`，只在确认SSH用户确为pi且该目录可写后使用。当前没有进行SSH连接。解压后应直接存在 `/home/pi/FruitDetect2/CMakeLists.txt`，不要额外嵌套一层目录。

## 4. 环境检查和编译

```bash
bash /home/pi/FruitDetect2/scripts/probe_linux.sh
```

期望显示AArch64、Qt和OpenCV版本。报缺少工具时先补齐对应实际依赖，不改系统软件源。

```bash
bash /home/pi/FruitDetect2/scripts/build_linux.sh
```

脚本引用已安装的OpenCV与MNN，只编译应用和CLI，输出到 `/home/pi/FruitDetect2/build-linux/bin`。出现libMNN找不到时检查 `/usr/local/include/MNN/Interpreter.hpp` 与 `/usr/local/lib/libMNN.so`。不要重复把include根目录设为include/MNN。

如已核实库在其他前缀，修改环境变量OpenCV_DIR和MNN_ROOT为其完整绝对路径。脚本默认不安装到/usr/local，也不复制Windows DLL。

## 5. 应用自身的RK3399 CPU优化

默认先不开启附加-mcpu参数，便于确认功能基线；板端已经优化好的OpenCV/MNN仍会按自身编译配置运行。确认GCC支持用户提供的参数后可以运行：

```bash
FRUIT_RK3399_TUNING=ON bash /home/pi/FruitDetect2/scripts/build_linux.sh
```

此选项只给本工程C++目标添加：

```text
-mcpu=cortex-a72.cortex-a53+crypto+crc
-mtune=cortex-a72.cortex-a53
```

CMake检查架构和编译器支持，不支持就明确失败，不偷偷切换参数。这个选项不会重建OpenCV/MNN。不要给RK3399基线强制启用ARMv8.2；若单独构建MNN，应检查固定版本选项并设置MNN_ARM82=OFF，CPU后端关闭OpenCL/Vulkan/CUDA，安装到独立前缀后再验收，不覆盖原库。

## 6. 启动和验收

脚本的ldd检查必须没有not found。启动时使用实际桌面会话：

```bash
/home/pi/FruitDetect2/build-linux/bin/FruitDetect2
```

默认模型在程序相邻models目录，默认摄像头/dev/video0只是候选节点，先用v4l2-ctl确认实际采集节点，再通过--camera传入对应绝对设备路径。程序启动不自动打开摄像头。

先通过界面打开一张已知图片验证水果检测，再用已知测试照片登记和比对人脸；最后测试真实摄像头。模型错误不等于未检测到水果；未登记人脸不等于陌生人判断成功。

模型目录、应用目录和照片目录需当前用户可写。不要将带Windows盘符的photo-settings.ini直接复用。若安装到只读系统目录，应先调整可写数据/缓存路径，当前默认建议在用户目录运行。

## 7. 性能对比

用同一模型、图片、阈值比较，先预热，再分别测试2、4、6线程。记录实际链接库、CPU频率/温度、预处理/推理/后处理与整体耗时；OpenCV和MNN都可能使用线程，需留意线程争用。没有RK3399实测前不写固定加速倍数或FPS。

参考：https://docs.opencv.org/4.x/db/d05/tutorial_config_reference.html
MNN固定版本配置：https://github.com/alibaba/MNN/blob/3.3.0/CMakeLists.txt

## 8. 无摄像头图片验收及性能采样

源码包附带公开人脸测试样本与一张水果验证图片，不包含用户登记照片。命令行程序不用打开Qt窗口：

```bash
/home/pi/FruitDetect2/build-linux/bin/FruitDetect2Cli --backend mnn --image /home/pi/FruitDetect2/tests/fixtures/apple_12.jpg --output-json /home/pi/FruitDetect2/build-linux/fruit-result.json
```

预期退出码0并输出DETECTION_PASS。模型缺失退出码3，图像或输出文件错误为4，推理失败为5。首次检测成功后再采样：

```bash
/home/pi/FruitDetect2/build-linux/bin/FruitDetect2Cli --backend mnn --threads 2 --warmup 20 --iterations 100 --image /home/pi/FruitDetect2/tests/fixtures/apple_12.jpg --output-json /home/pi/FruitDetect2/build-linux/performance-2.json
/home/pi/FruitDetect2/build-linux/bin/FruitDetect2Cli --backend mnn --threads 4 --warmup 20 --iterations 100 --image /home/pi/FruitDetect2/tests/fixtures/apple_12.jpg --output-json /home/pi/FruitDetect2/build-linux/performance-4.json
/home/pi/FruitDetect2/build-linux/bin/FruitDetect2Cli --backend mnn --threads 6 --warmup 20 --iterations 100 --image /home/pi/FruitDetect2/tests/fixtures/apple_12.jpg --output-json /home/pi/FruitDetect2/build-linux/performance-6.json
```

输出含预处理、推理、后处理和P50/P95采样时间；这不是摄像头端到端FPS，不包括采集和GUI显示耗时。
