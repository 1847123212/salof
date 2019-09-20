#include <tos.h>

__API__ uint32_t tos_cpu_clz(uint32_t val)
{
#if defined(TOS_CFG_CPU_LEAD_ZEROS_ASM_PRESENT) && (TOS_CFG_CPU_LEAD_ZEROS_ASM_PRESENT == 0u)
    uint32_t nbr_lead_zeros = 0;

    if (!(val & 0XFFFF0000)) {
        val <<= 16;
        nbr_lead_zeros += 16;
    }

    if (!(val & 0XFF000000)) {
        val <<= 8;
        nbr_lead_zeros += 8;
    }

    if (!(val & 0XF0000000)) {
        val <<= 4;
        nbr_lead_zeros += 4;
    }

    if (!(val & 0XC0000000)) {
        val <<= 2;
        nbr_lead_zeros += 2;
    }

    if (!(val & 0X80000000)) {
        nbr_lead_zeros += 1;
    }

    if (!val) {
        nbr_lead_zeros += 1;
    }

    return (nbr_lead_zeros);
#else
    return port_clz(val);
#endif
}

__API__ void tos_cpu_int_disable(void)
{
    port_int_disable();
}

__API__ void tos_cpu_int_enable(void)
{
    port_int_enable();
}

__API__ cpu_cpsr_t tos_cpu_irq_save(void)
{
    return port_irq_save();
}

__API__ void tos_cpu_irq_restore(cpu_cpsr_t cpsr)
{
    port_irq_restore(cpsr);
}

__KERNEL__ void cpu_init(void)
{
    k_cpu_cycle_per_tick = TOS_CFG_CPU_CLOCK / k_cpu_tick_per_second;
    cpu_systick_init(k_cpu_cycle_per_tick);

#if (TOS_CFG_CPU_HRTIMER_EN > 0)
    tos_cpu_hrtimer_init();
#endif
}

__KERNEL__ void cpu_reset(void)
{
    port_cpu_reset();
}

__KERNEL__ void cpu_sched_start(void)
{
    port_sched_start();
}

__KERNEL__ void cpu_context_switch(void)
{
    port_context_switch();
}

__KERNEL__ void cpu_irq_context_switch(void)
{
    port_irq_context_switch();
}

__KERNEL__ void cpu_systick_init(k_cycle_t cycle_per_tick)
{
    port_systick_priority_set(TOS_CFG_CPU_SYSTICK_PRIO);
    port_systick_config(cycle_per_tick);
}

#if TOS_CFG_TICKLESS_EN > 0u

/**
 * @brief Set value to systick reload value register 
 *
 * @param cycles The value set to register
 *
 * @return None
 */
__STATIC_INLINE__ void cpu_systick_reload(k_cycle_t cycle_per_tick)
{
    port_systick_reload(cycle_per_tick);
}

/**
 * @brief Enable systick interrupt
 *
 * @return None
 */
__KERNEL__ void cpu_systick_resume(void)
{   
    port_systick_resume();
}

/**
 * @brief Disable systick interrupt
 *
 * @return None
 */
__KERNEL__ void cpu_systick_suspend(void)
{
    port_systick_suspend();
}

__KERNEL__ k_time_t cpu_systick_max_delay_millisecond(void)
{
    return port_systick_max_delay_millisecond();
}

__KERNEL__ void cpu_systick_expires_set(k_time_t millisecond)
{
    k_cycle_t cycles;

    cycles = (k_cycle_t)((uint64_t)millisecond * TOS_CFG_CPU_CLOCK / K_TIME_MILLISEC_PER_SEC); /* CLOCK means cycle per second */

    cpu_systick_reload(cycles - 12); /* interrupt delay */
}

__KERNEL__ void cpu_systick_pending_reset(void)
{
    port_systick_pending_reset();
}

__KERNEL__ void cpu_systick_reset(void)
{
    cpu_systick_reload(k_cpu_cycle_per_tick);
}

#endif /* TOS_CFG_TICKLESS_EN */

#if TOS_CFG_PWR_MGR_EN > 0u

__KERNEL__ void cpu_sleep_mode_enter(void)
{
    port_sleep_mode_enter();
}

__KERNEL__ void cpu_stop_mode_enter(void)
{
    port_stop_mode_enter();
}

__KERNEL__ void cpu_standby_mode_enter(void)
{
    port_standby_mode_enter();
}

#endif /* TOS_CFG_PWR_MGR_EN */

__KERNEL__ k_stack_t *cpu_task_stk_init(void *entry,
                                              void *arg,
                                              void *exit,
                                              k_stack_t *stk_base,
                                              size_t stk_size)
{
    cpu_data_t *sp;

    sp = (cpu_data_t *)&stk_base[stk_size];
    sp = (cpu_data_t *)((cpu_addr_t)(sp) & 0xFFFFFFF8);

    /* auto-saved on exception(pendSV) by hardware */
    *--sp = (cpu_data_t)0x01000000u;    /* xPSR     */
    *--sp = (cpu_data_t)entry;          /* entry    */
    *--sp = (cpu_data_t)exit;           /* R14 (LR) */
    *--sp = (cpu_data_t)0x12121212u;    /* R12      */
    *--sp = (cpu_data_t)0x03030303u;    /* R3       */
    *--sp = (cpu_data_t)0x02020202u;    /* R2       */
    *--sp = (cpu_data_t)0x01010101u;    /* R1       */
    *--sp = (cpu_data_t)arg;            /* R0: arg  */

    /* Remaining registers saved on process stack */
    /* EXC_RETURN = 0xFFFFFFFDL
       Initial state: Thread mode +  non-floating-point state + PSP
       31 - 28 : EXC_RETURN flag, 0xF
       27 -  5 : reserved, 0xFFFFFE
       4       : 1, basic stack frame; 0, extended stack frame
       3       : 1, return to Thread mode; 0, return to Handler mode
       2       : 1, return to PSP; 0, return to MSP
       1       : reserved, 0
       0       : reserved, 1
     */
#if defined (TOS_CFG_CPU_ARM_FPU_EN) && (TOS_CFG_CPU_ARM_FPU_EN == 1U)
    *--sp = (cpu_data_t)0xFFFFFFFDL;
#endif

    *--sp = (cpu_data_t)0x11111111u;    /* R11      */
    *--sp = (cpu_data_t)0x10101010u;    /* R10      */
    *--sp = (cpu_data_t)0x09090909u;    /* R9       */
    *--sp = (cpu_data_t)0x08080808u;    /* R8       */
    *--sp = (cpu_data_t)0x07070707u;    /* R7       */
    *--sp = (cpu_data_t)0x06060606u;    /* R6       */
    *--sp = (cpu_data_t)0x05050505u;    /* R5       */
    *--sp = (cpu_data_t)0x04040404u;    /* R4       */

    return (k_stack_t *)sp;
}

#if TOS_CFG_FAULT_BACKTRACE_EN > 0u

#if defined (TOS_CFG_CPU_ARM_FPU_EN) && (TOS_CFG_CPU_ARM_FPU_EN == 1U)
__KERNEL__ void cpu_flush_fpu(void)
{
    (void)__get_FPSCR();
}
#endif

__KERNEL__ void cpu_fault_diagnosis(void)
{
    port_fault_diagnosis();
}

#endif /* TOS_CFG_FAULT_BACKTRACE_EN */

