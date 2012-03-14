SECTIONS {

/*
 * For use with ld -r, will put the .data..percpu* sections into .data
 * with start/end symbols created for a second linking run with the normal
 * linker script
 */

jiffies = jiffies_64;

.data : {
	. = ALIGN(16);
	__per_cpu_start_info = .;
    *(.data..percpuinfo)
	__per_cpu_end_info = .;
    *(.data..percpuarrays)
	__per_cpu_start = .;
	*(.data..percpu)
	__per_cpu_end = .;
}


}
