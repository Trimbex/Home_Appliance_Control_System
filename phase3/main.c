#include <stdio.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include <stdint.h>

// Bit positions for the magnetic switch, lamp (LED), and plug control
#define LED_LAMP   (1U << 1)   // PF1 - Lamp
#define PLUG_CTRL  (1U << 2)   // PF2 - Plug control
#define MAG_SWITCH (1U << 0)   // PB0 - Magnetic switch input

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

void UART0_WriteString(const char *str) {
    while (*str) {
        while ((UART0_FR_R & 0x20) != 0); // Wait until TXFF is 0 (transmit FIFO not full)
        UART0_DR_R = *str++;
    }
}

char UART0_ReadChar(void) {
    while ((UART0_FR_R & 0x10) != 0); // Wait until RXFE is 0
    return (char)(UART0_DR_R & 0xFF);
}

void init_gpio(void) {
    // Enable clock for PORTF and PORTB
    SYSCTL_RCGCGPIO_R |= (1U << 5) | (1U << 1);
    while ((SYSCTL_PRGPIO_R & ((1U << 5) | (1U << 1))) == 0) {} // Wait until ready
    
    // Configure lamp (LED) and plug control as output on PORTF
    GPIO_PORTF_DIR_R |= (LED_LAMP | PLUG_CTRL);
    
    // Configure magnetic switch as input on PORTB
    GPIO_PORTB_DIR_R &= ~MAG_SWITCH;
    
    // Enable digital function for relevant pins
    GPIO_PORTF_DEN_R |= (LED_LAMP | PLUG_CTRL);
    GPIO_PORTB_DEN_R |= MAG_SWITCH;
    
    // Enable pull-up resistor for the magnetic switch on PORTB
    GPIO_PORTB_PUR_R |= MAG_SWITCH;
}

int main(void) {
    UART0_Init();
    init_gpio();
    
    uint8_t prev_mag_switch_state = 0xFF; // Initialize to an invalid state
    
    while (1) {
        // Check if there's a pending command
        if (!(UART0_FR_R & 0x10)) { // Check if RXFE is 0 (data available)
            char command = UART0_ReadChar();
            switch (command) {
                case 'T':
                    GPIO_PORTF_DATA_R ^= LED_LAMP; // Toggle lamp (LED)
                    break;
                case 'P':
                    GPIO_PORTF_DATA_R ^= PLUG_CTRL; // Toggle plug control
                    break;
                default:
                    break;
            }
        }
        
        // Read the current magnetic switch state from PORTB
        uint8_t mag_switch_state = (GPIO_PORTB_DATA_R & MAG_SWITCH) ? 1 : 0;
        
        // Send a message only if the state has changed
        if (mag_switch_state != prev_mag_switch_state) {
            if (mag_switch_state) {
                UART0_WriteString("CLOSED\n");
            } else {
                UART0_WriteString("OPEN\n");
            }
            prev_mag_switch_state = mag_switch_state; // Update the previous state
        }
    }
    
    return 0;
}