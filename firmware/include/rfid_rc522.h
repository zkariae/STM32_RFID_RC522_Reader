/**
a * @file rfid_rc522.h
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
#define RC522_REG_COMMAND          0x01   // Adresse du registre de commande
#define RC522_REG_COMM_IE_N        0x02
#define RC522_REG_DIV_IE_N         0x03
#define RC522_REG_COMM_IRQ         0x04   // Adresse du registre d'interruption de communication
#define RC522_REG_DIV_IRQ          0x05
#define RC522_REG_ERROR            0x06   // Adresse du registre d'erreur
#define RC522_REG_STATUS_2         0x08   // Adresse du registre d'état 2 du RC522 (temp, ...) 
#define RC522_REG_FIFO_DATA        0x09   // Adresse du registre de données du buffer FIFO
#define RC522_REG_FIFO_LEVEL       0x0A   // Adresse du registre indiquant le nbr d'octets dans le FIFO
#define RC522_REG_CONTROL          0x0C    
#define RC522_REG_BIT_FRAMING      0x0D   // Adresse du registre de configuration du cadrage des bits
#define RC522_REG_COLL             0x0E

/* Page 1 : Mode */
#define RC522_REG_RESERVED_10      0x10
#define RC522_REG_MODE             0x11
#define RC522_REG_TX_MODE          0x12
#define RC522_REG_RX_MODE          0x13
#define RC522_REG_TX_CONTROL       0x14   // Adresse du registre de contrôle de l'emetteur (antenne TX) 
#define RC522_REG_TX_ASK           0x15   // Adresse du registre de configuration de la transmission automatique
#define RC522_REG_TX_CRC_PSEL      0x16
#define RC522_REG_RX_CRC_PSEL      0x17
#define RC522_REG_TX_CRC_INIT0     0x18
#define RC522_REG_TX_CRC_INIT1     0x19
#define RC522_REG_MOD_WIDTH        0x1C

/* Page 2 : RF */
#define RC522_REG_RESERVED_20      0x20
#define RC522_REG_RX_GAIN          0x26   // Adresse du registre de configuration du gain du récepteur RF
#define RC522_REG_DEMOD            0x27

/*  Configuration du timer   */
#define RC522_REG_TMode            0x2A   // Adresse du registre de configuration du mode du timer
#define RC522_REG_TPrescaler       0x2B   // Adresse du registre du prescaler du timer interne
#define RC522_REG_TReloadL         0x2C   // Adresse du registre de rechargement bas (octet faible) du timer
#define RC522_REG_TReloadH         0x2D   // Adresse du registre de rechargement haut (octet fort) du timer
                                          
#define RC522_REG_Demod            0x19   // Adresse du registre de configuration du démodulateur 
#define RC522_REG_ModWidth         0x24   // Adresse du registre de configuration de la largeur
#define RC522_REG_GsN              0x27   // Adresse du registre de la conductance de l'émetteur RF
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
#define RC522_REG_VERSION          0x37   // Adresse du registre contenant la version du chip RC522
#define RC522_REG_ANALOG_TEST      0x38
#define RC522_REG_TEMP_SENSOR      0x39
#define RC522_REG_TEST_BUS_AUTO    0x3A

/* ============================================================================
 * COMMANDES PCD (MFRC522 Commands)
 * ============================================================================ */
#define RC522_PCD_IDLE              0x00  // Commande pour mettre le RC en mode inactif 
#define RC522_PCD_MEM              0x01  /* Transfert FIFO → buffer interne */
#define RC522_PCD_GENERATE_RANDOMID 0x02 /* Génère un ID aléatoire */
#define RC522_PCD_CALC_CRC         0x03  /* Calcul CRC */
#define RC522_PCD_TRANSMIT         0x04  /* Envoie des données de la FIFO */
#define RC522_PCD_NO_CHANGE        0x07  /* Pas de changement */
#define RC522_PCD_RECEIVE          0x08  /* Réception de données */
#define RC522_PCD_TRANSCEIVE       0x0C  // Commande pour émettre et recevoir des données via l'antenne RF
#define RC522_PCD_AUTHENT          0x0E  /* Authentification MIFARE */
#define RC522_PCD_RESET            0x0F  // Commande pour effectuer une réinitialisation logicielle du RC522

/* ============================================================================
 * COMMANDES PICC (ISO/IEC 14443-3 Type A)
 * ============================================================================ */
#define PICC_CMD_REQA              0x26  // Commande REQA pour demander la présence d'une carte RFID
#define PICC_CMD_WUPA              0x52  /* Wake-up all */
#define PICC_CMD_ANTICOLL_1        0x93  // Commande de sélection de la cascade level 1 (anticollision)
#define PICC_CMD_SELECT_CL1        0x93  /* Sélection CL1 */
#define PICC_CMD_ANTICOLL_2        0x95  /* Anti-collision CL2 */
#define PICC_CMD_SELECT_CL2        0x95  /* Sélection CL2 */
#define PICC_CMD_ANTICOLL_3        0x97  /* Anti-collision CL3 */
#define PICC_CMD_SELECT_CL3        0x97  /* Sélection CL3 */
#define PICC_CMD_HLTA              0x50  /* Halt */
#define PICC_CMD_RATS              0xE0  /* Request ATS (Type A) */

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
    RC522_STATUS_ERROR = 1,
    RC522_STATUS_COLLISION,
    RC522_STATUS_TIMEOUT = 2,
    RC522_STATUS_NO_ROOM,
    RC522_STATUS_INTERNAL_ERROR,
    RC522_STATUS_INVALID = 3,
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

/**@brief Nbr tentatives pour détecter une carte MIFAIRE */
#define MAX_TIMEOUT_COUNT 10

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


/**
 * @brief utilise les macros de spi/cs/rst 
 */
typedef struct {
    uint32_t cs_Port;
    uint16_t cs_Pin;
    uint32_t rst_Port;
    uint16_t rst_Pin;
} MFRC522_t;

/* ============================================================================
 * FONCTIONS PUBLIQUES DU DRIVER
 * ============================================================================ */

/**
 * @brief Initialise le module RC522.
 *        Configure le SPI, effectue un reset logiciel et vérifie la connexion.
 * @return RC522_STATUS_OK si succès, sinon code d'erreur.
 */
RC522_Status rfid_rc522_init(MFRC522_t *dev);

/**
 * @brief Reset logiciel du module RC522.
 */
void rfid_rc522_reset(MFRC522_t *dev);

/**
 * @brief Active l'antenne RF.
 * @return RC522_STATUS_OK si succès.
 */
void rfid_rc522_antenna_on(MFRC522_t *dev);

/**
 * @brief Désactive l'antenne RF.
 */
void rfid_rc522_antenna_off(MFRC522_t *dev);

/**
 * @brief Lit la version du firmware du RC522.
 * @return Version du chip (0x88, 0x90, etc.) ou 0xFF si erreur.
 */
uint8_t rfid_rc522_get_version(MFRC522_t *dev);

/**
 * @brief Envoie une commande REQA pour détecter les cartes à proximité.
 * @param[out] atqa Buffer pour ATQA (2 bytes).
 * @return RC522_STATUS_OK si carte détectée.
 */
RC522_Status rfid_rc522_request(MFRC522_t *dev, uint8_t *atqa);

/**
 * @brief Effectue l'anti-collision et récupère l'UID de la carte.
 * @param[out] uid Structure pour stocker l'UID.
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_anticoll(MFRC522_t *dev, RC522_UID *uid);

/**
 * @brief Sélectionne la carte avec son UID.
 * @param[in] uid UID de la carte à sélectionner.
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_select(MFRC522_t *dev, RC522_UID *uid);

/**
 * @brief Authentifie un bloc avec une clé MIFARE.
 * @param block Adresse du bloc (0-63 pour MIFARE 1K).
 * @param key_type PICC_CMD_MIFARE_AUTH_KEY_A ou _B.
 * @param key Clé d'authentification.
 * @param uid UID de la carte.
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_auth(MFRC522_t *dev, uint8_t block, uint8_t key_type,
                              const RC522_Key *key, const RC522_UID *uid);

/**
 * @brief Lit un bloc MIFARE Classic.
 * @param block Adresse du bloc (0-63).
 * @param[out] data Buffer pour les données lues (16 bytes).
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_read_block(MFRC522_t *dev, uint8_t block, uint8_t *data);

/**
 * @brief Écrit un bloc MIFARE Classic.
 * @param block Adresse du bloc (0-63).
 * @param data Données à écrire (16 bytes).
 * @return RC522_STATUS_OK si succès.
 */
RC522_Status rfid_rc522_write_block(MFRC522_t *dev, uint8_t block, const uint8_t *data);

/**
 * @brief Arrête la communication avec la carte (halt).
 */
void rfid_rc522_halt(MFRC522_t *dev);

/**
 * @brief Renvoie le dernier code d'erreur du module.
 * @return Code d'erreur.
 */
uint8_t rfid_rc522_get_error(void);

/**
 * @brief Lit le registre Crypto1 Status.
 * @return Valeur du registre.
 */
uint8_t rfid_rc522_get_crypto_status(MFRC522_t *dev);

/**
 * @brief Écrit un octet dans un registre du RC522.
 * @param addr Adresse du registre.
 * @param value Valeur à écrire.
 */
void rfid_rc522_write_reg(MFRC522_t *dev, uint8_t addr, uint8_t value);

/**
 * @brief Lit un octet depuis un registre du RC522.
 * @param addr Adresse du registre.
 * @return Valeur lue.
 */
uint8_t rfid_rc522_read_reg(MFRC522_t *dev, uint8_t addr);


/**
 * @brief Efface un ou plusieurs bits d'un registre du MFRC522.
 *
 * @param[in] dev   Pointeur vers la structure MFRC522 (périphérique cible).
 * @param[in] reg   Adresse du registre à modifier.
 * @param[in] mask  Masque des bits à effacer (1 = bit effacé, 0 = bit inchangé).
 *
 * @return void
 */
void rfid_rc522_clear_bit_mask(MFRC522_t *dev, uint8_t reg, uint8_t mask);


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
uint8_t rfid_rc522_poll_card(MFRC522_t *dev);

/**
 * @brief Envoie une commande REQA et récupère l'ATQA de la carte.
 * @param[in] dev Pointeur vers la structure MFRC522.
 * @param[out] atqa Buffer de 2 octets recevant l'ATQA.
 * @return RC522_STATUS_OK si une carte répond, sinon code d'erreur.
 */
uint8_t rfid_rc522_request_a(MFRC522_t *dev, uint8_t *atqa);

/**
 * @brief Réinitialise le module MFRC522 après une erreur ou un blocage.
 * @param[in] dev Pointeur vers la structure MFRC522 (périphérique cible).
 */
void rfid_rc522_recover(MFRC522_t *dev);

/**
 * @brief Exécute l'anticollision ISO 14443-A et récupère UID + BCC.
 * @param[in] dev Pointeur vers la structure MFRC522.
 * @param[out] uid Buffer de 5 octets : UID[0..3] + BCC.
 * @return RC522_STATUS_OK si succès, sinon code d'erreur.
 */
uint8_t rfid_rc522_anticoll_raw(MFRC522_t *dev, uint8_t *uid);

/**
 * @brief Lit l'UID 4 octets d'une carte RFID.
 * @param[in] dev Pointeur vers la structure MFRC522.
 * @param[out] uid Buffer de 4 octets recevant l'UID.
 * @return RC522_STATUS_OK si succès, sinon code d'erreur.
 */
uint8_t rfid_rc522_read_uid(MFRC522_t *dev, uint8_t *uid);

/**
 * @brief Attend le retrait de la carte RFID.
 * @param[in] dev Pointeur vers la structure MFRC522.
 * @return RC522_STATUS_OK si la carte est retirée, sinon code d'erreur.
 */
uint8_t rfid_rc522_wait_card_removal(MFRC522_t *dev);



#endif /* RFID_RC522_H */
