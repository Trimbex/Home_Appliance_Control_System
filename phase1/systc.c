#include "systc.h"
#include "DIO.h"
#include "tm4c123gh6pm.h"

// Initialize SysTick Timer
void SysTick_Init(uint32_t reloadValue) {
    NVIC_ST_CTRL_R = 0;               // Disable SysTick during setup
    NVIC_ST_RELOAD_R = reloadValue;   // Set reload value
    NVIC_ST_CURRENT_R = 0;            // Clear current timer value
    NVIC_ST_CTRL_R = 0x05;            // Enable SysTick with system clock, no interrupt
}

void clc_initialization(int time){
  NVIC_ST_RELOAD_R = time;
  NVIC_ST_CTRL_R = 0x5; // 0000 0101
}

void SysTickEnable(){
  NVIC_ST_CTRL_R |= 0x05;
}

void SysTickDisable(){
  NVIC_ST_CTRL_R &= ~0x01;
}

// Set the SysTick Timer period
void SysTick_PeriodSet(uint32_t ui32Period) {
    NVIC_ST_RELOAD_R = ui32Period - 1; // Set the reload value
}

// Get the SysTick Timer period
uint32_t SysTick_PeriodGet(void) {
    return NVIC_ST_RELOAD_R + 1;      // Return reload value
}

// Get the current value of the SysTick Timer
uint32_t SysTick_ValueGet(void) {
    return NVIC_ST_CURRENT_R;         // Return current value
}

// Check if SysTick Timer has timed out
bool SysTick_Is_Time_Out(void) {
    return (NVIC_ST_CTRL_R & 0x10000) != 0; // Check COUNT flag
}