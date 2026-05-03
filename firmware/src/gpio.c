/**
 * @file gpio.c
 * @brief Driver d'initialisation des broches GPIO via libopencm3.
 *
 * Ce module configure les broches du port GPIOA utilisées par les
 * périphériques USART2 et SPI1 sur un microcontrôleur STM32 :
 *
 * | Broche | Fonction        | Mode              |
 * |--------|-----------------|-------------------|
 * | PA2    | UART2 TX        | Alternate Function AF7 |
 * | PA3    | UART2 RX        | Alternate Function AF7 |
 * | PA4    | SPI1 NSS (CS)   | Sortie push-pull (logiciel) |
 * | PA5    | SPI1 SCK        | Alternate Function AF5 |
 * | PA6    | SPI1 MISO       | Alternate Function AF5 |
 * | PA7    | SPI1 MOSI       | Alternate Function AF5 |
 *
 * @note Le signal NSS (Chip Select du RC522) est géré en logiciel :
 *       la broche est maintenue à l'état haut (idle HIGH) après l'init.
 *
 * @dependencies libopencm3
 */

#include "gpio.h"

/**
 * @brief Initialise les broches GPIO du port A pour USART2 et SPI1.
 *
 * Cette fonction doit être appelée une seule fois au démarrage,
 * avant d'initialiser les pilotes USART2 ou SPI1.
 *
 * Séquence d'initialisation :
 *  1. Activation de l'horloge du port GPIOA via RCC.
 *  2. Configuration de PA2/PA3 en fonction alternée AF7 (USART2).
 *  3. Configuration de PA4 en sortie push-pull (NSS logiciel, idle HIGH).
 *  4. Configuration de PA5/PA6/PA7 en fonction alternée AF5 (SPI1).
 *
 * @return void
 */
void gpio_driver_init(void)
{
    /* ------------------------------------------------------------------ */
    /* Étape 1 : Activation de l'horloge périphérique du port GPIOA       */
    /* Sans cette étape, toute écriture dans les registres GPIO est sans   */
    /* effet (registres non alimentés).                                    */
    /* ------------------------------------------------------------------ */
    rcc_periph_clock_enable(RCC_GPIOA);

    /* ------------------------------------------------------------------ */
    /* Étape 2 : PA2 (TX) et PA3 (RX) — USART2, Alternate Function 7     */
    /* AF7 correspond à USART2 sur STM32F4 (voir datasheet, table des AF) */
    /* Aucun pull-up/pull-down : la ligne UART est pilotée en permanence. */
    /* ------------------------------------------------------------------ */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

    /* ------------------------------------------------------------------ */
    /* Étape 3 : PA4 (NSS) — Chip Select logiciel du module RC522        */
    /*                                                                      */
    /* Configurée en sortie push-pull à 50 MHz.                            */
    /* gpio_set() place la broche à l'état haut (HIGH) dès l'init afin   */
    /* de désélectionner le RC522 tant qu'aucune transaction SPI n'est    */
    /* en cours (NSS actif à l'état bas par convention).                   */
    /* ------------------------------------------------------------------ */
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO4);
    gpio_set(GPIOA, GPIO4); /* NSS = HIGH → RC522 désélectionné */

    /* ------------------------------------------------------------------ */
    /* Étape 4 : PA5 (SCK), PA6 (MISO), PA7 (MOSI) — SPI1, AF5          */
    /* AF5 correspond à SPI1 sur STM32F4 (voir datasheet, table des AF).  */
    /*                                                                      */
    /* SCK et MOSI sont configurés en push-pull 50 MHz (signaux de sortie */
    /* pilotés activement).                                                 */
    /* MISO est laissé sans option de sortie (broche d'entrée).           */
    /* ------------------------------------------------------------------ */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO5 | GPIO6 | GPIO7);
    gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO5 | GPIO7); /* SCK + MOSI uniquement */
}