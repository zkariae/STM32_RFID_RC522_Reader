/**
 * @file uart.c
 * @brief Driver USART2 — 115200 8N1, TX=PA2, RX=PA3 (libopencm3).
 */

#include "uart.h"
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>

/**
 * @brief Initialise USART2 en 115200 8N1, sans contrôle de flux.
 * @pre   PA2/PA3 configurés en AF7 (voir gpio_driver_init).
 */
void uart_init(void)
{
    rcc_periph_clock_enable(RCC_USART2);
    usart_set_baudrate(USART2, 115200);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART2, USART_MODE_TX_RX);
    usart_enable(USART2);
}

/**
 * @brief Envoie un caractère (bloquant).
 * @param[in] c  Caractère à transmettre.
 */
void uart_send_char(char c)
{
    usart_send_blocking(USART2, (uint8_t)c);
}

/**
 * @brief Envoie une chaîne terminée par '\0' (bloquant).
 * @param[in] str  Chaîne à transmettre.
 */
void uart_send_string(const char *str)
{
    while (*str)
        uart_send_char(*str++);
}

/**
 * @brief Envoie un octet sous forme hexadécimale ASCII (ex. 0xFA).
 * @param[in] val  Octet à afficher.
 */
void uart_send_hex(uint8_t val)
{
    const char hex[] = "0123456789ABCDEF";
    uart_send_char('0');
    uart_send_char('x');
    uart_send_char(hex[(val >> 4) & 0x0F]);  /* nibble haut */
    uart_send_char(hex[val & 0x0F]);          /* nibble bas  */
}