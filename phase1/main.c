#include <stdio.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include "DIO.h"
#include "systc.h"
#include <stdint.h>

// Bit positions for the red LED and SW2
#define LED_RED   (1U << 1)
#define SWITCH2   (1U << 0)

void UART0_Init(void) {
    SYSCTL_RCGCUART_R |= (1U << 0); // Enable UART0
    SYSCTL_RCGCGPIO_R |= (1U << 0); // Enable PORTA

    // Configure PA0 and PA1 for UART
    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) | 0x00000011;
    GPIO_PORTA_DEN_R |= 0x03;

    UART0_CTL_R &= ~0x01;           // Disable UART
    UART0_IBRD_R = 104;             // 16 MHz / (16 * 9600) = 104.166666...
    UART0_FBRD_R = 11;              // Fractional part
    UART0_LCRH_R = 0x60;            // 8-bit, no parity, 1-stop
    UART0_CTL_R |= 0x01;            // Enable UART
}

char UART0_ReadChar(void) {
    while ((UART0_FR_R & 0x10) != 0); // Wait until RXFE is 0
    return (char)(UART0_DR_R & 0xFF);
}

int main(void) {
    UART0_Init();

    SYSCTL_RCGCGPIO_R |= (1U << 5);
    while ((SYSCTL_PRGPIO_R & (1U << 5)) == 0) {}

    GPIO_PORTF_DIR_R |= LED_RED;
    GPIO_PORTF_DEN_R |= LED_RED;

    while (1) {
        char command = UART0_ReadChar();
        if (command == 'T') {
            GPIO_PORTF_DATA_R ^= LED_RED; // Toggle LED
        }
    }

    return 0;
}
