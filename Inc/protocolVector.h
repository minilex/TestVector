#ifndef PROTOCOLVECTOR_H
#define PROTOCOLVECTOR_H

#include <stdint.h>

#define DATA_NUMBER 0
#define DATA_RAND 1

void generateData (uint32_t dataType);
void sendBufer(uint16_t paramMask);

#endif