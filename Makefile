
PWD 			= $(shell pwd)
DIR_BUILD		= build
SRC_DIR 		= $(PWD)/src
CROSS_COMPILE 	?= riscv64-unknown-linux-gnu-

INCLUDE			= $(PWD)/include
CC				= $(CROSS_COMPILE)gcc
CFLAGS			= -g -O2 -MMD -I$(INCLUDE) -static

LINKER_SCRIPT	= $(PWD)/linker.lds

# SRC FILE, may used standard library
SRC_FILES_C		= $(wildcard $(SRC_DIR)/*.c)
SRC_FILES_ASM	= $(wildcard $(SRC_DIR)/*.S)


BASENAME		= $(notdir $(SRC_FILES_C) $(SRC_FILES_ASM))
OBJ_FILES		= $(addprefix $(DIR_BUILD)/, \
						$(addsuffix .o, $(basename $(BASENAME))))

# OBJ_FILES		= $(patsubst $(SRC_DIR)/%.c,$(DIR_BUILD)/%.o,$(SRC_FILES_C)) \
# 				  $(patsubst $(SRC_DIR)/%.S,$(DIR_BUILD)/%.o,$(SRC_FILES_ASM))

TARGET			= $(DIR_BUILD)/user_loader



all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJ_FILES) -T$(LINKER_SCRIPT)
	@echo + LD $(TARGET)
	scp -P 12055 $(TARGET) localhost:

$(DIR_BUILD):
	@mkdir -p $(DIR_BUILD)


$(DIR_BUILD)/%.o: $(SRC_DIR)/%.c | $(DIR_BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo + CC $@

$(DIR_BUILD)/%.o: $(SRC_DIR)/%.S | $(DIR_BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo + CC $@

dtc:
	@dtc -I dts -O dtb -o $(DIR_BUILD)/output.dtb $(PWD)/output.dts

.PHONY: clean

clean:
	rm -rf $(DIR_BUILD)



# Dependencies
DEPS = $(addprefix $(DIR_BUILD)/, \
						$(addsuffix .d, $(basename $(BASENAME))))
-include $(DEPS)