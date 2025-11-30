# 使用SDK自带的交叉编译工具链
SDK_PATH = /home/xs/develop/TaiShanPie/Linux-sdk/sdk/Release
TOOLCHAIN_PATH = $(SDK_PATH)/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin
CROSS_COMPILE = $(TOOLCHAIN_PATH)/aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

# 目录路径
SRC_DIR = /home/xs/桌面/Project/01-v4l2/src
BUILD_DIR = $(SRC_DIR)/build
INCLUDE_DIR = /home/xs/桌面/Project/01-v4l2/inc
SOURCE_DIR = /home/xs/桌面/Project/01-v4l2/src
SRCS = $(wildcard $(SOURCE_DIR)/*.c)
OBJS = $(SRCS:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o)

# 目标文件名
TARGET = v4l2_app

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

# 链接库 - 修复：添加 -ldrm
LIBS = -lv4l2 -lpthread -ldrm

# 默认目标
all: check-env check-drm $(BUILD_DIR) $(TARGET)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 修复：链接顺序很重要！OBJS要在LIBS前面
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

# 新增：检查libdrm库
check-drm:
	@echo "检查libdrm库..."
	@if [ -f "$(SYSROOT_PATH)/usr/include/libdrm/drm.h" ]; then \
		echo "✓ 找到drm.h头文件"; \
	else \
		echo "⚠ 警告: 未找到drm.h，尝试查找..."; \
		find $(SYSROOT_PATH) -name "drm.h" 2>/dev/null | head -1 | while read file; do \
			echo "找到drm.h在: $$file"; \
			dir=$$(dirname $$file); \
			echo "添加额外包含路径: -I$$dir"; \
		done; \
	fi
	@echo "检查libdrm库文件..."
	@if [ -f "$(SYSROOT_PATH)/usr/lib/libdrm.so" ]; then \
		echo "✓ 找到libdrm.so"; \
	else \
		echo "⚠ 警告: 未找到libdrm.so，尝试查找..."; \
		find $(SYSROOT_PATH) -name "libdrm.so*" 2>/dev/null | head -3 | while read file; do \
			echo "找到: $$file"; \
		done; \
	fi

# 显示详细路径信息
info:
	@echo "编译器路径: $(CC)"
	@echo "源文件目录: $(SRC_DIR)"
	@echo "构建目录: $(BUILD_DIR)"
	@echo "SDK路径: $(SDK_PATH)"
	@echo "Sysroot路径: $(SYSROOT_PATH)"
	@echo "头文件路径: $(INCLUDE_PATH)"
	@echo "编译标志: $(CFLAGS)"
	@echo "链接标志: $(LDFLAGS)"
	@echo "链接库: $(LIBS)"

# 测试编译（只编译不链接）
test-compile: $(BUILD_DIR)
	@echo "测试编译atomic_drm.c..."
	$(CC) $(CFLAGS) -c $(SRC_DIR)/atomic_drm.c -o $(BUILD_DIR)/atomic_drm_test.o
	@echo "检查目标文件符号..."
	$(CROSS_COMPILE)nm $(BUILD_DIR)/atomic_drm_test.o | grep -E "drmOpen|drmModeGetResources" || echo "未定义符号正常（将在链接时解析）"

# 测试链接
test-link: $(BUILD_DIR)/atomic_drm.o
	@echo "测试链接..."
	$(CC) $(LDFLAGS) -o $(BUILD_DIR)/test_link $(BUILD_DIR)/atomic_drm.o -ldrm
	@if [ $$? -eq 0 ]; then \
		echo "✓ 链接测试通过"; \
	else \
		echo "✗ 链接测试失败"; \
	fi

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# 显示帮助信息
help:
	@echo "可用目标:"
	@echo "  all           - 编译项目（默认）"
	@echo "  check-env     - 检查编译环境"
	@echo "  check-drm     - 检查libdrm库"
	@echo "  info          - 显示详细路径信息"
	@echo "  test-compile  - 测试编译过程"
	@echo "  test-link     - 测试链接过程"
	@echo "  clean         - 清理生成的文件"
	@echo "  help          - 显示此帮助信息"

.PHONY: all clean help check-env check-drm info test-compile test-link