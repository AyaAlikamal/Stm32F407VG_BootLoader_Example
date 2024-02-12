#include "bootloader.h"

static uint8_t BL_Host_Buffer[BL_HOST_BUFFER_RX_LENGTH];

static void BootLoader_Get_Version(uint8_t *Host_Buffer);
static void BootLoader_Get_Help(uint8_t *Host_Buffer);
static void BootLoader_Get_Chip_Identification_Number(uint8_t *Host_Buffer);
static void BootLoader_Read_Protection_Level(uint8_t *Host_Buffer);
static void BootLoader_Jump_To_Address(uint8_t *Host_Buffer);
static void BootLoader_Erase_Flash(uint8_t *Host_Buffer);
static void BootLoader_Memory_Write(uint8_t *Host_Buffer);
static void BootLoader_Enable_RW_Protection(uint8_t *Host_Buffer);
static void BootLoader_Memory_Read(uint8_t *Host_Buffer);
static void BootLoader_Get_Sector_Protection_Statues(uint8_t *Host_Buffer);
static void BootLoader_Read_OTP(uint8_t *Host_Buffer);
static void BootLoader_Change_Read_Protection_Levels(uint8_t *Host_Buffer);


static uint8_t BootLoader_CRC_Verify(uint8_t *pdata, uint32_t Data_Len, uint32_t Host_CRC);
static void Boot_Loader_Send_ACK(uint8_t Replay_Len);
static void Boot_Loader_Send_Data_To_Host(uint8_t *Host_Buffer, uint8_t Data_Length);
static void Boot_Loader_Jump_to_user_app(void);

static uint8_t Boat_Loader_Supported_CMDs[12] = {
			CBL_GET_VER_CMD,
			CBL_GET_HELP_CMD,
			CBL_GET_CID_CMD,
			CBL_GET_RDP_STATUES_CMD,
			CBL_GO_TO_ADDR_CMD,
			CBL_FLASH_EREASE_CMD,
			CBL_MEM_WRITE_CMD,
			CBL_EN_R_W_PROTECT_CMD,
			CBL_MEM_READ_CMD,
			CBL_READ_SECTOR_STATUES_CMD,
			CBL_OTP_READ_CMD,
			CBL_ChANGE_R_w_PROTECT_CMD
};

BL_STATUES BL_UART_Featch_Host_Command(void){
BL_STATUES statues =  BL_NOK;
HAL_StatusTypeDef Hal_Statues = HAL_ERROR;
uint8_t Data_Length = 0;

memset(BL_Host_Buffer,0, BL_HOST_BUFFER_RX_LENGTH);
Hal_Statues = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART, BL_Host_Buffer, 1,HAL_MAX_DELAY);
	if(Hal_Statues != HAL_OK){
		statues = BL_NOK;
	}
	else{
	Data_Length = BL_Host_Buffer[0];
	Hal_Statues = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART, &BL_Host_Buffer[1], Data_Length, HAL_MAX_DELAY);
	if(	Hal_Statues == BL_NOK){
			statues = BL_NOK;
	}
	else{
	switch(BL_Host_Buffer[1]){
		case CBL_GET_VER_CMD:
			BootLoader_Get_Version(BL_Host_Buffer);
		break;
		case CBL_GET_HELP_CMD:
			BootLoader_Get_Help(BL_Host_Buffer);
		break;
	case CBL_GET_CID_CMD:
		BootLoader_Get_Chip_Identification_Number(BL_Host_Buffer);
		break;
	case CBL_GET_RDP_STATUES_CMD:
		BootLoader_Read_Protection_Level(BL_Host_Buffer);
		break;
	case CBL_GO_TO_ADDR_CMD:
		BootLoader_Jump_To_Address(BL_Host_Buffer);
		break;
	case CBL_FLASH_EREASE_CMD:
		BootLoader_Erase_Flash(BL_Host_Buffer);
		break;
	case CBL_MEM_WRITE_CMD:
		BootLoader_Memory_Write(BL_Host_Buffer);
		break;
	case CBL_EN_R_W_PROTECT_CMD:
		BootLoader_Enable_RW_Protection(BL_Host_Buffer);
		break;
	case CBL_MEM_READ_CMD:
		BootLoader_Memory_Read(BL_Host_Buffer);
		break;
	case CBL_READ_SECTOR_STATUES_CMD:
		BootLoader_Get_Sector_Protection_Statues(BL_Host_Buffer);
		break;
	case CBL_OTP_READ_CMD:
		BootLoader_Read_OTP(BL_Host_Buffer);
		break;	
	case CBL_ChANGE_R_w_PROTECT_CMD:
		BootLoader_Change_Read_Protection_Levels(BL_Host_Buffer);
		break;	
	default:
		break;
	}
	}
	}
return statues;
}

static void Boot_Loader_Jump_to_user_app(void){
  uint32_t MSP_Value = *((volatile uint32_t *) FLASH_SECTOR2_BASE_ADDRESS);
	uint32_t MainAppAddr = *((volatile uint32_t *)FLASH_SECTOR2_BASE_ADDRESS + 4);
	pMainApp ResetHundler_Address = (pMainApp)MainAppAddr;
  __set_MSP(MSP_Value);
	HAL_RCC_DeInit();
  ResetHundler_Address();
}



static uint8_t BootLoader_CRC_Verify(uint8_t *pdata, uint32_t Data_Len, uint32_t Host_CRC){
  uint8_t CRC_Statues = CRC_VERIFICATION_FAILED;
	uint32_t MCU_CRC_Calculate = 0;
	uint8_t Data_Counter = 0;
	uint32_t Data_Buffer = 0;
	
	for(Data_Counter = 0; Data_Counter < Data_Len ; Data_Counter++){
	MCU_CRC_Calculate = HAL_CRC_Accumulate(CRC_ENGINE_OBJ,&Data_Buffer, 1);
	}
    __HAL_CRC_DR_RESET(CRC_ENGINE_OBJ);	
	if(MCU_CRC_Calculate == Host_CRC){
	CRC_Statues = CRC_VERIFICATION_PASSED;
	}
	else{
		CRC_Statues = CRC_VERIFICATION_FAILED;
	}
	return CRC_Statues;
}

static void Boot_Loader_Send_ACK(uint8_t Replay_Len){
uint8_t ACK_Value[2] = {0};
ACK_Value[0] = CBL_SEND_ACK;
ACK_Value[1] = Replay_Len;
HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, ACK_Value, 2, HAL_MAX_DELAY);
}


static void Boot_Loader_Send_NACK(){
  uint8_t ACK_Value = CBL_SEND_NACK;
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &ACK_Value, 1, HAL_MAX_DELAY);
}


static void Boot_Loader_Send_Data_To_Host(uint8_t *Host_Buffer, uint8_t Data_Length){
HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, Host_Buffer ,Data_Length, HAL_MAX_DELAY);
}
	

static void BootLoader_Get_Version(uint8_t *Host_Buffer){
uint8_t BL_Version[4] = {CBL_VENDOR_ID,CBL_SW_MAJOR_VERSION,CBL_SW_MINOR_VERSION, CBL_SW_PATCH_VERSION};
uint16_t Host_CMD_Packet_Len = 0;
uint32_t Host_CRC32 = 0;

Host_CMD_Packet_Len = Host_Buffer[0] + 1;
Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len)- CRC_TYPE_SIZE_BYTE));

if(CRC_VERIFICATION_PASSED == BootLoader_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_CMD_Packet_Len - 4, Host_CRC32)){
  Boot_Loader_Send_ACK(4);
	Boot_Loader_Send_Data_To_Host((uint8_t *)BL_Version,4);
}
else{
  Boot_Loader_Send_NACK();
}
}

static void BootLoader_Get_Help(uint8_t *Host_Buffer){
uint16_t Host_CMD_Packet_Len = 0;
	uint32_t Host_CRC32 = 0;
#if(BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Read the Comman Supported by the BootLoader \r\n");
#endif
	
	Host_CMD_Packet_Len = Host_Buffer[0] + 1;
  Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len)- CRC_TYPE_SIZE_BYTE));
	
	if(CRC_VERIFICATION_PASSED == BootLoader_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_CMD_Packet_Len - 4, Host_CRC32)){
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Passed\r\n");
#endif 	

	Boot_Loader_Send_ACK(12);
	Boot_Loader_Send_Data_To_Host((uint8_t *)(&Boat_Loader_Supported_CMDs[0]),12);
	
	}
	else{
	#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Failed\r\n");
#endif 	
		  Boot_Loader_Send_NACK();
	}
}


static void BootLoader_Get_Chip_Identification_Number(uint8_t *Host_Buffer){
uint16_t Host_CMD_Packet_Len = 0;
uint32_t Host_CRC32 = 0;
uint16_t MCU_Identification_Number = 0;
#if(BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Read the MCU Chip Identification Number\r\n");
#endif
	
	Host_CMD_Packet_Len = Host_Buffer[0] + 1;
  Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len)- CRC_TYPE_SIZE_BYTE));
	if(CRC_VERIFICATION_PASSED == BootLoader_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_CMD_Packet_Len - 4, Host_CRC32)){
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Passed\r\n");
#endif 	
	MCU_Identification_Number = (uint16_t)((DBGMCU->IDCODE) & 0X00000FFF);
	Boot_Loader_Send_ACK(2);
	Boot_Loader_Send_Data_To_Host((uint8_t *)&MCU_Identification_Number,2);
	
	}
	else{
	#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Failed\r\n");
#endif 	
		  Boot_Loader_Send_NACK();
	}
}


static uint8_t Host_Address_Verification(uint32_t Jump_Address){
	uint8_t Address_Verification = ADDRESS_IS_INVALID;
	if((Jump_Address >= SRAM1_BASE) && (Jump_Address <= SRAM1_END)){
	    Address_Verification = ADDRESS_IS_VALID;
	}
	else if((Jump_Address >= SRAM2_BASE) && (Jump_Address <= SRAM2_END)){
	    Address_Verification = ADDRESS_IS_VALID;
	}
	else if((Jump_Address >= CCMDATARAM_BASE) && (Jump_Address <= SRAM3_END)){
	    Address_Verification = ADDRESS_IS_VALID;
	}
		else if((Jump_Address >= FLASH_BASE) && (Jump_Address <= FLASH_END_VERTUALE)){
	   Address_Verification = ADDRESS_IS_VALID;
	}
		else{
		Address_Verification = ADDRESS_IS_INVALID;
		}
		return Address_Verification;
}


static void BootLoader_Jump_To_Address(uint8_t *Host_Buffer){
uint8_t Host_CMD_Packet_Len = 0;
	uint32_t Host_CRC32 = 0;
	uint32_t Host_Jump_Address = 0;
	uint8_t Address_Verification = ADDRESS_IS_INVALID;
#if(BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Jump to BootLoader Specific Address \r\n");
#endif
	Host_CMD_Packet_Len = Host_Buffer[0] + 1;
	 Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len)- CRC_TYPE_SIZE_BYTE));

	if(CRC_VERIFICATION_PASSED == BootLoader_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_CMD_Packet_Len - 4, Host_CRC32)){
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Passed\r\n");
#endif 	

	Boot_Loader_Send_ACK(1);
	Host_Jump_Address = *((uint32_t *) &Host_Buffer[2]);
		Address_Verification = Host_Address_Verification(Host_Jump_Address);
		if(Address_Verification == ADDRESS_IS_VALID){
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Address Varification Passed\r\n");
#endif 	
			Boot_Loader_Send_Data_To_Host((uint8_t *)&Address_Verification,1);
			Jump_Ptr Jump_Address = (Jump_Ptr)(Host_Jump_Address + 1);
		
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
		BL_Print_Message("Jump to : 0x%X\r\n", Jump_Address);
#endif 	
	      Jump_Address();
		}
		else{
		Boot_Loader_Send_Data_To_Host((uint8_t *)&Address_Verification, 1);
		}
	}
	else{
	#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Failed\r\n");
#endif 	
		  Boot_Loader_Send_NACK();
	}
}

static uint8_t CBL_STM32_Get_RDP_Level(){
FLASH_OBProgramInitTypeDef Flash_OBProgram;
	HAL_FLASHEx_OBGetConfig(&Flash_OBProgram);
	return (uint8_t)(Flash_OBProgram.RDPLevel);
}


static void BootLoader_Read_Protection_Level(uint8_t *Host_Buffer){
 uint8_t Host_CMD_Packet_Len = 0;
	uint32_t Host_CRC32 = 0;
	uint8_t RDP_Level = 0;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Read the Flash Read Protection Out Level\r\n");
#endif 	
	Host_CMD_Packet_Len = Host_Buffer[0] + 1;
	Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len)- CRC_TYPE_SIZE_BYTE));
if(CRC_VERIFICATION_PASSED == BootLoader_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_CMD_Packet_Len - 4, Host_CRC32)){
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Passed\r\n");
#endif 	
Boot_Loader_Send_ACK(1);
	RDP_Level = CBL_STM32_Get_RDP_Level();
		Boot_Loader_Send_Data_To_Host((uint8_t *)&RDP_Level, 1);
}
else{
	#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Failed\r\n");
#endif 	
		  Boot_Loader_Send_NACK();
	}
}



static uint8_t Perform_Flash_Erase(uint8_t Sector_Number, uint8_t Number_of_Sector){
uint8_t Sector_Validity_Statues = INVALID_SECTOR_NUMBER;
FLASH_EraseInitTypeDef pEraseInit;
	uint8_t Remaining_Sectors = 0;
	HAL_StatusTypeDef Hal_Statues = HAL_ERROR;
	uint32_t SectorError = 0;
	
	if(Number_of_Sector > CBL_FLASH_MAX_SECTOR_NUMBER){
	Sector_Validity_Statues = INVALID_SECTOR_NUMBER;
	}
	else{
	if((Sector_Number <= (CBL_FLASH_MAX_SECTOR_NUMBER -1)) || (Sector_Number == CBL_FLASH_MASS_ERASE)){
	if(CBL_FLASH_MASS_ERASE == Sector_Number){
	pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Flash Mass erase activation  \r\n");
#endif 	
	}
	else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("User Need Sector Erase \r\n");
#endif 	
	Remaining_Sectors = CBL_FLASH_MAX_SECTOR_NUMBER - Sector_Number;
		if(Number_of_Sector > Remaining_Sectors){
		     Number_of_Sector = Remaining_Sectors;
		}
		else{}
			pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
			pEraseInit.Sector = Sector_Number;
			pEraseInit.NbSectors = Number_of_Sector;
	}
	pEraseInit.Banks = FLASH_BANK_1;
	pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	Hal_Statues = HAL_FLASH_Unlock();
	
Hal_Statues = HAL_FLASHEx_Erase(&pEraseInit, &SectorError);
	if(SectorError == HAL_SUCCESSFUL_ERASE){
	Sector_Validity_Statues = SUCCESSFUL_ERASE;
	}
	else{
	Sector_Validity_Statues = UNSUCCESSFUL_ERASE;
	}
	Hal_Statues = HAL_FLASH_Lock();
	}
	else{
		Sector_Validity_Statues = UNSUCCESSFUL_ERASE;
	    }
	}
	return Sector_Validity_Statues;
}



static void BootLoader_Erase_Flash(uint8_t *Host_Buffer){
  uint8_t Host_CMD_Packet_Len = 0;
	uint32_t Host_CRC32 = 0;
	uint8_t Erase_Statues = 0;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Mass Erase or Sector Erase of the user flash \r\n");
#endif 	
   Host_CMD_Packet_Len = Host_Buffer[0] + 1;
	 Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len)- CRC_TYPE_SIZE_BYTE));
if(CRC_VERIFICATION_PASSED == BootLoader_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_CMD_Packet_Len - 4, Host_CRC32)){
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Passed\r\n");
#endif 	
Boot_Loader_Send_ACK(1);
Erase_Statues = Perform_Flash_Erase(Host_Buffer[2], Host_Buffer[3]);
if(Erase_Statues == SUCCESSFUL_ERASE){
Boot_Loader_Send_Data_To_Host((uint8_t *) &Erase_Statues,1);
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Successful Erase \r\n");
#endif 		
}
else{
Boot_Loader_Send_Data_To_Host((uint8_t *) &Erase_Statues,1);
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Erase Failed \r\n");
#endif 	
}
}
else{
		#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
			BL_Print_Message("CRC Varification Failed \r\n");
		#endif 	
			Boot_Loader_Send_NACK();

		}
}






static uint8_t Flash_Memory_Write_Payload(uint8_t *Host_Payload, uint32_t Payload_Start_Address, uint16_t Payload_Len){
HAL_StatusTypeDef Hal_Statues = HAL_ERROR;
	uint8_t Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
	uint16_t Payload_Counter = 0;
	Hal_Statues = HAL_FLASH_Unlock();
if(Hal_Statues != HAL_OK){
Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
}
else{
for(Payload_Counter = 0 ; Payload_Counter < Payload_Len ; Payload_Counter++){
Hal_Statues = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,Payload_Start_Address + Payload_Counter, Host_Payload[Payload_Counter]);
if(Hal_Statues != HAL_OK){
Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
	break;
}else{
Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_PASSED;
}
}
}
if((Flash_Payload_Write_Status == FLASH_PAYLOAD_WRITE_PASSED) && (Hal_Statues == HAL_OK)){
Hal_Statues = HAL_FLASH_Lock();
	if(Hal_Statues != HAL_OK){
	Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
	}
	else{
	Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_PASSED;	
	}
}
else{
	Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
}
return Flash_Payload_Write_Status;
}


static void BootLoader_Memory_Write(uint8_t *Host_Buffer){
  uint8_t Host_CMD_Packet_Len = 0;
	uint32_t Host_CRC32 = 0;
  uint32_t Host_Address = 0;
	uint8_t Payload_Len = 0;
	uint8_t Address_Verification = ADDRESS_IS_INVALID;
	uint8_t Flash_Payload_Write_Statues = FLASH_PAYLOAD_WRITE_FAILED;
	
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Mass Erase or Sector Erase of the user flash \r\n");
#endif 	
   Host_CMD_Packet_Len = Host_Buffer[0] + 1;
	 Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len)- CRC_TYPE_SIZE_BYTE));
if(CRC_VERIFICATION_PASSED == BootLoader_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_CMD_Packet_Len - 4, Host_CRC32)){
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Passed\r\n");
#endif 	
Boot_Loader_Send_ACK(1);
Host_Address = *((uint32_t *)(&Host_Buffer[2]));
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Host_Address = 0x%X \r\n", Host_Address);
#endif 		
Payload_Len = Host_Buffer[6];
Address_Verification = Host_Address_Verification(Host_Address);
	if(Address_Verification == ADDRESS_IS_VALID){
	Flash_Payload_Write_Statues = Flash_Memory_Write_Payload((uint8_t *)&Host_Buffer[7], Host_Address, Payload_Len);
		if(Flash_Payload_Write_Statues == FLASH_PAYLOAD_WRITE_PASSED){
		Boot_Loader_Send_Data_To_Host((uint8_t *) &Flash_Payload_Write_Statues, 1);
		}
		else{
	#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("PayLoad Valid \r\n");
#endif 	
Boot_Loader_Send_Data_To_Host((uint8_t *) &Flash_Payload_Write_Statues, 1);	
	}
	}
	else{
	Address_Verification = ADDRESS_IS_INVALID;
  Boot_Loader_Send_Data_To_Host((uint8_t *) &Address_Verification, 1);	
	}
}
else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Failed \r\n");
#endif 	
			Boot_Loader_Send_NACK();
		}	
}


static uint8_t Change_ROP_Level(uint8_t ROP_Level){
HAL_StatusTypeDef HAL_Status = HAL_ERROR;
FLASH_OBProgramInitTypeDef Flash_OBProgram;
uint8_t ROP_Level_Statues = ROP_LEVEL_CHANGE_INVALID;
HAL_Status = HAL_FLASH_OB_Unlock();	
	if(HAL_Status != HAL_OK){
	ROP_Level_Statues = ROP_LEVEL_CHANGE_INVALID;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Failed -> Unlock the FLASH Option Control Registers access \r\n");
#endif 			
	}
	else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Passed -> Unlock the FLASH Option Control Registers access \r\n");
#endif 	
Flash_OBProgram.OptionType = OPTIONBYTE_RDP;
Flash_OBProgram.Banks = FLASH_BANK_1;
Flash_OBProgram.RDPLevel = ROP_Level;
HAL_Status = HAL_FLASHEx_OBProgram(&Flash_OBProgram);
		if(HAL_Status != HAL_OK){
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Failed -> Program Option Bytes \r\n");
#endif 		
HAL_Status = HAL_FLASH_OB_Lock();
ROP_Level_Statues = ROP_LEVEL_CHANGE_INVALID;				
		}
		else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Passed-> Program Option Bytes \r\n");
#endif 		
HAL_Status = HAL_FLASH_OB_Launch();
			if(HAL_Status != HAL_OK){
ROP_Level_Statues = ROP_LEVEL_CHANGE_INVALID;					
			}
else{
HAL_Status = HAL_FLASH_OB_Lock();
	if(HAL_Status != HAL_OK){
	ROP_Level_Statues = ROP_LEVEL_CHANGE_INVALID;		
	}
	else{
	ROP_Level_Statues = ROP_LEVEL_CHANGE_VALID;	
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
		BL_Print_Message("Passed-> Program ROP to Level: 0x%X \r\n", ROP_Level);
#endif 			
	     }
    }			
	}
}
return ROP_Level_Statues;
}
	
	
	



static void BootLoader_Change_Read_Protection_Levels(uint8_t *Host_Buffer){
  uint8_t Host_CMD_Packet_Len = 0;
	uint32_t Host_CRC32 = 0;
	uint8_t ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
	uint8_t Host_ROP_Level = 0;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Mass Erase or Sector Erase of the user flash \r\n");
#endif 	
   Host_CMD_Packet_Len = Host_Buffer[0] + 1;
	 Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len)- CRC_TYPE_SIZE_BYTE));
if(CRC_VERIFICATION_PASSED == BootLoader_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_CMD_Packet_Len - 4, Host_CRC32)){
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Passed\r\n");
#endif 	
  Boot_Loader_Send_ACK(1);
	Host_ROP_Level = Host_Buffer[2];
	if((CBL_ROP_LEVEL_2 == Host_ROP_Level) || (OB_RDP_LEVEL2 == Host_ROP_Level)){
	   ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
	}
	else{
	if(CBL_ROP_LEVEL_0 == Host_ROP_Level){
	   Host_ROP_Level = 0xAA;
	}
else if(CBL_ROP_LEVEL_1 == Host_ROP_Level){
	   Host_ROP_Level = 0x55;
	}
ROP_Level_Status = Change_ROP_Level(Host_ROP_Level);
	}
Boot_Loader_Send_Data_To_Host((uint8_t *) &ROP_Level_Status, 1);	
}
else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("CRC Varification Failed \r\n");
#endif 	
			Boot_Loader_Send_NACK();
		}	
}




static void BootLoader_Enable_RW_Protection(uint8_t *Host_Bufer){

}

static void BootLoader_Memory_Read(uint8_t *Host_Bufer){

}
static void BootLoader_Get_Sector_Protection_Statues(uint8_t *Host_Bufer){

}
static void BootLoader_Read_OTP(uint8_t *Host_Bufer){

}



void BL_Print_Message(char *formate, ...){
char Message[100] = {0};
va_list args;
va_start(args, formate);
vsprintf(Message, formate, args);
#if (BL_DEBUG_METHOD == BL_ENABLE_UART_DEBUG_MESSAGE)
HAL_UART_Transmit(BL_DEBUG_UART,(uint8_t *)Message, sizeof(Message), HAL_MAX_DELAY);
#elif (BL_DEBUG_METHOD == BL_ENABLE_SPI_DEBUG_MESSAGE)
#elif (BL_DEBUG_METHOD == BL_ENABLE_CAN_DEBUG_MESSAGE) 
#endif

va_end(args);
}
	