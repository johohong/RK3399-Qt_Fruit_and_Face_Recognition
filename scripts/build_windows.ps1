<#
.SYNOPSIS
    Windows 一键构建脚本：CMake 配置 + 编译 + 依赖部署。

.DESCRIPTION
    补齐 README 中引用但原工程未提供的构建入口。
    默认使用 Release 配置，产物输出到 build-win/bin/。

    依赖（需自行准备，见 README 第四、六节）：
      - Qt 5.12.12 MSVC2017 64bit
      - OpenCV 4.8.0 Windows 预编译包
      - MNN Windows x64（third_party/mnn-windows）
      - 人脸模型（scripts/download_face_models.ps1）

.PARAMETER OpenCVDir
    OpenCV 的 CMake 配置目录。注意要指到实际的 vc 版本子目录，
    否则可能被自动推断成 vc14（详见 docs/windows-build.md）。

.PARAMETER MnnRoot
    MNN 安装前缀，需包含 include/MNN、lib、bin。

.PARAMETER BuildTests
    构建离线测试并注册 ctest。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\build_windows.ps1
    powershell -ExecutionPolicy Bypass -File scripts\build_windows.ps1 `
        -OpenCVDir "D:/opencv-4.8.0/build/x64/vc16/lib/cmake/opencv4" -BuildTests
#>

[CmdletBinding()]
param(
    [string]$OpenCVDir = 'E:/Tool/opencv-4.8.0-windows/opencv/build',
    [string]$MnnRoot   = './third_party/mnn-windows',
    [string]$BuildDir  = 'build-win',
    [switch]$BuildTests
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $repoRoot

Write-Host "仓库根目录 : $repoRoot"
Write-Host "OpenCV_DIR : $OpenCVDir"
Write-Host "MNN_ROOT   : $MnnRoot"
Write-Host "构建目录   : $BuildDir"
Write-Host ''

# ---------- 前置检查 ----------
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "找不到 cmake，请确认已安装并加入 PATH（需 >= 3.10）。"
}

if (-not (Test-Path -LiteralPath (Join-Path $repoRoot 'CMakeLists.txt'))) {
    throw "找不到 CMakeLists.txt，请在仓库根目录运行本脚本。"
}

# MNN 平台匹配检查：Windows 库不能用于板端，反之亦然
$mnnInclude = Join-Path $MnnRoot 'include\MNN\Interpreter.hpp'
if (-not (Test-Path -LiteralPath $mnnInclude)) {
    Write-Warning "未找到 MNN 头文件: $mnnInclude"
    Write-Warning "请先从 MNN Releases 获取 Windows x64 库并放到 third_party/mnn-windows/。"
    Write-Warning "CMake 会在链接阶段直接 FATAL_ERROR，不会静默降级。"
}

# 人脸模型检查（缺失不阻断构建，但运行时会失败）
$sface = Join-Path $repoRoot 'face-models\face_recognition_sface_2021dec.onnx'
if (-not (Test-Path -LiteralPath $sface)) {
    Write-Warning "人脸模型尚未下载，构建可继续但人脸功能无法运行。"
    Write-Warning "请执行: powershell -ExecutionPolicy Bypass -File scripts/download_face_models.ps1"
}

# ---------- 配置 ----------
$cmakeArgs = @(
    '-S', $repoRoot
    '-B', (Join-Path $repoRoot $BuildDir)
    '-DCMAKE_BUILD_TYPE=Release'
    "-DOpenCV_DIR=$OpenCVDir"
    "-DMNN_ROOT=$MnnRoot"
)
if ($BuildTests) { $cmakeArgs += '-DFRUIT_BUILD_TESTS=ON' }

Write-Host "[1/3] CMake 配置..."
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败（退出码 $LASTEXITCODE）。" }

# ---------- 编译 ----------
Write-Host ''
Write-Host "[2/3] 编译 Release..."
& cmake --build (Join-Path $repoRoot $BuildDir) --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "编译失败（退出码 $LASTEXITCODE）。" }

# ---------- 测试 ----------
if ($BuildTests) {
    Write-Host ''
    Write-Host "[3/3] 运行离线测试 (ctest)..."
    Push-Location -LiteralPath (Join-Path $repoRoot $BuildDir)
    try {
        & ctest -C Release --output-on-failure
        if ($LASTEXITCODE -ne 0) { throw "离线测试失败（退出码 $LASTEXITCODE）。" }
    }
    finally { Pop-Location }
} else {
    Write-Host ''
    Write-Host "[3/3] 跳过测试（未指定 -BuildTests）"
}

# ---------- 结果 ----------
$binDir = Join-Path $repoRoot "$BuildDir\bin"
Write-Host ''
Write-Host "构建完成。产物目录: $binDir" -ForegroundColor Green
if (Test-Path -LiteralPath $binDir) {
    Get-ChildItem -LiteralPath $binDir -File | ForEach-Object {
        Write-Host ("  {0,10:N1} KB  {1}" -f ($_.Length/1KB), $_.Name)
    }
}

Write-Host ''
Write-Host "下一步："
Write-Host "  1. 运行 .\$BuildDir\bin\FruitDetect2.exe --backend mnn --threads 4 --camera 0"
Write-Host "  2. 程序启动后不会自动开摄像头，需在界面点击「打开摄像头」；管理员初始密码 123"
Write-Host "  3. 建议先用 --smoke-image 做无界面验收（见 docs/windows-build.md 第四节）"
exit 0
