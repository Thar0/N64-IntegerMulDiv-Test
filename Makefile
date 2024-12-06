.PHONY: all clean run

TARGET := imuldiv_test.z64
ROM_NAME := "IMULDIV TEST"

all: $(TARGET)

BUILD_DIR = build
include $(N64_INST)/include/n64.mk

SRC_DIRS := $(shell find src -type d)
C_FILES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
S_FILES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.S))

OBJS := $(foreach f,$(C_FILES:.c=.o),build/$f) $(foreach f,$(S_FILES:.S=.o),build/$f)

$(TARGET): N64_ROM_TITLE = $(ROM_NAME)

$(BUILD_DIR)/$(TARGET:.z64=.elf): $(OBJS)

clean:
	rm -rf $(BUILD_DIR) *.z64

run: all
	python3 ../ed64_uploader.py --keep-alive $(TARGET)

-include $(wildcard $(BUILD_DIR)/*.d))
