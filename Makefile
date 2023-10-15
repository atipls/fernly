include mkenv.mk
include magic.mk

BUILD = build
CROSS_COMPILE=arm-none-eabi-

CFLAGS = -march=armv5te -mfloat-abi=soft -Wall -I.
CFLAGS += -Os -Iinclude -marm -fno-stack-protector -nostartfiles
CFLAGS += -fno-builtin -fno-common -ffunction-sections -fdata-sections

AFLAGS = 

LDFLAGS = --nostdlib -T fernvale.ld -Map=$(BUILD)/stage2.map --gc-sections

LIBS = lib/libgcc-armv5.a

STAGE1_LDFLAGS = --nostdlib -T stage1.ld -Map=$(BUILD)/stage1.map --gc-sections

STAGE1_SRC_C = \
	drv/serial.c \
	util/utils.c \
	stage1.c \
	vectors.c

STAGE1_SRC_S = \
	start.S

STAGE1_SRC_C := $(addprefix source/, $(STAGE1_SRC_C))
STAGE1_SRC_S := $(addprefix source/, $(STAGE1_SRC_S))
STAGE1_OBJ = $(addprefix $(BUILD)/, $(STAGE1_SRC_S:.S=.o) $(STAGE1_SRC_C:.c=.o))

STAGE2_SRC_C = \
	cmd/cmd-hex.c \
	cmd/cmd-irq.c \
	cmd/cmd-peekpoke.c \
	cmd/cmd-reboot.c \
	cmd/cmd-sleep.c \
	cmd/cmd-spi.c \
	cmd/cmd-led.c \
	cmd/cmd-load.c \
	cmd/cmd-bl.c \
	cmd/cmd-lcd.c \
	cmd/cmd-keypad.c \
	drv/emi.c \
	drv/gpio.c \
	drv/irq.c \
	drv/lcd.c \
	drv/pwm.c \
	drv/serial.c \
	drv/spi.c \
	util/bionic.c \
	util/memio.c \
	util/utils.c \
	util/vsprintf.c \
	main.c \
	scriptic_core.c \
	scriptic_helper.c \
	vectors.c

STAGE2_SRC_S = \
	scripts/set-plls.S \
	scripts/enable-psram.S \
	scripts/spi.S \
	scripts/spi-blockmode.S \
	scripts/keypad.S \
	start.S

STAGE2_SRC_C := $(addprefix source/, $(STAGE2_SRC_C))
STAGE2_SRC_S := $(addprefix source/, $(STAGE2_SRC_S))
STAGE2_OBJ = $(addprefix $(BUILD)/, $(STAGE2_SRC_S:.S=.o) $(STAGE2_SRC_C:.c=.o))


$(BUILD)/stage1.bin: $(BUILD)/stage1.elf
	$(OBJCOPY) -S -O binary $(BUILD)/stage1.elf $@

$(BUILD)/stage1.elf: $(STAGE1_OBJ)
	$(LD) $(STAGE1_LDFLAGS) -o $@ $(STAGE1_OBJ) $(LIBS)

$(BUILD)/stage2.bin: $(BUILD)/stage2.elf
	$(OBJCOPY) -S -O binary $(BUILD)/stage2.elf $@

$(BUILD)/stage2.elf: $(STAGE2_OBJ)
	$(LD) $(LDFLAGS) --entry=reset_handler -o $@ $(STAGE2_OBJ) $(LIBS)

$(BUILD)/%.bin: $(BUILD)/%.o
	$(OBJCOPY) -S -O binary $< $@

$(BUILD)/usb-loader: usb-loader.c
	$(CC_NATIVE) usb-loader.c -o $@

all: $(BUILD)/stage1.bin \
	$(BUILD)/stage2.bin \
	$(BUILD)/usb-loader

clean:
	$(RM) -rf $(BUILD)
	$(MKDIR) $(BUILD)
	$(MKDIR) $(BUILD)/source
	$(MKDIR) $(BUILD)/source/cmd
	$(MKDIR) $(BUILD)/source/drv
	$(MKDIR) $(BUILD)/source/scripts
	$(MKDIR) $(BUILD)/source/util
	$(MKDIR) $(BUILD)/stage1
	$(MKDIR) $(BUILD)/stage1/source

shell: all
	$(BUILD)/usb-loader -s -w /dev/fernvale prebuilt/usb-loader.bin $(BUILD)/firmware.bin
