@echo off
REM ============================================================================
REM build.bat - KRAY 项目一键编译脚本（Windows）
REM
REM 用法：
REM   build.bat                默认：Debug 模式编译主项目 + USB 示例
REM   build.bat main           仅编译 KRAY 主项目
REM   build.bat usb            仅编译 USB 控制器示例
REM   build.bat clean          清理构建目录
REM   build.bat --release      Release 模式
REM   build.bat --debug        Debug 模式（默认）
REM   build.bat --no-example   主项目不编译示例
REM   build.bat --with-example 主项目强制编译示例
REM ============================================================================

setlocal enabledelayedexpansion

REM 项目根目录
set "ROOT_DIR=%~dp0"
REM 去掉末尾反斜杠
set "ROOT_DIR=%ROOT_DIR:~0,-1%"
REM 构建根目录
set "BUILD_ROOT=%ROOT_DIR%\build"
REM 并行数
set "JOBS=%NUMBER_OF_PROCESSORS%"
REM 构建类型（默认 Debug）
set "BUILD_TYPE=Debug"
REM 是否编译示例（空值表示使用 CMakeLists.txt 默认值）
set "EXAMPLES_FLAG="

REM 解析参数
set "TARGET=all"
:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="main"           set "TARGET=main"
if /i "%~1"=="usb"            set "TARGET=usb"
if /i "%~1"=="clean"          set "TARGET=clean"
if /i "%~1"=="--release"      set "BUILD_TYPE=Release"
if /i "%~1"=="--debug"        set "BUILD_TYPE=Debug"
if /i "%~1"=="--no-example"   set "EXAMPLES_FLAG=-DKRY_BUILD_EXAMPLE=OFF"
if /i "%~1"=="--with-example" set "EXAMPLES_FLAG=-DKRY_BUILD_EXAMPLE=ON"
shift
goto parse_args
:end_parse

REM 构建目录
set "BUILD_DIR=%BUILD_ROOT%"

REM ============================================================================
REM 清理构建目录
REM ============================================================================
if "%TARGET%"=="clean" (
    echo [INFO] 清理构建目录...
    if exist "%BUILD_ROOT%" rmdir /s /q "%BUILD_ROOT%"
    echo [INFO] 已清理 %BUILD_ROOT%
    goto :eof
)

REM ============================================================================
REM 检查依赖工具
REM ============================================================================
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] 未找到 cmake，请先安装并添加到 PATH
    exit /b 1
)

REM ============================================================================
REM 编译 KRAY 主项目（需要 Qt）
REM ============================================================================
if "%TARGET%"=="main" goto build_main
if "%TARGET%"=="all"  goto build_main
goto skip_main

:build_main
echo [INFO] ========== 编译 KRAY 主项目 ==========
echo [INFO] 构建类型: %BUILD_TYPE%

set "MAIN_BUILD=%BUILD_DIR%\main"
if not exist "%MAIN_BUILD%" mkdir "%MAIN_BUILD%"

echo [INFO] CMake 配置...
cmake -S "%ROOT_DIR%" -B "%MAIN_BUILD%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% %EXAMPLES_FLAG%
if errorlevel 1 (
    echo [ERROR] CMake 配置失败
    exit /b 1
)

echo [INFO] 编译中... (并行 %JOBS%)
cmake --build "%MAIN_BUILD%" --config %BUILD_TYPE% -j%JOBS%
if errorlevel 1 (
    echo [ERROR] 编译失败
    exit /b 1
)

echo [INFO] KRAY 主项目编译完成: %MAIN_BUILD%\bin\
if "%TARGET%"=="main" goto done

:skip_main

REM ============================================================================
REM 编译 USB 控制器示例（不需要 Qt）
REM ============================================================================
if "%TARGET%"=="usb" goto build_usb
if "%TARGET%"=="all" goto build_usb
goto skip_usb

:build_usb
echo [INFO] ========== 编译 USB 控制器示例 ==========
echo [INFO] 构建类型: %BUILD_TYPE%

set "USB_SRC=%ROOT_DIR%\examples\usb_controller_fun3"
set "USB_BUILD=%BUILD_DIR%\usb"
if not exist "%USB_BUILD%" mkdir "%USB_BUILD%"

echo [INFO] CMake 配置...
cmake -S "%USB_SRC%" -B "%USB_BUILD%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo [ERROR] CMake 配置失败
    exit /b 1
)

echo [INFO] 编译中... (并行 %JOBS%)
cmake --build "%USB_BUILD%" --config %BUILD_TYPE% -j%JOBS%
if errorlevel 1 (
    echo [ERROR] 编译失败
    exit /b 1
)

echo [INFO] USB 示例编译完成:
echo [INFO]   可执行文件: %USB_BUILD%\bin\
echo [INFO]   单元测试:   %USB_BUILD%\bin\test_*.exe
echo [INFO]   C API 库:   %USB_BUILD%\Debug\lib\dynamic\libusb_ctrl_capi.dll

:skip_usb

:done
echo [INFO] ========== 全部完成 ==========
endlocal
