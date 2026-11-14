# Windows 构建说明

开发验证平台：Windows 10/11 x64 · Qt 5.12.12 (MSVC2017 64bit) · OpenCV 4.8.0 官方预编译包 · MSVC 2017。

---

## 一、准备依赖

| 依赖 | 版本 | 放置位置 |
|---|---|---|
| Qt | 5.12.12 MSVC2017 64bit | 默认安装路径，需在 Qt Creator 中配置套件 |
| OpenCV | 4.8.0 Windows 预编译包 | 任意目录，通过 `OpenCV_DIR` / `OPENCV_ROOT` 指定 |
| MNN | Windows x64 头文件 + `.lib` + `.dll` | `third_party/mnn-windows/{include,lib,bin}` |

> MNN 的 Windows 二进制因体积与许可原因**不随仓库提供**，请从 [MNN Releases](https://github.com/alibaba/MNN/releases) 获取。
> 转换模型所用的 MNN 版本记录在 `config/mnn-source.json`（tag 3.3.0）。

人脸模型需先下载（详见 [README 第六节](../README.md#六模型获取)）：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/download_face_models.ps1
```

---

## 二、方式一：CMake（推荐）

```powershell
cmake -S . -B build-win -DCMAKE_BUILD_TYPE=Release `
      -DOpenCV_DIR="E:/Tool/opencv-4.8.0-windows/opencv/build" `
      -DMNN_ROOT="./third_party/mnn-windows" `
      -DFRUIT_BUILD_TESTS=ON
cmake --build build-win --config Release --parallel
```

产物在 `build-win/bin/`：

```text
FruitDetect2.exe        GUI 主程序
FruitDetect2Cli.exe     命令行工具
models/                 自动拷贝的水果模型
face-models/            自动拷贝的人脸模型
```

### 常见坑：OpenCV 库目录被推断成 vc14

首次配置时，若把 `OpenCV_DIR` 指向 OpenCV 顶层目录，CMake 可能自动推断为 `vc14`，
而实际是 MSVC2017 编译的 `vc16`。**显式指定到实际的配置目录**：

```powershell
-DOpenCV_DIR="E:/Tool/opencv-4.8.0-windows/opencv/build/x64/vc16/lib/cmake/opencv4"
```

这是 Windows 侧现象，与 Linux 配置无关。

### 用 qmake 的 OpenCV 路径变量

`FruitDetect2.pro` 的 `win32` 分支链接的是 `-lopencv_world480`，默认根目录为
`E:/Tool/opencv-4.8.0-windows/opencv/build`。路径不同时用环境变量覆盖：

```powershell
$env:OPENCV_ROOT = "D:/your/opencv-4.8.0/build"
$env:MNN_ROOT    = "./third_party/mnn-windows"
```

---

## 三、方式二：Qt Creator + qmake

1. 用 **Qt 5.12.12 MSVC2017 64bit Release** 套件打开 `FruitDetect2.pro`；
2. 运行 qmake；
3. 构建。

`.pro` 的 `QMAKE_POST_LINK` 会自动调用 `scripts/deploy_windows.ps1`，把模型与 DLL 部署到输出目录。

也可以直接跑构建脚本：

```powershell
& .\scripts\build_windows.ps1
```

> `FruitDetect2.pro.user` 保存的是**本机** Qt Creator 设置，含绝对路径，已在 `.gitignore` 中排除，不要提交。

---

## 四、运行

```powershell
.\build-win\bin\FruitDetect2.exe --backend mnn --threads 4 --camera 0
```

Windows 下 `--camera` 默认值是 `0`（DirectShow/MSMF 索引），不是 Linux 的 `/dev/video0`。

程序启动后**不会自动打开摄像头**，需在界面点击"打开摄像头"。管理员初始密码 `123`。

### 无界面冒烟测试

```powershell
# 图片检测冒烟
.\build-win\bin\FruitDetect2.exe --smoke-image "D:\abs\path\tests\fixtures\apple_12.jpg"

# 摄像头冒烟：开关 3 次后连续检测 10 秒
.\build-win\bin\FruitDetect2.exe --smoke-camera 10 --camera-cycles 3
```

成功分别输出 `GUI_IMAGE_PASS` / `GUI_CAMERA_COMPLETED`。

---

## 五、离线测试

```powershell
cmake -S . -B build-win -DFRUIT_BUILD_TESTS=ON
cmake --build build-win --config Release --parallel
cd build-win
ctest -C Release --output-on-failure
```

覆盖管理员密码流程（`AdminTests`）与拍照/人脸登记/陌生人/重载（`PhotoFaceTests`）。测试只写独立临时数据，不含用户真实照片。

---

## 六、已验证结论

来自 `reports/acceptance.md`（2026-11-08）：

- ✅ Qt 5.12.12 MSVC2017 x64 Release **qmake** 构建与依赖部署通过
- ✅ **CMake 3.12** Windows 构建 FruitDetect2 / FruitDetect2Cli / AdminTests / PhotoFaceTests 通过
- ✅ 默认 MNN 水果图片检测通过；缺失模型时退出码 `3`
- ✅ 照片保存、相册列表、姓名多样本登记、陌生人判定、数据库重载及错误分支离线测试通过
- ✅ 默认密码、旧密码校验、长度边界、二次确认、持久保存与退出登录测试通过
- ✅ GUI 初始化与图片推理 offscreen 冒烟通过

**注意边界**：offscreen 冒烟**不是**桌面视觉/摄像头验收。Windows 侧验证的是构建、模型加载与离线流程；
摄像头与真实桌面表现以 RK3399 板端实测为准（见 [rk3399-runbook.md](rk3399-runbook.md)）。

---

## 七、运行时数据

| 文件 | 位置 | 说明 |
|---|---|---|
| `photo-settings.ini` | exe 旁 | 照片目录设置，**含盘符路径，不能直接拷到 Linux 用** |
| `admin-credentials.json` | exe 旁 | 管理员密码 |
| `photos/` | exe 旁 | 抓拍照片 |
| `face_library/` | 照片目录下 | 人脸特征库 |

不要直接删除含照片、人脸库和密码文件的 `bin` 目录。这些文件已在 `.gitignore` 中排除，**切勿提交**。
