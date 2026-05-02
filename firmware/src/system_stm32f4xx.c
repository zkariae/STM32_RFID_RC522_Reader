/**
 * system_stm32f4xx.c
 * SystemInit : activation FPU + relocalisation VTOR
 */

#include <stdint.h>

/* ─── Registres Cortex-M4 ─── */
#define SCB_CPACR   (*(volatile uint32_t *)0xE000ED88U)
#define SCB_VTOR    (*(volatile uint32_t *)0xE000ED08U)

/* Adresse de base FLASH */
#define FLASH_BASE  0x08000000U

void SystemInit(void)
{
    /* 1 — Activer FPU (CP10 + CP11 = accès complet) */
    SCB_CPACR |= (0xFU << 20);

    /* 2 — Relocaliser la table des vecteurs en FLASH */
    SCB_VTOR = FLASH_BASE;
}