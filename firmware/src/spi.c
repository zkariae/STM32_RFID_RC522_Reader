/**
 * @file spi.c
 * @brief Driver SPI1 — Mode 0, Master, 8 bits MSB, ~5.25 MHz (libopencm3).
 *        SCK=PA5, MISO=PA6, MOSI=PA7, NSS logiciel sur PA4.
 */

#include "spi.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/rcc.h>

/**
 * @brief Initialise SPI1 en maître, Mode 0 (CPOL=0, CPHA=0), 8 bits MSB.
 * @pre   PA5/PA6/PA7 configurés en AF5, PA4 en sortie (voir gpio_driver_init).
 */
void spi_driver_init(void)
{
    rcc_periph_clock_enable(RCC_SPI1);
    //spi_reset(SPI1);
    spi_init_master(SPI1,
                    SPI_CR1_BAUDRATE_FPCLK_DIV_16,    /* PCLK/16 ≈ 5.25 MHz  */
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,  /* CPOL = 0             */
                    SPI_CR1_CPHA_CLK_TRANSITION_1,     /* CPHA = 0             */
                    SPI_CR1_DFF_8BIT,                  /* trame 8 bits         */
                    SPI_CR1_MSBFIRST);                 /* MSB en premier       */

    spi_enable_software_slave_management(SPI1); /* NSS géré par logiciel (PA4) */
    spi_set_nss_high(SPI1);                     /* SSI=1 → évite MODF fault    */
    spi_enable(SPI1);
}

/**
 * @brief Échange un octet en full-duplex (bloquant).
 * @param[in] data  Octet à émettre sur MOSI.
 * @return          Octet reçu sur MISO.
 */
uint8_t spi_transfer(uint8_t data)
{
    return (uint8_t)spi_xfer(SPI1, data);
}