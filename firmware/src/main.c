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
static uint8_t version_ok = 0;
static RC522_UID current_uid;
static uint8_t card_detected = 0;

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
        return INIT_OK;
    } else {
        uart_send_string("[INIT] Driver initialization FAILED\r\n");
        return INIT_FAILED;
    }
}

/**
 * @brief State: Verify RC522 by reading version register
 */
static uint8_t state_verify(void)
{
    return 1;
}



/**
 * @brief State: Idle - wait for card detection
 */
static uint8_t state_idle(void)
{
   static uint8_t timeoutCount = 0;
   if (rfid_rc522_PollCard(&rfID) == RC522_STATUS_OK)
   {
       timeoutCount = 0; // Reset compteur si carte détectée
       LOG_DEBUG("Card detected");
       return RC522_STATUS_OK;
   }
   else
   {
       timeoutCount++;
       LOG_DEBUG_INT("Card not detected, timeout count = %d", timeoutCount);
       if(timeoutCount >= MAX_TIMEOUT_COUNT)
       {    
           timeoutCount = 0;
           LOG_INFO("MFRC522 stuck, recovering...");
           rfid_rc522_Recover(&rfID);
       }
       delay_ms(100);  /* Poll toutes les 100ms */
       return RC522_STATUS_INVALID;
   }
}

/**
 * @brief State: Streaming - read and display card UID
 */
static void state_streaming(void)
{
    uart_send_string("\r\n=== STATE: STREAMING ===\r\n");
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
    version_ok = 0;
    card_detected = 0;

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
                uint8_t result = state_idle();
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
                state_streaming();
                uart_send_string("\r\n>>> STATE: STREAMING\r\n");
                delay_ms(1000);
                current_state = STATE_IDLE;
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
