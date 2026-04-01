@echo off
REM 生成 VS2022 x64 项目配置

echo 清理旧的 build 目录...
if exist build (
    rmdir /s /q build
)

echo 创建 build 目录...
mkdir build

echo 生成 VS2022 x64 配置...
cd build
cmake .. -G "Visual Studio 17 2022" -A x64

echo 完成！
pause
