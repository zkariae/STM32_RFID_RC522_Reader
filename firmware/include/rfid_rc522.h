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
#define RC522_REG_MOD_WIDTH        0x24

/* Page 2 : RF */
#define RC522_REG_RESERVED_20      0x20
#define RC522_REG_CRC_RESULT_H     0x21   // Résultat CRC octet haut
#define RC522_REG_CRC_RESULT_L     0x22   // Résultat CRC octet bas
#define RC522_REG_RF_CFG           0x26   // Adresse du registre de configuration RF / gain du récepteur
#define RC522_REG_DEMOD            0x27

/*  Configuration du timer   */
#define RC522_REG_TMode            0x2A   // Adresse du registre de configuration du mode du timer
#define RC522_REG_TPrescaler       0x2B   // Adresse du registre du prescaler du timer interne
#define RC522_REG_TReloadH         0x2C   // Adresse du registre de rechargement haut (octet fort) du timer
#define RC522_REG_TReloadL         0x2D   // Adresse du registre de rechargement bas (octet faible) du timer
                                           
#define RC522_REG_Demod            0x19   // Adresse du registre de configuration du démodulateur 
#define RC522_REG_GSN              0x27   // Adresse du registre de la conductance de l'émetteur RF
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

/* Bits du registre CommIrqReg */
#define RC522_IRQ_TX               0x40
#define RC522_IRQ_RX               0x20
#define RC522_IRQ_IDLE             0x10
#define RC522_IRQ_HIALERT          0x08
#define RC522_IRQ_LOALERT          0x04
#define RC522_IRQ_ERR              0x02
#define RC522_IRQ_TIMER            0x01
#define RC522_IRQ_CLEAR            0x7F

#define RC522_IRQ_TRANSCEIVE_DONE  (RC522_IRQ_RX | RC522_IRQ_IDLE)
#define RC522_IRQ_TRANSCEIVE_FAIL  (RC522_IRQ_TIMER | RC522_IRQ_ERR)

/* Bits du registre DivIrqReg */
#define RC522_DIV_IRQ_CRC          0x04   // Fin de calcul CRC

/* Masques FIFO / modes */
#define RC522_FIFO_FLUSH           0x80   // Vide le FIFO
#define RC522_MODE_CRC_PRESET_6363 0x3D   // Preset CRC_A ISO14443-A

/* ============================================================================
 * COMMANDES PICC (ISO/IEC 14443-3 Type A)
 * ============================================================================ */
#define PICC_CMD_REQA              0x26  // Commande REQA pour demander la présence d'une carte RFID
#define PICC_CMD_WUPA              0x52  /* Wake-up all */
#define PICC_CMD_CL1               0x93  /* Cascade level 1 */
#define PICC_CMD_CL2               0x95  /* Cascade level 2 */
#define PICC_CMD_CL3               0x97  /* Cascade level 3 */
#define PICC_CMD_ANTICOLL_1        0x93  // Commande de sélection de la cascade level 1 (anticollision)
#define PICC_CMD_SELECT_CL1        0x93  /* Sélection CL1 */
#define PICC_CMD_ANTICOLL_2        0x95  /* Anti-collision CL2 */
#define PICC_CMD_SELECT_CL2        0x95  /* Sélection CL2 */
#define PICC_CMD_ANTICOLL_3        0x97  /* Anti-collision CL3 */
#define PICC_CMD_SELECT_CL3        0x97  /* Sélection CL3 */
#define PICC_CMD_HLTA              0x50  /* Halt */
#define PICC_CMD_RATS              0xE0  /* Request ATS (Type A) */

#define PICC_CASCADE_TAG           0x88  /* UID continue au niveau suivant */
#define PICC_NVB_ANTICOLL          0x20  /* NVB pour anticollision */
#define PICC_NVB_SELECT            0x70  /* NVB pour sélection */
#define PICC_SAK_CASCADE           0x04  /* Bit cascade dans SAK */
#define PICC_UID_PART_SIZE         4     /* Octets UID par niveau */
#define PICC_UID_BCC_SIZE          1     /* Octet BCC */
#define PICC_UID_FRAME_SIZE        5     /* UID part + BCC */

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
    RC522_STATUS_TIMEOUT = 2,
    RC522_STATUS_INVALID = 3,
    RC522_STATUS_BCC_MISMATCH,
    RC522_STATUS_INVALID_UID
} RC522_Status;

typedef enum {
    RC522_CARD_TYPE_UNKNOWN = 0,
    RC522_CARD_TYPE_MIFARE_MINI,
    RC522_CARD_TYPE_MIFARE_CLASSIC_1K,
    RC522_CARD_TYPE_MIFARE_CLASSIC_4K,
    RC522_CARD_TYPE_MIFARE_ULTRALIGHT,
    RC522_CARD_TYPE_MIFARE_PLUS,
    RC522_CARD_TYPE_ISO_14443_4,
    RC522_CARD_TYPE_ISO_18092,
    RC522_CARD_TYPE_NOT_COMPLETE
} RC522_CardType;

/* ============================================================================
 * STRUCTURES DE DONNÉES
 * ============================================================================ */

/** @brief Taille de la FIFO RC522 (64 bytes) */
#define RC522_FIFO_SIZE            64

/** @brief Tailles UID ISO14443-A */
#define RC522_UID_SINGLE_SIZE      4     /* UID simple */
#define RC522_UID_DOUBLE_SIZE      7     /* UID double */
#define RC522_UID_TRIPLE_SIZE      10    /* UID triple */
#define RC522_UID_MAX_SIZE         RC522_UID_TRIPLE_SIZE /* Taille max UID */

/**@brief Nbr tentatives pour détecter une carte MIFAIRE */
#define MAX_TIMEOUT_COUNT 100

typedef struct {
    uint8_t uid[RC522_UID_MAX_SIZE]; /* Octets UID */
    uint8_t size;                    /* Taille UID */
    uint8_t sak;                     /* Select acknowledge */
    uint8_t atqa[2];                 /* Réponse REQA */
} RC522_UID;

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
 * @brief Active l'antenne RF.
 * @return RC522_STATUS_OK si succès.
 */
void rfid_rc522_antenna_on(MFRC522_t *dev);

/**
 * @brief Désactive l'antenne RF.
 */
void rfid_rc522_antenna_off(MFRC522_t *dev);

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
 * @param[out] atqa Buffer de 2 octets recevant l'ATQA.
 * @return STATUS_OK si une carte répond, sinon code d'erreur.
 */
RC522_Status rfid_rc522_poll_card(MFRC522_t *dev, uint8_t *atqa);

/**
 * @brief Envoie une commande REQA et récupère l'ATQA de la carte.
 * @param[in] dev Pointeur vers la structure MFRC522.
 * @param[out] atqa Buffer de 2 octets recevant l'ATQA.
 * @return RC522_STATUS_OK si une carte répond, sinon code d'erreur.
 */
RC522_Status rfid_rc522_request_a(MFRC522_t *dev, uint8_t *atqa);

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
RC522_Status rfid_rc522_anticoll_raw(MFRC522_t *dev, uint8_t *uid);

/**
 * @brief Lit l'UID complet d'une carte RFID (CL1/CL2/CL3).
 * @param[in] dev Pointeur vers la structure MFRC522.
 * @param[out] uid Structure recevant ATQA, UID, taille et SAK.
 * @param[in] atqa ATQA obtenu pendant le polling.
 * @return RC522_STATUS_OK si succès, sinon code d'erreur.
 */
RC522_Status rfid_rc522_read_uid_full(MFRC522_t *dev, RC522_UID *uid, const uint8_t *atqa);

/**
 * @brief Lit l'UID 4 octets d'une carte RFID.
 * @param[in] dev Pointeur vers la structure MFRC522.
 * @param[out] uid Buffer de 4 octets recevant l'UID.
 * @return RC522_STATUS_OK si succès, sinon code d'erreur.
 */
RC522_Status rfid_rc522_read_uid(MFRC522_t *dev, uint8_t *uid);

/**
 * @brief Déduit le type de carte depuis SAK/ATQA.
 * @param[in] uid Structure UID complète.
 * @return Type de carte détecté.
 */
RC522_CardType rfid_rc522_get_card_type(const RC522_UID *uid);

/**
 * @brief Convertit un type de carte en texte lisible.
 * @param[in] type Type de carte.
 * @return Nom du type de carte.
 */
const char *rfid_rc522_card_type_name(RC522_CardType type);

/**
 * @brief Attend le retrait de la carte RFID.
 * @param[in] dev Pointeur vers la structure MFRC522.
 * @return RC522_STATUS_OK si la carte est retirée, sinon code d'erreur.
 */
RC522_Status rfid_rc522_wait_card_removal(MFRC522_t *dev);



#endif /* RFID_RC522_H */
