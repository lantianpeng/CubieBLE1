;*************************************************************************************************/
;*!
;*  \file   startup.s
;*
;*  \brief  CMSIS Cortex-M0 Core Device Startup File for Actions Semiconductor SOC.
;*
;*          $Date: 2016-10-10 14:42:39 -0400 (Mon, 10 Oct 2016) $
;*          $Revision: 9434 $
;*
;*  Copyright (c) 2016-2017 ARM Ltd., all rights reserved.
;*  ARM confidential and proprietary.
;*
;*  IMPORTANT.  Your use of this file is governed by a Software License Agreement
;*  ("Agreement") that must be accepted in order to download or otherwise receive a
;*  copy of this file.  You may not use or copy this file for any purpose other than
;*  as described in the Agreement.  If you do not agree to all of the terms of the
;*  Agreement do not use this file and delete all copies in your possession or control;
;*  if you do not have a copy of the Agreement, you must contact ARM Ltd. prior
;*  to any use, copying or further distribution of this software.
;*/
;*************************************************************************************************/

CONFIG_ISR_STACK_SIZE      EQU     640

    AREA    STACK, NOINIT, READWRITE, ALIGN=3
    EXPORT    _interrupt_stack
_interrupt_stack       SPACE   CONFIG_ISR_STACK_SIZE
__initial_sp


Heap_Size       EQU     0x00000000

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

;**************************************************************************************************
; Vector Table
;*************************************************************************************************/
    AREA    RESET, DATA, READONLY
    EXPORT  systemVectors
    EXPORT  __nmi
    EXPORT  __hard_fault
    EXPORT  __svc
    EXPORT  __pendsv
    EXPORT  _timer_int_handler
    EXPORT  _isr_wrapper
systemVectors\
    DCD     __initial_sp             ;  0  Top of Stack
    DCD     Reset_Handler            ;  1  Reset Handler
    DCD     __nmi                    ;  2  NMI Handler
    DCD     __hard_fault             ;  3  Hard Fault Handler
    DCD     __hard_fault             ;  4  Reserved
    DCD     __hard_fault             ;  5  Reserved
    DCD     __hard_fault             ;  6  Reserved
    DCD     __hard_fault             ;  7  Reserved
    DCD     __hard_fault             ;  8  Reserved
    DCD     __hard_fault             ;  9  Reserved
    DCD     __hard_fault             ; 10  Reserved
    DCD     __svc                    ; 11  SVCall Handler (not implemented)
    DCD     __hard_fault             ; 12  Debug Monitor Handler
    DCD     __hard_fault             ; 13  Reserved
    DCD     __pendsv                 ; 14  PendSV Handler (not implemented)
    DCD     _timer_int_handler       ; 15  SysTick Handler (not implemented)

    ; External Interrupts
    DCD     _isr_wrapper  ;TIM0_IRQHandler
    DCD     _isr_wrapper  ;TIM1_IRQHandler
    DCD     _isr_wrapper  ;RTC_IRQHandler
    DCD     _isr_wrapper  ;WATCHDOG_IRQHandler
    DCD     _isr_wrapper  ;BLE0_IRQHandler
    DCD     _isr_wrapper  ;BLE1_IRQHandler
    DCD     _isr_wrapper  ;BLE2_IRQHandler
    DCD     _isr_wrapper  ;BLE3_IRQHandler
    DCD     _isr_wrapper  ;BLE4_IRQHandler
    DCD     _isr_wrapper  ;BLE5_IRQHandler
    DCD     _isr_wrapper  ;BLE6_IRQHandler
    DCD     _isr_wrapper  ;BLE7_IRQHandler
    DCD     _isr_wrapper  ;BLE8_IRQHandler
    DCD     _isr_wrapper  ;DMA_IRQHandler
    DCD     _isr_wrapper  ;EXTI_IRQHandler
    DCD     _isr_wrapper  ;UART0_IRQHandler
    DCD     _isr_wrapper  ;UART1_IRQHandler
    DCD     _isr_wrapper  ;UART2_IRQHandler
    DCD     _isr_wrapper  ;SPI0_IRQHandler
    DCD     _isr_wrapper  ;SPI1_IRQHandler
    DCD     _isr_wrapper  ;SPI2_IRQHandler
    DCD     _isr_wrapper  ;SARADC_IRQHandler
    DCD     _isr_wrapper  ;UART_Wakeup_IRQHandler
    DCD     _isr_wrapper  ;IRC_IRQHandler
    DCD     _isr_wrapper  ;I2C0_IRQHandler
    DCD     _isr_wrapper  ;I2C1_IRQHandler
    DCD     _isr_wrapper  ;Audio_IRQHandler
    DCD     _isr_wrapper  ;KEY_IRQHandler
    DCD     _isr_wrapper  ;SARADC_Wakeup_IRQHandler
    DCD     _isr_wrapper  ;TIM2_IRQHandler
    DCD     _isr_wrapper  ;TIM3_IRQHandler
    DCD     _isr_wrapper  ;KEY_Wakeup_IRQHandler

    IMPORT  |Image$$ER_IROM$$Base|
    IMPORT  |Image$$ER_IROM$$Limit|	
    IMPORT  |Image$$RW_IRAM3$$Base|		
			
;APP INFO
MAGIC         DCB     'A', 'T', 'B', 0
ROM_START     DCD     |Image$$ER_IROM$$Base|
ROM_END       
  IF :DEF:CONFIG_SPI0_XIP
              DCD     |Image$$ER_IROM$$Limit| + |Image$$RW_IRAM3$$Base| - 0x20001000
  ELSE
              DCD     |Image$$RW_IRAM3$$Base| 
  ENDIF
ENTRY         DCD     Reset_Handler
NOR_ADDR
  IF :DEF:ASM_APP_NOR_ADDR
              DCD     ASM_APP_NOR_ADDR
  ELSE
              DCD     0x0
  ENDIF
EXT           DCD     0
CHEKSUM_DATA  DCD     0
CHEKSUM_HDR   DCD     0
;SIG          SPACE   64


;*************************************************************************************************/
;*!
;*  \brief      Reset handler.
;*
;*  \param      None.
;*
;*  \return     None.
;*/
;*************************************************************************************************/
    AREA    |.asm_patch|, CODE, READONLY
    ALIGN
p_llcc_rx_cmd_valid PROC
    EXPORT  p_llcc_rx_cmd_valid
    IMPORT  compute_checksum [WEAK]
    movs	r0, r1
    movs	r1, r3

    ldr		r2, =compute_checksum
    blx		r2
    movs	r2, r0

    push	{r0,r1}
    ldr		r0,[pc,#4]
    str		r0,[sp,#4]
    pop		{r0,pc}

    ALIGN
p_llcc_rx_cmd_valid_data DCD 0xecd9

    ENDP

    AREA    |.asm_patch|, CODE, READONLY
    ALIGN
p_llcc_rx_dmal_done PROC
    EXPORT  p_llcc_rx_dmal_done
    IMPORT  compute_checksum [WEAK]

    movs	r0, r5
    movs	r1, r7
    ldr		r2, =compute_checksum
    blx		r2
    movs	r2, r0

    push	{r0,r1}
    ldr		r0,[pc,#4]
    str		r0,[sp,#4]
    pop		{r0,pc}

    ALIGN
p_llcc_rx_dmal_done_data DCD 0xedef
    ENDP  

    AREA    |.text|, CODE, READONLY
    ALIGN
Reset_Handler\
    PROC
    EXPORT  Reset_Handler          [WEAK]
    IMPORT  SystemInit
    IMPORT  __main
    IMPORT __aeabi_memset

    ldr r0, =0    ; for warning: A1581W: Added 2 bytes of padding at address 0x2e
    ldr r0, =_interrupt_stack
    ldr r1, =CONFIG_ISR_STACK_SIZE
    ldr r2, =0xaa
    bl __aeabi_memset

;* Set PSP and use it to boot without using MSP, so that it
;* gets set to _interrupt_stack during nanoInit().
   
    ldr r0, =_interrupt_stack
    ldr r1, =CONFIG_ISR_STACK_SIZE
    adds r0, r0, r1
    msr PSP, r0
    movs.n r0, #2 ;switch to using PSP (bit1 of CONTROL reg)
    msr CONTROL, r0
  
    LDR     R0, =SystemInit
    BLX     R0
    LDR     R0, =__main
    BX      R0
    ENDP

_arch_irq_lock_s PROC
    EXPORT  _arch_irq_lock_s
    MRS r0, PRIMASK
    cpsid i
    BX LR
    ENDP

_arch_irq_unlock_s PROC
    EXPORT  _arch_irq_unlock_s
    cmp r0, #0 
    bne |_arch_irq_unlock_s_out|
    cpsie i
|_arch_irq_unlock_s_out|
    BX LR
    ENDP  

;***************************************************************************************************
;* DUMMY EXCEPTION HANDLERS
;***************************************************************************************************

__nmi\
    PROC
    EXPORT  __nmi            [WEAK]
    B       .
    ENDP

__hard_fault\
    PROC
    EXPORT  __hard_fault      [WEAK]
    B       .
    ENDP

__svc\
    PROC
    EXPORT  __svc            [WEAK]
    B       .
    ENDP

__pendsv\
    PROC
    EXPORT  __pendsv         [WEAK]
    B       .
    ENDP

_timer_int_handler\
    PROC
    EXPORT  _timer_int_handler        [WEAK]
    B       .
    ENDP

Default_Handler\
    PROC
    EXPORT _isr_wrapper     [WEAK]
_isr_wrapper

    B       .
    ENDP


    IF :DEF:__MICROLIB

        EXPORT  __initial_sp
        EXPORT  __heap_base
        EXPORT  __heap_limit
    ELSE

    IMPORT  __use_two_region_memory
    EXPORT  __user_initial_stackheap

__user_initial_stackheap
    LDR     R0, = Heap_Mem
    LDR     R1, =(_interrupt_stack + CONFIG_ISR_STACK_SIZE)
    LDR     R2, =(Heap_Mem +  Heap_Size)
    LDR     R3, = _interrupt_stack
    BX      LR

    ALIGN

    ENDIF

    END
