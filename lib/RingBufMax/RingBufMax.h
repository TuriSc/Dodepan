/* RingBufMax
@brief  Stores values in a ring buffer and returns the highest one
@author Turi Scandurra
@date   2023.01.27
*/
#ifndef RINGBUFMAX_H
#define RINGBUFMAX_H

#include "stdint.h"

#define buflength 10

typedef struct {
    uint16_t buf[buflength];
    uint16_t index;
} RingBufMax;

uint16_t ringBufMax(RingBufMax * buffer, uint16_t n);

#endif
