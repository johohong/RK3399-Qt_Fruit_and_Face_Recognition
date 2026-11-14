# 上传到 GitHub 的操作步骤

> 本文件是给作者看的操作备忘，**上传前请删除或加入 `.gitignore`**，不要提交进仓库。

---

## 一、先整理出干净的仓库目录

当前工程分散在多个目录，且混有构建缓存、用户照片、密码文件和大体积二进制。
建议先把要上传的内容拼到一个新目录（例如 `D:\RK3399-Qt_Fruit_and_Face_Recognition`），结构如下：

```text
D:\RK3399-Qt_Fruit_and_Face_Recognition\
├── README.md                  ← 本骨架已生成
├── .gitignore                 ← 本骨架已生成
├── .gitattributes             ← 本骨架已生成
├── LICENSE                    ← 需自行选择并添加
├── CMakeLists.txt             ← 复制自 FruitDetect2-window\FruitDetect2\
├── FruitDetect2.pro           ← 同上
├── start-FruitDetect2.sh      ← 复制自 FruitDetect2-linux\
├── main.cpp / mainwindow.*    ← 根目录那几个"未参与新 .pro 构建"的基础文件，
│                                 若确认无用就不要复制（见下方第二节说明）
├── src\                       ← 复制自 FruitDetect2-window\FruitDetect2\src\
├── models\                    ← 复制 trained_fp32.mnn（12 MB）
├── face-models\               ← 只复制 sources.json + 两个 LICENSE-*.txt
│                                 ❌ 不要复制 .onnx（36.9 MB，用脚本下载）
├── scripts\                   ← 复制自 FruitDetect2-window\FruitDetect2\scripts\
│   │                             + 本骨架的 download_face_models.ps1/.sh
│   └── board-env\             ← 复制自 "Qt_OpenCV摄像头在板端移植命令文件\"（6 个 .sh）
├── tests\                     ← 复制自 FruitDetect2-window\FruitDetect2\tests\
├── config\                    ← 复制自 FruitDetect2-window\FruitDetect2\config\
├── reports\                   ← 复制自 FruitDetect2-window\FruitDetect2\reports\（验证证据，建议保留）
└── docs\
    ├── model-pipeline.md      ← 本骨架已生成
    ├── windows-build.md       ← 由原 README.md 的 Windows 段落整理
    ├── linux-deploy.md        ← 复制原 LINUX_DEPLOY.md 并改名
    ├── rk3399-runbook.md      ← 复制原 RK3399_RUNBOOK.md 并改名
    ├── acceptance.md          ← 复制原 reports\acceptance.md
    └── images\                ← 放截图
```

**关键取舍**

| 项目 | 处理 | 原因 |
|---|---|---|
| `FruitDetect2-window` 与 `FruitDetect2-linux` 两份源码 | **只保留一份** | 已用 MD5 逐个比对，`src/` 下 23 个文件**完全一致**。同一套代码靠 CMake 跨平台构建即可，没必要在仓库里放两份 |
| `build-*` 目录 | ❌ 不上传 | 全是编译缓存，`.gitignore` 已排除 |
| `*.pro.user` / `CMakeLists.txt.user` | ❌ 不上传 | 含本机绝对路径，`.gitignore` 已排除 |
| `face-models/*.onnx` | ❌ 不上传 | SFace 36.9 MB；官方公开模型，用脚本按 SHA256 校验下载 |
| `third_party/mnn-windows/` | ❌ 不上传 | 含 60 MB DLL，且许可需自行确认 |
| `models/trained_fp32.mnn` | ✅ 上传 | 12 MB，在 GitHub 100 MB 单文件限制内，且是自训练产物无法外部下载 |
| `backup-basic-*/` | ❌ 不上传 | 移植前的临时备份 |
| `photos/` / `admin-credentials.json` | ❌ **绝对不能上传** | 含用户照片与密码，隐私风险 |
| `reports/` | ✅ 建议上传 | 构建日志与验收证据，是项目可信度的支撑 |
| 5.88 MB 的 `.docx` 部署手册 | 可选 | 建议转成 Markdown 放进 `docs/`，比 docx 更适合仓库浏览 |

---

## 二、关于根目录那几个"游离"文件（已核实）

`FruitDetect2-window\FruitDetect2\` 根目录下有 `main.cpp`、`mainwindow.h`、`mainwindow.cpp`、`mainwindow.ui`。
原 README 写着"未参与新 .pro 构建，实际界面位于 src"。已逐项核实，**确认是纯残留**：

| 核实项 | 结果 |
|---|---|
| 根目录 `main.cpp` 引用的头文件 | `#include "mainwindow.h"` → 引的是**根目录**那个 `mainwindow.h`，与 `src/` 无关 |
| `src/` 下是否引用 `ui_mainwindow.h` 或 `.ui` | ❌ 全目录检索无任何匹配 |
| `CMakeLists.txt` 是否开启 `AUTOUIC` 或列出 `.ui` | ❌ 都没有（只开了 `CMAKE_AUTOMOC`） |
| `FruitDetect2.pro` 的 `SOURCES`/`HEADERS` | 只指向 `$$PWD/src/*`，未包含根目录文件 |

**结论**：`mainwindow.ui` 从未被任何构建系统处理过，根目录这 4 个文件是一套独立的、被弃用的初始脚手架。

**建议**：**不要复制**这 4 个文件到新仓库。理由：

1. 会让人误以为界面是 Designer 拖出来的，而实际界面是 `src/mainwindow.cpp` 纯代码构建的；
2. 根目录 `main.cpp` 里的 `MainWindow w;` 无参构造，与 `src/main.cpp` 的 `MainWindow(options, camera)` 签名不同，同名类并存极易误导；
3. 删掉后 CMake / qmake 构建完全不受影响（本来就没引用）。

如果想保留这段演进痕迹，更好的做法是放到 Git 历史里（首次提交时就带上，之后单独提交删除），而不是让它在最终仓库里当僵尸文件。

---

## 二·补、`start-FruitDetect2.sh` 需要改（已核实）

原脚本内容如下，**硬编码了作者板端的绝对路径，别人克隆后直接跑必然失败**：

```bash
export DISPLAY="${DISPLAY:-:0}"
export XAUTHORITY="${XAUTHORITY:-/home/pi/.Xauthority}"     # ← 写死 pi 用户
exec /home/pi/work/qt/FruitDetect2/build-agent-release/bin/FruitDetect2 \  # ← 写死板端路径
 --backend mnn --threads 4 \
 --camera /dev/v4l/by-id/usb-ARKMICRO_USB2.0_PC_CAMERA-video-index0 "$@"
```

三处问题：`XAUTHORITY` 写死 `/home/pi`、可执行文件路径写死 `build-agent-release`、摄像头 by-id 写死成特定型号 ARKMICRO 的序列号路径。

本骨架已提供改进版 [`start-FruitDetect2.sh`](start-FruitDetect2.sh)：自动定位脚本所在目录下的构建产物、`XAUTHORITY` 跟随 `$HOME`、摄像头节点支持环境变量覆盖并在 by-id 不存在时自动回退枚举。**上传时用它替换原脚本**，同时保留 README/runbook 里对原始实测路径的记录（那是历史事实，不该抹掉）。

---

## 三、仓库命名与简介（建议）

### 仓库名

推荐 **`RK3399-Qt_Fruit_and_Face_Recognition`**

理由：
- 与你已上传成功的 `STM32F407-Industrial_protocol_gateway` 命名风格一致（`芯片型号-功能描述`）；
- `RK3399` 放最前面，一眼看出是嵌入式板端项目，这是它区别于普通桌面 Qt demo 的核心亮点；
- 全大写芯片名 + 下划线分词，GitHub 搜索和 URL 里都清晰。

备选：

| 仓库名 | 适用场景 |
|---|---|
| `RK3399-Qt_Fruit_and_Face_Recognition` | **推荐**，突出硬件平台 |
| `Qt-YOLOv8-Fruit-Face-RK3399` | 突出算法栈，用连字符风格 |
| `FruitDetect2-RK3399` | 沿用工程内部代号，最简短，但外人看不懂是什么 |
| `EdgeAI-Qt-ObjectRecognition-RK3399` | 强调"边缘 AI"方向，适合投简历时突出技术域 |

### 仓库简介（About）

GitHub 的 About 限制 350 字符，建议控制在 100 字内。

**中文版**

```text
基于 Qt5 + OpenCV4.8 + YOLOv8(MNN) 的水果识别与人脸识别系统，Windows 开发、RK3399(ARM64) 板端实机部署。含人脸登记库、陌生人判定、管理员权限、拍照相册与命令行验收工具。
```

**英文版（推荐，GitHub 检索更友好）**

```text
Fruit & face recognition system built with Qt5 + OpenCV 4.8 + YOLOv8 (MNN). Developed on Windows, deployed and verified on RK3399 (ARM64). CPU inference, GUI + CLI, face enrollment & stranger detection.
```

**中英混合（信息量最大）**

```text
Qt5 + OpenCV4.8 + YOLOv8(MNN) fruit & face recognition | Windows dev → RK3399 ARM64 deployment | CPU inference, face enrollment, stranger detection, admin auth, CLI benchmarking
```

### Topics（标签）

在仓库 About 右侧点 "Manage topics" 添加，能显著提升被搜到的概率：

```text
qt5  opencv  yolov8  mnn  rk3399  arm64  embedded  edge-ai  cpp17  cmake
face-recognition  object-detection  fruit-detection  yunet  sface  nanoPC-t4
computer-vision  cross-platform
```

---

## 四、Git 上传命令

参照你之前成功的方式，SSH 协议：

```powershell
cd "D:\RK3399-Qt_Fruit_and_Face_Recognition"

# 1. 初始化
git init

# 2. 先看一眼将要提交什么，确认没有照片/密码/大文件
git add .
git status

# 3. 确认无误后再提交
git commit -m "Initial commit: Qt5 + OpenCV 4.8 + YOLOv8(MNN) fruit & face recognition on RK3399"

# 4. 关联远程仓库（先在 GitHub 网页上建好空仓库，不要勾选自动生成 README）
git remote add origin git@github.com:johohong/RK3399-Qt_Fruit_and_Face_Recognition.git
git branch -M main
git push -u origin main
```

### 提交前的自检清单

```powershell
# 检查有没有超过 50 MB 的文件（GitHub 会警告，超过 100 MB 直接拒绝）
git ls-files | ForEach-Object { $f = Get-Item $_ -ErrorAction SilentlyContinue; if ($f -and $f.Length -gt 50MB) { "{0,8:N2} MB  {1}" -f ($f.Length/1MB), $_ } }

# 检查有没有误提交隐私文件
git ls-files | Select-String -Pattern 'photos/|admin-credentials|photo-settings|\.pro\.user|\.onnx$|\.dll$'

# 查看仓库总体积
"{0:N2} MB" -f ((git ls-files | ForEach-Object { (Get-Item $_ -ErrorAction SilentlyContinue).Length } | Measure-Object -Sum).Sum / 1MB)
```

三条命令**都应该没有输出**（第三条除外，它会打印总体积）。

### 如果不小心提交了大文件

`git push` 被拒绝时，不要直接 `git push -f`。先确认是哪个文件：

```powershell
git rm --cached face-models/face_recognition_sface_2021dec.onnx
git commit --amend -m "Initial commit: ..."
git push -u origin main
```

`--amend` 只在**首次推送且远程还没有该提交**时安全。已经推上去的历史要清理，需要 `git filter-repo`，那会改写历史，操作前先备份整个目录。

---

## 四·补、LICENSE（已在 GitHub 侧完成）

许可证已选定 **Apache 2.0**，且在 GitHub 新建仓库时通过 "Add a license" 勾选生成。
因此：

- ✅ **本地不需要再造 LICENSE 文件**，仓库骨架目录里没有它属于正常现象；
- ✅ 与依赖的 OpenCV 4.x、MNN（均为 Apache 2.0）许可一致，兼容性最好；
- ⚠️ **但这会带来首次推送冲突**，见下一节。

> 注意：Apache 2.0 **不覆盖 YuNet / SFace 模型自身的许可**。README 第十三节已单独声明
> "模型各有独立许可，使用前请自行确认合规"，这句话必须保留。

---

## 四·补2、首次推送必然遇到的 non-fast-forward 冲突

因为你在 GitHub 建仓库时勾选了自动生成 LICENSE（可能还有 README / .gitignore），
**远程已经存在一个初始提交**。此时在本地 `git init` + `git push` 会被拒绝：

```text
! [rejected]        main -> main (fetch first)
error: failed to push some refs to 'github.com:johohong/...'
hint: Updates were rejected because the remote contains work that you do not have locally.
```

### 正确处理方式（推荐：先拉取合并）

```powershell
cd "D:\0qianwen\简历\RK3399仓库骨架"

git init
git add .
git commit -m "Initial commit: Qt5 + OpenCV 4.8 + YOLOv8(MNN) fruit & face recognition on RK3399"

git remote add origin git@github.com:johohong/<你的仓库名>.git
git branch -M main

# 关键一步：先拉取远程的 LICENSE 提交并允许合并无关历史
git pull origin main --allow-unrelated-histories --no-rebase

# 若出现冲突（本地也有 README.md / .gitignore 时很可能冲突），按下面说明解决后再提交
git push -u origin main
```

`--allow-unrelated-histories` 是必需的：本地 `git init` 的历史与 GitHub 自动生成的历史没有共同祖先，
不加这个参数 Git 会直接拒绝合并。

### 冲突怎么解决

最可能冲突的是 **README.md** 和 **.gitignore**（GitHub 生成的版本 vs 你本地的完整版）。
**一律保留本地版本**——本地那份内容完整得多：

```powershell
git checkout --ours README.md
git checkout --ours .gitignore
git add README.md .gitignore
git commit -m "Merge GitHub-generated LICENSE with local repository content"
```

`LICENSE` 文件不会冲突（本地没有），合并后会自动出现在工作目录里，保留它即可。

### 备选方式：让远程的 LICENSE 提交作废

如果你更想要一条干净的单提交历史，且**确认远程除了 LICENSE 没有任何你需要的内容**：

```powershell
git push -u origin main --force
```

⚠️ 这会丢弃 GitHub 自动生成的那个提交。`main` 分支的强制推送默认被保护，
可能需要先到仓库 **Settings → Branches** 临时解除保护规则，推完再恢复。
**推荐用上面的 pull 合并方式，更安全，且历史里能保留 LICENSE 的来源。**

---

## 五、上传后到 GitHub 网页上补三件事

1. **About**：填上面的简介 + Topics。
2. **Releases**（可选）：如果想让别人直接用，把 Windows 编译好的 `FruitDetect2.exe` + 依赖 DLL 打成 zip 传到 Release。**不要**把二进制提交进 Git 历史。
3. **项目主页截图**：README 里的 `docs/images/` 配图上传后，取消 README 第二节那两行注释。

---

## 六、关于另外两个大目录

`工具软件与资源文件`（约 4.9 GB）和 `官方烧录镜像工具`（约 1.66 GB）**不在项目目录内**，
本来就不会被 `git add` 带进去。它们的内容与获取方式已写在 README 第十一节，读者能自行下载。

具体体积：

| 资源 | 体积 |
|---|---|
| `qt-opensource-windows-x86-5.12.12.exe` | 3.80 GB |
| `opencv-4.8.0-windows/`（7615 个文件） | 843 MB |
| `opencv-4.8.0-windows.rar` | 168 MB |
| `opencv-4.8.0.zip` | 92 MB |
| `opencv_contrib-4.8.0.zip` | 59 MB |
| `vs2017_community.zip` | 1.09 MB |
| `rk3399-usb-debian-bullseye-lxde-desktop-4.19-arm64-20260421.zip` | 1.66 GB |

对比之下，`Qt_OpenCV摄像头在板端移植命令文件` 里的 6 个 `.sh` **总共才约 4 KB**，
已建议放进 `scripts/board-env/` 一起上传——它们是有价值的板端环境复现脚本，和上面那些安装包不是一回事。
