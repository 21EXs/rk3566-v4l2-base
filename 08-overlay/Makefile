# 08-overlay: DRM Overlay Plane 最小实现示例
# 使用SDK自带的交叉编译工具链

SDK_PATH = /home/xs/develop/TaiShanPie/Linux-sdk/sdk/Release
TOOLCHAIN_PATH = $(SDK_PATH)/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin
CROSS_COMPILE = $(TOOLCHAIN_PATH)/aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

SYSROOT_PATH = $(SDK_PATH)/buildroot/output/rockchip_rk3566/host/aarch64-buildroot-linux-gnu/sysroot
INCLUDE_PATH = $(SYSROOT_PATH)/usr/include

CFLAGS = -Wall -O2
CFLAGS += -I$(INCLUDE_PATH)
CFLAGS += -I$(SYSROOT_PATH)/usr/include/libdrm
CFLAGS += --sysroot=$(SYSROOT_PATH)

LDFLAGS = -L$(SYSROOT_PATH)/usr/lib -L$(SYSROOT_PATH)/lib
LDFLAGS += --sysroot=$(SYSROOT_PATH)
LIBS = -ldrm -lpthread

TARGET = overlay_test

all: $(TARGET)

$(TARGET): overlay_test.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

run:
	@echo "请将 $(TARGET) 复制到 RK3566 开发板运行"
	@echo "  ./$(TARGET)"

.PHONY: all clean run
