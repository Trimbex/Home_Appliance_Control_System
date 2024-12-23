#include "tm4c123gh6pm.h"
#include <stdint.h>
#include <stdbool.h>

void SysTick_Init(uint32_t reloadValue);
void clc_initialization(int time);
void SysTickDisable(void);
void SysTickEnable(void);
uint32_t SysTickPeriodGet(void);
void SysTickPeriodSet(uint32_t ui32Period);
uint32_t SysTickValueGet(void);
bool SysTick_Is_Time_Out(void);
