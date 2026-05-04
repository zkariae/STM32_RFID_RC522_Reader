#ifndef LOG_H
#define LOG_H

#include "uart.h"

/*==============================================================
  Système de logs avec origine (fichier + ligne)

  Utilisation :
    LOG_INFO("Message")
    LOG_WARN("Message")
    LOG_ERROR("Message")
    LOG_DEBUG("Message")
    LOG_DEBUG_INT("valeur =", 42)
==============================================================*/

/* Niveaux de log */
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

/* Niveau actuel — changer pour filtrer les messages */
#define LOG_LEVEL_CURRENT LOG_LEVEL_DEBUG

#define LOG_DEBUG(msg)                            \
    do                                            \
    {                                             \
        if (LOG_LEVEL_CURRENT <= LOG_LEVEL_DEBUG) \
        {                                         \
            uart_send_string("[DBG]  [");         \
            uart_send_string(__FILE__);           \
            uart_send_string(":");                \
            uart_send_int(__LINE__);              \
            uart_send_string("] ");               \
            uart_send_string(msg);                \
            uart_send_string("\r\n");             \
        }                                         \
    } while (0)

#define LOG_INFO(msg)                            \
    do                                           \
    {                                            \
        if (LOG_LEVEL_CURRENT <= LOG_LEVEL_INFO) \
        {                                        \
            uart_send_string("[INFO] [");        \
            uart_send_string(__FILE__);          \
            uart_send_string(":");               \
            uart_send_int(__LINE__);             \
            uart_send_string("] ");              \
            uart_send_string(msg);               \
            uart_send_string("\r\n");            \
        }                                        \
    } while (0)

#define LOG_WARN(msg)                            \
    do                                           \
    {                                            \
        if (LOG_LEVEL_CURRENT <= LOG_LEVEL_WARN) \
        {                                        \
            uart_send_string("[WARN] [");        \
            uart_send_string(__FILE__);          \
            uart_send_string(":");               \
            uart_send_int(__LINE__);             \
            uart_send_string("] ");              \
            uart_send_string(msg);               \
            uart_send_string("\r\n");            \
        }                                        \
    } while (0)

#define LOG_ERROR(msg)                            \
    do                                            \
    {                                             \
        if (LOG_LEVEL_CURRENT <= LOG_LEVEL_ERROR) \
        {                                         \
            uart_send_string("[ERR]  [");         \
            uart_send_string(__FILE__);           \
            uart_send_string(":");                \
            uart_send_int(__LINE__);              \
            uart_send_string("] ");               \
            uart_send_string(msg);                \
            uart_send_string("\r\n");             \
        }                                         \
    } while (0)

#define LOG_DEBUG_INT(msg, val)                   \
    do                                            \
    {                                             \
        if (LOG_LEVEL_CURRENT <= LOG_LEVEL_DEBUG) \
        {                                         \
            uart_send_string("[DBG]  [");         \
            uart_send_string(__FILE__);           \
            uart_send_string(":");                \
            uart_send_int(__LINE__);              \
            uart_send_string("] ");               \
            uart_send_string(msg);                \
            uart_send_int(val);                   \
            uart_send_string("\r\n");             \
        }                                         \
    } while (0)

#endif /* LOG_H */