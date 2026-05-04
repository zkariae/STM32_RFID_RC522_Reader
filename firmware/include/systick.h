#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

/*==============================================================
  Registres SysTick — Cortex-M4
==============================================================*/
#define SYSTICK_BASE 0xE000E010UL

#define SYSTICK_CTRL  (*(volatile uint32_t *)(SYSTICK_BASE + 0x00UL))
#define SYSTICK_LOAD  (*(volatile uint32_t *)(SYSTICK_BASE + 0x04UL))
#define SYSTICK_VAL   (*(volatile uint32_t *)(SYSTICK_BASE + 0x08UL))
#define SYSTICK_CALIB (*(volatile uint32_t *)(SYSTICK_BASE + 0x0CUL))

/* Bits de contrôle SYSTICK_CTRL */
#define SYSTICK_CTRL_ENABLE    (1UL << 0)  /* Activer le timer    */
#define SYSTICK_CTRL_TICKINT   (1UL << 1)  /* Activer interruption */
#define SYSTICK_CTRL_CLKSOURCE (1UL << 2)  /* 1=AHB, 0=AHB/8     */
#define SYSTICK_CTRL_COUNTFLAG (1UL << 16) /* Flag débordement    */

/* Fréquence HSI par défaut STM32F407 */
#define SYSTEM_CLOCK_HZ  16000000UL /* 16 MHz HSI             */
#define SYSTICK_LOAD_1MS (SYSTEM_CLOCK_HZ / 1000UL - 1UL) /* 15999 */

/*==============================================================
  Interface publique
==============================================================*/
void     systick_init(void);
void     delay_ms(uint32_t ms);
uint32_t systick_get_tick(void);

#endif /* SYSTICK_H */