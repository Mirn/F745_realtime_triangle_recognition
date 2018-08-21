OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
/* Internal Memory Map*/
MEMORY
{
	rom (rx)  : ORIGIN = 0x08008000, LENGTH = 0x00078000
	ram (rwx) : ORIGIN = 0x20010000, LENGTH = 0x00040000
	dtcm (rw) : ORIGIN = 0x20000000, LENGTH = 0x00010000
	itcm (rwx): ORIGIN = 0x00000000, LENGTH = 16K
	NOTUSED (rw) : ORIGIN = 0, LENGTH = 0
}

/*
//__ram_start__  = ORIGIN(ram);
//__ram_end__  =  ORIGIN(ram) + LENGTH(ram);
 
__stack = ORIGIN(ram) + LENGTH(ram);
_sstack = ORIGIN(ram);
*/
_estack = ORIGIN(dtcm) + LENGTH(dtcm); 	/* STM specific definition */

/*
//__Main_Stack_Size = LENGTH(ram);
//PROVIDE ( _Main_Stack_Size = __Main_Stack_Size ) ;
//__Main_Stack_Limit = __stack  - __Main_Stack_Size ;
//PROVIDE ( _Main_Stack_Limit = __Main_Stack_Limit ) ;
//_Minimum_Stack_Size = 1024;


PROVIDE ( _Heap_Begin = _end_noinit ) ;
PROVIDE ( _Heap_Limit = ORIGIN(ram) + LENGTH(ram)) ;
*/

/* Section Definitions */ 
SECTIONS 
{ 
    .text : 
    { 
	   	. = 0x00008000;
    	__rom_from = .;
        KEEP(*(.isr_vector .isr_vector.*)) 
    	*(.itcm_code_loader .itcm_code_loader*)
        *(.text .text.* .gnu.linkonce.t.*) 	      
        *(.glue_7t) *(.glue_7)
		. = ALIGN(4);
        *(.rodata .rodata* .gnu.linkonce.r.*)
    } > rom
    
    .ARM.extab : 
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > rom
    
    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __rom_to = .;
    } > rom
    __exidx_end = .;
    
    _etext = .;
    _sidata = .;
    
    .data : AT (_etext) 
    { 
        _sdata = .; 
        *(.data .data.*)        		
		. = ALIGN(4);
        _edata = . ;
        _sedata_size = _edata - _sdata;
    } > dtcm

    /*. = ALIGN(4);*/
    /*. = 0x08000000 + 0x2000 - SIZEOF(.data);*/
    /*. = 0x08000000 + (__rom_to - __rom_from);*/

    _itcm_copy = _etext + (_sedata_size - 0x20000000);
    .itcm_text : AT(_itcm_copy)    
     {
    	_itcm_image_from = .;
		. = ALIGN(4);
     	*(.itcm_data .itcm_data*)    	
		_itcm_image_to = .;
     } >itcm 

     
    /* .bss section which is used for uninitialized data */ 
    .bss (NOLOAD) : 
    { 
        _sbss = . ;
        
    	. = ALIGN(512); 
        *(.isr_vector_RAM*)
    	
        *(.bss .bss.*)
        *(COMMON) 
        . = ALIGN(4); 
        _ebss = . ; 
    } > dtcm
    
    .noinit (NOLOAD) :
    {
	    . = ALIGN(4);
        _noinit = .;        
        *(.noinit .noinit.*)
         . = ALIGN(4) ;
        _end_noinit = .;   
    } > dtcm
    
    .data_big (NOLOAD):
    {
    	. = ALIGN(4);
    	KEEP(*(.data_big*))
    } > ram
    
    .co_stack (NOLOAD):
    {
        . = ALIGN(8);
        *(.co_stack .co_stack.*)
    } > dtcm
   
    . = ALIGN(4); 
    _end = . ; 
} 