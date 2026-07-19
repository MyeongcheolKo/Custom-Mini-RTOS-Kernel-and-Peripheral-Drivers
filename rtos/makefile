CC = arm-none-eabi-gcc
MACH = cortex-m4
BUILD_DIR = build

# header file seach paths
INCLUDES += -Iapp -Ibsp -Iconfig -Ikernel -Iport/arm/cortex_m4

CFLAGS = -c -mcpu=$(MACH) -mthumb -mfloat-abi=soft -std=gnu11 -Wall -O0 -g $(INCLUDES)
LDFLAGS = -mcpu=$(MACH) -mthumb -mfloat-abi=soft --specs=nano.specs -T bsp/linker_script.ld -Wl,-Map=$(BUILD_DIR)/final.map
LDFLAGS_SH = -mcpu=$(MACH) -mthumb -mfloat-abi=soft --specs=rdimon.specs -T bsp/linker_script.ld -Wl,-Map=$(BUILD_DIR)/final.map

# every source, with its subdir
SRCS = app/main.c \
       kernel/scheduler.c \
       port/arm/cortex_m4/port.c \
       bsp/startup.c \
       bsp/syscalls.c \
       bsp/sysmem.c

# semihosting: drop syscalls.c, rdimon.specs supplies the I/O syscalls
OBJS_SH = $(filter-out $(BUILD_DIR)/bsp/syscalls.o,$(OBJS))

# app/main.c -> build/app/main.o, kernel/scheduler.c -> build/kernel/scheduler.o, ...
OBJS = $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

all: $(BUILD_DIR)/final.elf
semi: $(BUILD_DIR)/final_sh.elf

# build all .c files into .o files in the build dir, preserving the subdir structure
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@


$(BUILD_DIR)/final.elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/final_sh.elf: $(OBJS_SH)
	$(CC) $(LDFLAGS_SH) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)

load:
	openocd -f board/st_nucleo_f4.cfg