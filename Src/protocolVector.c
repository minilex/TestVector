#include <stdint.h>
#include <string.h>

#define NPAR 15 //Количество параметров

#define PARAM_IN_PACKET_MAX 6
#define PREAMBLE_SIZE 4
#define CODE_SIZE 1
#define VALUE_SIZE 4
#define CRC_SIZE 4

union Parameter {
    float p_f;
    uint32_t p_u32;
    uint32_t p_u8[4];
};

union Parameter param[NPAR];

uint8_t outBuf[(PREAMBLE_SIZE+CRC_SIZE)*(NPAR/PARAM_IN_PACKET_MAX+1)+NPAR*(CODE_SIZE+VALUE_SIZE)];

void sendBufer(uint16_t paramMask)
{
  uint32_t nParamMask, packetNo = 0;
  
  
  *((uint32_t*)outBuf)=0xAAAAAAAA;
  
  for (uint8_t i = 0; i < NPAR; i++)
  { //Если параметр нужно передавать   
    if ((paramMask & (1<<i))!=0)
    {
      memcpy(outBuf+PREAMBLE_SIZE+CODE_SIZE+nParamMask*5,param[i].p_u8, 4);
      nParamMask++;
      if (nParamMask==PARAM_IN_PACKET_MAX)
      {
        
      }
    }
    
  }
}