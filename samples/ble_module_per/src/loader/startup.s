;/*
; * Copyright (c) 2018 Actions Semiconductor Co., Ltd
; *
; * SPDX-License-Identifier: Apache-2.0
; */

CONFIG_ISR_STACK_SIZE      EQU     0

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
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
systemVectors\
    DCD     __initial_sp                                            ;  0  Top of Stack
    DCD     Reset_Handler                                           ;  1  Reset Handler
    DCD     __nmi                                                   ;  2  NMI Handler
    DCD     __hard_fault                                            ;  3  Hard Fault Handler
    DCD     __hard_fault                                            ;  4  Reserved
    DCD     __hard_fault                                            ;  5  Reserved
    DCD     __hard_fault                                            ;  6  Reserved
    DCD     __hard_fault                                            ;  7  Reserved
    DCD     __hard_fault                                            ;  8  Reserved
    DCD     __hard_fault                                            ;  9  Reserved
    DCD     __hard_fault                                            ; 10  Reserved
    DCD     __svc                                                   ; 11  SVCall Handler (not implemented)
    DCD     __hard_fault                                            ; 12  Debug Monitor Handler
    DCD     __hard_fault                                            ; 13  Reserved
    DCD     __pendsv                                                ; 14  PendSV Handler (not implemented)
    DCD     _timer_int_handler                                      ; 15  SysTick Handler (not implemented)

    ; External Interrupts
    DCD     _isr_wrapper	;TIM0_IRQHandler
    DCD     _isr_wrapper	;TIM1_IRQHandler
    DCD     _isr_wrapper	;RTC_IRQHandler
    DCD     _isr_wrapper	;WATCHDOG_IRQHandler
    DCD     _isr_wrapper	;BLE0_IRQHandler
    DCD     _isr_wrapper	;BLE1_IRQHandler
    DCD     _isr_wrapper	;BLE2_IRQHandler
    DCD     _isr_wrapper	;BLE3_IRQHandler
    DCD     _isr_wrapper	;BLE4_IRQHandler
    DCD     _isr_wrapper	;BLE5_IRQHandler
    DCD     _isr_wrapper	;BLE6_IRQHandler
    DCD     _isr_wrapper	;BLE7_IRQHandler
    DCD     _isr_wrapper	;BLE8_IRQHandler
    DCD     _isr_wrapper	;DMA_IRQHandler
    DCD     _isr_wrapper	;EXTI_IRQHandler
    DCD     _isr_wrapper	;UART0_IRQHandler
    DCD     _isr_wrapper	;UART1_IRQHandler
    DCD     _isr_wrapper	;UART2_IRQHandler
    DCD     _isr_wrapper	;SPI0_IRQHandler
    DCD     _isr_wrapper	;SPI1_IRQHandler
    DCD     _isr_wrapper	;SPI2_IRQHandler
    DCD     _isr_wrapper	;SARADC_IRQHandler
    DCD     _isr_wrapper	;UART_Wakeup_IRQHandler
    DCD     _isr_wrapper	;IRC_IRQHandler
    DCD     _isr_wrapper	;I2C0_IRQHandler
    DCD     _isr_wrapper	;I2C1_IRQHandler
    DCD     _isr_wrapper	;Audio_IRQHandler
    DCD     _isr_wrapper	;KEY_IRQHandler
    DCD     _isr_wrapper	;SARADC_Wakeup_IRQHandler
    DCD     _isr_wrapper	;TIM2_IRQHandler
    DCD     _isr_wrapper	;TIM3_IRQHandler
    DCD     _isr_wrapper	;KEY_Wakeup_IRQHandler

    IMPORT  |Image$$ER_IROM$$Base|
    IMPORT  |Image$$RW_IRAM2$$Base|	
			
;APP INFO
MAGIC         DCB     'A', 'T', 'B', 0
ROM_START     DCD     |Image$$ER_IROM$$Base|
ROM_END       DCD     |Image$$RW_IRAM2$$Base|	
ENTRY         DCD     Reset_Handler
NOR_ADDR      DCD     0x0	;0x1000
EXT           DCD     0
CHEKSUM_DATA  DCD     0
CHEKSUM_HDR	  DCD     0

;*************************************************************************************************/
;*!
;*  \brief      Reset handler.
;*
;*  \param      None.
;*
;*  \return     None.
;*/
;*************************************************************************************************/
    AREA    |.text|, CODE, READONLY
    ALIGN
Reset_Handler\
    PROC
    EXPORT  Reset_Handler          [WEAK]
    IMPORT  SystemInit
    IMPORT  __main

    LDR     R0, =SystemInit
    BLX     R0
    LDR     R0, =__main
    BX      R0
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
                EXPORT _isr_wrapper   	[WEAK]
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
