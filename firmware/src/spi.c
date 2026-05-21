/**
 * @file spi.c
 * @brief Driver SPI2 hardware — Mode 0, Master, 8 bits MSB.
 *        SCK=PB10, MISO=PC2, MOSI=PC3, NSS logiciel sur PB4.
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

    /* SPI2: Mode 0 (CPOL=0, CPHA=0), Master, 8 bits, MSB first, ~500 kHz (DIV_32) */
    spi_init_master(SPI2,
                    SPI_CR1_BAUDRATE_FPCLK_DIV_32,   /* ~500 kHz avec PCLK1=16 MHz */
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,  /* CPOL = 0 */
                    SPI_CR1_CPHA_CLK_TRANSITION_1,    /* CPHA = 0 */
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);

    /* Configuration explicite équivalente HAL pour SPI2. */
    SPI_CR1(SPI2) &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);  /* CPOL = 0, CPHA = 0 */
    spi_set_unidirectional_mode(SPI2);                /* SPI_DIRECTION_2LINES */
    spi_set_full_duplex_mode(SPI2);
    SPI_CR2(SPI2) &= ~SPI_CR2_FRF;                    /* SPI_TIMODE_DISABLE */
    spi_disable_crc(SPI2);                            /* SPI_CRCCALCULATION_DISABLE */
    SPI2_CRCPR = 10;                                  /* CRCPolynomial = 10 */

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
    for (volatile int i = 0; i < 20; i++) { }
}

/**
 * @brief Lit un octet (envoie 0x00 pour générer l'horloge).
 */
uint8_t rc522_spi_read(void)
{
    uint8_t result = spi_xfer(SPI2, 0x00);
    for (volatile int i = 0; i < 20; i++) { }
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
