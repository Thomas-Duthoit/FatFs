// /*-----------------------------------------------------------------------*/
// /* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2023        */
// /*                                                                       */
// /*   Portions COPYRIGHT 2017-2023 STMicroelectronics                     */
// /*   Portions Copyright (C) 2013, ChaN, all right reserved               */
// /*-----------------------------------------------------------------------*/
// /* If a working storage control module is available, it should be        */
// /* attached to the FatFs via a glue function rather than modifying it.   */
// /* This is an example of glue functions to attach various existing      */
// /* storage control modules to the FatFs module with a defined API.       */
// /*-----------------------------------------------------------------------*/

// /* Includes ------------------------------------------------------------------*/
// #include "diskio.h"
// #include "ff_gen_drv.h"

// #if defined ( __GNUC__ )
// #ifndef __weak
// #define __weak __attribute__((weak))
// #endif
// #endif

// /* Private typedef -----------------------------------------------------------*/
// /* Private define ------------------------------------------------------------*/
// /* Private variables ---------------------------------------------------------*/
// extern Disk_drvTypeDef  disk;

// /* Private function prototypes -----------------------------------------------*/
// /* Private functions ---------------------------------------------------------*/

// /**
//   * @brief  Gets Disk Status
//   * @param  pdrv: Physical drive number (0..)
//   * @retval DSTATUS: Operation status
//   */
// DSTATUS disk_status (
// 	BYTE pdrv		/* Physical drive number to identify the drive */
// )
// {
//   DSTATUS stat;

//   stat = disk.drv[pdrv]->disk_status(disk.lun[pdrv]);
//   return stat;
// }

// /**
//   * @brief  Initializes a Drive
//   * @param  pdrv: Physical drive number (0..)
//   * @retval DSTATUS: Operation status
//   */
// DSTATUS disk_initialize (
// 	BYTE pdrv				/* Physical drive nmuber to identify the drive */
// )
// {
//   DSTATUS stat = RES_OK;

//   if(disk.is_initialized[pdrv] == 0)
//   {
//     stat = disk.drv[pdrv]->disk_initialize(disk.lun[pdrv]);
//     if(stat == RES_OK)
//     {
//       disk.is_initialized[pdrv] = 1;
//     }
//   }
//   return stat;
// }

// /**
//   * @brief  Reads Sector(s)
//   * @param  pdrv: Physical drive number (0..)
//   * @param  *buff: Data buffer to store read data
//   * @param  sector: Sector address (LBA)
//   * @param  count: Number of sectors to read (1..128)
//   * @retval DRESULT: Operation result
//   */
// DRESULT disk_read (
// 	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
// 	BYTE *buff,		/* Data buffer to store read data */
// 	LBA_t sector,   /* Sector address in LBA */
// 	UINT count		/* Number of sectors to read */
// )

// {
//   DRESULT res;

//   res = disk.drv[pdrv]->disk_read(disk.lun[pdrv], buff, sector, count);
//   return res;
// }

// /**
//   * @brief  Writes Sector(s)
//   * @param  pdrv: Physical drive number (0..)
//   * @param  *buff: Data to be written
//   * @param  sector: Sector address (LBA)
//   * @param  count: Number of sectors to write (1..128)
//   * @retval DRESULT: Operation result
//   */

// DRESULT disk_write (
// 	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
// 	const BYTE *buff,	/* Data to be written */
// 	LBA_t sector,		/* Sector address in LBA */
// 	UINT count        	/* Number of sectors to write */
// )
// {
//   DRESULT res;

//   res = disk.drv[pdrv]->disk_write(disk.lun[pdrv], buff, sector, count);
//   return res;
// }


// /**
//   * @brief  I/O control operation
//   * @param  pdrv: Physical drive number (0..)
//   * @param  cmd: Control code
//   * @param  *buff: Buffer to send/receive control data
//   * @retval DRESULT: Operation result
//   */

// DRESULT disk_ioctl (
// 	BYTE pdrv,		/* Physical drive nmuber (0..) */
// 	BYTE cmd,		/* Control code */
// 	void *buff		/* Buffer to send/receive control data */
// )
// {
//   DRESULT res;

//   res = disk.drv[pdrv]->disk_ioctl(disk.lun[pdrv], cmd, buff);
//   return res;
// }


// /**
//   * @brief  Gets Time from RTC
//   * @param  None
//   * @retval Time in DWORD
//   */
// __weak DWORD get_fattime (void)
// {
//   return 0;
// }

// /************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/







#include "diskio.h"
#include "layout.h"
#include "cs_manager.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <string.h>

// Commandes SD
#define CMD0    0       // GO_IDLE_STATE
#define CMD1    1       // SEND_OP_COND (MMC)
#define CMD8    8       // SEND_IF_COND
#define CMD9    9       // SEND_CSD
#define CMD10   10      // SEND_CID
#define CMD12   12      // STOP_TRANSMISSION
#define CMD16   16      // SET_BLOCKLEN
#define CMD17   17      // READ_SINGLE_BLOCK
#define CMD18   18      // READ_MULTIPLE_BLOCK
#define CMD23   23      // SET_BLOCK_COUNT (MMC)
#define CMD24   24      // WRITE_BLOCK
#define CMD25   25      // WRITE_MULTIPLE_BLOCK
#define CMD55   55      // APP_CMD
#define CMD58   58      // READ_OCR
#define ACMD23  23      // SET_WR_BLK_ERASE_COUNT (SDC)
#define ACMD41  41      // SEND_OP_COND (SDC)

// Réponses SD
#define R1_READY_STATE      0x00
#define R1_IDLE_STATE       0x01
#define R1_ILLEGAL_COMMAND  0x04

// Tokens de données
#define TOKEN_CMD25         0xFC
#define TOKEN_STOP_TRAN     0xFD
#define TOKEN_DATA          0xFE

// Variables globales
static bool sd_initialized = false;
static bool sd_high_capacity = false;

// Fonction pour changer la vitesse SPI
static void set_spi_speed(uint32_t baudrate) {
    spi_set_baudrate(SPI_PORT, baudrate);
}

// Envoi d'un octet SPI et réception
static uint8_t spi_transfer(uint8_t data) {
    uint8_t received;
    spi_write_read_blocking(SPI_PORT, &data, &received, 1);
    return received;
}

// Attendre que la carte soit prête
static uint8_t wait_ready(uint32_t timeout_ms) {
    uint8_t response;
    uint32_t start = to_ms_since_boot(get_absolute_time());
    
    do {
        response = spi_transfer(0xFF);
        if (response == 0xFF) return 0xFF;
    } while ((to_ms_since_boot(get_absolute_time()) - start) < timeout_ms);
    
    return response;
}

// Envoi d'une commande SD
static uint8_t send_cmd(uint8_t cmd, uint32_t arg) {
    uint8_t crc = 0xFF;
    uint8_t response;
    int i;
    
    // CRC pour les commandes critiques
    if (cmd == CMD0) crc = 0x95;
    if (cmd == CMD8) crc = 0x87;
    
    // Attendre que la carte soit prête
    if (wait_ready(500) != 0xFF) return 0xFF;
    
    // Envoyer la commande
    spi_transfer(0x40 | cmd);
    spi_transfer((uint8_t)(arg >> 24));
    spi_transfer((uint8_t)(arg >> 16));
    spi_transfer((uint8_t)(arg >> 8));
    spi_transfer((uint8_t)arg);
    spi_transfer(crc);
    
    // Attendre la réponse (maximum 10 tentatives)
    for (i = 0; i < 10; i++) {
        response = spi_transfer(0xFF);
        if (!(response & 0x80)) break;
    }
    
    return response;
}

// Envoi d'une commande d'application (ACMD)
static uint8_t send_acmd(uint8_t cmd, uint32_t arg) {
    send_cmd(CMD55, 0);
    return send_cmd(cmd, arg);
}

// Lecture d'un bloc de données
static bool read_datablock(uint8_t *buff, uint32_t len) {
    uint8_t token;
    uint32_t start = to_ms_since_boot(get_absolute_time());
    
    // Attendre le token de données
    do {
        token = spi_transfer(0xFF);
    } while ((token == 0xFF) && ((to_ms_since_boot(get_absolute_time()) - start) < 200));
    
    if (token != TOKEN_DATA) return false;
    
    // Lire les données
    spi_read_blocking(SPI_PORT, 0xFF, buff, len);
    
    // Ignorer le CRC (2 octets)
    spi_transfer(0xFF);
    spi_transfer(0xFF);
    
    return true;
}

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0) return RES_PARERR;
    
    if (sd_initialized) return 0;
    
    cs_select_sd();
    set_spi_speed(SPI_SD_INIT_BAUDRATE);
    
    // Envoyer 80+ cycles d'horloge pour initialiser la carte
    for (int i = 0; i < 10; i++) {
        spi_transfer(0xFF);
    }
    
    // CMD0: Reset de la carte
    uint8_t response = send_cmd(CMD0, 0);
    if (response != R1_IDLE_STATE) {
        cs_no_select();
        return STA_NOINIT;
    }
    
    // CMD8: Vérifier la tension et la version
    response = send_cmd(CMD8, 0x1AA);
    if (response == R1_IDLE_STATE) {
        // SD version 2.0+
        uint8_t ocr[4];
        for (int i = 0; i < 4; i++) {
            ocr[i] = spi_transfer(0xFF);
        }
        
        if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
            // Initialisation ACMD41 avec HCS bit
            uint32_t start = to_ms_since_boot(get_absolute_time());
            do {
                response = send_acmd(ACMD41, 0x40000000);
            } while (response && ((to_ms_since_boot(get_absolute_time()) - start) < 1000));
            
            if (response == 0) {
                // Vérifier le type de carte (CMD58)
                response = send_cmd(CMD58, 0);
                if (response == 0) {
                    for (int i = 0; i < 4; i++) {
                        ocr[i] = spi_transfer(0xFF);
                    }
                    sd_high_capacity = (ocr[0] & 0x40) ? true : false;
                }
            }
        }
    } else {
        // SD version 1.x ou MMC
        response = send_acmd(ACMD41, 0);
        if (response <= 1) {
            // SD version 1.x
            uint32_t start = to_ms_since_boot(get_absolute_time());
            do {
                response = send_acmd(ACMD41, 0);
            } while (response && ((to_ms_since_boot(get_absolute_time()) - start) < 1000));
        } else {
            // MMC
            uint32_t start = to_ms_since_boot(get_absolute_time());
            do {
                response = send_cmd(CMD1, 0);
            } while (response && ((to_ms_since_boot(get_absolute_time()) - start) < 1000));
        }
        
        if (response == 0) {
            // Définir la taille de bloc à 512 octets
            send_cmd(CMD16, 512);
        }
    }
    
    cs_no_select();
    
    if (response == 0) {
        sd_initialized = true;
        set_spi_speed(SPI_SD_READ_BAUDRATE);
        return 0;
    }
    
    return STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return sd_initialized ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || !sd_initialized || count == 0) return RES_PARERR;
    
    cs_select_sd();
    set_spi_speed(SPI_SD_READ_BAUDRATE);
    
    // Convertir l'adresse secteur si nécessaire
    if (!sd_high_capacity) sector *= 512;
    
    DRESULT result = RES_OK;
    
    if (count == 1) {
        // Lecture d'un seul bloc
        if (send_cmd(CMD17, sector) == 0) {
            if (!read_datablock(buff, 512)) {
                result = RES_ERROR;
            }
        } else {
            result = RES_ERROR;
        }
    } else {
        // Lecture de plusieurs blocs
        if (send_cmd(CMD18, sector) == 0) {
            for (UINT i = 0; i < count; i++) {
                if (!read_datablock(buff, 512)) {
                    result = RES_ERROR;
                    break;
                }
                buff += 512;
            }
            // Arrêter la transmission
            send_cmd(CMD12, 0);
        } else {
            result = RES_ERROR;
        }
    }
    
    cs_no_select();
    return result;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    // Écriture non implémentée pour le moment (lecture seule)
    return RES_WRPRT;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (pdrv != 0 || !sd_initialized) return RES_PARERR;
    
    DRESULT result = RES_ERROR;
    
    switch (cmd) {
        case CTRL_SYNC:
            // Synchronisation - toujours OK en lecture seule
            result = RES_OK;
            break;
            
        case GET_SECTOR_COUNT:
            // Nombre de secteurs - à implémenter si nécessaire
            *(DWORD*)buff = 0;
            result = RES_OK;
            break;
            
        case GET_SECTOR_SIZE:
            // Taille d'un secteur
            *(WORD*)buff = 512;
            result = RES_OK;
            break;
            
        case GET_BLOCK_SIZE:
            // Taille d'un bloc d'effacement
            *(DWORD*)buff = 1;
            result = RES_OK;
            break;
            
        default:
            result = RES_PARERR;
            break;
    }
    
    return result;
}