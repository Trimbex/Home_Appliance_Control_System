#include <stdio.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include "DIO.h"
#include "systc.h"
#include <stdint.h>

// Bit positions for the red LED and SW2
#define LED_RED   (1U << 1)
#define SWITCH2   (1U << 0)

int main(void) {
    // Enable clock for PORTF
    SYSCTL_RCGCGPIO_R |= (1U << 5); 
    while ((SYSCTL_PRGPIO_R & (1U << 5)) == 0) {}

    // Unlock PF0 (SW2) for configuration
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= SWITCH2;

    // Configure PF1 (red LED) as output and PF0 (SW2) as input
    GPIO_PORTF_DIR_R |= LED_RED;    // PF1 as output
    GPIO_PORTF_DIR_R &= ~SWITCH2;   // PF0 as input
    GPIO_PORTF_DEN_R |= (LED_RED | SWITCH2); // Enable digital function
    GPIO_PORTF_PUR_R |= SWITCH2;    // Enable pull-up resistor for SW2

    // State variable for toggling
    bool lastSwitchState = false;

    while (1) {
        // Read the current state of SW2 (active-low)
        bool currentSwitchState = !(GPIO_PORTF_DATA_R & SWITCH2);

        // If the switch is pressed and was not pressed before, toggle the LED
        if (currentSwitchState && !lastSwitchState) {
            GPIO_PORTF_DATA_R ^= LED_RED; // Toggle LED
            delay(300); // Debounce delay
        }

        // Update the last switch state
        lastSwitchState = currentSwitchState;

        // Small delay to prevent rapid polling
        delay(50);
    }

    return 0;
} 