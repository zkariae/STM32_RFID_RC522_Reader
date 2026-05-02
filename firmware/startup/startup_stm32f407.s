.syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.global  g_pfnVectors
.global  Default_Handler

.word  _sidata
.word  _sdata
.word  _edata
.word  _sbss
.word  _ebss

  .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function

Reset_Handler:
  movs  r1, #0
  b  LoopCopyDataInit

CopyDataInit:
  ldr  r3, =_sidata
  ldr  r3, [r3, r1]
  str  r3, [r0, r1]
  adds  r1, r1, #4

LoopCopyDataInit:
  ldr  r0, =_sdata
  ldr  r3, =_edata
  adds  r2, r0, r1
  cmp  r2, r3
  bcc  CopyDataInit
  ldr  r2, =_sbss
  b  LoopFillZerobss

FillZerobss:
  movs  r3, #0
  str  r3, [r2], #4

LoopFillZerobss:
  ldr  r3, =_ebss
  cmp  r2, r3
  bcc  FillZerobss
  bl  SystemInit
  bl  main
  bx  lr
  .size  Reset_Handler, .-Reset_Handler

  .section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler

  .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object

g_pfnVectors:
  .word  _estack
  .word  Reset_Handler       @ Reset
  .word  Default_Handler     @ NMI
  .word  HardFault_Handler   @ HardFault
  .word  Default_Handler     @ MemManage
  .word  Default_Handler     @ BusFault
  .word  Default_Handler     @ UsageFault
  .word  0                   @ Reserved
  .word  0                   @ Reserved
  .word  0                   @ Reserved
  .word  0                   @ Reserved
  .word  Default_Handler     @ SVC
  .word  Default_Handler     @ DebugMon
  .word  0                   @ Reserved
  .word  Default_Handler     @ PendSV
  .word  SysTick_Handler     @ SysTick

  /* External Interrupts */
  .word  Default_Handler     @ WWDG
  .word  Default_Handler     @ PVD
  .word  Default_Handler     @ TAMP_STAMP
  .word  Default_Handler     @ RTC_WKUP
  .word  Default_Handler     @ FLASH
  .word  Default_Handler     @ RCC
  .word  EXTI0_IRQHandler    @ EXTI0
  .word  Default_Handler     @ EXTI1
  .word  Default_Handler     @ EXTI2
  .word  Default_Handler     @ EXTI3
  .word  Default_Handler     @ EXTI4
  .word  Default_Handler     @ DMA1_Stream0
  .word  Default_Handler     @ DMA1_Stream1
  .word  Default_Handler     @ DMA1_Stream2
  .word  Default_Handler     @ DMA1_Stream3
  .word  Default_Handler     @ DMA1_Stream4
  .word  Default_Handler     @ DMA1_Stream5
  .word  Default_Handler     @ DMA1_Stream6
  .word  Default_Handler     @ ADC
  .word  Default_Handler     @ CAN1_TX
  .word  Default_Handler     @ CAN1_RX0
  .word  Default_Handler     @ CAN1_RX1
  .word  Default_Handler     @ CAN1_SCE
  .word  Default_Handler     @ EXTI9_5
  .word  Default_Handler     @ TIM1_BRK_TIM9
  .word  Default_Handler     @ TIM1_UP_TIM10
  .word  Default_Handler     @ TIM1_TRG_COM_TIM11
  .word  Default_Handler     @ TIM1_CC
  .word  Default_Handler     @ TIM2
  .word  Default_Handler     @ TIM3
  .word  Default_Handler     @ TIM4
  .word  Default_Handler     @ I2C1_EV
  .word  Default_Handler     @ I2C1_ER
  .word  Default_Handler     @ I2C2_EV
  .word  Default_Handler     @ I2C2_ER
  .word  SPI1_IRQHandler     @ SPI1  <- RC522
  .word  Default_Handler     @ SPI2
  .word  Default_Handler     @ USART1
  .word  Default_Handler     @ USART2
  .word  Default_Handler     @ USART3
  .word  Default_Handler     @ EXTI15_10
  .word  Default_Handler     @ RTC_Alarm
  .word  Default_Handler     @ OTG_FS_WKUP
  .word  Default_Handler     @ TIM8_BRK_TIM12
  .word  Default_Handler     @ TIM8_UP_TIM13
  .word  Default_Handler     @ TIM8_TRG_COM_TIM14
  .word  Default_Handler     @ TIM8_CC
  .word  Default_Handler     @ DMA1_Stream7
  .word  Default_Handler     @ FSMC
  .word  Default_Handler     @ SDIO
  .word  Default_Handler     @ TIM5
  .word  Default_Handler     @ SPI3
  .word  Default_Handler     @ UART4
  .word  Default_Handler     @ UART5
  .word  Default_Handler     @ TIM6_DAC
  .word  Default_Handler     @ TIM7
  .word  Default_Handler     @ DMA2_Stream0
  .word  Default_Handler     @ DMA2_Stream1
  .word  Default_Handler     @ DMA2_Stream2
  .word  Default_Handler     @ DMA2_Stream3
  .word  Default_Handler     @ DMA2_Stream4
  .word  Default_Handler     @ ETH
  .word  Default_Handler     @ ETH_WKUP
  .word  Default_Handler     @ CAN2_TX
  .word  Default_Handler     @ CAN2_RX0
  .word  Default_Handler     @ CAN2_RX1
  .word  Default_Handler     @ CAN2_SCE
  .word  Default_Handler     @ OTG_FS
  .word  Default_Handler     @ DMA2_Stream5
  .word  Default_Handler     @ DMA2_Stream6
  .word  Default_Handler     @ DMA2_Stream7
  .word  Default_Handler     @ USART6
  .word  Default_Handler     @ I2C3_EV
  .word  Default_Handler     @ I2C3_ER
  .word  Default_Handler     @ OTG_HS_EP1_OUT
  .word  Default_Handler     @ OTG_HS_EP1_IN
  .word  Default_Handler     @ OTG_HS_WKUP
  .word  Default_Handler     @ OTG_HS
  .word  Default_Handler     @ DCMI
  .word  Default_Handler     @ CRYP
  .word  Default_Handler     @ HASH_RNG
  .word  Default_Handler     @ FPU

  /* Weak aliases */
  .weak  NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

  .weak  HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler

  .weak  SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler

  .weak  EXTI0_IRQHandler
  .thumb_set EXTI0_IRQHandler,Default_Handler

  .weak  SPI1_IRQHandler
  .thumb_set SPI1_IRQHandler,Default_Handler