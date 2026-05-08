/**
 * @file spi.c
 * @brief Driver SPI2 hardware — Mode 0, Master, 8 bits MSB.
 *        SCK=PB13, MISO=PB14, MOSI=PB15, NSS logiciel sur PB4.
 */

#include "spi.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/rcc.h>

/**
 * @brief Initialise SPI2 en maître, Mode 0 (CPOL=0, CPHA=0), 8 bits MSB.
 */
void spi_driver_init(void)
{
    rcc_periph_clock_enable(RCC_SPI2);

    /* SPI2: Mode 1 (CPOL=1, CPHA=0), Master, 8 bits, MSB first, ~5.25MHz */
    spi_init_master(SPI2,
                    SPI_CR1_BAUDRATE_FPCLK_DIV_8,    /* ~5.25 MHz */
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
 * @brief Échange un octet en full-duplex (hardware SPI).
 */
uint8_t spi_transfer(uint8_t data)
{
    return (uint8_t)spi_xfer(SPI2, data);
}