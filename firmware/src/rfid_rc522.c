/**
 * @file rfid_rc522.c
 * @brief Implémentation du driver pour le module RFID RC522 (MFRC522).
 *        Communication SPI via spi.h.
 */

#include "rfid_rc522.h"
#include "log.h"
#include <libopencm3/stm32/gpio.h>
#include <stddef.h>

/* ============================================================================
 * MACROS PRIVÉES
 * ============================================================================ */

/** @brief Bit pour lecture (addr pair) */
#define RC522_READ_BIT             0x80

/** @brief Bit pour écriture (addr impair) */
#define RC522_WRITE_BIT           0x00

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
static void rc522_write_reg(uint8_t addr, uint8_t value)
{
    RC522_CS_LOW();
    for (volatile int i = 0; i < 500; i++) { }
    spi_transfer(addr | RC522_WRITE_BIT);
    spi_transfer(value);
    RC522_CS_HIGH();
}

/**
 * @brief Lit un octet depuis un registre du RC522.
 */
static uint8_t rc522_read_reg(uint8_t addr)
{
    uint8_t value;
    RC522_CS_LOW();
    for (volatile int i = 0; i < 500; i++) { }
    spi_transfer(addr | RC522_READ_BIT);
    value = spi_transfer(0x00);
    RC522_CS_HIGH();
    return value;
}

/**
 * @brief Modifie des bits dans un registre.
 */
static void rc522_set_bits(uint8_t addr, uint8_t mask, uint8_t value)
{
    uint8_t tmp = rc522_read_reg(addr);
    tmp = (tmp & ~mask) | value;
    rc522_write_reg(addr, tmp);
}

/* ============================================================================
 * FONCTIONS FIFO
 * ============================================================================ */

static void rc522_write_fifo(const uint8_t *data, uint8_t len)
{
    RC522_CS_LOW();
    for (volatile int i = 0; i < 200; i++) { }
    spi_transfer(RC522_REG_FIFO_DATA | RC522_WRITE_BIT);
    for (uint8_t i = 0; i < len; i++) {
        spi_transfer(data[i]);
    }
    RC522_CS_HIGH();
}

static void rc522_read_fifo(uint8_t *data, uint8_t len)
{
    RC522_CS_LOW();
    for (volatile int i = 0; i < 200; i++) { }
    spi_transfer(RC522_REG_FIFO_DATA | RC522_READ_BIT);
    for (uint8_t i = 0; i < len; i++) {
        data[i] = spi_transfer(0x00);
    }
    RC522_CS_HIGH();
}

static uint8_t rc522_fifo_count(void)
{
    return rc522_read_reg(RC522_REG_FIFO_LEVEL) & 0x7F;
}

static void rc522_clear_fifo(void)
{
    rc522_set_bits(RC522_REG_FIFO_LEVEL, 0x80, 0x80);
}

/* ============================================================================
 * FONCTIONS DE COMMUNICATION
 * ============================================================================ */

static RC522_Status rc522_transceive(uint8_t cmd,
                                      const uint8_t *send_data,
                                      uint8_t send_len,
                                      uint8_t *recv_data,
                                      uint8_t recv_len)
{
    uint8_t irq, wait_irq;
    uint32_t timeout;

    switch (cmd) {
        case RC522_PCD_TRANSCEIVE:
            wait_irq = 0x30;
            break;
        case RC522_PCD_AUTHENT:
            wait_irq = 0x12;
            break;
        default:
            wait_irq = 0x00;
            break;
    }

    rc522_write_reg(RC522_REG_COMM_IRQ, 0x7F);
    rc522_write_reg(RC522_REG_DIV_IRQ, 0x7F);
    rc522_write_reg(RC522_REG_COMMAND, RC522_PCD_IDLE);
    rc522_clear_fifo();

    if (send_len > 0) {
        rc522_write_fifo(send_data, send_len);
    }

    rc522_write_reg(RC522_REG_COMMAND, cmd);

    if (cmd == RC522_PCD_TRANSCEIVE) {
        rc522_set_bits(RC522_REG_BIT_FRAMING, 0x07, 0x00);
    }

    timeout = 10000;
    do {
        irq = rc522_read_reg(RC522_REG_COMM_IRQ);
        timeout--;
    } while (!(irq & wait_irq) && timeout);

    _error_code = rc522_read_reg(RC522_REG_ERROR);
    if (_error_code & 0x13) {
        return RC522_STATUS_ERROR;
    }

    if (timeout == 0) {
        return RC522_STATUS_TIMEOUT;
    }

    if (irq & 0x01) {
        rc522_write_reg(RC522_REG_COMMAND, RC522_PCD_IDLE);
    }

    if (cmd == RC522_PCD_TRANSCEIVE && recv_len > 0) {
        uint8_t count = rc522_fifo_count();
        if (count > recv_len) {
            count = recv_len;
        }
        if (count > 0) {
            rc522_read_fifo(recv_data, count);
        }
    }

    return RC522_STATUS_OK;
}

/* ============================================================================
 * FONCTIONS PUBLIQUES
 * ============================================================================ */

RC522_Status rfid_rc522_init(void)
{
    rfid_rc522_reset();

    for (volatile int i = 0; i < 100000; i++) { }

    uint8_t version = rfid_rc522_get_version();
    LOG_DEBUG_INT("version", version);

    if (version != 0x80 && version != 0x88 && version != 0x90) {
        LOG_ERROR("RC522 non detecte");
        return RC522_STATUS_ERROR;
    }

    LOG_INFO("RC522 detecte");

    rc522_write_reg(0x2A, 0x03);
    rc522_write_reg(0x2B, 0x00);
    rc522_write_reg(0x2C, 0x30);

    rc522_set_bits(RC522_REG_TX_ASK, 0x40, 0x40);

    rc522_write_reg(RC522_REG_TX_CRC_INIT0, 0x63);
    rc522_write_reg(RC522_REG_TX_CRC_INIT1, 0x63);
    rc522_write_reg(RC522_REG_RX_CRC_PSEL, 0x00);

    rfid_rc522_antenna_on();

    LOG_INFO("RC522 initialise");
    return RC522_STATUS_OK;
}

void rfid_rc522_reset(void)
{
    rc522_write_reg(RC522_REG_COMMAND, RC522_PCD_RESET);
    for (volatile int i = 0; i < 10000; i++) { }
}

RC522_Status rfid_rc522_antenna_on(void)
{
    uint8_t val = rc522_read_reg(RC522_REG_TX_CONTROL);
    if ((val & 0x03) != 0x03) {
        rc522_set_bits(RC522_REG_TX_CONTROL, 0x03, 0x03);
    }
    return RC522_STATUS_OK;
}

void rfid_rc522_antenna_off(void)
{
    rc522_set_bits(RC522_REG_TX_CONTROL, 0x03, 0x00);
}

uint8_t rfid_rc522_get_version(void)
{
    return rc522_read_reg(RC522_REG_VERSION);
}

RC522_Status rfid_rc522_request(uint8_t *atqa)
{
    uint8_t send_data[2];
    uint8_t recv_data[4];
    RC522_Status status;

    rc522_write_reg(RC522_REG_RX_MODE, 0x07);

    send_data[0] = PICC_CMD_REQA;
    send_data[1] = 0;

    rc522_set_bits(RC522_REG_BIT_FRAMING, 0x07, 0x00);

    status = rc522_transceive(RC522_PCD_TRANSCEIVE,
                               send_data, 1,
                               recv_data, 2);

    if (status == RC522_STATUS_OK) {
        atqa[0] = recv_data[0];
        atqa[1] = recv_data[1];
    }

    return status;
}

RC522_Status rfid_rc522_anticoll(RC522_UID *uid)
{
    uint8_t send_data[9];
    uint8_t recv_data[12];
    RC522_Status status;

    uid->size = 0;
    for (int i = 0; i < RC522_UID_MAX_SIZE; i++) {
        uid->uid[i] = 0;
    }

    rc522_write_reg(RC522_REG_RX_MODE, 0x00);

    send_data[0] = PICC_CMD_ANTICOLL_1;
    send_data[1] = 0x20;  /* NVB: 2 bytes */

    rc522_set_bits(RC522_REG_BIT_FRAMING, 0x07, 0x00);

    status = rc522_transceive(RC522_PCD_TRANSCEIVE,
                               send_data, 2,
                               recv_data, 12);

    if (status == RC522_STATUS_OK) {
        uint8_t count = rc522_fifo_count();
        if (count >= 5) {
            uid->size = 4;
            for (int i = 0; i < 4; i++) {
                uid->uid[i] = recv_data[i];
            }
            /* BCC is at recv_data[4], can be used for verification */
        } else {
            return RC522_STATUS_ERROR;
        }
    }

    return status;
}

RC522_Status rfid_rc522_select(RC522_UID *uid)
{
    uint8_t send_data[9];
    uint8_t recv_data[4];
    RC522_Status status;

    /* Calculate BCC (Block Check Character) */
    uint8_t bcc = uid->uid[0] ^ uid->uid[1] ^ uid->uid[2] ^ uid->uid[3];

    send_data[0] = PICC_CMD_SELECT_CL1;
    send_data[1] = 0x70;  /* NVB: 7 bytes valid */
    for (int i = 0; i < 4; i++) {
        send_data[2 + i] = uid->uid[i];
    }
    send_data[6] = bcc;  /* BCC */

    rc522_set_bits(RC522_REG_BIT_FRAMING, 0x07, 0x00);

    status = rc522_transceive(RC522_PCD_TRANSCEIVE,
                               send_data, 7,
                               recv_data, 4);

    if (status == RC522_STATUS_OK) {
        uid->sak = recv_data[0];
    }

    return status;
}

RC522_Status rfid_rc522_auth(uint8_t block, uint8_t key_type,
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
    rc522_write_reg(RC522_REG_ERROR, 0x00);

    /* Write data to FIFO first */
    rc522_clear_fifo();
    rc522_write_fifo(send_data, 12);

    /* Start authentication command */
    rc522_write_reg(RC522_REG_COMMAND, RC522_PCD_AUTHENT);

    /* Wait for completion */
    for (volatile int i = 0; i < 5000; i++) { }

    /* Check if authentication was successful by reading Crypto1 status */
    uint8_t crypto_status = rc522_read_reg(RC522_REG_CRYPT_STATUS);
    if (crypto_status & 0x08) {
        return RC522_STATUS_OK;
    }

    /* Check for errors */
    uint8_t error = rc522_read_reg(RC522_REG_ERROR);
    if (error) {
        return RC522_STATUS_ERROR;
    }

    return RC522_STATUS_MIFARE_AUTH_ERROR;
}

RC522_Status rfid_rc522_read_block(uint8_t block, uint8_t *data)
{
    uint8_t send_data[4];
    uint8_t recv_data[RC522_BLOCK_SIZE + 2];
    RC522_Status status;

    send_data[0] = PICC_CMD_MIFARE_READ;
    send_data[1] = block;

    status = rc522_transceive(RC522_PCD_TRANSCEIVE,
                               send_data, 2,
                               recv_data, RC522_BLOCK_SIZE + 2);

    if (status == RC522_STATUS_OK) {
        for (int i = 0; i < RC522_BLOCK_SIZE; i++) {
            data[i] = recv_data[i];
        }
    }

    return status;
}

RC522_Status rfid_rc522_write_block(uint8_t block, const uint8_t *data)
{
    uint8_t send_data[4];
    uint8_t recv_data[4];
    RC522_Status status;

    send_data[0] = PICC_CMD_MIFARE_WRITE;
    send_data[1] = block;

    status = rc522_transceive(RC522_PCD_TRANSCEIVE,
                               send_data, 2,
                               recv_data, 1);

    if (status != RC522_STATUS_OK || recv_data[0] != 0x0A) {
        return RC522_STATUS_ERROR;
    }

    rc522_clear_fifo();
    rc522_write_fifo(data, RC522_BLOCK_SIZE);

    rc522_write_reg(RC522_REG_COMM_IRQ, 0x7F);
    rc522_write_reg(RC522_REG_COMMAND, RC522_PCD_TRANSCEIVE);
    rc522_set_bits(RC522_REG_BIT_FRAMING, 0x07, 0x00);

    volatile uint32_t timeout = 10000;
    while (!(rc522_read_reg(RC522_REG_COMM_IRQ) & 0x30) && timeout--) { }

    uint8_t count = rc522_fifo_count();
    if (count > 0) {
        rc522_read_fifo(recv_data, 1);
        if (recv_data[0] == 0x0A) {
            return RC522_STATUS_OK;
        }
    }

    return RC522_STATUS_ERROR;
}

void rfid_rc522_halt(void)
{
    uint8_t send_data[4];
    send_data[0] = PICC_CMD_HLTA;
    send_data[1] = 0x00;
    rc522_transceive(RC522_PCD_TRANSCEIVE, send_data, 2, NULL, 0);
}

uint8_t rfid_rc522_get_error(void)
{
    return _error_code;
}

uint8_t rfid_rc522_get_crypto_status(void)
{
    return rc522_read_reg(RC522_REG_CRYPT_STATUS);
}