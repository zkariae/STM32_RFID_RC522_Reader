/**
 * @file spi.c
 * @brief Driver SPI2 hardware — Mode 0, Master, 8 bits MSB.
 *        SCK=PB13, MISO=PB14, MOSI=PB15, NSS logiciel sur PB4.
 */

#include "spi.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/rcc.h>

/**
 * @brief Initialise SPI2 en maître, Mode 0, Master, 8 bits MSB.
 */
void spi_driver_init(void)
{
    rcc_periph_clock_enable(RCC_SPI2);

    /* SPI2: Mode 1 (CPOL=1, CPHA=0), Master, 8 bits, MSB first, ~0.65MHz (DIV_128) */
    spi_init_master(SPI2,
                    SPI_CR1_BAUDRATE_FPCLK_DIV_128,  /* ~0.65 MHz - plus lent */
                    SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,  /* CPOL = 1 */
                    SPI_CR1_CPHA_CLK_TRANSITION_1,    /* CPHA = 0 */
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);

    /* NSS géré par logiciel (PB4) */
    spi_enable_software_slave_management(SPI2);
    spi_set_nss_high(SPI2);
    spi_enable(SPI2);
}

/**
 * @brief Écrit un octet sans lire de réponse (pour RC522).
 */
void rc522_spi_write(uint8_t data)
{
    spi_xfer(SPI2, data);
    /* Delai pour RC522 */
    for (volatile int i = 0; i < 10; i++) { }
}

/**
 * @brief Lit un octet (envoie 0x00 pour générer l'horloge).
 */
uint8_t rc522_spi_read(void)
{
    uint8_t result = spi_xfer(SPI2, 0x00);
    /* Delai pour RC522 */
    for (volatile int i = 0; i < 10; i++) { }
    return result;
}

/**
 * @brief Échange un octet en full-duplex (hardware SPI).
 */
uint8_t spi_transfer(uint8_t data)
{
    uint8_t result = spi_xfer(SPI2, data);
    /* Petit delai pour permettre au RC522 de traiter */
    for (volatile int i = 0; i < 10; i++) { }
    return result;
}