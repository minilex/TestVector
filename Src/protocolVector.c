#include "protocolVector.h"
#include "usbd_cdc_if.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define NPAR 15 /* Количество параметров */

#define PARAM_IN_PACKET_MAX 6
#define PREAMBLE_SIZE 4
#define N_SIZE 1
#define CODE_SIZE 1
#define VALUE_SIZE 4
#define CRC_SIZE 4

#define PREAMBLE 0xAAAAAAAA;

#define PACKET_MAX_SIZE (PREAMBLE_SIZE + N_SIZE + PARAM_IN_PACKET_MAX*(CODE_SIZE + VALUE_SIZE) + CRC_SIZE)
#define PACKET_N (NPAR/PARAM_IN_PACKET_MAX+1)
#define OUTBUF_SIZE ((PREAMBLE_SIZE + N_SIZE + CRC_SIZE )*(NPAR/PARAM_IN_PACKET_MAX + 1) + NPAR*(CODE_SIZE + VALUE_SIZE))



union Parameter {
    float p_f;
    uint32_t p_u32;
    uint32_t p_u8[4];
};

union Parameter param[NPAR];

/* Буфер для единовременной передачи в USB */
static uint8_t outBuf[OUTBUF_SIZE];
/* Массив пакетов */ 
static uint8_t packet[PACKET_N][PACKET_MAX_SIZE];


/*Вычисление контрольной суммы*/
uint32_t CRC32(uint32_t Crc, uint32_t Size, uint8_t *Buffer)
{
  while(Size--)
  {
    static const uint32_t CrcTable[] = { // 0xEDB88320 polynomial, right shifting  -
      0x00000000,0x1DB71064,0x3B6E20C8,0x26D930AC,0x76DC4190,0x6B6B51F4,0x4DB26158,0x5005713C,
      0xEDB88320,0xF00F9344,0xD6D6A3E8,0xCB61B38C,0x9B64C2B0,0x86D3D2D4,0xA00AE278,0xBDBDF21C };
    
    Crc = Crc ^ (uint32_t)*Buffer++; 
    Crc = (Crc >> 4) ^ CrcTable[Crc & 0x0F]; 
    Crc = (Crc >> 4) ^ CrcTable[Crc & 0x0F];
  }
  return(Crc);
}

/* Функция заполняет параметры случайными значениями или их номером */
void generateData (uint32_t dataType)
{
  static uint32_t a;
  switch (dataType)
  {
  case DATA_NUMBER:
    {
      for (uint8_t i = 0; i < NPAR; i++)
      {
        param[i].p_u32 = i;
      }
      break;
    }
  case DATA_RAND:
    {
      srand(a++);
      for (uint8_t i = 0; i < NPAR; i++)
      {
        param[i].p_u32 = rand();
      }
      break;
    }
  default: break;
  }  
}

/* Функция заполняет необходимое количетво пакетов, 
  копирует их в выходной буфер, передает по USB */
void sendBufer(uint16_t paramMask)
{
  uint32_t currentPacket = 0, currentParam = 0, totalParam = 0;
  uint32_t size = 0, offset = 0;
  
  /*Очистка буферов*/
  memset(outBuf, 0, sizeof(outBuf));
  for (uint8_t i = 0; i < PACKET_N; i++)
  {
    memset(&packet[i][0], 0, sizeof(&packet[0][0]));
  }
  
  for (uint8_t i = 0; i < NPAR; i++)
  { 
    /* Если параметр нужно передавать*/   
    if ((paramMask & (1<<i))!=0)
    {
      packet[currentPacket][PREAMBLE_SIZE+N_SIZE+currentParam*5] = i;/*code*/
      memcpy(&packet[currentPacket][0]+PREAMBLE_SIZE+N_SIZE+CODE_SIZE+currentParam*5,param[i].p_u8, 4);
      currentParam++;
      totalParam++;
      //Если заполнили один пакет
      if (currentParam==PARAM_IN_PACKET_MAX)
      {
        currentParam = 0;
        
        *((uint32_t*)(&packet[currentPacket][0])) = PREAMBLE; 
        packet[currentPacket][4] = PARAM_IN_PACKET_MAX;
        *((uint32_t*)(&packet[currentPacket][PREAMBLE_SIZE + N_SIZE + PARAM_IN_PACKET_MAX * (CODE_SIZE + VALUE_SIZE)])) = \
        CRC32(0,PACKET_MAX_SIZE-4,&packet[currentPacket][0] ); /*CRC*/
      
        currentPacket++;
      }
    }
    
    
  }
  /* Если хотя бы один параметр нужно передавать */
  if (totalParam>0)
  {
    /* Завершаем заполнение неполного пакета */
    *((uint32_t*)(&packet[currentPacket][0])) = PREAMBLE; /*Preamble*/
    packet[currentPacket][4] = currentParam;
    *((uint32_t*)(&packet[currentPacket][PREAMBLE_SIZE + N_SIZE + currentParam * (CODE_SIZE + VALUE_SIZE)])) = \
      CRC32(0, PREAMBLE_SIZE + N_SIZE + currentParam * (CODE_SIZE + VALUE_SIZE), &packet[currentPacket][0] ); /*CRC*/
    
    /* Формируем один буфер */
    for (uint8_t i = 0; i <= currentPacket ; i++)
    {
      size = PREAMBLE_SIZE + N_SIZE + packet[i][4]*(CODE_SIZE + VALUE_SIZE) + CRC_SIZE; 
      memcpy(outBuf + offset,(&packet[i][0]), size);
      offset += size;
    } 
    CDC_Transmit_FS(outBuf, (uint16_t)* &offset);
  }
}