$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path (Split-Path -Parent $root) ('FruitDetect2-linux-source-'+(Get-Date -Format yyyyMMdd-HHmmss)+'.zip')
if(Test-Path -LiteralPath $out){throw 'Archive already exists'}
$files=@('src','models','face-models','tests','scripts','CMakeLists.txt','FruitDetect2.pro','README.md','LINUX_DEPLOY.md','config') | ForEach-Object {Join-Path $root $_}
Compress-Archive -LiteralPath $files -DestinationPath $out
Write-Output "LINUX_SOURCE_EXPORT_PASS $out"
exit 0
