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
    INIT_FAILED,
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
    uart_send_string("\r\n=== STATE: VERIFY ===\r\n");
    uart_send_string("[VERIFY] Reading version register...\r\n");

    uint8_t version = rfid_rc522_get_version();
    
    uart_send_string("[VERIFY] Version: ");
    uart_send_hex(version);
    uart_send_string("\r\n");

/* SPI stability test - write and read 3 times */
    uart_send_string("[VERIFY] SPI test: ");
    uint8_t test_ok = 1;
    for (int i = 0; i < 3; i++) {
        rfid_rc522_write_reg(&rfID, 0x27, 0x55);  // Test register (FIFO)
        uint8_t val = rfid_rc522_read_reg(0x27);
        uart_send_hex(val);
        uart_send_string(" ");
        if (val != 0x55) {
            test_ok = 0;
        }
    }
    uart_send_string("\r\n");
    if (test_ok) {
        uart_send_string("[VERIFY] SPI OK\r\n");
    } else {
        uart_send_string("[VERIFY] SPI FAILED\r\n");
    }

    /* Check if version is valid - 0x90, 0x91, or 0x92 per datasheet */
    if (version == 0x90 || version == 0x91 || version == 0x92) {
        uart_send_string("[VERIFY] Version OK!\r\n");
    } else {
        uart_send_string("[VERIFY] Version INVALID - will retry init\r\n");
        return 0;
    }

    /* Verify antenna is ON by reading TxControl register (0x14) */
    uart_send_string("[VERIFY] Checking antenna...\r\n");
    uint8_t tx_control = rfid_rc522_read_reg(0x14);
    uart_send_string("[VERIFY] TxControl: ");
    uart_send_hex(tx_control);
    uart_send_string("\r\n");
    
    if ((tx_control & 0x03) == 0x03) {
        uart_send_string("[VERIFY] Antenna ON!\r\n");
    } else {
        uart_send_string("[VERIFY] Antenna OFF - enabling...\r\n");
        /* Try to enable antenna */
        rfid_rc522_write_reg(&rfID, 0x14, tx_control | 0x03);
        tx_control = rfid_rc522_read_reg(0x14);
        uart_send_string("[VERIFY] TxControl after enable: ");
        uart_send_hex(tx_control);
        uart_send_string("\r\n");
    }

    /* Check RF configuration registers */
    uart_send_string("[VERIFY] RF config:\r\n");
    uint8_t tx_mode = rfid_rc522_read_reg(0x12);
    uint8_t rx_mode = rfid_rc522_read_reg(0x13);
    uint8_t tx_ask = rfid_rc522_read_reg(0x15);
    uart_send_string("  TxMode: ");
    uart_send_hex(tx_mode);
    uart_send_string(" (expect 0x00)\r\n");
    uart_send_string("  RxMode: ");
    uart_send_hex(rx_mode);
    uart_send_string(" (expect 0x07)\r\n");
    uart_send_string("  TxAsk: ");
    uart_send_hex(tx_ask);
    uart_send_string(" (expect 0x40)\r\n");

    return 1;
}

/**
 * @brief State: Idle - wait for card detection
 */
static uint8_t state_idle(void)
{
    /* Only print state occasionally to avoid flooding */
    static uint8_t counter = 0;
    counter++;
    if (counter > 10) {
        uart_send_string(".\r\n");
        counter = 0;
    }

    uint8_t atqa[2];
    RC522_Status status = rfid_rc522_request(&rfID, atqa);
    
    /* Display raw ATQA for debugging */
    uart_send_string("[IDLE] ATQA raw: ");
    uart_send_hex(atqa[0]);
    uart_send_string(" ");
    uart_send_hex(atqa[1]);
    uart_send_string("\r\n");
    
    if (status == RC522_STATUS_OK) {
        /* Validate ATQA - only accept valid values */
        if ((atqa[0] == 0xFF && atqa[1] == 0xFF) ||
            (atqa[0] == 0x00 && atqa[1] == 0x00)) {
            return 0;
        }
        
        uart_send_string("[IDLE] Card detected!\r\n");
        return 1;
    }
    
    return 0;
}

/**
 * @brief State: Streaming - read and display card UID
 */
static void state_streaming(void)
{
    uart_send_string("\r\n=== STATE: STREAMING ===\r\n");
    uart_send_string("[STREAM] Reading card UID...\r\n");

    /* Perform anticollision */
    RC522_Status status = rfid_rc522_anticoll(&rfID, &current_uid);
    
    if (status == RC522_STATUS_OK) {
        /* Small delay before select */
        for (volatile int i = 0; i < 1000; i++) { }
        
        /* Perform SELECT to get SAK */
        status = rfid_rc522_select(&rfID, &current_uid);
        
        uart_send_string("[STREAM] UID: ");
        for (int i = 0; i < current_uid.size; i++) {
            uart_send_hex(current_uid.uid[i]);
            uart_send_string(" ");
        }
        uart_send_string("\r\n");
        
        uart_send_string("[STREAM] Size: ");
        uart_send_int(current_uid.size);
        uart_send_string(" bytes\r\n");
        
        uart_send_string("[STREAM] Select status: ");
        uart_send_hex(status);
        uart_send_string("\r\n");
        
        if (status == RC522_STATUS_OK) {
            uart_send_string("[STREAM] SAK: ");
            uart_send_hex(current_uid.sak);
            uart_send_string("\r\n");
            card_detected = 1;
        } else {
            uart_send_string("[STREAM] Failed to select card\r\n");
        }
    } else {
        uart_send_string("[STREAM] Failed to read UID\r\n");
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
                    // current_state = STATE_VERIFY;
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
                version_ok = state_verify();
                
                if (version_ok) {
                    uart_send_string("[MAIN] Version OK -> IDLE\r\n");
                    current_state = STATE_IDLE;
                } else {
                    uart_send_string("[MAIN] Version invalid -> INIT\r\n");
                    current_state = STATE_INIT;
                }
                break;
            }
            
            /* ======================================== */
            case STATE_IDLE:
            /* ======================================== */
            {
                /* Check for card without printing state constantly */
                uint8_t detected = state_idle();
                
                if (detected) {
                    uart_send_string("[MAIN] Card detected -> STREAMING\r\n");
                    current_state = STATE_STREAMING;
                } else {
                    /* Stay in IDLE - check again after delay */
                    for (volatile int i = 0; i < 500000; i++) { }
                }
                break;
            }
            
            /* ======================================== */
            case STATE_STREAMING:
            /* ======================================== */
            {
                state_streaming();
                
                if (card_detected) {
                    uart_send_string("[MAIN] UID displayed -> IDLE\r\n");
                    card_detected = 0;
                }
                
                /* Small delay before going back to IDLE */
                for (volatile int i = 0; i < 1000000; i++) { }
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
