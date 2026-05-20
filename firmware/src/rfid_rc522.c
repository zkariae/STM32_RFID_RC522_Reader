/**
 * @file rfid_rc522.c
 * @brief Implémentation du driver pour le module RFID RC522 (MFRC522).
 *        Communication SPI via spi.h.
 */

#include "rfid_rc522.h"
#include "log.h"
#include "spi.h"
#include "systick.h"
#include <libopencm3/stm32/gpio.h>
#include <stddef.h>

/* ============================================================================
 * MACROS PRIVÉES
 * ============================================================================ */

/**
 * @brief Adresse SPI pour lecture: bit 7=1, bit 6=1, bits 5-0 = register address
 *        Format: (addr << 1) | 0x80
 */
#define RC522_READ_ADDR(addr)      (((addr << 1) & 0x7E) | 0x80)

/**
 * @brief Adresse SPI pour écriture: bit 7=0, bit 6=0, bits 5-0 = register address
 *        Format: (addr << 1) | 0x00
 */
#define RC522_WRITE_ADDR(addr)     ((addr << 1) & 0x7E)

/* ============================================================================
 * VARIABLES STATIQUES
 * ============================================================================ */

static uint8_t _error_code = 0;

/* ============================================================================
 * FONCTIONS DE BASE : LECTURE/ÉCRITURE REGISTRES
 * ============================================================================ */

/**
 * @brief Écrit un octet dans un registre du RC522.
 */
void rfid_rc522_write_reg(MFRC522_t *dev, uint8_t addr, uint8_t value)
{
    gpio_clear(dev->cs_Port, dev->cs_Pin);
    for (volatile int i = 0; i < 2000; i++) { }
    rc522_spi_write(RC522_WRITE_ADDR(addr));
    for (volatile int i = 0; i < 1000; i++) { }
    rc522_spi_write(value);
    for (volatile int i = 0; i < 2000; i++) { }
    gpio_set(dev->cs_Port, dev->cs_Pin);
    for (volatile int i = 0; i < 10000; i++) { }
}

/**
 * @brief Lit un octet depuis un registre du RC522.
 */
uint8_t rfid_rc522_read_reg(uint8_t addr)
{
    uint8_t value;
    RC522_CS_LOW();
    for (volatile int i = 0; i < 2000; i++) { }
    rc522_spi_write(RC522_READ_ADDR(addr));
    for (volatile int i = 0; i < 1000; i++) { }
    value = rc522_spi_read();
    for (volatile int i = 0; i < 2000; i++) { }
    RC522_CS_HIGH();
    for (volatile int i = 0; i < 10000; i++) { }
    return value;
}

/**
 * @brief Modifie des bits dans un registre.
 */
static void rc522_set_bits(MFRC522_t *dev, uint8_t addr, uint8_t mask, uint8_t value)
{
    uint8_t tmp = rfid_rc522_read_reg(addr);
    tmp = (tmp & ~mask) | value;
    rfid_rc522_write_reg(dev, addr, tmp);
}

/* ============================================================================
 * FONCTIONS FIFO
 * ============================================================================ */

static void rc522_write_fifo(const uint8_t *data, uint8_t len)
{
    RC522_CS_LOW();
    for (volatile int i = 0; i < 1000; i++) { }
    rc522_spi_write(RC522_WRITE_ADDR(RC522_REG_FIFO_DATA));
    for (uint8_t i = 0; i < len; i++) {
        for (volatile int j = 0; j < 500; j++) { }
        rc522_spi_write(data[i]);
    }
    for (volatile int i = 0; i < 1000; i++) { }
    RC522_CS_HIGH();
    for (volatile int i = 0; i < 5000; i++) { }
}

static void rc522_read_fifo(uint8_t *data, uint8_t len)
{
    RC522_CS_LOW();
    for (volatile int i = 0; i < 1000; i++) { }
    rc522_spi_write(RC522_READ_ADDR(RC522_REG_FIFO_DATA));
    for (uint8_t i = 0; i < len; i++) {
        for (volatile int j = 0; j < 500; j++) { }
        data[i] = rc522_spi_read();
    }
    for (volatile int i = 0; i < 1000; i++) { }
    RC522_CS_HIGH();
    for (volatile int i = 0; i < 5000; i++) { }
}

static uint8_t rc522_fifo_count(void)
{
    return rfid_rc522_read_reg(RC522_REG_FIFO_LEVEL) & 0x7F;
}

static void rc522_clear_fifo(MFRC522_t *dev)
{
    rc522_set_bits(dev, RC522_REG_FIFO_LEVEL, 0x80, 0x80);
}

/* ============================================================================
 * FONCTIONS DE COMMUNICATION
 * ============================================================================ */

static RC522_Status rc522_transceive(MFRC522_t *dev,
                                      uint8_t cmd,
                                      const uint8_t *send_data,
                                      uint8_t send_len,
                                      uint8_t *recv_data,
                                      uint8_t recv_len)
{
    uint8_t irq, wait_irq;
    uint32_t timeout;

    switch (cmd) {
        case RC522_PCD_TRANSCEIVE:
            wait_irq = 0x30;  /* RxIRq (bit 5) + IdleIRq (bit 4) */
            break;
        case RC522_PCD_AUTHENT:
            wait_irq = 0x12;  /* CRCIRq (bit 1) + IdleIRq (bit 4) */
            break;
        default:
            wait_irq = 0x00;
            break;
    }

    /* Clear interrupts and reset state */
    rfid_rc522_write_reg(dev, RC522_REG_COMM_IRQ, 0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_DIV_IRQ, 0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    rc522_clear_fifo(dev);

    if (send_len > 0) {
        rc522_write_fifo(send_data, send_len);
    }

    /* Start the command */
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, cmd);

    /* StartSend bit must be set AFTER command for transceive */
    if (cmd == RC522_PCD_TRANSCEIVE) {
        uint8_t current = rfid_rc522_read_reg(RC522_REG_BIT_FRAMING);
        rfid_rc522_write_reg(dev, RC522_REG_BIT_FRAMING, current | 0x80);
    }

    /* Wait for RxIRq (data received) or IdleIRq (command finished) */
    timeout = 20000;  /* Increased timeout */
    do {
        irq = rfid_rc522_read_reg(RC522_REG_COMM_IRQ);
        timeout--;
    } while (!(irq & wait_irq) && timeout);

    /* Read actual error register */
    _error_code = rfid_rc522_read_reg(RC522_REG_ERROR);

    /* Debug output */
    {
        uint8_t fifo = rc522_fifo_count();
        uart_send_string("[XFER] IRQ:");
        uart_send_hex(irq);
        uart_send_string(" TO:");
        uart_send_int(timeout);
        uart_send_string(" FIFO:");
        uart_send_hex(fifo);
        uart_send_string(" ERR:");
        uart_send_hex(_error_code);
        uart_send_string("\r\n");
    }

    if (timeout == 0) {
        return RC522_STATUS_TIMEOUT;
    }

    /* Check IRQ bits (per MFRC522 library):
     *   0x10 = RxIRq  - data received in FIFO
     *   0x20 = IdleIRq - command finished
     *   0x04 = HiAlertIRq  */
    if (irq & 0x10) {
        /* RxIRq = card responded with data in FIFO */
        if (cmd == RC522_PCD_TRANSCEIVE && recv_len > 0 && recv_data != NULL) {
            uint8_t count = rc522_fifo_count();
            if (count > recv_len) count = recv_len;
            if (count > 0) {
                rc522_read_fifo(recv_data, count);
            }
        }
        
        if (_error_code & 0x13) {
            return RC522_STATUS_ERROR;
        }
        return RC522_STATUS_OK;
    }

    /* IdleIRq (0x20) without RxIRq = command finished, no response */
    if (irq & 0x20) {
        if (_error_code & 0x13) {
            return RC522_STATUS_ERROR;
        }
        return RC522_STATUS_TIMEOUT;  /* Card didn't respond */
    }

    /* Any other IRQ without expected bits = unknown, treat as error */
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    return RC522_STATUS_ERROR;
}

/* ============================================================================
 * FONCTIONS PUBLIQUES
 * ============================================================================ */

RC522_Status rfid_rc522_init(MFRC522_t *dev)
{
    

    LOG_DEBUG("MFRC522 Min Init started");
    LOG_DEBUG(" Starting hardware initialization ...");
    gpio_clear(dev->rst_Port, dev->rst_Pin);
    delay_ms(10);    
    gpio_set(dev->rst_Port, dev->rst_Pin);
    delay_ms(50);    
    
   
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_RESET);
    delay_ms(50);
    LOG_DEBUG("Hardware reset complete");


    /* Configuration CRC - disabled for REQA/anticollision */
    rfid_rc522_write_reg(dev, RC522_REG_TMode,      0x80);
    rfid_rc522_write_reg(dev, RC522_REG_TPrescaler, 0xA9);
    rfid_rc522_write_reg(dev, RC522_REG_TReloadH,   0x03);
    rfid_rc522_write_reg(dev, RC522_REG_TReloadL,   0xE8);
    rfid_rc522_write_reg(dev, RC522_REG_TX_ASK,     0x40);
    rfid_rc522_write_reg(dev, RC522_REG_RX_GAIN,    0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_ModWidth,   0x26);  // ajouter
    rfid_rc522_write_reg(dev, RC522_REG_GsN,        0x48);  // ModGsN corrigé 


    /* Forcer TxControlReg avant AntenneOn */
    rfid_rc522_write_reg(dev, RC522_REG_TX_CONTROL, 0x83);
    delay_ms(10);

    /* Lire la version de MFRC522 */
    uint8_t version = rfid_rc522_read_reg(RC522_REG_VERSION);
    if ((version != 0x91) && (version != 0x92)) {
        LOG_DEBUG_INT("Warning: unexpected version 0x%02X", version);
        return RC522_STATUS_ERROR; 
    } else {
        LOG_DEBUG_INT("Version: 0x%02X OK", version);
        LOG_INFO("RC522 initialisé");
        return RC522_STATUS_OK;
    } 
}

void rfid_rc522_reset(MFRC522_t *dev)
{
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_RESET);
    for (volatile int i = 0; i < 10000; i++) { }
}

RC522_Status rfid_rc522_antenna_on(MFRC522_t *dev)
{
    uint8_t val = rfid_rc522_read_reg(RC522_REG_TX_CONTROL);
    if ((val & 0x03) != 0x03) {
        rc522_set_bits(dev, RC522_REG_TX_CONTROL, 0x03, 0x03);
    }
    return RC522_STATUS_OK;
}

void rfid_rc522_antenna_off(MFRC522_t *dev)
{
    rc522_set_bits(dev, RC522_REG_TX_CONTROL, 0x03, 0x00);
}

uint8_t rfid_rc522_get_version(void)
{
    return rfid_rc522_read_reg(RC522_REG_VERSION);
}

RC522_Status rfid_rc522_request(MFRC522_t *dev, uint8_t *atqa)
{
    uint8_t recv_data[4];
    RC522_Status status;

    /* Reset RC522 state before request */
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    rc522_clear_fifo(dev);
    rfid_rc522_write_reg(dev, RC522_REG_COMM_IRQ, 0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_DIV_IRQ, 0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_ERROR, 0x00);
    for (volatile int i = 0; i < 500; i++) { }

    /* RF config: no CRC, 106 kbps */
    rfid_rc522_write_reg(dev, RC522_REG_TX_MODE, 0x00);
    rfid_rc522_write_reg(dev, RC522_REG_RX_MODE, 0x00);
    rfid_rc522_write_reg(dev, RC522_REG_TX_ASK, 0x40);

    /* REQA is a 7-bit short frame command */
    rfid_rc522_write_reg(dev, RC522_REG_BIT_FRAMING, 0x07);  /* 7 bits for last byte */

    uint8_t cmd = PICC_CMD_REQA;
    status = rc522_transceive(dev, RC522_PCD_TRANSCEIVE, &cmd, 1, recv_data, 2);

    /* Restore BitFraming to 8 bits for subsequent commands */
    rfid_rc522_write_reg(dev, RC522_REG_BIT_FRAMING, 0x00);

    if (status == RC522_STATUS_OK) {
        atqa[0] = recv_data[0];
        atqa[1] = recv_data[1];
        uart_send_string("[REQUEST] ATQA: ");
        uart_send_hex(atqa[0]);
        uart_send_string(" ");
        uart_send_hex(atqa[1]);
        uart_send_string("\r\n");
    }

    return status;
}

RC522_Status rfid_rc522_anticoll(MFRC522_t *dev, RC522_UID *uid)
{
    uint8_t send_data[9];
    uint8_t recv_data[12];
    RC522_Status status;

    uid->size = 0;
    for (int i = 0; i < RC522_UID_MAX_SIZE; i++) {
        uid->uid[i] = 0;
    }

    /* Thorough reset of RC522 state before anticollision */
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    rc522_clear_fifo(dev);
    rfid_rc522_write_reg(dev, RC522_REG_COMM_IRQ, 0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_DIV_IRQ, 0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_ERROR, 0x00);
    
    /* Reset CRC and mode registers */
    rfid_rc522_write_reg(dev, RC522_REG_RX_MODE, 0x00);
    rfid_rc522_write_reg(dev, RC522_REG_TX_MODE, 0x00);
    
    /* Small delay for RC522 to settle */
    for (volatile int i = 0; i < 500; i++) { }

    send_data[0] = PICC_CMD_ANTICOLL_1;
    send_data[1] = 0x20;  /* NVB: 2 bytes */

    /* Set BitFraming to 0x00 for anticollision (per reference) */
    rfid_rc522_write_reg(dev, RC522_REG_BIT_FRAMING, 0x00);

    status = rc522_transceive(dev,
                               RC522_PCD_TRANSCEIVE,
                               send_data, 2,
                               recv_data, 12);

    if (status == RC522_STATUS_OK) {
        /* Log ERROR register for debugging */
        uint8_t error = rfid_rc522_read_reg(RC522_REG_ERROR);
        uart_send_string("[ANTICOLL] Error: ");
        uart_send_hex(error);
        uart_send_string("\r\n");
        
        /* Check for collision */
        if (error & 0x01) {
            uart_send_string("[ANTICOLL] Collision!\r\n");
            return RC522_STATUS_COLLISION;
        }
        
        /* Validate UID - reject 0xFF which indicates invalid data */
        if (recv_data[0] == 0xFF && recv_data[1] == 0xFF && 
            recv_data[2] == 0xFF && recv_data[3] == 0xFF) {
            uart_send_string("[ANTICOLL] Invalid UID (all 0xFF)\r\n");
            return RC522_STATUS_ERROR;
        }
        
        /* Data is already in recv_data from transceive */
        /* First byte is UID bytes, 5th byte is BCC */
        uid->size = 4;
        for (int i = 0; i < 4; i++) {
            uid->uid[i] = recv_data[i];
        }
    }

    return status;
}

RC522_Status rfid_rc522_select(MFRC522_t *dev, RC522_UID *uid)
{
    uint8_t send_data[9];
    uint8_t recv_data[4];
    RC522_Status status;

    /* Reset RC522 state before select */
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    rc522_clear_fifo(dev);
    rfid_rc522_write_reg(dev, RC522_REG_COMM_IRQ, 0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_ERROR, 0x00);

    /* Calculate BCC (Block Check Character) */
    uint8_t bcc = uid->uid[0] ^ uid->uid[1] ^ uid->uid[2] ^ uid->uid[3];

    send_data[0] = PICC_CMD_SELECT_CL1;
    send_data[1] = 0x70;  /* NVB: 7 bytes valid */
    for (int i = 0; i < 4; i++) {
        send_data[2 + i] = uid->uid[i];
    }
    send_data[6] = bcc;  /* BCC */

    /* Enable StartSend bit for select */
    rc522_set_bits(dev, RC522_REG_BIT_FRAMING, 0x80, 0x80);

    status = rc522_transceive(dev,
                               RC522_PCD_TRANSCEIVE,
                               send_data, 7,
                               recv_data, 4);

    if (status == RC522_STATUS_OK) {
        uid->sak = recv_data[0];
    }

    return status;
}

RC522_Status rfid_rc522_auth(MFRC522_t *dev, uint8_t block, uint8_t key_type,
                              const RC522_Key *key, const RC522_UID *uid)
{
    uint8_t send_data[12];
    RC522_Status status;

    /* Prepare authentication command */
    send_data[0] = key_type;
    send_data[1] = block;

    /* Copy key (6 bytes) */
    for (int i = 0; i < RC522_KEY_SIZE; i++) {
        send_data[2 + i] = key->key[i];
    }

    /* Copy UID (4 bytes) */
    for (int i = 0; i < 4; i++) {
        send_data[8 + i] = uid->uid[i];
    }

    /* Clear any error flags first */
    rfid_rc522_write_reg(dev, RC522_REG_ERROR, 0x00);

    /* Write data to FIFO first */
    rc522_clear_fifo(dev);
    rc522_write_fifo(send_data, 12);

    /* Start authentication command */
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_AUTHENT);

    /* Wait for completion */
    for (volatile int i = 0; i < 5000; i++) { }

    /* Check if authentication was successful by reading Crypto1 status */
    uint8_t crypto_status = rfid_rc522_read_reg(RC522_REG_CRYPT_STATUS);
    if (crypto_status & 0x08) {
        return RC522_STATUS_OK;
    }

    /* Check for errors */
    uint8_t error = rfid_rc522_read_reg(RC522_REG_ERROR);
    if (error) {
        return RC522_STATUS_ERROR;
    }

    return RC522_STATUS_MIFARE_AUTH_ERROR;
}

RC522_Status rfid_rc522_read_block(MFRC522_t *dev, uint8_t block, uint8_t *data)
{
    uint8_t send_data[4];
    uint8_t recv_data[RC522_BLOCK_SIZE + 2];
    RC522_Status status;

    send_data[0] = PICC_CMD_MIFARE_READ;
    send_data[1] = block;

    status = rc522_transceive(dev,
                               RC522_PCD_TRANSCEIVE,
                               send_data, 2,
                               recv_data, RC522_BLOCK_SIZE + 2);

    if (status == RC522_STATUS_OK) {
        for (int i = 0; i < RC522_BLOCK_SIZE; i++) {
            data[i] = recv_data[i];
        }
    }

    return status;
}

RC522_Status rfid_rc522_write_block(MFRC522_t *dev, uint8_t block, const uint8_t *data)
{
    uint8_t send_data[4];
    uint8_t recv_data[4];
    RC522_Status status;

    send_data[0] = PICC_CMD_MIFARE_WRITE;
    send_data[1] = block;

    status = rc522_transceive(dev,
                               RC522_PCD_TRANSCEIVE,
                               send_data, 2,
                               recv_data, 1);

    if (status != RC522_STATUS_OK || recv_data[0] != 0x0A) {
        return RC522_STATUS_ERROR;
    }

    rc522_clear_fifo(dev);
    rc522_write_fifo(data, RC522_BLOCK_SIZE);

    rfid_rc522_write_reg(dev, RC522_REG_COMM_IRQ, 0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_TRANSCEIVE);
    rc522_set_bits(dev, RC522_REG_BIT_FRAMING, 0x07, 0x00);

    volatile uint32_t timeout = 10000;
    while (!(rfid_rc522_read_reg(RC522_REG_COMM_IRQ) & 0x30) && timeout--) { }

    uint8_t count = rc522_fifo_count();
    if (count > 0) {
        rc522_read_fifo(recv_data, 1);
        if (recv_data[0] == 0x0A) {
            return RC522_STATUS_OK;
        }
    }

    return RC522_STATUS_ERROR;
}

void rfid_rc522_halt(MFRC522_t *dev)
{
    uint8_t send_data[4];
    send_data[0] = PICC_CMD_HLTA;
    send_data[1] = 0x00;
    rc522_transceive(dev, RC522_PCD_TRANSCEIVE, send_data, 2, NULL, 0);
}

uint8_t rfid_rc522_get_error(void)
{
    return _error_code;
}

uint8_t rfid_rc522_get_crypto_status(void)
{
    return rfid_rc522_read_reg(RC522_REG_CRYPT_STATUS);
}
