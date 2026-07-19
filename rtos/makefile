CC = arm-none-eabi-gcc
MACH = cortex-m4
CFLAGS = -c -mcpu=$(MACH) -mthumb -mfloat-abi=soft -std=gnu11 -Wall -O0 -g
LDFLAGS = -mcpu=$(MACH) -mthumb -mfloat-abi=soft --specs=nano.specs -T linker_script.ld -Wl,-Map=final.map
LDFLAGS_SH = -mcpu=$(MACH) -mthumb -mfloat-abi=soft --specs=rdimon.specs -T linker_script.ld -Wl,-Map=final.map

all:main.o scheduler.o syscalls.o sysmem.o startup.o final.elf

semi:main.o scheduler.o sysmem.o startup.o final_sh.elf

main.o:main.c
	$(CC) $(CFLAGS) $^ -o $@

scheduler.o:scheduler.c
	$(CC) $(CFLAGS) $^ -o $@

sysmem.o:sysmem.c
	$(CC) $(CFLAGS) $^ -o $@

syscalls.o:syscalls.c
	$(CC) $(CFLAGS) $^ -o $@

startup.o:startup.c
	$(CC) $(CFLAGS) $^ -o $@

final.elf:main.o scheduler.o startup.o syscalls.o sysmem.o
	$(CC) $(LDFLAGS) $^ -o $@

final_sh.elf:main.o scheduler.o startup.o sysmem.o
	$(CC) $(LDFLAGS_SH) $^ -o $@
clean:
	rm -rf *.o *.elf

load:
	openocd -f board/st_nucleo_f4.cfg