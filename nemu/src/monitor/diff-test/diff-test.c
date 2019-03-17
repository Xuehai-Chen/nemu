#include <dlfcn.h>

#include "nemu.h"
#include "monitor/monitor.h"
#include "diff-test.h"

static void (*ref_difftest_memcpy_from_dut)(paddr_t dest, void *src, size_t n);
static void (*ref_difftest_getregs)(void *c);
static void (*ref_difftest_setregs)(const void *c);
static void (*ref_difftest_exec)(uint64_t n);

static bool is_skip_ref;
static bool is_skip_dut;
int is_detach_mode = 0;

void difftest_skip_ref() { is_skip_ref = true; }
void difftest_skip_dut() { is_skip_dut = true; }

void init_difftest(char *ref_so_file, long img_size) {
#ifndef DIFF_TEST
	return;
#endif

	assert(ref_so_file != NULL);

	void *handle;
	handle = dlopen(ref_so_file, RTLD_LAZY | RTLD_DEEPBIND);
	//Log("diff_so_file:%s", ref_so_file);
	assert(handle);

	ref_difftest_memcpy_from_dut = dlsym(handle, "difftest_memcpy_from_dut");
	assert(ref_difftest_memcpy_from_dut);

	ref_difftest_getregs = dlsym(handle, "difftest_getregs");
	assert(ref_difftest_getregs);

	ref_difftest_setregs = dlsym(handle, "difftest_setregs");
	assert(ref_difftest_setregs);

	ref_difftest_exec = dlsym(handle, "difftest_exec");
	assert(ref_difftest_exec);

	void (*ref_difftest_init)(void) = dlsym(handle, "difftest_init");
	assert(ref_difftest_init);

	Log("Differential testing: \33[1;32m%s\33[0m", "ON");
	Log("The result of every instruction will be compared with %s. "
			"This will help you a lot for debugging, but also significantly reduce the performance. "
			"If it is not necessary, you can turn it off in include/common.h.", ref_so_file);

	ref_difftest_init();
	ref_difftest_memcpy_from_dut(ENTRY_START, guest_to_host(ENTRY_START), img_size);
	ref_difftest_setregs(&cpu);
}

void difftest_step(uint32_t eip) {
	CPU_state ref_r;

	if (is_detach_mode) return;

	if (is_skip_dut) {
		is_skip_dut = false;
		return;
	}

	if (is_skip_ref) {
		// to skip the checking of an instruction, just copy the reg state to reference design
		ref_difftest_setregs(&cpu);
		is_skip_ref = false;
		return;
	}

	ref_difftest_exec(1);
	ref_difftest_getregs(&ref_r);

	// TODO: Check the registers state with the reference design.
	// Set `nemu_state` to `NEMU_ABORT` if they are not the same.
	
	bool diff = false;
	for(int i = 0; i < 8; i++){
		if(ref_r.gpr[i]._32 != cpu.gpr[i]._32){
			diff = true;
			break;
		}
	}
	if(cpu.eip != ref_r.eip) diff = true;
	/**
	 if(cpu.eflags.CF != (r.eflags & 0x1)) diff = true;
	 if(cpu.eflags.ZF != ((r.eflags >> 6) & 0x1)) diff = true;
	 if(cpu.eflags.SF != ((r.eflags >> 7) & 0x1)) diff = true;
	 if(cpu.eflags.IF != ((r.eflags >> 9) & 0x1)) diff = true;
	 if(cpu.eflags.OF != ((r.eflags >> 11) & 0x1)) diff = true;
	 */
	if (diff) {
		for(int i = 0; i < 8; i++){
			Log("reg %s:0x%-10x\t0x%-10x", reg_name(i, 4), cpu.gpr[i]._32, ref_r.gpr[i]._32);
		}
		Log("reg eip:0x%-10x\t0x%-10x", cpu.eip, ref_r.eip);
		Log("eflags: 0x%-10x\t0x%-10x", cpu.eflags.val, ref_r.eflags.val);
		nemu_state = NEMU_ABORT;
	}
}
