
OUTPUT_ARCH(arm)
ENTRY(_stext)
SECTIONS
{
	. = 0x70009000;
	.text : {
		_stext = ABSOLUTE(.);
		KEEP(*(vectors))
		*(.text)
		*(.text.*)
		*(.fixup)
		*(.rodata)
		*(.rodata.*)
		_etext = ABSOLUTE(.);
	}

	_eronly = ABSOLUTE(.);
	. = ALIGN(4096);

	.data : {
		_sdata = ABSOLUTE(.);
		*(.data)
		*(.data.*)
		CONSTRUCTORS
		_edata = ABSOLUTE(.);
	}

	.bss : {
		_sbss = ABSOLUTE(.);
		*(.bss)
		*(COMMON)
		_ebss = ABSOLUTE(.);
	}

	.stab 0 : { *(.stab) }
	.stabstr 0 : { *(.stabstr) }
	.stab.excl 0 : { *(.stab.excl) }
	.stab.exclstr 0 : { *(.stab.exclstr) }
	.stab.index 0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment 0 : { *(.comment) }
	.debug_abbrev 0 : { *(.debug_abbrev) }
	.debug_info 0 : { *(.debug_info) }
	.debug_line 0 : { *(.debug_line) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	.debug_aranges 0 : { *(.debug_aranges) }
}
