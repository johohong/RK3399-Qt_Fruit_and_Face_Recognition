# 模型流水线：YOLOv8 训练 → ONNX → MNN

本文档说明水果检测模型 `models/trained_fp32.mnn` 的来历与复现方式。

> ⚠️ **待补充**：标注为 `TODO` 的段落需要作者根据实际训练记录填写。
> 本仓库现有文件中**没有**训练数据集、训练脚本与超参记录，
> 为避免写入无法核实的内容，这些字段一律留空待补，不做推测。

---

## 一、模型事实（已从工程内文件核实）

以下信息来自仓库中的实际文件，可直接引用：

| 项目 | 值 | 来源 |
|---|---|---|
| 模型文件 | `models/trained_fp32.mnn` | 工程实际文件 |
| 文件大小 | 12.08 MB | 实测 |
| 精度 | FP32（未量化） | 文件名 `fp32` + `config/mnn-source.json` |
| 输出张量 | `[1, 7, 8400]` | `src/postprocess.cpp` 中的 `ultralytics_single_head` 判定 |
| 张量含义 | 4 坐标 + 3 类别，8400 个候选框 | `7 = 4 + 3` |
| 类别 | `apple` / `banana` / `orange` | `src/postprocess.cpp:27` 的 `names[]` |
| 输入尺寸 | 640 | `src/detector.cpp` 预处理 |
| 默认置信度阈值 | 0.25 | `src/detection_types.h` |
| 默认 NMS IoU 阈值 | 0.45（类别感知） | `src/detection_types.h` |
| 最大保留框数 | 300 | `src/postprocess.cpp` NMS 循环上限 |
| 转换器版本 | MNN 3.3.0 | `config/mnn-source.json` |
| 转换器 commit | `5047919a1ae594d090ee54c7ea3403a9dee8cad4` | `config/mnn-source.json` |
| 转换器包 SHA256 | `08424890d610b01a62df3f9dfa654cd37dd39bf921521639a1c233cc062c58c1` | `config/mnn-source.json` |
| 板端运行时 MNN | 3.6.2 | `docs/rk3399-runbook.md` |

**关于转换版本与运行版本不一致**：转换用 MNN 3.3.0，板端运行 MNN 3.6.2。该组合已通过 RK3399 上的真实水果推理验证（输出 `[1,7,8400]` 正确解析），但**不能据此声称所有 MNN 模型都跨版本兼容**。更换模型或更换运行库版本后需重新验证。

---

## 二、训练（TODO）

| 项目 | 值 |
|---|---|
| 基础模型 | TODO（如 `yolov8n` / `yolov8s`） |
| 数据集 | TODO（名称、来源、图片数量） |
| 训练/验证划分 | TODO |
| 标注工具 | TODO |
| 训练框架版本 | TODO（如 `ultralytics==8.x.x`） |
| 训练轮次 epochs | TODO |
| 输入分辨率 imgsz | TODO |
| batch size | TODO |
| 优化器与学习率 | TODO |
| 数据增强策略 | TODO |
| 训练硬件 | TODO |
| 训练时长 | TODO |

### 训练命令（TODO）

```bash
# TODO: 填入实际使用的 yolo 训练命令
```

### 精度指标（TODO）

| 指标 | apple | banana | orange | 全部 |
|---|---|---|---|---|
| mAP@0.5 | TODO | TODO | TODO | TODO |
| mAP@0.5:0.95 | TODO | TODO | TODO | TODO |
| Precision | TODO | TODO | TODO | TODO |
| Recall | TODO | TODO | TODO | TODO |

> 建议附上 `results.png`、`confusion_matrix.png`、`PR_curve.png` 到 `docs/images/`。
>
> **注意**：以上应为训练框架在验证集上给出的指标。工程内的板端实测只验证了"推理能跑通、输出形状正确"，
> **没有**做过精度评估（见 `docs/rk3399-runbook.md` 的"未完成"清单），两者不要混为一谈。

---

## 三、导出 ONNX（TODO）

```bash
# TODO: 填入实际导出命令，例如
# yolo export model=best.pt format=onnx opset=12 simplify=True
```

| 项目 | 值 |
|---|---|
| opset 版本 | TODO |
| 是否 simplify | TODO |
| 是否含 NMS 节点 | TODO（从输出 `[1,7,8400]` 单头看，NMS 在 C++ 侧做，模型内应不含） |
| ONNX 文件大小 | TODO |

---

## 四、ONNX → MNN 转换

转换器版本已由 `config/mnn-source.json` 固定为 **MNN 3.3.0**，下载来源与 SHA256 均已记录，可复现。

### 4.1 构建转换器

```bash
# 拉取固定版本（tag 3.3.0，commit 5047919a1ae594d090ee54c7ea3403a9dee8cad4）
git clone https://github.com/alibaba/MNN.git
cd MNN
git checkout 3.3.0

# 构建转换工具（需打开 MNN_BUILD_CONVERTER）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DMNN_BUILD_CONVERTER=ON
cmake --build build --parallel
```

构建产物：`build/MNNConvert`（Linux/macOS）或 `build/Release/MNNConvert.exe`（Windows）。

### 4.2 执行转换

```bash
./MNNConvert -f ONNX \
             --modelFile  best.onnx \
             --MNNModel   trained_fp32.mnn \
             --bizCode    fruit
```

> TODO: 确认实际使用的参数（是否加了 `--fp16`、`--weightQuantBits` 等）。
> 从模型名 `trained_fp32.mnn` 与 12 MB 体积判断为 **FP32 未量化**，请勿在文档里写成量化模型。

### 4.3 转换后校验

转换完成后应校验输出张量形状与工程预期一致：

```text
期望输出: [1, 7, 8400]
```

`src/postprocess.cpp` 的 `validateOutputs()` 会做严格校验：

- 只接受 `outputs.size()==1 && shape=={1,7,8400}`（`ultralytics_single_head`），或 `outputs.size()==9` 的九头拆分格式；
- 维度必须在 `(0, 100000]` 且张量元素总数不超过约 1000 万；
- 张量长度必须与 `shape` 各维乘积完全一致；
- 所有值必须有限（`std::isfinite`），出现 NaN/Inf 直接抛 `MODEL_SCHEMA_UNSUPPORTED`。

**任何一项不满足都会抛异常，不会静默产出错误结果。** 换模型后先用 `FruitDetect2Cli` 跑一张已知图片：

```bash
./build-linux/bin/FruitDetect2Cli --backend mnn \
  --image    /abs/path/tests/fixtures/apple_12.jpg \
  --output-json /abs/path/check.json
# 退出码 0 且输出 DETECTION_PASS 才算通过；退出码 3 = 模型加载失败，5 = 推理失败
```

---

## 五、部署到工程

```text
trained_fp32.mnn  →  models/trained_fp32.mnn
```

CMake 的 `FruitModels` 目标在每次构建时自动把 `models/` 与 `face-models/` 拷贝到 `bin/`，无需手动复制。

程序默认加载路径为 `<可执行文件目录>/models/trained_fp32.mnn`，可用 `--model` 传**绝对路径**覆盖（相对路径会被拒绝，退出码 2）。

---

## 六、更换模型 / 增加类别的改动点

如果重新训练了模型或改了类别数，**必须同步修改以下位置**，否则后处理会解析错位：

| 文件 | 需要改什么 |
|---|---|
| `src/postprocess.cpp:27` | `const QString names[]={"apple","banana","orange"};` — 类别名数组，顺序必须与训练时一致 |
| `src/postprocess.cpp` | `validateOutputs()` 里 `{1,7,8400}` 的 `7`，等于 `4 + 类别数` |
| `src/postprocess.cpp` | NMS 后 `kept.size()>=300` 的上限，按需调整 |
| `models/trained_fp32.mnn` | 替换为新模型文件 |
| `config/mnn-source.json` | 若换了 MNN 转换器版本，更新 tag / commit / SHA256 |
| 本文档第二节 | 补上新模型的训练记录与精度指标 |

---

## 七、参考资料

- MNN 3.3.0 构建选项：<https://github.com/alibaba/MNN/blob/3.3.0/CMakeLists.txt>
- MNN 模型转换文档：<https://mnn-docs.readthedocs.io/en/latest/tools/convert.html>
- Ultralytics YOLOv8 导出文档：<https://docs.ultralytics.com/modes/export/>
