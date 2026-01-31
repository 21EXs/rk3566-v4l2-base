# ============================================================================
# Makefile 配置
# ============================================================================

# -------------------------------
# SDK 和工具链配置
# -------------------------------
SDK_PATH         := /home/xs/develop/TaiShanPie/Linux-sdk/sdk/Release
TOOLCHAIN_PATH   := $(SDK_PATH)/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin
CROSS_COMPILE    := $(TOOLCHAIN_PATH)/aarch64-linux-gnu-
CC              := $(CROSS_COMPILE)gcc

# -------------------------------
# MPP 安装路径（使用预编译的库）
# -------------------------------
MPP_INSTALL_DIR  := $(SDK_PATH)/external/mpp/build/linux/aarch64/install
MPP_INCLUDE_DIR  := $(MPP_INSTALL_DIR)/include
MPP_LIB_DIR      := $(MPP_INSTALL_DIR)/lib

# -------------------------------
# FFmpeg 路径
# -------------------------------
FFMPEG_PATH      := /home/xs/develop/TaiShanPie/ffmpeg-rockchip/ffmpeg_rk_build
FFMPEG_INCLUDE   := $(FFMPEG_PATH)/include
FFMPEG_LIB       := $(FFMPEG_PATH)/lib

# -------------------------------
# x264 路径
# -------------------------------
X264_PATH        := /home/xs/develop/TaiShanPie/x264/x264build
X264_INCLUDE     := $(X264_PATH)/include
X264_LIB         := $(X264_PATH)/lib

# -------------------------------
# 系统路径配置
# -------------------------------
SYSROOT_PATH     := $(SDK_PATH)/buildroot/output/rockchip_rk3566/host/aarch64-buildroot-linux-gnu/sysroot
SYSROOT_INCLUDE  := $(SYSROOT_PATH)/usr/include
SYSROOT_LIB      := $(SYSROOT_PATH)/usr/lib

# -------------------------------
# 项目配置
# -------------------------------
PROJECT_ROOT     := /home/xs/桌面/Project/ssh/rk3566-v4l2-base/02-shm
SRC_DIR          := $(PROJECT_ROOT)/src
INCLUDE_DIR      := $(PROJECT_ROOT)/inc
BUILD_DIR        := $(SRC_DIR)/build
TARGET          := $(PROJECT_ROOT)/shm_app

# -------------------------------
# 源文件和目标文件
# -------------------------------
SRCS            := $(wildcard $(SRC_DIR)/*.c)
OBJS            := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# -------------------------------
# 编译选项
# -------------------------------
CFLAGS          := -Wall -O2
CFLAGS          += -I$(SYSROOT_INCLUDE)    # 系统头文件
CFLAGS          += -I$(INCLUDE_DIR)        # 项目头文件
CFLAGS          += -I$(MPP_INCLUDE_DIR)    # MPP 头文件
CFLAGS          += -I$(MPP_INCLUDE_DIR)/rockchip  # Rockchip 特定头文件
CFLAGS          += -I$(FFMPEG_INCLUDE)     # FFmpeg 头文件
CFLAGS          += -I$(X264_INCLUDE)       # x264 头文件
CFLAGS          += --sysroot=$(SYSROOT_PATH)  # 系统根目录

# 添加必要的预处理器定义
CFLAGS          += -D_GNU_SOURCE
CFLAGS          += -D_LARGEFILE_SOURCE
CFLAGS          += -D_LARGEFILE64_SOURCE
CFLAGS          += -D_FILE_OFFSET_BITS=64

# -------------------------------
# 链接选项
# -------------------------------
LDFLAGS         := -L$(SYSROOT_LIB)        # 系统库路径
LDFLAGS         += -L$(MPP_LIB_DIR)        # MPP 库路径
LDFLAGS         += -L$(FFMPEG_LIB)         # FFmpeg 库路径
LDFLAGS         += -L$(X264_LIB)           # x264 库路径
LDFLAGS         += --sysroot=$(SYSROOT_PATH)  # 系统根目录

# 添加运行时库搜索路径（非常重要！）
LDFLAGS         += -Wl,-rpath,$(FFMPEG_LIB)
LDFLAGS         += -Wl,-rpath,$(X264_LIB)
LDFLAGS         += -Wl,-rpath,$(MPP_LIB_DIR)

# -------------------------------
# 链接库
# -------------------------------
LIBS            := -lpthread -lrt
LIBS            += -lrockchip_mpp          # 主 MPP 库

# FFmpeg 库（按依赖顺序）
LIBS            += -lavformat
LIBS            += -lavcodec
LIBS            += -lavutil
LIBS            += -lavdevice
LIBS            += -lavfilter
LIBS            += -lswscale
LIBS            += -lswresample
LIBS            += -lpostproc

# x264 库
LIBS            += -lx264

# ============================================================================
# 构建目标
# ============================================================================

.PHONY: all clean distclean help check-env check-project check-ffmpeg info

# 默认目标
all: check-env check-ffmpeg $(BUILD_DIR) $(TARGET)

# 创建构建目录
$(BUILD_DIR):
	@echo "[INFO] 创建构建目录: $(BUILD_DIR)"
	@mkdir -p $(BUILD_DIR)

# 链接目标
$(TARGET): $(OBJS)
	@echo "[INFO] 链接目标: $(TARGET)"
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "[SUCCESS] 构建完成: $(TARGET)"

# 编译源文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "[CC] 编译: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# 辅助目标
# ============================================================================

# 检查环境
check-env:
	@echo "========================================"
	@echo "检查编译环境"
	@echo "========================================"
	
	@echo -n "✓ 检查编译器... "
	@if [ -f "$(CC)" ]; then \
		echo "找到: $(notdir $(CC))"; \
	else \
		echo "失败: $(CC)"; \
		exit 1; \
	fi
	
	@echo -n "✓ 检查标准头文件... "
	@if [ -f "$(SYSROOT_INCLUDE)/stdio.h" ]; then \
		echo "找到"; \
	else \
		echo "失败: $(SYSROOT_INCLUDE)"; \
		exit 1; \
	fi
	
	@echo -n "✓ 检查 MPP 头文件... "
	@if [ -d "$(MPP_INCLUDE_DIR)" ]; then \
		echo "找到: $(MPP_INCLUDE_DIR)"; \
	else \
		echo "警告: 未找到 MPP 头文件目录"; \
	fi
	
	@echo -n "✓ 检查 MPP 库文件... "
	@if [ -f "$(MPP_LIB_DIR)/librockchip_mpp.so" ]; then \
		echo "找到: librockchip_mpp.so"; \
	else \
		echo "警告: 未找到 MPP 库文件"; \
	fi

# 检查FFmpeg和x264
check-ffmpeg:
	@echo "----------------------------------------"
	@echo -n "✓ 检查 FFmpeg 头文件... "
	@if [ -d "$(FFMPEG_INCLUDE)" ]; then \
		echo "找到: $(FFMPEG_INCLUDE)"; \
	else \
		echo "失败: $(FFMPEG_INCLUDE)"; \
		exit 1; \
	fi
	
	@echo -n "✓ 检查 FFmpeg 库文件... "
	@if [ -d "$(FFMPEG_LIB)" ]; then \
		echo "找到: $(FFMPEG_LIB)"; \
		@echo "  FFmpeg 库列表:"; \
		@ls -1 $(FFMPEG_LIB)/libav*.so $(FFMPEG_LIB)/libpostproc*.so $(FFMPEG_LIB)/libsw*.so 2>/dev/null | xargs -I {} basename {} || true; \
	else \
		echo "失败: $(FFMPEG_LIB)"; \
		exit 1; \
	fi
	
	@echo -n "✓ 检查 x264 头文件... "
	@if [ -d "$(X264_INCLUDE)" ]; then \
		echo "找到: $(X264_INCLUDE)"; \
	else \
		echo "失败: $(X264_INCLUDE)"; \
		exit 1; \
	fi
	
	@echo -n "✓ 检查 x264 库文件... "
	@if [ -f "$(X264_LIB)/libx264.so" -o -f "$(X264_LIB)/libx264.so.*" ]; then \
		echo "找到: $(X264_LIB)"; \
		@ls -1 $(X264_LIB)/libx264* 2>/dev/null | xargs -I {} basename {} || true; \
	else \
		echo "失败: $(X264_LIB)"; \
		exit 1; \
	fi
	@echo "----------------------------------------"

# 检查项目结构
check-project:
	@echo "========================================"
	@echo "检查项目结构"
	@echo "========================================"
	
	@echo -n "✓ 源文件目录... "
	@if [ -d "$(SRC_DIR)" ]; then \
		echo "存在"; \
		echo "  源文件列表:"; \
		for f in $(SRCS); do echo "    $$(basename $$f)"; done; \
	else \
		echo "失败: $(SRC_DIR)"; \
		exit 1; \
	fi
	
	@echo -n "✓ 头文件目录... "
	@if [ -d "$(INCLUDE_DIR)" ]; then \
		echo "存在"; \
	else \
		echo "警告: 不存在"; \
	fi
	
	@echo "----------------------------------------"

# 显示详细信息
info:
	@echo "========================================"
	@echo "项目构建信息"
	@echo "========================================"
	@echo "项目名称:    $(notdir $(PROJECT_ROOT))"
	@echo "项目路径:    $(PROJECT_ROOT)"
	@echo "目标文件:    $(TARGET)"
	@echo "编译器:     $(CC)"
	@echo "构建目录:    $(BUILD_DIR)"
	@echo "源文件数:    $(words $(SRCS))"
	@echo "系统根目录:  $(SYSROOT_PATH)"
	@echo "MPP 头文件目录: $(MPP_INCLUDE_DIR)"
	@echo "MPP 库目录:   $(MPP_LIB_DIR)"
	@echo "FFmpeg 头文件目录: $(FFMPEG_INCLUDE)"
	@echo "FFmpeg 库目录: $(FFMPEG_LIB)"
	@echo "x264 头文件目录: $(X264_INCLUDE)"
	@echo "x264 库目录:   $(X264_LIB)"
	@echo ""
	@echo "编译选项:"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo ""
	@echo "链接选项:    $(LDFLAGS)"
	@echo "链接库:     $(LIBS)"
	@echo "========================================"

# 清理
clean:
	@echo "[INFO] 清理构建文件..."
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "[SUCCESS] 清理完成"

# 完全清理
distclean: clean
	@echo "[INFO] 完全清理完成"

# 帮助
help:
	@echo "========================================"
	@echo "可用命令"
	@echo "========================================"
	@echo "make              - 构建项目 (默认)"
	@echo "make all          - 同 make"
	@echo "make clean        - 清理构建文件"
	@echo "make distclean    - 完全清理"
	@echo "make check-env    - 检查编译环境"
	@echo "make check-ffmpeg - 检查FFmpeg和x264"
	@echo "make check-project - 检查项目结构"
	@echo "make info         - 显示详细配置"
	@echo "make help         - 显示此帮助"
	@echo "========================================"