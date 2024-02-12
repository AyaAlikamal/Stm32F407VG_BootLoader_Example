#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"
#include "crc.h"

#define BL_DEBUG_UART   &huart2
#define BL_HOST_COMMUNICATION_UART &huart3

#define CRC_ENGINE_OBJ    &hcrc

#define ADDRESS_IS_INVALID     0x00
#define ADDRESS_IS_VALID       0x01


#define SRAM1_SIZE     (112 * 1024)
#define SRAM2_SIZE     (16 * 1024)
#define SRAM3_SIZE     (64 * 1024)
#define FLASH_SIZE     (1024 * 1024)

#define SRAM1_END             (SRAM1_BASE + SRAM1_SIZE)
#define SRAM2_END             (SRAM2_BASE + SRAM2_SIZE)
#define SRAM3_END             (CCMDATARAM_BASE + SRAM3_SIZE)
#define FLASH_END_VERTUALE    (FLASH_BASE + FLASH_SIZE)

#define BL_HOST_BUFFER_RX_LENGTH      200
#define BL_ENABLE_UART_DEBUG_MESSAGE  0x00
#define BL_ENABLE_SPI_DEBUG_MESSAGE   0x01
#define BL_ENABLE_CAN_DEBUG_MESSAGE   0x02
#define BL_DEBUG_METHOD (BL_ENABLE_UART_DEBUG_MESSAGE)

#define BL_DEBUG_ENABLE                  1
#define BL_DEBUG_DISABLE                 0
#define DEBUG_INFO_ENABLE (BL_DEBUG_ENABLE)
#define FLASH_SECTOR2_BASE_ADDRESS       0x8008000

#define CBL_GET_VER_CMD                  0x10
#define CBL_GET_HELP_CMD                 0x11
#define CBL_GET_CID_CMD                  0x12
#define CBL_GET_RDP_STATUES_CMD          0x13
#define CBL_GO_TO_ADDR_CMD               0x14
#define CBL_FLASH_EREASE_CMD             0x15
#define CBL_MEM_WRITE_CMD                0x16
#define CBL_EN_R_W_PROTECT_CMD           0x17
#define CBL_MEM_READ_CMD                 0x18
#define CBL_READ_SECTOR_STATUES_CMD      0x19
#define CBL_OTP_READ_CMD                 0x20
#define CBL_ChANGE_R_w_PROTECT_CMD       0x21


#define CBL_VENDOR_ID                    100
#define CBL_SW_MAJOR_VERSION             1
#define CBL_SW_MINOR_VERSION             0
#define CBL_SW_PATCH_VERSION             0

#define CRC_TYPE_SIZE_BYTE               4

#define CRC_VERIFICATION_FAILED          0x00
#define CRC_VERIFICATION_PASSED          0x01

#define CBL_SEND_NACK                    0xAB
#define CBL_SEND_ACK                     0xCD


#define CBL_FLASH_MAX_SECTOR_NUMBER    12
#define CBL_FLASH_MASS_ERASE           0xFF

#define INVALID_SECTOR_NUMBER          0x00
#define VALID_SECTOR_NUMBER            0x01
#define UNSUCCESSFUL_ERASE             0x02
#define SUCCESSFUL_ERASE               0x03

#define FLASH_PAYLOAD_WRITE_FAILED     0x00
#define FLASH_PAYLOAD_WRITE_PASSED     0x01

#define FLASH_LOCK_WRITE_FAILED        0x00
#define FLASH_LOCK_WRITE_PASSED        0x01

#define ROP_LEVEL_READ_INVALID         0x00
#define ROP_LEVE_READL_VALID           0X01

#define HAL_SUCCESSFUL_ERASE           0xFFFFFFFFU

#define ROP_LEVEL_CHANGE_INVALID     0x00
#define ROP_LEVEL_CHANGE_VALID       0X01

#define CBL_ROP_LEVEL_0              0x00
#define CBL_ROP_LEVEL_1              0x01
#define CBL_ROP_LEVEL_2              0x02


typedef void (*pMainApp)(void);
typedef void (*Jump_Ptr) (void);

typedef enum{
BL_NOK = 0,
BL_OK
}BL_STATUES;

BL_STATUES BL_UART_Featch_Host_Command(void);
void BL_Print_Message(char *formate, ...);
#endif  