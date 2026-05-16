#!/bin/bash
# ============================================================================
# build.sh - KRAY 项目一键编译脚本（Linux/macOS）
#
# 用法：
#   ./build.sh              # 默认：Debug 模式编译主项目 + USB 示例
#   ./build.sh main         # 仅编译 KRAY 主项目
#   ./build.sh usb          # 仅编译 USB 控制器示例
#   ./build.sh clean        # 清理构建目录
#   ./build.sh -j8          # 指定并行数
#   ./build.sh --release    # Release 模式
#   ./build.sh --debug      # Debug 模式（默认）
#   ./build.sh --no-example # 主项目不编译示例
#   ./build.sh --with-example # 主项目编译示例
# ============================================================================

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # 无颜色

# 项目根目录（脚本所在目录）
ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
# 构建根目录
BUILD_ROOT="${ROOT_DIR}/build"
# 并行数（默认使用 CPU 核心数）
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
# 构建类型（默认 Debug）
BUILD_TYPE=Debug
# 是否编译示例（空值表示使用 CMakeLists.txt 默认值）
EXAMPLES_FLAG=""

# ============================================================================
# 打印带颜色的信息
# ============================================================================
info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

# ============================================================================
# 解析参数
# ============================================================================
TARGET="all"
while [[ $# -gt 0 ]]; do
    case "$1" in
        main)           TARGET="main" ;;
        usb)            TARGET="usb" ;;
        clean)          TARGET="clean" ;;
        -j*)            JOBS="${1#-j}" ;;
        --release)      BUILD_TYPE=Release ;;
        --debug)        BUILD_TYPE=Debug ;;
        --no-example)   EXAMPLES_FLAG="-DKRY_BUILD_EXAMPLE=OFF" ;;
        --with-example) EXAMPLES_FLAG="-DKRY_BUILD_EXAMPLE=ON" ;;
        *)              warn "未知参数: $1" ;;
    esac
    shift
done

# 构建目录
BUILD_DIR="${BUILD_ROOT}"

# ============================================================================
# 清理构建目录
# ============================================================================
if [[ "$TARGET" == "clean" ]]; then
    info "清理构建目录..."
    rm -rf "${BUILD_ROOT}"
    info "已清理 ${BUILD_ROOT}"
    exit 0
fi

# ============================================================================
# 检查依赖工具
# ============================================================================
check_tool() {
    if ! command -v "$1" &>/dev/null; then
        error "未找到 $1，请先安装"
    fi
}

check_tool cmake
check_tool make

# ============================================================================
# 编译 KRAY 主项目（需要 Qt）
# ============================================================================
build_main() {
    info "========== 编译 KRAY 主项目 =========="
    info "构建类型: ${BUILD_TYPE}"

    local main_build="${BUILD_DIR}/main"
    mkdir -p "${main_build}"

    # CMake 配置
    info "CMake 配置..."
    cmake -S "${ROOT_DIR}" -B "${main_build}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        ${EXAMPLES_FLAG} \
        || error "CMake 配置失败"

    # 编译
    info "编译中... (并行 ${JOBS})"
    cmake --build "${main_build}" -j"${JOBS}" \
        || error "编译失败"

    info "KRAY 主项目编译完成: ${main_build}/${BUILD_TYPE,,}/bin/"
}

# ============================================================================
# 编译 USB 控制器示例（不需要 Qt）
# ============================================================================
build_usb() {
    info "========== 编译 USB 控制器示例 =========="
    info "构建类型: ${BUILD_TYPE}"

    local usb_src="${ROOT_DIR}/examples/usb_controller_fun3"
    local usb_build="${BUILD_DIR}/usb"
    mkdir -p "${usb_build}"

    # CMake 配置
    info "CMake 配置..."
    cmake -S "${usb_src}" -B "${usb_build}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        || error "CMake 配置失败"

    # 编译
    info "编译中... (并行 ${JOBS})"
    cmake --build "${usb_build}" -j"${JOBS}" \
        || error "编译失败"

    info "USB 示例编译完成:"
    info "  可执行文件: ${usb_build}/${BUILD_TYPE,,}/bin/"
    info "  单元测试:   ${usb_build}/${BUILD_TYPE,,}/bin/test_*"
    info "  C API 库:   ${usb_build}/${BUILD_TYPE,,}/lib/dynamic/libusb_ctrl_capi.so"
    info ""
    info "运行 Python UI 测试:"
    info "  cd ${usb_src}/test && python3 usb_test_ui.py"
}

# ============================================================================
# 执行编译
# ============================================================================
case "$TARGET" in
    main) build_main ;;
    usb)  build_usb ;;
    all)
        build_main
        echo ""
        build_usb
        ;;
esac

info "========== 全部完成 =========="
