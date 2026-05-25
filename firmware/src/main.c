/**
 * @file main.c
 * @brief RFID RC522 Reader - State Machine Implementation
 *        States: INIT -> VERIFY -> IDLE -> STREAMING
 */

#include "gpio.h"
#include "uart.h"
#include "systick.h"
#include "log.h"
#include "spi.h"
#include "rfid_rc522.h"

/* ============================================================================
 * STATE MACHINE DEFINITIONS
 * ============================================================================ */

typedef enum {
    STATE_INIT = 0,
    STATE_VERIFY,
    STATE_IDLE,
    STATE_STREAMING
} State;

typedef enum {
    INIT_OK = 0,
    INIT_FAILED ,
    INIT_RETRY
} InitResult;

/* ============================================================================
 * VARIABLES
 * ============================================================================ */

static State current_state = STATE_INIT;
static uint8_t current_atqa[2];

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

MFRC522_t rfID = {
    .cs_Port = GPIOB,
    .cs_Pin = GPIO4,
    .rst_Port = GPIOB,
    .rst_Pin = GPIO5,
};

/* ============================================================================
 * STATE FUNCTIONS
 * ============================================================================ */

/**
 * @brief State: Initialize RC522 hardware
 */
static InitResult state_init(void)
{
    uart_send_string("\r\n=== STATE: INIT ===\r\n");
    
    /* Initialize RC522 via driver */
    RC522_Status status = rfid_rc522_init(&rfID);
    
    if (status == RC522_STATUS_OK) {
        uart_send_string("[INIT] Driver initialization OK\r\n");
        delay_ms(2000);
        return INIT_OK;
    } else {
        uart_send_string("[INIT] Driver initialization FAILED\r\n");
        return INIT_FAILED;
        delay_ms(2000);
    }
}

/**
 * @brief State: Idle - wait for card detection
 */
static RC522_Status state_idle(void)
{
   static uint8_t timeoutCount = 0;
   if (rfid_rc522_poll_card(&rfID, current_atqa) == RC522_STATUS_OK)
   {
       timeoutCount = 0; // Reset compteur si carte détectée
       LOG_DEBUG("Card detected");
       delay_ms(2000);
       return RC522_STATUS_OK;
   }
   else
   {
       timeoutCount++;
       LOG_DEBUG_INT("Card not detected, timeout count = ", timeoutCount);
       if(timeoutCount >= MAX_TIMEOUT_COUNT)
       {    
           timeoutCount = 0;
           LOG_INFO("MFRC522 stuck, recovering...");
           delay_ms(2000);
           rfid_rc522_recover(&rfID);
       }
       delay_ms(100);  /* Poll toutes les 100ms */
       return RC522_STATUS_INVALID;
   }
}

/**
 * @brief State: Streaming - read and display card UID
 */
static RC522_Status state_streaming(void)
{
    RC522_UID full_uid;

    if(rfid_rc522_read_uid_full(&rfID, &full_uid, current_atqa) == RC522_STATUS_OK)
    {
            LOG_DEBUG_HEX("ATQA[0]: ", full_uid.atqa[0]);
            LOG_DEBUG_HEX("ATQA[1]: ", full_uid.atqa[1]);
            LOG_DEBUG_HEX("SAK: ", full_uid.sak);
            LOG_DEBUG_INT("UID size: ", full_uid.size);

            RC522_CardType card_type = rfid_rc522_get_card_type(&full_uid);
            LOG_DEBUG(rfid_rc522_card_type_name(card_type));
            delay_ms(5000);
            return RC522_STATUS_OK;
    }
    else 
    {
                LOG_DEBUG(" UID READ ERROR ");
                delay_ms(5000);
                return RC522_STATUS_ERROR;
    }
}

/* ============================================================================
 * MAIN - STATE MACHINE LOOP
 * ============================================================================ */

int main(void)
{
    systick_init();
    gpio_driver_init();
    uart_init();
    spi_driver_init();

    uart_send_string("\r\n");
    uart_send_string("========================================\r\n");
    uart_send_string("   RFID RC522 Reader - State Machine   \r\n");
    uart_send_string("========================================\r\n");

    /* Initialize state */
    current_state = STATE_INIT;

    /* Main state machine loop */
    while (1) {
        switch (current_state) {
            
            /* ======================================== */
            case STATE_INIT:
            /* ======================================== */
            {
                uart_send_string("\r\n>>> STATE: INIT\r\n");
                InitResult result = state_init();
                if (result == INIT_OK) {
                    uart_send_string("[MAIN] Init successful -> VERIFY\r\n");
                    current_state = STATE_VERIFY;
                } else {
                    uart_send_string("[MAIN] Init failed -> retry INIT\r\n");
                    for (volatile int i = 0; i < 1000000; i++) { }
                    /* Stay in INIT state */
                }
                break;
            }
            
            /* ======================================== */
            case STATE_VERIFY:
            /* ======================================== */
            {
                uart_send_string("\r\n>>> STATE: VERIFY\r\n");
                current_state = STATE_IDLE;
                break;
            }
            
            /* ======================================== */
            case STATE_IDLE:
            /* ======================================== */
            {
                /* Check for card without printing state constantly */
                uart_send_string("\r\n>>> STATE: IDLE\r\n");
                RC522_Status result = state_idle();
                if(result == RC522_STATUS_OK)
                {
                    current_state = STATE_STREAMING;
                    delay_ms(100);
                }
                else if(result == RC522_STATUS_INVALID)
                {
                    current_state = STATE_IDLE;
                }
                break;
            }
            
            /* ======================================== */
            case STATE_STREAMING:
            /* ======================================== */
            {
                uart_send_string("\r\n>>> STATE: STREAMING\r\n");
                state_streaming();
                RC522_Status result = rfid_rc522_wait_card_removal(&rfID);
                if(result == RC522_STATUS_OK)
                {
                    current_state = STATE_IDLE;                
                }
                else
                {
                    current_state = STATE_INIT;
                }
                break;
            }
            
            /* ======================================== */
            default:
            /* ======================================== */
            {
                uart_send_string("[MAIN] Unknown state -> INIT\r\n");
                current_state = STATE_INIT;
                break;
            }
        }
    }

    return 0;
}
