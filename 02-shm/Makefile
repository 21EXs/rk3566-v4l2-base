# SDK 和工具链配置
SDK_PATH         := /home/xs/develop/TaiShanPie/Linux-sdk/sdk/Release
TOOLCHAIN_PATH   := $(SDK_PATH)/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin
CROSS_COMPILE    := $(TOOLCHAIN_PATH)/aarch64-linux-gnu-
CC              := $(CROSS_COMPILE)gcc

# 依赖库路径
MPP_INSTALL_DIR  := $(SDK_PATH)/external/mpp/build/linux/aarch64/install
FFMPEG_PATH      := /home/xs/develop/TaiShanPie/ffmpeg-rockchip/ffmpeg_rk_build
X264_PATH        := /home/xs/develop/TaiShanPie/x264/x264build
SYSROOT_PATH     := $(SDK_PATH)/buildroot/output/rockchip_rk3566/host/aarch64-buildroot-linux-gnu/sysroot

# RGA 库路径配置
# 方式1: 使用 buildroot sysroot 中的 librga（推荐）
RGA_INCLUDE_DIR  := $(SYSROOT_PATH)/usr/include/rga
RGA_LIB_DIR      := $(SYSROOT_PATH)/usr/lib

# 项目路径
PROJECT_ROOT     := /home/xs/桌面/Project/ssh/rk3566-v4l2-base/02-shm
SRC_DIR          := $(PROJECT_ROOT)/src
INCLUDE_DIR      := $(PROJECT_ROOT)/inc
BUILD_DIR        := $(SRC_DIR)/build
TARGET          := $(PROJECT_ROOT)/shm_app

# 源文件和目标文件
SRCS            := $(wildcard $(SRC_DIR)/*.c)
OBJS            := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# 编译选项
CFLAGS          := -Wall -O2
CFLAGS          += -I$(SYSROOT_PATH)/usr/include
CFLAGS          += -I$(INCLUDE_DIR)
CFLAGS          += -I$(MPP_INSTALL_DIR)/include
CFLAGS          += -I$(MPP_INSTALL_DIR)/include/rockchip
CFLAGS          += -I$(FFMPEG_PATH)/include
CFLAGS          += -I$(X264_PATH)/include
# RGA 头文件路径
CFLAGS          += -I$(RGA_INCLUDE_DIR)
CFLAGS          += --sysroot=$(SYSROOT_PATH)
CFLAGS          += -D_GNU_SOURCE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

# 链接选项
LDFLAGS         := -L$(SYSROOT_PATH)/usr/lib
LDFLAGS         += -L$(MPP_INSTALL_DIR)/lib
LDFLAGS         += -L$(FFMPEG_PATH)/lib
LDFLAGS         += -L$(X264_PATH)/lib
# RGA 库路径
LDFLAGS         += -L$(RGA_LIB_DIR)
LDFLAGS         += --sysroot=$(SYSROOT_PATH)
LDFLAGS         += -Wl,-rpath,$(FFMPEG_PATH)/lib -Wl,-rpath,$(X264_PATH)/lib -Wl,-rpath,$(MPP_INSTALL_DIR)/lib
# RGA 运行时库路径（如果 librga 不在系统默认路径）
LDFLAGS         += -Wl,-rpath,$(RGA_LIB_DIR)

# 链接库
# -lrga: 使用 librga 硬件加速库（RGA 2D加速）
# 如果使用 librga2，改为 -lrga2
LIBS            := -lpthread -lrt -lrockchip_mpp -lrga
LIBS            += -lavformat -lavcodec -lavutil -lavdevice -lavfilter -lswscale -lswresample -lpostproc
LIBS            += -lx264

# 构建规则
.PHONY: all clean check-rga info

all: check-rga $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 检查 RGA 环境
check-rga:
	@echo "检查 RGA 环境..."
	@if [ -d "$(RGA_INCLUDE_DIR)" ]; then \
		echo "  ✓ RGA 头文件目录: $(RGA_INCLUDE_DIR)"; \
		ls $(RGA_INCLUDE_DIR)/*.h 2>/dev/null | head -5 | while read f; do \
			echo "    - $$(basename $$f)"; \
		done; \
	else \
		echo "  ⚠ 警告: RGA 头文件目录不存在: $(RGA_INCLUDE_DIR)"; \
		echo "  ⚠ 将使用 CPU 软件方式实现格式转换"; \
	fi
	@if [ -f "$(RGA_LIB_DIR)/librga.so" ]; then \
		echo "  ✓ RGA 库文件: $(RGA_LIB_DIR)/librga.so"; \
	elif [ -f "$(RGA_LIB_DIR)/librga2.so" ]; then \
		echo "  ✓ RGA 库文件: $(RGA_LIB_DIR)/librga2.so"; \
	else \
		echo "  ⚠ 警告: 未找到 librga.so 或 librga2.so"; \
		echo "  ⚠ 将使用 CPU 软件方式实现格式转换"; \
	fi
	@echo ""

# 显示详细路径信息
info:
	@echo "=================== 02-shm 项目信息 ==================="
	@echo "项目根目录: $(PROJECT_ROOT)"
	@echo "编译器: $(CC)"
	@echo "目标文件: $(TARGET)"
	@echo ""
	@echo "--- RGA 配置 ---"
	@echo "RGA 头文件路径: $(RGA_INCLUDE_DIR)"
	@echo "RGA 库路径: $(RGA_LIB_DIR)"
	@echo "RGA 链接库: -lrga"
	@echo ""
	@echo "--- 路径信息 ---"
	@echo "SDK路径: $(SDK_PATH)"
	@echo "Sysroot路径: $(SYSROOT_PATH)"
	@echo "源文件目录: $(SRC_DIR)"
	@echo "头文件目录: $(INCLUDE_DIR)"
	@echo "构建目录: $(BUILD_DIR)"
	@echo ""
	@echo "--- 源文件 ---"
	@for src in $(SRCS); do \
		echo "  $$src"; \
	done
	@echo ""
	@echo "--- 编译选项 ---"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "LIBS: $(LIBS)"
	@echo "========================================================"

clean:
	@rm -rf $(BUILD_DIR) $(TARGET)
