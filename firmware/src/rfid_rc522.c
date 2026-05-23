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
    LOG_INFO("Waiting for card removal...");
    uint8_t atqa[2];
    uint8_t missCount = 0; // Compteur d'échecs consécutifs de détection
    uint32_t timeout = systick_get_tick() + 10000; // 10 seconds max
    while (1)
    {
        if (systick_get_tick() > timeout)
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

    /* Attente réponse carte : RxIRq (données reçues) ou TimerIRq (timeout), 25 ms max */
    uint32_t timeout = systick_get_tick() + 25;
    uint8_t irq = 0;
    while (systick_get_tick() < timeout) {
        irq = rfid_rc522_read_reg(dev, RC522_REG_COMM_IRQ);
        if (irq & 0x20) break;  /* RxIRq  : données reçues */
        if (irq & 0x04) break;  /* TimerIRq : pas de réponse */
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
    if ((irq & 0x20) && fifoLvl >= 2) {
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
    uint8_t tmp = rfid_rc522_read_reg(dev, reg); // Lecture de la valeur actuelle du registre
    rfid_rc522_write_reg(dev, reg, tmp | mask);  // Fusion par OR : force les bits du mask à 1
    LOG_DEBUG_HEX("SetBitMask reg: ", reg);
    LOG_DEBUG_HEX("SetBitMask mask: ", mask);
}

/**
 * @brief  Éteint l'antenne RF en désactivant les pilotes Tx1 et Tx2.
 * @param  dev  Pointeur vers le périphérique MFRC522.
 */
void rfid_rc522_antenna_off(MFRC522_t *dev) {
    rfid_rc522_clear_bit_mask(dev, RC522_REG_TX_CONTROL, 0x03); // Bits 0-1 à 0 : désactive Tx1 et Tx2
    LOG_DEBUG("Antenna off");
}

/**
 * @brief  Allume l'antenne RF en activant les pilotes Tx1 et Tx2.
 * @param  dev  Pointeur vers le périphérique MFRC522.
 */
void rfid_rc522_antenna_on(MFRC522_t *dev) {
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
RC522_Status rfid_rc522_poll_card(MFRC522_t *dev)
{
    uint8_t atqa[2];
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
    LOG_DEBUG("MFRC522 recovering...");
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_RESET); // Soft reset matériel
    delay_ms(50);                                                 // Attente stabilisation (≥50 ms)
    (void)rfid_rc522_init(dev);                                   // Réinitialisation complète
    rfid_rc522_antenna_on(dev);                                   // Réactivation de l'antenne RF
    LOG_DEBUG("MFRC522 recovered");
}


/**
 * @brief  Exécute l'anticollision ISO 14443 : envoie SEL+CL1 et récupère l'UID (4 octets) + BCC.
 * @param  dev  Pointeur vers le périphérique MFRC522.
 * @param  uid  Buffer de sortie (5 octets) : uid[0..3] = UID, uid[4] = BCC.
 * @return STATUS_OK en cas de succès, STATUS_ERROR si erreur RF, BCC invalide ou timeout.
 */
RC522_Status rfid_rc522_anticoll_raw(MFRC522_t *dev, uint8_t *uid) {
    LOG_DEBUG("Anticoll");

    // Préparation du module : reset des IRQ, vidage FIFO, trame complète
    rfid_rc522_write_reg(dev, RC522_REG_COMM_IRQ, 0x7F);
    rfid_rc522_write_reg(dev, RC522_REG_FIFO_LEVEL, 0x80);
    rfid_rc522_write_reg(dev, RC522_REG_BIT_FRAMING, 0x00);

    // Commande anticollision : SEL CL1 (0x93) + NVB 0x20 (2 octets, pas de CRC)
    rfid_rc522_write_reg(dev, RC522_REG_FIFO_DATA, PICC_CMD_ANTICOLL_1);
    rfid_rc522_write_reg(dev, RC522_REG_FIFO_DATA, 0x20);
    delay_ms(2);

    // Lancement de la transmission RF
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_TRANSCEIVE);
    rfid_rc522_set_bit_mask(dev, RC522_REG_BIT_FRAMING, 0x80); // Start Send

    // Attente de fin de commande via CommIrqReg (RxIRq/IdleIRq) ou erreur/timeout
    uint32_t timeout = systick_get_tick() + 25;
    while (systick_get_tick() < timeout) {

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

            // Vérification que le FIFO contient bien 5 octets (UID + BCC)
            uint8_t fifoLvl = rfid_rc522_read_reg(dev, RC522_REG_FIFO_LEVEL);
            if (fifoLvl == 5) {

                // Lecture des 5 octets depuis le FIFO
                for (int i = 0; i < 5; i++)
                    uid[i] = rfid_rc522_read_reg(dev, RC522_REG_FIFO_DATA);

                // Validation du BCC : BCC = UID[0] ^ UID[1] ^ UID[2] ^ UID[3]
                uint8_t calcBcc = uid[0] ^ uid[1] ^ uid[2] ^ uid[3];
                if (uid[4] != calcBcc) {
                    LOG_DEBUG_HEX("Anticoll bad BCC calc: ", calcBcc);
                    LOG_DEBUG_HEX("Anticoll bad BCC got: ", uid[4]);
                    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
                    return RC522_STATUS_ERROR;
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
            return RC522_STATUS_ERROR;
        }

        delay_ms(1);
    }

    // Timeout dépassé sans réponse de la carte
    LOG_DEBUG("Anticoll timeout");
    rfid_rc522_write_reg(dev, RC522_REG_COMMAND, RC522_PCD_IDLE);
    return RC522_STATUS_TIMEOUT; 
}

/**
 * @brief  Lit l'UID d'une carte RFID via anticollision et le stocke dans uid[4].
 * @param  dev  Pointeur vers le périphérique MFRC522.
 * @param  uid  Buffer de sortie (4 octets) recevant l'UID de la carte.
 * @return STATUS_OK en cas de succès, STATUS_ERROR sinon.
 */
RC522_Status rfid_rc522_read_uid(MFRC522_t *dev, uint8_t *uid) {
    LOG_DEBUG("Reading UID...");

    uint8_t rawUid[5]; // 4 octets UID + 1 octet BCC
    if (rfid_rc522_anticoll_raw(dev, rawUid) != RC522_STATUS_OK) {
        LOG_DEBUG("Anticollision failed");
        return RC522_STATUS_ERROR;
    }

    for (int i = 0; i < 4; i++)
        uid[i] = rawUid[i]; // Copie l'UID en ignorant le BCC (rawUid[4])

    LOG_DEBUG_HEX("Card UID[0]: ", uid[0]);
    LOG_DEBUG_HEX("Card UID[1]: ", uid[1]);
    LOG_DEBUG_HEX("Card UID[2]: ", uid[2]);
    LOG_DEBUG_HEX("Card UID[3]: ", uid[3]);
    return RC522_STATUS_OK;
}
