 ENTRY(Reset_Handler)

 MEMORY
 {
    FLASH(rx): ORIGIN = 0x08000000, LENGTH = 512K
    SRAM(rwx): ORIGIN = 0x20000000, LENGTH = 128K 
 }

_estack = ORIGIN(SRAM) + LENGTH(SRAM);
_Min_Stack_Size = 0x400;
_Min_Heap_Size = 0x200;

 SECTIONS
 {
    .isr_vector :
    {
        . = ALIGN(4);
        *(.isr_vector) 
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text.*)
        *(.init)
        *(.fini)
        *(.)
        . = ALIGN(4);
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT> FLASH
    _sidata = LOADADDR(.data); 

    .bss : 
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .; 
        __bss_end__ = _ebss;
        . = ALIGN(4);
        _end = .;
        __end__ = .;
        end = .;
    } > SRAM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > SRAM

 }