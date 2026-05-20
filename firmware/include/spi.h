/**
 * @file spi.h
 * @brief Interface du driver SPI2 — Mode 0, Master, 8 bits (libopencm3).
 *        SCK sur PB10, MISO sur PC2, MOSI sur PC3,
 *        NSS logiciel sur PB4 pour le module RC522, RST sur PB5.
 */

#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>

/** @brief Sélectionne le RC522 (NSS bas). */
#define RC522_CS_LOW()   gpio_clear(GPIOB, GPIO4)

/** @brief Désélectionne le RC522 (NSS haut). */
#define RC522_CS_HIGH()  gpio_set(GPIOB, GPIO4)

/** @brief Reset RC522 (RST bas). */
#define RC522_RST_LOW()  gpio_clear(GPIOB, GPIO5)

/** @brief Sortie reset RC522 (RST haut). */
#define RC522_RST_HIGH() gpio_set(GPIOB, GPIO5)

/** @brief Initialise SPI2 en maître, Mode 0, 8 bits. @pre PB4/PB5/PB10, PC2/PC3 configurés. */
void spi_driver_init(void);

/** @brief Écrit un octet sans réponse (pour RC522). */
void rc522_spi_write(uint8_t data);

/** @brief Lit un octet (envoie 0x00 pour horloge). */
uint8_t rc522_spi_read(void);

/** @brief Échange un octet en full-duplex. @param[in] data Octet à émettre. @return Octet reçu. */
uint8_t spi_transfer(uint8_t data);

#endif /* SPI_H */
