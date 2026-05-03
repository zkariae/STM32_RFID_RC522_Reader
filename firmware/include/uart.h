/**
 * @file uart.h
 * @brief Interface du driver USART2 — 115200 8N1, TX=PA2, RX=PA3 (libopencm3).
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

/** @brief Initialise USART2 en 115200 8N1. @pre PA2/PA3 en AF7. */
void uart_init(void);

/** @brief Envoie un caractère (bloquant). @param[in] c Caractère à transmettre. */
void uart_send_char(char c);

/** @brief Envoie une chaîne terminée par '\\0'. @param[in] str Chaîne à transmettre. */
void uart_send_string(const char *str);

/** @brief Envoie un octet en hexadécimal ASCII (ex. 0xFA). @param[in] val Octet à afficher. */
void uart_send_hex(uint8_t val);

#endif /* UART_H */