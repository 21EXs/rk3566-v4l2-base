# 使用SDK自带的交叉编译工具链
SDK_PATH = /home/xs/develop/TaiShanPie/Linux-sdk/sdk/Release
TOOLCHAIN_PATH = $(SDK_PATH)/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin
CROSS_COMPILE = $(TOOLCHAIN_PATH)/aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

# 项目根目录
PROJECT_ROOT = /home/xs/桌面/Project/ssh/rk3566-v4l2-base/02-shm

# 目录路径
SRC_DIR = $(PROJECT_ROOT)/src
BUILD_DIR = $(SRC_DIR)/build
INCLUDE_DIR = $(PROJECT_ROOT)/inc
SOURCE_DIR = $(PROJECT_ROOT)/src
SRCS = $(wildcard $(SOURCE_DIR)/*.c)
OBJS = $(SRCS:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o)

# 目标文件名
TARGET = $(PROJECT_ROOT)/shm_app

# 使用正确的头文件路径
SYSROOT_PATH = $(SDK_PATH)/buildroot/output/rockchip_rk3566/host/aarch64-buildroot-linux-gnu/sysroot
INCLUDE_PATH = $(SYSROOT_PATH)/usr/include

# 编译标志
CFLAGS = -Wall -O2
CFLAGS += -I$(INCLUDE_PATH) -I$(INCLUDE_DIR)
# 添加sysroot确保使用正确的库
CFLAGS += --sysroot=$(SYSROOT_PATH)

# 链接标志
LDFLAGS = -L$(SYSROOT_PATH)/usr/lib -L$(SYSROOT_PATH)/lib
LDFLAGS += --sysroot=$(SYSROOT_PATH)

# 链接库 - 02-shm项目可能需要不同的库
# 如果使用共享内存，可能需要 -lrt
LIBS = -lpthread -lrt

# 默认目标
all: check-env $(BUILD_DIR) $(TARGET)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 链接目标
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# 编译源文件
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 检查环境
check-env:
	@echo "检查工具链..."
	@if [ -f "$(CC)" ]; then \
		echo "✓ 工具链存在: $(CC)"; \
	else \
		echo "✗ 错误: 工具链不存在: $(CC)"; \
		exit 1; \
	fi
	@echo "检查头文件..."
	@if [ -f "$(INCLUDE_PATH)/stdio.h" ]; then \
		echo "✓ 找到stdio.h在: $(INCLUDE_PATH)"; \
	else \
		echo "✗ 错误: 未找到标准头文件"; \
		exit 1; \
	fi

# 检查项目结构
check-project:
	@echo "检查项目结构..."
	@if [ -d "$(SRC_DIR)" ]; then \
		echo "✓ 源文件目录存在: $(SRC_DIR)"; \
		ls -la $(SRC_DIR)/*.c 2>/dev/null | wc -l | xargs echo "找到源文件数量:"; \
	else \
		echo "✗ 错误: 源文件目录不存在: $(SRC_DIR)"; \
		exit 1; \
	fi
	@if [ -d "$(INCLUDE_DIR)" ]; then \
		echo "✓ 头文件目录存在: $(INCLUDE_DIR)"; \
	else \
		echo "⚠ 警告: 头文件目录不存在: $(INCLUDE_DIR)"; \
	fi

# 显示详细路径信息
info:
	@echo "项目根目录: $(PROJECT_ROOT)"
	@echo "项目名称: 02-shm"
	@echo "编译器路径: $(CC)"
	@echo "源文件目录: $(SRC_DIR)"
	@echo "构建目录: $(BUILD_DIR)"
	@echo "包含目录: $(INCLUDE_DIR)"
	@echo "SDK路径: $(SDK_PATH)"
	@echo "Sysroot路径: $(SYSROOT_PATH)"
	@echo "头文件路径: $(INCLUDE_PATH)"
	@echo "编译标志: $(CFLAGS)"
	@echo "链接标志: $(LDFLAGS)"
	@echo "链接库: $(LIBS)"
	@echo "目标文件: $(TARGET)"
	@echo "源文件: $(SRCS)"
	@echo "目标文件: $(OBJS)"

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# 完全清理
distclean: clean
	@echo "已清理所有生成文件"

# 显示帮助信息
help:
	@echo "可用目标:"
	@echo "  all           - 编译项目（默认）"
	@echo "  check-env     - 检查编译环境"
	@echo "  check-project - 检查项目结构"
	@echo "  info          - 显示详细路径信息"
	@echo "  clean         - 清理生成的文件"
	@echo "  distclean     - 完全清理（同clean）"
	@echo "  help          - 显示此帮助信息"

.PHONY: all clean distclean help check-env check-project info