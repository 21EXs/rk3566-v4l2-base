# 使用SDK自带的交叉编译工具链
SDK_PATH = /home/xs/develop/TaiShanPie/Linux-sdk/sdk/Release
TOOLCHAIN_PATH = $(SDK_PATH)/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin
CROSS_COMPILE = $(TOOLCHAIN_PATH)/aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

# 项目根目录
PROJECT_ROOT = /home/xs/桌面/Project/ssh/rk3566-v4l2-base/03-drm

# 目录路径
SRC_DIR = $(PROJECT_ROOT)/src
BUILD_DIR = $(PROJECT_ROOT)/build
INCLUDE_DIR = $(PROJECT_ROOT)/inc
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# 目标文件名
TARGET = $(PROJECT_ROOT)/drm_app

# 使用正确的头文件路径
SYSROOT_PATH = $(SDK_PATH)/buildroot/output/rockchip_rk3566/host/aarch64-buildroot-linux-gnu/sysroot
INCLUDE_PATH = $(SYSROOT_PATH)/usr/include

# 编译标志
CFLAGS = -Wall -O2
CFLAGS += -I$(INCLUDE_PATH) -I$(INCLUDE_DIR)
# 添加DRM相关头文件路径
CFLAGS += -I$(SYSROOT_PATH)/usr/include/libdrm
# 添加sysroot确保使用正确的库
CFLAGS += --sysroot=$(SYSROOT_PATH)

# 链接标志
LDFLAGS = -L$(SYSROOT_PATH)/usr/lib -L$(SYSROOT_PATH)/lib
LDFLAGS += --sysroot=$(SYSROOT_PATH)

# 链接库 - 03-drm项目需要的库
LIBS = -ldrm -lpthread -lm

# 默认目标
all: check-env $(BUILD_DIR) $(TARGET)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 链接目标
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# 编译源文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
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
	@echo "检查libdrm库..."
	@if [ -f "$(SYSROOT_PATH)/usr/include/libdrm/drm.h" ]; then \
		echo "✓ 找到libdrm头文件"; \
	else \
		echo "⚠ 警告: 未找到libdrm头文件，可能无法编译DRM程序"; \
	fi

# 检查项目结构
check-project:
	@echo "检查项目结构..."
	@echo "项目路径: $(PROJECT_ROOT)"
	@if [ -d "$(SRC_DIR)" ]; then \
		echo "✓ 源文件目录存在: $(SRC_DIR)"; \
		echo "源文件列表:"; \
		ls -la $(SRC_DIR)/*.c 2>/dev/null || echo "  (无源文件)"; \
	else \
		echo "✗ 错误: 源文件目录不存在: $(SRC_DIR)"; \
		exit 1; \
	fi
	@if [ -d "$(INCLUDE_DIR)" ]; then \
		echo "✓ 头文件目录存在: $(INCLUDE_DIR)"; \
		echo "头文件列表:"; \
		ls -la $(INCLUDE_DIR)/*.h 2>/dev/null || echo "  (无头文件)"; \
	else \
		echo "⚠ 警告: 头文件目录不存在: $(INCLUDE_DIR)"; \
	fi
	@if [ -d "$(BUILD_DIR)" ]; then \
		echo "✓ 构建目录存在: $(BUILD_DIR)"; \
	else \
		echo "构建目录不存在，编译时会自动创建"; \
	fi

# 显示详细路径信息
info:
	@echo "=================== 项目信息 ==================="
	@echo "项目名称: 03-drm (DRM显示程序)"
	@echo "项目根目录: $(PROJECT_ROOT)"
	@echo "编译器: $(CC)"
	@echo "源文件目录: $(SRC_DIR)"
	@echo "构建目录: $(BUILD_DIR)"
	@echo "头文件目录: $(INCLUDE_DIR)"
	@echo "SDK路径: $(SDK_PATH)"
	@echo "Sysroot路径: $(SYSROOT_PATH)"
	@echo "目标文件: $(TARGET)"
	@echo "源文件数量: $(words $(SRCS))"
	@echo "源文件列表:"
	@for src in $(SRCS); do \
		echo "  $$src"; \
	done
	@echo "编译标志: $(CFLAGS)"
	@echo "链接标志: $(LDFLAGS)"
	@echo "链接库: $(LIBS)"
	@echo "================================================"

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "已清理构建文件"

# 完全清理
distclean: clean
	@echo "已完全清理项目"

# 运行程序（在主机上不可运行，仅为提示）
run:
	@echo "注意：这是一个ARM架构的交叉编译程序"
	@echo "请将 $(TARGET) 复制到RK3566开发板运行"
	@echo ""
	@echo "在开发板上的运行命令示例："
	@echo "  ./drm_app"
	@echo "  LD_LIBRARY_PATH=/usr/lib ./drm_app"

# 部署到开发板（需要配置SSH）
DEPLOY_HOST = 192.168.1.100
DEPLOY_USER = root
DEPLOY_PATH = /root

deploy:
	@echo "部署程序到开发板..."
	@if [ -f "$(TARGET)" ]; then \
		echo "正在复制 $(TARGET) 到 $(DEPLOY_USER)@$(DEPLOY_HOST):$(DEPLOY_PATH)/"; \
		scp $(TARGET) $(DEPLOY_USER)@$(DEPLOY_HOST):$(DEPLOY_PATH)/; \
		if [ $$? -eq 0 ]; then \
			echo "✓ 部署成功"; \
			echo "在开发板上运行: ssh $(DEPLOY_USER)@$(DEPLOY_HOST) 'cd $(DEPLOY_PATH) && ./drm_app'"; \
		else \
			echo "✗ 部署失败"; \
		fi \
	else \
		echo "错误: 目标文件 $(TARGET) 不存在，请先运行 'make' 编译"; \
	fi

# 显示帮助信息
help:
	@echo "=================== 03-drm项目 Makefile 帮助 ==================="
	@echo "可用目标:"
	@echo "  all           - 编译项目（默认）"
	@echo "  check-env     - 检查编译环境"
	@echo "  check-project - 检查项目结构"
	@echo "  info          - 显示详细项目信息"
	@echo "  clean         - 清理生成的文件"
	@echo "  distclean     - 完全清理"
	@echo "  run           - 显示运行提示"
	@echo "  deploy        - 部署程序到开发板（需配置DEPLOY_HOST等变量）"
	@echo "  help          - 显示此帮助信息"
	@echo ""
	@echo "使用示例:"
	@echo "  make           # 编译程序"
	@echo "  make clean     # 清理"
	@echo "  make deploy    # 部署到开发板"
	@echo ""
	@echo "部署配置: 编辑Makefile修改DEPLOY_HOST等变量"
	@echo "================================================================"

.PHONY: all clean distclean help check-env check-project info run deploy