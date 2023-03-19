/* RingBufMax
@brief  Stores values in a ring buffer and finds the highest one
@author Turi Scandurra
@date   2023.01.27
*/

#include "RingBufMax.h"

uint16_t ringBufMax(RingBufMax * buffer, uint16_t n){
    uint16_t max;
    buffer->buf[buffer->index] = n;
    if(buffer->index++ >= buflength-1) buffer->index = 0;
    for (uint8_t i = 0; i < buflength; i++) {
    if (max < buffer->buf[i]) {
      max = buffer->buf[i];
    }
  }
  return max;
};