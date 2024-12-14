#include <stdio.h> 
#include <stdlib.h> 
#include "tm4c123gh6pm.h" 
#include <stdint.h> 

// Bit positions for the red LED, switches, and plug control 
#define LED_RED   (1U << 1)   // PF1 - Red LED
#define SWITCH1   (1U << 3)   // PF4 - SW1 (left switch)
#define SWITCH2   (1U << 0)   // PF0 - SW2 (right switch)
#define PLUG_CTRL (1U << 4)   // PF3 - Plug Control Pin


void init_gpio(void) {
    // Enable PORTF for LED, switches, and plug control 
    SYSCTL_RCGCGPIO_R |= (1U << 5); 
    while ((SYSCTL_PRGPIO_R & (1U << 5)) == 0) {} 

    // Configure LED and PLUG_CTRL as output 
    GPIO_PORTF_DIR_R |= (LED_RED | PLUG_CTRL); 
    
    // Configure SWITCH1 and SWITCH2 as input 
    GPIO_PORTF_DIR_R &= ~(SWITCH1 | SWITCH2);

    // Enable digital function for all pins
    GPIO_PORTF_DEN_R |= (LED_RED | SWITCH1 | SWITCH2 | PLUG_CTRL); 

    // Enable internal pull-up resistors for the switches
    GPIO_PORTF_PUR_R |= (SWITCH1 | SWITCH2);

    // Initialize outputs to OFF 
    GPIO_PORTF_DATA_R &= ~(LED_RED | PLUG_CTRL); 
}

int main(void) { 
    init_gpio();
    
    uint8_t prev_switch1_state = 1;  // Initial state of SW1 (pulled up)
    uint8_t prev_switch2_state = 1;  // Initial state of SW2 (pulled up)
    uint8_t plug_state = 0;          // Initial plug state is OFF
    uint8_t led_state = 0;           // Initial LED state is OFF

    while (1) { 
        // Read current switch states
        uint8_t current_switch1_state = (GPIO_PORTF_DATA_R & SWITCH1) ? 1 : 0;
        uint8_t current_switch2_state = (GPIO_PORTF_DATA_R & SWITCH2) ? 1 : 0;

        // Switch 1 (Left Switch) - Control LED
        if (current_switch1_state == 0 && prev_switch1_state == 1) {
            delay(50000);  // Debounce
            if ((GPIO_PORTF_DATA_R & SWITCH1) == 0) {
                // Toggle LED
                GPIO_PORTF_DATA_R ^= LED_RED;
                led_state = !led_state;
            }
        }

        // Switch 2 (Right Switch) - Control Plug
        if (current_switch2_state == 0 && prev_switch2_state == 1) {
            delay(50000);  // Debounce
            if ((GPIO_PORTF_DATA_R & SWITCH2) == 0) {
                // Toggle Plug
                if (plug_state == 0) {
                    GPIO_PORTF_DATA_R |= PLUG_CTRL;   // Turn plug ON
                    plug_state = 1;
                } else {
                    GPIO_PORTF_DATA_R &= ~PLUG_CTRL;  // Turn plug OFF
                    plug_state = 0;
                }
            }
        }

        // Update previous switch states
        prev_switch1_state = current_switch1_state;
        prev_switch2_state = current_switch2_state;
    } 

    return 0; 
}
