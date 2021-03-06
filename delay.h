#ifndef __DELAY_H__
#define __DELAY_H__
#include <stdint.h>
#include "timeout.h"

void delay_init(void);
void delay_us(uint32_t us);
#define delay_ms(ms) timeout_delay_ms(ms)


#endif // __DELAY_H__
