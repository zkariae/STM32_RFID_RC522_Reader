/**
 * @file rfid_rc522.h
 * @brief Interface du driver pour le module RFID RC522 (MFRC522).
 *        Communication SPI via spi.h.
 */

#ifndef RFID_RC522_H
#define RFID_RC522_H

#include <stdint.h>
#include "spi.h"

/* ============================================================================
 * ADRESSES DES REGISTRES DU MFRC522
 * ============================================================================ */

/* Page 0 : Commandes et status */
#define RC522_REG_RESERVED_00      0x00
#define RC522_REG_COMMAND          0x01
#define RC522_REG_COMM_IE_N        0x02
#define RC522_REG_DIV_IE_N         0x03
#define RC522_REG_COMM_IRQ         0x04
#define RC522_REG_DIV_IRQ          0x05
#define RC522_REG_ERROR            0x06
#define RC522_REG_STATUS_2         0x08
#define RC522_REG_FIFO_DATA        0x09
#define RC522_REG_FIFO_LEVEL       0x0A
#define RC522_REG_CONTROL          0x0C
#define RC522_REG_BIT_FRAMING      0x0D
#define RC522_REG_COLL             0x0E

/* Page 1 : Mode */
#define RC522_REG_RESERVED_10      0x10
#define RC522_REG_MODE             0x11
#define RC522_REG_TX_MODE          0x12
#define RC522_REG_RX_MODE          0x13
#define RC522_REG_TX_CONTROL       0x14
#define RC522_REG_TX_ASK           0x15
#define RC522_REG_TX_CRC_PSEL      0x16
#define RC522_REG_RX_CRC_PSEL      0x17
#define RC522_REG_TX_CRC_INIT0     0x18
#define RC522_REG_TX_CRC_INIT1     0x19
#define RC522_REG_MOD_WIDTH        0x1C

/* Page 2 : RF */
#define RC522_REG_RESERVED_20      0x20
#define RC522_REG_RX_GAIN          0x26
#define RC522_REG_DEMOD            0x27

/* Page 3 : TypeB */
#define RC522_REG_RESERVED_30      0x30

/* Page 4 : Crypto */
#define RC522_REG_RESERVED_40      0x40
#define RC522_REG_RF_LEVEL         0x14  /* Alias */
#define RC522_REG_CRYPT_STATUS     0x24

/* Page 5 : FIFO et Test */
#define RC522_REG_FIFO_SIZE        0x29
#define RC522_REG_PAGE_SEL         0x26  /* Alias */

/* Test */
#define RC522_REG_TEST_SEL_1       0x31
#define RC522_REG_TEST_SEL_2       0x32
#define RC522_REG_TEST_PIN_EN      0x33
#define RC522_REG_TEST_PIN_OUT     0x34
#define RC522_REG_TEST_BUS         0x35
#define RC522_REG_AUTO_TEST        0x36
#define RC522_REG_VERSION           0x37
#define RC522_REG_ANALOG_TEST       0x38
#define RC522_REG_TEMP_SENSOR      0x39
#define RC522_REG_TEST_BUS_AUTO    0x3A

/* ============================================================================
 * COMMANDES PCD (MFRC522 Commands)
 * ============================================================================ */
#define RC522_PCD_IDLE              0x00  /* Pas d'action, annule les commandes */
#define RC522_PCD_MEM              0x01  /* Transfert FIFO → buffer interne */
#define RC522_PCD_GENERATE_RANDOMID 0x02 /* Génère un ID aléatoire */
#define RC522_PCD_CALC_CRC         0x03  /* Calcul CRC */
#define RC522_PCD_TRANSMIT         0x04  /* Envoie des données de la FIFO */
#define RC522_PCD_NO_CHANGE        0x07  /* Pas de changement */
#define RC522_PCD_RECEIVE          0x08  /* Réception de données */
#define RC522_PCD_TRANSCEIVE       0x0C  /* Envoie + attend réponse */
#define RC522_PCD_AUTHENT          0x0E  /* Authentification MIFARE */
#define RC522_PCD_RESET            0x0F  /* Soft reset */

/* ============================================================================
 * COMMANDES PICC (ISO/IEC 14443-3 Type A)
 * ============================================================================ */
#define PICC_CMD_REQA               0x26  /* Demande inactive */
#define PICC_CMD_WUPA               0x52  /* Wake-up all */
#define PICC_CMD_ANTICOLL_1        0x93  /* Anti-collision CL1 */
#define PICC_CMD_SELECT_CL1        0x93  /* Sélection CL1 */
#define PICC_CMD_ANTICOLL_2        0x95  /* Anti-collision CL2 */
#define PICC_CMD_SELECT_CL2        0x95  /* Sélection CL2 */
#define PICC_CMD_ANTICOLL_3        0x97  /* Anti-collision CL3 */
#define PICC_CMD_SELECT_CL3        0x97  /* Sélection CL3 */
#define PICC_CMD_HLTA               0x50  /* Halt */
#define PICC_CMD_RATS               0xE0  /* Request ATS (Type A) */

/* Commandes MIFARE Classic */
#define PICC_CMD_MIFARE_READ        0x30  /* Lecture bloc */
#define PICC_CMD_MIFARE_WRITE       0xA0  /* Écriture bloc */
#define PICC_CMD_MIFARE_DECREMENT   0xC0  /* Décrément */
#define PICC_CMD_MIFARE_INCREMENT   0xC1  /* Incrément */
#define PICC_CMD_MIFARE_RESTORE     0xC2  /* Restaure bloc */
#define PICC_CMD_MIFARE_TRANSFER    0xB0  /* Transfert */
#define PICC_CMD_MIFARE_AUTH_KEY_A  0x60  /* Auth avec clé A */
#define PICC_CMD_MIFARE_AUTH_KEY_B  0x61  /* Auth avec clé B */

/* ============================================================================
 * CODES DE STATUT RC522
 * ============================================================================ */
typedef enum {
    RC522_STATUS_OK = 0,
    RC522_STATUS_ERROR,
    RC522_STATUS_COLLISION,
    RC522_STATUS_TIMEOUT,
    RC522_STATUS_NO_ROOM,
    RC522_STATUS_INTERNAL_ERROR,
    RC522_STATUS_INVALID,
    RC522_STATUS_CRC_WRONG,
    RC522_STATUS_MIFARE_AUTH_ERROR,
    RC522_STATUS_BITCOUNT_FRAMING,
    RC522_STATUS_BITFRAMING_ERROR,
    RC522_STATUS_ABORTED,
    RC522_STATUS_INVALID_UID,
    RC522_STATUS_NOT_IMPLEMENTED
} RC522_Status;

/* ============================================================================
 * STRUCTURES DE DONNÉES
 * ============================================================================ */

/** @brief Taille maximale de l'UID (4 octets pour UID simple, 7 pour cascade) */
#define RC522_UID_MAX_SIZE         7

/** @brief Taille d'un bloc MIFARE Classic (16 bytes) */
#define RC522_BLOCK_SIZE           16

/** @brief Taille de la clé MIFARE (6 bytes) */
#define RC522_KEY_SIZE             6

/** @brief Taille de la FIFO RC522 (64 bytes) */
#define RC522_FIFO_SIZE            64

/**
 * @brief Structure représentant l'UID d'une carte.
 */
typedef struct {
    uint8_t size;           /**< Taille de l'UID en octets (4, 7 ou 10) */
    uint8_t uid[RC522_UID_MAX_SIZE]; /**< Octets de l'UID */
    uint8_t sak;            /**< Select Acknowledge (réponse à SELECT) */
} RC522_UID;

/**
 * @brief Clé d'authentification MIFARE.
 */
typedef struct {
    uint8_t key[RC522_KEY_SIZE];
} RC522_Key;

/**
 * @brief Configuration du driver RC522.
 */
typedef struct {
    /* Utilise les macros RC522_CS_LOW/RC522_CS_HIGH de spi.h */
} RC522_Config;

/**
 * @brief Résultat d'une opération de lecture/écriture.
 */
typedef struct {
    RC522_Status status;
    uint8_t data[RC522_BLOCK_SIZE];
    uint8_t data_size;
} RC522_BlockData;

/* ============================================================================
 * FONCTIONS PUBLIQUES DU DRIVER
 * ============================================================================ */

/**
 * @brief Initialise le module RC522.
 *        Configure le SPI, effectue un reset logiciel et vérifie la connexion.
 * @return RC522_STATUS_OK si succès, sinon code d'erreur.
 */
RC522_Status rfid_rc522_init(void);

/**
 * @brief Reset logiciel du module RC522.
 */
void rfid_rc522_reset(void);

/**
 * @brief Active l'antenne RF.
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_antenna_on(void);

/**
 * @brief Désactive l'antenne RF.
 */
void rfid_rc522_antenna_off(void);

/**
 * @brief Lit la version du firmware du RC522.
 * @return Version du chip (0x88, 0x90, etc.) ou 0xFF si erreur.
 */
uint8_t rfid_rc522_get_version(void);

/**
 * @brief Envoie une commande REQA pour détecter les cartes à proximité.
 * @param[out] atqa Buffer pour ATQA (2 bytes).
 * @return RC522_STATUS_OK si carte détectée.
 */
RC522_Status rfid_rc522_request(uint8_t *atqa);

/**
 * @brief Effectue l'anti-collision et récupère l'UID de la carte.
 * @param[out] uid Structure pour stocker l'UID.
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_anticoll(RC522_UID *uid);

/**
 * @brief Sélectionne la carte avec son UID.
 * @param[in] uid UID de la carte à sélectionner.
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_select(RC522_UID *uid);

/**
 * @brief Authentifie un bloc avec une clé MIFARE.
 * @param block Adresse du bloc (0-63 pour MIFARE 1K).
 * @param key_type PICC_CMD_MIFARE_AUTH_KEY_A ou _B.
 * @param key Clé d'authentification.
 * @param uid UID de la carte.
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_auth(uint8_t block, uint8_t key_type,
                              const RC522_Key *key, const RC522_UID *uid);

/**
 * @brief Lit un bloc MIFARE Classic.
 * @param block Adresse du bloc (0-63).
 * @param[out] data Buffer pour les données lues (16 bytes).
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_read_block(uint8_t block, uint8_t *data);

/**
 * @brief Écrit un bloc MIFARE Classic.
 * @param block Adresse du bloc (0-63).
 * @param data Données à écrire (16 bytes).
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_write_block(uint8_t block, const uint8_t *data);

/**
 * @brief Arrête la communication avec la carte (halt).
 */
void rfid_rc522_halt(void);

/**
 * @brief Renvoie le dernier code d'erreur du module.
 * @return Code d'erreur.
 */
uint8_t rfid_rc522_get_error(void);

/**
 * @brief Lit le registre Crypto1 Status.
 * @return Valeur du registre.
 */
uint8_t rfid_rc522_get_crypto_status(void);

#endif /* RFID_RC522_H */