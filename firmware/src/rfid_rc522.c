/**
 * @file rfid_rc522.c
 * @brief Implémentation du driver pour le module RFID RC522 (MFRC522).
 *        Communication SPI via spi.h.
 */

#include "rfid_rc522.h"
#include "log.h"
#include "uart.h"
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
 * FONCTIONS DE BASE : LECTURE/ÉCRITURE REGISTRES
 * ============================================================================ */

/**
 * @brief Écrit un octet dans un registre du RC522.
 */
void rfid_rc522_write_reg(MFRC522_t *dev, uint8_t addr, uint8_t value)
{
    if (dev == NULL) {
        return;
    }

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
uint8_t rfid_rc522_read_reg(MFRC522_t *dev, uint8_t addr)
{
    if (dev == NULL) {
        return 0;
    }

    uint8_t value;
    gpio_clear(dev->cs_Port, dev->cs_Pin);
    for (volatile int i = 0; i < 2000; i++) { }
    rc522_spi_write(RC522_READ_ADDR(addr));
    for (volatile int i = 0; i < 1000; i++) { }
    value = rc522_spi_read();
    for (volatile int i = 0; i < 2000; i++) { }
    gpio_set(dev->cs_Port, dev->cs_Pin);
    for (volatile int i = 0; i < 10000; i++) { }
    return value;
}


/* ============================================================================
 * FONCTIONS PUBLIQUES
 * ============================================================================ */

RC522_Status rfid_rc522_init(MFRC522_t *dev)
{
    if (dev == NULL) {
        return RC522_STATUS_INVALID;
    }

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
    rfid_rc522_write_reg(dev, RC522_REG_RF_CFG,     0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_MOD_WIDTH,  0x26);  // ajouter
    rfid_rc522_write_reg(dev, RC522_REG_GSN,        0x48);  // ModGsN corrigé 


    /* Forcer TxControlReg avant AntenneOn */
    rfid_rc522_write_reg(dev, RC522_REG_TX_CONTROL, 0x83);
    delay_ms(10);

    /* Lire la version de MFRC522 */
    uint8_t version = rfid_rc522_read_reg(dev, RC522_REG_VERSION);
    if ((version != 0x91) && (version != 0x92)) {
        LOG_DEBUG_HEX("Warning: unexpected version :", version);
        return RC522_STATUS_ERROR; 
    } else {
        LOG_DEBUG_HEX("OK --> version :", version);
        LOG_INFO("RC522 initialisé");
        return RC522_STATUS_OK;
    } 
}

/**
 * @brief  Attend le retrait de la carte en sondant périodiquement sa présence via REQA.
 *         3 échecs consécutifs confirment le retrait effectif, puis nettoie l'état du module.
 * @param  dev  Pointeur vers le périphérique MFRC522.
 * @return RC522_STATUS_OK une fois la carte retirée.
 */
RC522_Status rfid_rc522_wait_card_removal(MFRC522_t *dev)
{
    if (dev == NULL) {
        return RC522_STATUS_INVALID;
    }

    LOG_INFO("Waiting for card removal...");
    uint8_t atqa[2];
    uint8_t missCount = 0; // Compteur d'échecs consécutifs de détection
    uint32_t start = systick_get_tick(); // 10 seconds max
    while (1)
    {
        if ((systick_get_tick() - start) >= 10000)
        {
            LOG_DEBUG("Card removal timeout ");
            return RC522_STATUS_ERROR; // Carte non retitée dans le temps        
        }

        if (rfid_rc522_request_a(dev, atqa) != RC522_STATUS_OK)
        {
            missCount++;
            if (missCount >= 3) // 3 timeouts consécutifs = carte vraiment retirée
            {
                // Nettoyage : commande Idle, reset IRQ et FIFO
                rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
                rfid_rc522_write_reg(dev, RC522_REG_COMM_IRQ, 0x7F);
                rfid_rc522_write_reg(dev, RC522_REG_FIFO_LEVEL, 0x80);
                // Désactive le chiffrement si une session crypto était active
                rfid_rc522_clear_bit_mask(dev, RC522_REG_STATUS_2, 0x08); // MFCrypto1On = 0
                LOG_INFO("Card removed");
                return RC522_STATUS_OK;
            }
        }
        else
        {
            missCount = 0; // Carte toujours présente, reset du compteur
        }

        delay_ms(100); // Sondage toutes les 100 ms
    }
}

/* Envoie une commande REQA et récupère l'ATQA (2 octets) de la carte.
 * Retourne STATUS_OK si une carte valide répond, STATUS_ERROR ou STATUS_TIMEOUT sinon. */
RC522_Status rfid_rc522_request_a(MFRC522_t *dev, uint8_t *atqa) {
    if (dev == NULL || atqa == NULL) {
        return RC522_STATUS_INVALID;
    }

    LOG_DEBUG("RequestA");

    /* Réinitialisation : arrêt, clear IRQ, flush FIFO */
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND,    RC522_PCD_IDLE);
    rfid_rc522_write_reg(dev, RC522_REG_COMM_IRQ,   0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_FIFO_LEVEL, 0x80);

    /* Trame courte 7 bits (exigence ISO 14443-A), puis dépôt du REQA dans le FIFO */
    rfid_rc522_write_reg(dev, RC522_REG_BIT_FRAMING, 0x07);
    rfid_rc522_write_reg(dev, RC522_REG_FIFO_DATA,   PICC_CMD_REQA);

    /* Lancement de l'émission RF */
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND,     RC522_PCD_TRANSCEIVE);
    rfid_rc522_write_reg(dev, RC522_REG_BIT_FRAMING, 0x87); /* bit StartSend = 1 */

    /* Attente réponse carte : RxIRq (données reçues) ou LoAlertIrq, 25 ms max */
    uint32_t start = systick_get_tick();
    uint8_t irq = 0;
    while ((systick_get_tick() - start) < 25) {
        irq = rfid_rc522_read_reg(dev, RC522_REG_COMM_IRQ);
        if (irq & RC522_IRQ_RX) break;       /* RxIRq  : données reçues */
        if (irq & RC522_IRQ_LOALERT) break;  /* LoAlertIrq : FIFO vide */
        delay_ms(1);
    }

    /* Lecture état final : erreurs et nombre d'octets dans le FIFO */
    uint8_t err     = rfid_rc522_read_reg(dev, RC522_REG_ERROR);
    uint8_t fifoLvl = rfid_rc522_read_reg(dev, RC522_REG_FIFO_LEVEL);
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE); /* retour au repos */
    LOG_DEBUG_HEX("RequestA IRQ: ", irq);
    LOG_DEBUG_HEX("RequestA ERR: ", err);
    LOG_DEBUG_HEX("RequestA FIFO: ", fifoLvl);

    /* Erreur RF détectée (parité, CRC, collision, overflow) */
    if (err & 0x1B) {
        LOG_DEBUG_HEX("RequestA error: ", err);
        return RC522_STATUS_ERROR;
    }

    /* Données reçues et FIFO contient bien les 2 octets attendus */
    if ((irq & RC522_IRQ_RX) && fifoLvl >= 2) {
        atqa[0] = rfid_rc522_read_reg(dev, RC522_REG_FIFO_DATA);
        atqa[1] = rfid_rc522_read_reg(dev, RC522_REG_FIFO_DATA);

        /* Réponse nulle = faux positif, on rejette */
        if (atqa[0] == 0x00 && atqa[1] == 0x00) {
            LOG_DEBUG("RequestA fake response");
            return RC522_STATUS_TIMEOUT;
        }

        LOG_DEBUG_HEX("RequestA ATQA[0]: ", atqa[0]);
        LOG_DEBUG_HEX("RequestA ATQA[1]: ", atqa[1]);
        return RC522_STATUS_OK;
    }

    /* Aucune carte détectée dans le délai imparti */
    LOG_DEBUG("RequestA timeout");
    return RC522_STATUS_TIMEOUT;
}

/**
 * @brief Efface un ou plusieurs bits d'un registre du MFRC522.
 *
 * @param[in] dev   Pointeur vers la structure MFRC522 (périphérique cible).
 * @param[in] reg   Adresse du registre à modifier.
 * @param[in] mask  Masque des bits à effacer (1 = bit effacé, 0 = bit inchangé).
 *
 * @return void
 */
void rfid_rc522_clear_bit_mask(MFRC522_t *dev, uint8_t reg, uint8_t mask)
{
    if (dev == NULL) {
        return;
    }

    uint8_t tmp = rfid_rc522_read_reg(dev, reg);
    rfid_rc522_write_reg(dev, reg, tmp & (~mask));
    LOG_DEBUG_HEX("ClearBitMask reg: ", reg);
    LOG_DEBUG_HEX("ClearBitMask mask: ", mask);
}

/**
 * @brief  Met à 1 les bits spécifiés par mask dans un registre, sans altérer les autres bits.
 * @param  dev   Pointeur vers le périphérique MFRC522.
 * @param  reg   Adresse du registre cible.
 * @param  mask  Masque de bits à forcer à 1 (opération OR).
 */
void rfid_rc522_set_bit_mask(MFRC522_t *dev, uint8_t reg, uint8_t mask) {
    if (dev == NULL) {
        return;
    }

    uint8_t tmp = rfid_rc522_read_reg(dev, reg); // Lecture de la valeur actuelle du registre
    rfid_rc522_write_reg(dev, reg, tmp | mask);  // Fusion par OR : force les bits du mask à 1
    LOG_DEBUG_HEX("SetBitMask reg: ", reg);
    LOG_DEBUG_HEX("SetBitMask mask: ", mask);
}

/**
 * @brief  Calcule le CRC_A ISO 14443-A via le coprocesseur CRC du MFRC522.
 * @param  dev   Pointeur vers le périphérique MFRC522.
 * @param  data  Données à traiter.
 * @param  len   Nombre d'octets à traiter.
 * @param  crc   Buffer de sortie: crc[0]=LSB, crc[1]=MSB.
 * @return STATUS_OK en cas de succès, STATUS_TIMEOUT ou STATUS_INVALID sinon.
 */
static RC522_Status __attribute__((unused)) rfid_rc522_calc_crc_a(MFRC522_t *dev,
                                                                  const uint8_t *data,
                                                                  uint8_t len,
                                                                  uint8_t crc[2])
{
    if (dev == NULL || data == NULL || crc == NULL || len == 0) {
        return RC522_STATUS_INVALID;
    }

    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    rfid_rc522_write_reg(dev, RC522_REG_DIV_IRQ, RC522_DIV_IRQ_CRC);
    rfid_rc522_write_reg(dev, RC522_REG_FIFO_LEVEL, RC522_FIFO_FLUSH);
    rfid_rc522_write_reg(dev, RC522_REG_MODE, RC522_MODE_CRC_PRESET_6363);

    for (uint8_t i = 0; i < len; i++) {
        rfid_rc522_write_reg(dev, RC522_REG_FIFO_DATA, data[i]);
    }

    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_CALC_CRC);

    uint32_t start = systick_get_tick();
    while ((systick_get_tick() - start) < 25) {
        uint8_t irq = rfid_rc522_read_reg(dev, RC522_REG_DIV_IRQ);
        if (irq & RC522_DIV_IRQ_CRC) {
            rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
            crc[0] = rfid_rc522_read_reg(dev, RC522_REG_CRC_RESULT_L);
            crc[1] = rfid_rc522_read_reg(dev, RC522_REG_CRC_RESULT_H);
            return RC522_STATUS_OK;
        }

        delay_ms(1);
    }

    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    return RC522_STATUS_TIMEOUT;
}

/**
 * @brief  Sélectionne un niveau cascade et lit le SAK.
 * @param  dev        Pointeur vers le périphérique MFRC522.
 * @param  sel_cmd    Commande SELECT CL1/CL2/CL3.
 * @param  uid_frame  UID partiel + BCC (5 octets).
 * @param  sak        Buffer de sortie SAK.
 * @return STATUS_OK en cas de succès, sinon code d'erreur.
 */
static RC522_Status rfid_rc522_select_level(MFRC522_t *dev,
                                            uint8_t sel_cmd,
                                            const uint8_t uid_frame[PICC_UID_FRAME_SIZE],
                                            uint8_t *sak)
{
    if (dev == NULL || uid_frame == NULL || sak == NULL) {
        return RC522_STATUS_INVALID;
    }

    uint8_t frame[9];
    uint8_t crc[2];

    frame[0] = sel_cmd;
    frame[1] = PICC_NVB_SELECT;
    for (uint8_t i = 0; i < PICC_UID_FRAME_SIZE; i++) {
        frame[i + 2] = uid_frame[i];
    }

    RC522_Status status = rfid_rc522_calc_crc_a(dev, frame, 7, crc);
    if (status != RC522_STATUS_OK) {
        return status;
    }

    frame[7] = crc[0];
    frame[8] = crc[1];

    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    rfid_rc522_write_reg(dev, RC522_REG_COMM_IRQ, RC522_IRQ_CLEAR);
    rfid_rc522_write_reg(dev, RC522_REG_FIFO_LEVEL, RC522_FIFO_FLUSH);
    rfid_rc522_write_reg(dev, RC522_REG_BIT_FRAMING, 0x00);

    for (uint8_t i = 0; i < sizeof(frame); i++) {
        rfid_rc522_write_reg(dev, RC522_REG_FIFO_DATA, frame[i]);
    }

    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_TRANSCEIVE);
    rfid_rc522_set_bit_mask(dev, RC522_REG_BIT_FRAMING, 0x80);

    uint32_t start = systick_get_tick();
    while ((systick_get_tick() - start) < 25) {
        uint8_t irq = rfid_rc522_read_reg(dev, RC522_REG_COMM_IRQ);

        if (irq & RC522_IRQ_TRANSCEIVE_FAIL) {
            rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
            return (irq & RC522_IRQ_TIMER) ? RC522_STATUS_TIMEOUT : RC522_STATUS_ERROR;
        }

        if (irq & RC522_IRQ_TRANSCEIVE_DONE) {
            uint8_t err = rfid_rc522_read_reg(dev, RC522_REG_ERROR);
            if (err & 0x1D) {
                rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
                return RC522_STATUS_ERROR;
            }

            uint8_t fifoLvl = rfid_rc522_read_reg(dev, RC522_REG_FIFO_LEVEL);
            if (fifoLvl < 1) {
                rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
                return RC522_STATUS_INVALID_UID;
            }

            *sak = rfid_rc522_read_reg(dev, RC522_REG_FIFO_DATA);
            rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
            return RC522_STATUS_OK;
        }

        delay_ms(1);
    }

    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    return RC522_STATUS_TIMEOUT;
}

/**
 * @brief  Éteint l'antenne RF en désactivant les pilotes Tx1 et Tx2.
 * @param  dev  Pointeur vers le périphérique MFRC522.
 */
void rfid_rc522_antenna_off(MFRC522_t *dev) {
    if (dev == NULL) {
        return;
    }

    rfid_rc522_clear_bit_mask(dev, RC522_REG_TX_CONTROL, 0x03); // Bits 0-1 à 0 : désactive Tx1 et Tx2
    LOG_DEBUG("Antenna off");
}

/**
 * @brief  Allume l'antenne RF en activant les pilotes Tx1 et Tx2.
 * @param  dev  Pointeur vers le périphérique MFRC522.
 */
void rfid_rc522_antenna_on(MFRC522_t *dev) {
    if (dev == NULL) {
        return;
    }

    rfid_rc522_set_bit_mask(dev, RC522_REG_TX_CONTROL, 0x03); // Bits 0-1 à 1 : active Tx1 et Tx2
    LOG_DEBUG("Antenna on");
}

/**
 * @brief Détecte la présence d'une carte RFID en une seule tentative.
 *
 * Réinitialise l'état interne du MFRC522 (commande, interruptions, FIFO,
 * chiffrement), puis envoie une trame REQA (ISO 14443A) pour sonder le champ RF.
 *
 * @param[in] dev  Pointeur vers la structure MFRC522 (périphérique cible).
 *
 * @return STATUS_OK      Une carte a répondu à la requête REQA.
 * @return STATUS_TIMEOUT Aucune carte détectée dans le champ RF.
 */
RC522_Status rfid_rc522_poll_card(MFRC522_t *dev, uint8_t *atqa)
{
    if (dev == NULL || atqa == NULL) {
        return RC522_STATUS_INVALID;
    }

    rfid_rc522_antenna_on(dev);
    rfid_rc522_write_reg(dev,    RC522_REG_COMMAND,      RC522_PCD_IDLE);
    rfid_rc522_write_reg(dev,    RC522_REG_COMM_IRQ,     0x7F);
    rfid_rc522_write_reg(dev,    RC522_REG_FIFO_LEVEL,   0x80);
    rfid_rc522_clear_bit_mask(dev, RC522_REG_STATUS_2,     0x08);
    return rfid_rc522_request_a(dev, atqa);
}

/**
 * @brief Réinitialise le module MFRC522 après une erreur ou un blocage.
 *        Effectue un soft reset, attend la stabilisation, puis relance
 *        l'initialisation complète et réactive l'antenne.
 * @param dev Pointeur vers la structure du périphérique MFRC522.
 */
void rfid_rc522_recover(MFRC522_t *dev)
{
    if (dev == NULL) {
        return;
    }

    LOG_DEBUG("MFRC522 recovering...");
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_RESET); // Soft reset matériel
    delay_ms(50);                                                 // Attente stabilisation (≥50 ms)
    (void)rfid_rc522_init(dev);                                   // Réinitialisation complète
    rfid_rc522_antenna_on(dev);                                   // Réactivation de l'antenne RF
    LOG_DEBUG("MFRC522 recovered");
}


/**
 * @brief  Exécute l'anticollision pour un niveau cascade donné.
 * @param  dev  Pointeur vers le périphérique MFRC522.
 * @param  cmd  Commande anticollision CL1/CL2/CL3.
 * @param  uid  Buffer de sortie: UID partiel + BCC.
 * @return STATUS_OK en cas de succès, STATUS_ERROR si erreur RF, BCC invalide ou timeout.
 */
static RC522_Status rfid_rc522_anticoll_level(MFRC522_t *dev, uint8_t cmd, uint8_t *uid)
{
    if (dev == NULL || uid == NULL) {
        return RC522_STATUS_INVALID;
    }

    LOG_DEBUG("Anticoll");

    // Préparation du module : reset des IRQ, vidage FIFO, trame complète
    rfid_rc522_write_reg(dev, RC522_REG_COMM_IRQ, 0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_FIFO_LEVEL, 0x80);
    rfid_rc522_write_reg(dev, RC522_REG_BIT_FRAMING, 0x00);

    // Commande anticollision : SEL CLx + NVB 0x20 (2 octets, pas de CRC)
    rfid_rc522_write_reg(dev, RC522_REG_FIFO_DATA, cmd);
    rfid_rc522_write_reg(dev, RC522_REG_FIFO_DATA, PICC_NVB_ANTICOLL);
    delay_ms(2);

    // Lancement de la transmission RF
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_TRANSCEIVE);
    rfid_rc522_set_bit_mask(dev, RC522_REG_BIT_FRAMING, 0x80); // Start Send

    // Attente de fin de commande via CommIrqReg (RxIRq/IdleIRq) ou erreur/timeout
    uint32_t start = systick_get_tick();
    while ((systick_get_tick() - start) < 25) {

        uint8_t irq = rfid_rc522_read_reg(dev, RC522_REG_COMM_IRQ);

        if (irq & RC522_IRQ_TRANSCEIVE_FAIL) {
            LOG_DEBUG_HEX("Anticoll IRQ error/timeout: ", irq);
            rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
            return (irq & RC522_IRQ_TIMER) ? RC522_STATUS_TIMEOUT : RC522_STATUS_ERROR;
        }

        if (irq & RC522_IRQ_TRANSCEIVE_DONE) { // Réponse reçue ou commande terminée

            // Vérification des erreurs (collision, parité, protocole…)
            uint8_t err = rfid_rc522_read_reg(dev, RC522_REG_ERROR);
            if (err & 0x1D) {
                LOG_DEBUG_HEX("Anticoll error: ", err);
                rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
                return RC522_STATUS_ERROR;
            }

            // Vérification que le FIFO contient bien UID partiel + BCC
            uint8_t fifoLvl = rfid_rc522_read_reg(dev, RC522_REG_FIFO_LEVEL);
            if (fifoLvl == PICC_UID_FRAME_SIZE) {

                // Lecture des octets depuis le FIFO
                for (uint8_t i = 0; i < PICC_UID_FRAME_SIZE; i++)
                    uid[i] = rfid_rc522_read_reg(dev, RC522_REG_FIFO_DATA);

                // Validation du BCC : XOR des 4 octets UID du niveau courant
                uint8_t calcBcc = 0;
                for (uint8_t i = 0; i < PICC_UID_PART_SIZE; i++) {
                    calcBcc ^= uid[i];
                }

                if (uid[PICC_UID_PART_SIZE] != calcBcc) {
                    LOG_DEBUG_HEX("Anticoll bad BCC calc: ", calcBcc);
                    LOG_DEBUG_HEX("Anticoll bad BCC got: ", uid[PICC_UID_PART_SIZE]);
                    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
                    return RC522_STATUS_BCC_MISMATCH;
                }

               // LOG_DEBUG_HEX("Anticoll UID[0]: ", uid[0]);
               //LOG_DEBUG_HEX("Anticoll UID[1]: ", uid[1]);
               // LOG_DEBUG_HEX("Anticoll UID[2]: ", uid[2]);
               // LOG_DEBUG_HEX("Anticoll UID[3]: ", uid[3]);
               // LOG_DEBUG_HEX("Anticoll BCC: ", uid[4]);
                rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
                delay_ms(2);
                return RC522_STATUS_OK;
            }

            // FIFO inattendu : réinitialisation et rallumage antenne
            LOG_DEBUG_HEX("Anticoll bad FIFO level: ", fifoLvl);
            rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
            rfid_rc522_antenna_on(dev);
            return RC522_STATUS_INVALID_UID;
        }
        delay_ms(1);
    }

    // Timeout dépassé sans réponse de la carte
    LOG_DEBUG("Anticoll timeout");
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    return RC522_STATUS_TIMEOUT; 
}

/**
 * @brief  Exécute l'anticollision CL1 et récupère UID + BCC.
 * @param  dev  Pointeur vers le périphérique MFRC522.
 * @param  uid  Buffer de sortie (5 octets) : UID[0..3] + BCC.
 * @return STATUS_OK en cas de succès, sinon code d'erreur.
 */
RC522_Status rfid_rc522_anticoll_raw(MFRC522_t *dev, uint8_t *uid)
{
    return rfid_rc522_anticoll_level(dev, PICC_CMD_CL1, uid);
}

/**
 * @brief  Lit UID 4/7 octets et SAK en réutilisant l'ATQA du polling.
 * @param  dev  Pointeur vers le périphérique MFRC522.
 * @param  uid  Structure de sortie UID complète.
 * @param  atqa ATQA obtenu pendant le polling.
 * @return STATUS_OK en cas de succès, sinon code d'erreur.
 */
RC522_Status rfid_rc522_read_uid_full(MFRC522_t *dev, RC522_UID *uid, const uint8_t *atqa)
{
    if (dev == NULL || uid == NULL || atqa == NULL) {
        return RC522_STATUS_INVALID;
    }

    for (uint8_t i = 0; i < RC522_UID_MAX_SIZE; i++) {
        uid->uid[i] = 0;
    }
    uid->size = 0;
    uid->sak = 0;
    uid->atqa[0] = atqa[0];
    uid->atqa[1] = atqa[1];

    uint8_t rawUid[PICC_UID_FRAME_SIZE];
    RC522_Status status = rfid_rc522_anticoll_raw(dev, rawUid);
    if (status != RC522_STATUS_OK) {
        return status;
    }

    status = rfid_rc522_select_level(dev, PICC_CMD_SELECT_CL1, rawUid, &uid->sak);
    if (status != RC522_STATUS_OK) {
        return status;
    }

    LOG_DEBUG_HEX("UID SAK: ", uid->sak);

    if ((rawUid[0] == PICC_CASCADE_TAG) || (uid->sak & PICC_SAK_CASCADE)) {
        if (rawUid[0] != PICC_CASCADE_TAG) {
            return RC522_STATUS_INVALID_UID;
        }

        LOG_DEBUG("UID cascade CL2");
        uid->uid[0] = rawUid[1];
        uid->uid[1] = rawUid[2];
        uid->uid[2] = rawUid[3];

        uint8_t rawUidCl2[PICC_UID_FRAME_SIZE];
        status = rfid_rc522_anticoll_level(dev, PICC_CMD_CL2, rawUidCl2);
        if (status != RC522_STATUS_OK) {
            return status;
        }

        status = rfid_rc522_select_level(dev, PICC_CMD_SELECT_CL2, rawUidCl2, &uid->sak);
        if (status != RC522_STATUS_OK) {
            return status;
        }

        LOG_DEBUG_HEX("UID SAK CL2: ", uid->sak);

        if (uid->sak & PICC_SAK_CASCADE) {
            if (rawUidCl2[0] != PICC_CASCADE_TAG) {
                return RC522_STATUS_INVALID_UID;
            }

            LOG_DEBUG("UID cascade CL3");
            uid->uid[3] = rawUidCl2[1];
            uid->uid[4] = rawUidCl2[2];
            uid->uid[5] = rawUidCl2[3];

            uint8_t rawUidCl3[PICC_UID_FRAME_SIZE];
            status = rfid_rc522_anticoll_level(dev, PICC_CMD_CL3, rawUidCl3);
            if (status != RC522_STATUS_OK) {
                return status;
            }

            status = rfid_rc522_select_level(dev, PICC_CMD_SELECT_CL3, rawUidCl3, &uid->sak);
            if (status != RC522_STATUS_OK) {
                return status;
            }

            LOG_DEBUG_HEX("UID SAK CL3: ", uid->sak);

            if (uid->sak & PICC_SAK_CASCADE) {
                return RC522_STATUS_INVALID_UID;
            }

            for (uint8_t i = 0; i < PICC_UID_PART_SIZE; i++) {
                uid->uid[i + 6] = rawUidCl3[i];
            }
            uid->size = RC522_UID_TRIPLE_SIZE;
            LOG_DEBUG_INT("UID size: ", uid->size);
            return RC522_STATUS_OK;
        }

        for (uint8_t i = 0; i < PICC_UID_PART_SIZE; i++) {
            uid->uid[i + 3] = rawUidCl2[i];
        }
        uid->size = RC522_UID_DOUBLE_SIZE;
        LOG_DEBUG_INT("UID size: ", uid->size);
        return RC522_STATUS_OK;
    }

    for (uint8_t i = 0; i < RC522_UID_SINGLE_SIZE; i++) {
        uid->uid[i] = rawUid[i];
    }
    uid->size = RC522_UID_SINGLE_SIZE;

    return RC522_STATUS_OK;
}

/**
 * @brief  Lit l'UID d'une carte RFID via anticollision et le stocke dans uid[4].
 * @param  dev  Pointeur vers le périphérique MFRC522.
 * @param  uid  Buffer de sortie (4 octets) recevant l'UID de la carte.
 * @return STATUS_OK en cas de succès, STATUS_ERROR sinon.
 */
RC522_Status rfid_rc522_read_uid(MFRC522_t *dev, uint8_t *uid) {
    if (dev == NULL || uid == NULL) {
        return RC522_STATUS_INVALID;
    }

    LOG_DEBUG("Reading UID...");

    uint8_t rawUid[5]; // 4 octets UID + 1 octet BCC
    RC522_Status status = rfid_rc522_anticoll_raw(dev, rawUid);
    if (status != RC522_STATUS_OK) {
        switch (status) {
        case RC522_STATUS_TIMEOUT:
            LOG_DEBUG("Anticollision timeout");
            break;

        case RC522_STATUS_BCC_MISMATCH:
            LOG_DEBUG("Anticollision BCC mismatch");
            break;

        case RC522_STATUS_INVALID_UID:
            LOG_DEBUG("Anticollision invalid UID frame");
            break;

        case RC522_STATUS_ERROR:
            LOG_DEBUG("Anticollision RF/error status");
            break;

        default:
            LOG_DEBUG("Anticollision failed");
            break;
        }
        return status;
    }

    for (int i = 0; i < 4; i++)
        uid[i] = rawUid[i]; // Copie l'UID en ignorant le BCC (rawUid[4])

    LOG_DEBUG_HEX("Card UID[0]: ", uid[0]);
    LOG_DEBUG_HEX("Card UID[1]: ", uid[1]);
    LOG_DEBUG_HEX("Card UID[2]: ", uid[2]);
    LOG_DEBUG_HEX("Card UID[3]: ", uid[3]);
    return RC522_STATUS_OK;
}

RC522_CardType rfid_rc522_get_card_type(const RC522_UID *uid)
{
    if (uid == NULL) {
        return RC522_CARD_TYPE_UNKNOWN;
    }

    uint8_t sak = uid->sak & 0x7F;
    switch (sak) {
    case 0x04:
        return RC522_CARD_TYPE_NOT_COMPLETE;
    case 0x09:
        return RC522_CARD_TYPE_MIFARE_MINI;
    case 0x08:
        return RC522_CARD_TYPE_MIFARE_CLASSIC_1K;
    case 0x18:
        return RC522_CARD_TYPE_MIFARE_CLASSIC_4K;
    case 0x00:
        return RC522_CARD_TYPE_MIFARE_ULTRALIGHT;
    case 0x10:
    case 0x11:
        return RC522_CARD_TYPE_MIFARE_PLUS;
    case 0x20:
        return RC522_CARD_TYPE_ISO_14443_4;
    case 0x40:
        return RC522_CARD_TYPE_ISO_18092;
    default:
        return RC522_CARD_TYPE_UNKNOWN;
    }
}

const char *rfid_rc522_card_type_name(RC522_CardType type)
{
    switch (type) {
    case RC522_CARD_TYPE_MIFARE_MINI:
        return "MIFARE Mini";
    case RC522_CARD_TYPE_MIFARE_CLASSIC_1K:
        return "MIFARE Classic 1K";
    case RC522_CARD_TYPE_MIFARE_CLASSIC_4K:
        return "MIFARE Classic 4K";
    case RC522_CARD_TYPE_MIFARE_ULTRALIGHT:
        return "MIFARE Ultralight/NTAG";
    case RC522_CARD_TYPE_MIFARE_PLUS:
        return "MIFARE Plus";
    case RC522_CARD_TYPE_ISO_14443_4:
        return "ISO 14443-4";
    case RC522_CARD_TYPE_ISO_18092:
        return "ISO 18092";
    case RC522_CARD_TYPE_NOT_COMPLETE:
        return "UID not complete";
    default:
        return "Unknown card";
    }
}
