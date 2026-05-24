
PWD 			= $(shell pwd)
BUILD_DIR		= build
SRC_DIR 		= $(PWD)/src
CROSS_COMPILE 	?= 
ARCH			?= RISCV 

CFLAGS			= -g -O0 -MMD -I$(INCLUDE) -static -Wno-unused-result -D$(ARCH) -DUSER

ifeq ($(ARCH), RISCV)
	CROSS_COMPILE = riscv64-unknown-linux-gnu-
	CFLAGS += -march=rv64gc_zicbom_zicboz_zicbop -mabi=lp64d
else
	CROSS_COMPILE = 
endif


INCLUDE			= $(PWD)/include
CC				= $(CROSS_COMPILE)gcc
# CFLAGS			= -g -O2 -MMD -I$(INCLUDE) -static -Wno-unused-result -march=rv64gc_zicbom_zicboz_zicbop -mabi=lp64d -D$(ARCH)

LINKER_SCRIPT	?=

ifeq ($(ARCH), RISCV)
	LINKER_SCRIPT = $(PWD)/riscv.lds
	CFLAGS += -DRISCV
else
	LINKER_SCRIPT = $(PWD)/x86.lds
	CFLAGS += -DX86
endif



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

# Jemalloc
JEMALLOC_DIR	= $(PWD)/jemalloc
JEMALLOC_TARGET = $(BUILD_DIR)//lib/libjemalloc.a


# device tree
DTC 			= dtc
DTS_DIR 		= $(PWD)/dts
DTS_FILE 		= $(DTS_DIR)/output.dts
DTB_FILE 		= $(BUILD_DIR)/output.dtb

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJ_FILES) -Wl,--no-relax -T$(LINKER_SCRIPT)
	@echo + LD $(TARGET)

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

# Jemalloc
$(JEMALLOC_TARGET): $(BUILD_DIR)
ifeq ($(wildcard $(JEMALLOC_DIR)/*),)
	git submodule update --init $(JEMALLOC_DIR)
endif
	cd $(JEMALLOC_DIR) && ./autogen.sh && ./configure --disable-dss CFLAGS="-g -ggdb3"
	$(MAKE) -C $(JEMALLOC_DIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo + CC $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo + CC $@

$(DTB_FILE): $(DTS_FILE) | $(BUILD_DIR)
	@$(DTC) -I dts -O dtb -o $(DTB_FILE) $(DTS_FILE)
	@echo + DTC $(DTB_FILE)



.PHONY: clean all dts

clean:
	rm -rf $(BUILD_DIR)
# 	@$(MAKE) -C $(OPENSBI_DIR) clean
# 	@$(MAKE) -C $(JEMALLOC_DIR) clean



# Dependencies
DEPS = $(addprefix $(BUILD_DIR)/, \
						$(addsuffix .d, $(basename $(BASENAME))))
-include $(DEPS)