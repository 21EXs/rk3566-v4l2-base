# 使用SDK自带的交叉编译工具链
SDK_PATH = /home/xs/develop/TaiShanPie/Linux-sdk/sdk/Release
TOOLCHAIN_PATH = $(SDK_PATH)/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin
CROSS_COMPILE = $(TOOLCHAIN_PATH)/aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

# 目录路径
SRC_DIR = /home/xs/桌面/Project/01-v4l2/src
BUILD_DIR = $(SRC_DIR)/build

# 目标文件名
TARGET = v4l2_app

# 使用正确的头文件路径（从find结果中选择）
# 这个路径包含了完整的标准C库头文件
SYSROOT_PATH = $(SDK_PATH)/buildroot/output/rockchip_rk3566/host/aarch64-buildroot-linux-gnu/sysroot
INCLUDE_PATH = $(SYSROOT_PATH)/usr/include

# 编译标志
CFLAGS = -Wall -O2
CFLAGS += -I$(INCLUDE_PATH)
# 添加sysroot确保使用正确的库
CFLAGS += --sysroot=$(SYSROOT_PATH)

# 链接标志
LDFLAGS = -L$(SYSROOT_PATH)/usr/lib -L$(SYSROOT_PATH)/lib
LDFLAGS += --sysroot=$(SYSROOT_PATH)

# 链接库
LIBS = -lv4l2 -lpthread

# 默认目标
all: check-env $(BUILD_DIR) $(TARGET)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译和链接
$(TARGET): $(BUILD_DIR)/main.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# 编译main.c
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c
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
	@echo "检查v4l2头文件..."
	@if [ -f "$(INCLUDE_PATH)/linux/videodev2.h" ]; then \
		echo "✓ 找到videodev2.h"; \
	else \
		echo "⚠ 警告: 未找到videodev2.h，尝试其他路径"; \
		find $(SYSROOT_PATH) -name "videodev2.h" 2>/dev/null | head -1 | while read file; do \
			echo "找到videodev2.h在: $$file"; \
			dirname "$$file" | head -1 | while read dir; do \
				echo "添加额外包含路径: $$dir"; \
			done; \
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

# 测试编译（只编译不链接）
test-compile: $(BUILD_DIR)
	@echo "测试编译..."
	$(CC) $(CFLAGS) -E $(SRC_DIR)/main.c > $(BUILD_DIR)/main.i 2>&1 && echo "✓ 预处理成功" || echo "✗ 预处理失败"
	$(CC) $(CFLAGS) -S $(SRC_DIR)/main.c -o $(BUILD_DIR)/main.s 2>&1 && echo "✓ 编译成功" || echo "✗ 编译失败"

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# 显示帮助信息
help:
	@echo "可用目标:"
	@echo "  all           - 编译项目（默认）"
	@echo "  check-env     - 检查编译环境"
	@echo "  info          - 显示详细路径信息"
	@echo "  test-compile  - 测试编译过程"
	@echo "  clean         - 清理生成的文件"
	@echo "  help          - 显示此帮助信息"

.PHONY: all clean help check-env info test-compile
