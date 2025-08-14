
PWD 			= $(shell pwd)
BUILD_DIR		= build
SRC_DIR 		= $(PWD)/src
CROSS_COMPILE 	?= riscv64-unknown-linux-gnu-

INCLUDE			= $(PWD)/include
CC				= $(CROSS_COMPILE)gcc
CFLAGS			= -g -O2 -MMD -I$(INCLUDE) -static -DRISCV

LINKER_SCRIPT	= $(PWD)/linker.lds

# SRC FILE, may used standard library
SRC_FILES_C		= $(wildcard $(SRC_DIR)/*.c)
SRC_FILES_ASM	= $(wildcard $(SRC_DIR)/*.S)


BASENAME		= $(notdir $(SRC_FILES_C) $(SRC_FILES_ASM))
OBJ_FILES		= $(addprefix $(BUILD_DIR)/, \
						$(addsuffix .o, $(basename $(BASENAME))))

# OBJ_FILES		= $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES_C)) \
# 				  $(patsubst $(SRC_DIR)/%.S,$(BUILD_DIR)/%.o,$(SRC_FILES_ASM))

TARGET			= $(BUILD_DIR)/user_loader

# Opensbi
OPENSBI_DIR     = $(PWD)/opensbi
OPENSBI_BIOS    = $(OPENSBI_DIR)/build/platform/generic/firmware/fw_jump.bin

# device tree
DTC 			= dtc
DTS_DIR 		= $(PWD)/dts
DTS_FILE 		= $(DTS_DIR)/output.dts
DTB_FILE 		= $(BUILD_DIR)/output.dtb

all: $(TARGET) $(OPENSBI_BIOS)

$(TARGET): $(OBJ_FILES)
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJ_FILES) -T$(LINKER_SCRIPT)
	@echo + LD $(TARGET)
	scp -P 12055 $(TARGET) localhost:

dts: $(BUILD_DIR)
	@$(DTC) -I dts -O dtb -o $(DTB_FILE) $(DTS_FILE)

# Opensbi
$(OPENSBI_BIOS): dts
ifeq ($(wildcard $(OPENSBI_DIR)/*),)
	git submodule update --init $(OPENSBI_DIR)
endif
	$(MAKE) \
	PLATFORM=generic \
	CROSS_COMPILE=$(CROSS_COMPILE) \
	FW_FDT_PATH=$(shell realpath $(DTB_FILE)) -j`nproc` -C $(OPENSBI_DIR)


$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo + CC $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo + CC $@




.PHONY: clean all dts

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(OPENSBI_DIR) clean



# Dependencies
DEPS = $(addprefix $(BUILD_DIR)/, \
						$(addsuffix .d, $(basename $(BASENAME))))
-include $(DEPS)