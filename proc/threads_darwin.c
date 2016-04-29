#include "threads_darwin.h"

kern_return_t
get_registers(mach_port_name_t task, x86_thread_state64_t *state) {
	kern_return_t kret;
	mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;
	// TODO(dp) - possible memory leak - vm_deallocate state
	return thread_get_state(task, x86_THREAD_STATE64, (thread_state_t)state, &stateCount);
}

kern_return_t
get_identity(mach_port_name_t task, thread_identifier_info_data_t *idinfo) {
	mach_msg_type_number_t idinfoCount = THREAD_IDENTIFIER_INFO_COUNT;
	return thread_info(task, THREAD_IDENTIFIER_INFO, (thread_info_t)idinfo, &idinfoCount);
}

kern_return_t
set_registers(mach_port_name_t task, x86_thread_state64_t *state) {
	mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;
	return thread_set_state(task, x86_THREAD_STATE64, (thread_state_t)state, stateCount);
}

kern_return_t
set_pc(thread_act_t task, uint64_t pc) {
	kern_return_t kret;
	x86_thread_state64_t state;
	mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;

	kret = thread_get_state(task, x86_THREAD_STATE64, (thread_state_t)&state, &stateCount);
	if (kret != KERN_SUCCESS) return kret;
	state.__rip = pc;

	return thread_set_state(task, x86_THREAD_STATE64, (thread_state_t)&state, stateCount);
}

kern_return_t
set_single_step_flag(thread_act_t thread) {
	kern_return_t kret;
	x86_thread_state64_t regs;
	mach_msg_type_number_t count = x86_THREAD_STATE64_COUNT;

	kret = thread_get_state(thread, x86_THREAD_STATE64, (thread_state_t)&regs, &count);
	if (kret != KERN_SUCCESS) return kret;

	// Set trap bit in rflags
	regs.__rflags |= 0x100UL;

	return thread_set_state(thread, x86_THREAD_STATE64, (thread_state_t)&regs, count);
}

kern_return_t
resume_thread(thread_act_t thread) {
	kern_return_t kret;
	struct thread_basic_info info;
	unsigned int info_count = THREAD_BASIC_INFO_COUNT;

	kret = thread_info((thread_t)thread, THREAD_BASIC_INFO, (thread_info_t)&info, &info_count);
	if (kret != KERN_SUCCESS) return kret;

	for (int i = 0; i < info.suspend_count; i++) {
		kret = thread_resume(thread);
		if (kret != KERN_SUCCESS) return kret;
	}
	return KERN_SUCCESS;
}

kern_return_t
clear_trap_flag(thread_act_t thread) {
	kern_return_t kret;
	x86_thread_state64_t regs;
	mach_msg_type_number_t count = x86_THREAD_STATE64_COUNT;

	kret = thread_get_state(thread, x86_THREAD_STATE64, (thread_state_t)&regs, &count);
	if (kret != KERN_SUCCESS) return kret;

	// Clear trap bit in rflags
	regs.__rflags ^= 0x100UL;

	return thread_set_state(thread, x86_THREAD_STATE64, (thread_state_t)&regs, count);
}

int
thread_blocked(thread_act_t thread) {
	kern_return_t kret;
	struct thread_basic_info info;
	unsigned int info_count = THREAD_BASIC_INFO_COUNT;

	kret = thread_info((thread_t)thread, THREAD_BASIC_INFO, (thread_info_t)&info, &info_count);
	if (kret != KERN_SUCCESS) return -1;

	return info.suspend_count;
}

int
num_running_threads(task_t task) {
	kern_return_t kret;
	thread_act_array_t list;
	mach_msg_type_number_t count;
	int i, n = 0;

	kret = task_threads(task, &list, &count);
	if (kret != KERN_SUCCESS) {
		return -kret;
	}

	for (i = 0; i < count; ++i) {
		thread_act_t thread = list[i];
		struct thread_basic_info info;
		unsigned int info_count = THREAD_BASIC_INFO_COUNT;

		kret = thread_info((thread_t)thread, THREAD_BASIC_INFO, (thread_info_t)&info, &info_count);

		if (kret == KERN_SUCCESS) {
			if (info.suspend_count == 0) {
				++n;
			} else {
			}
		}
	}

	kret = vm_deallocate(mach_task_self(), (vm_address_t) list, count * sizeof(list[0]));
	if (kret != KERN_SUCCESS) return -kret;

	return n;
}
