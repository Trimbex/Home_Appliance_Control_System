#include <stdio.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"
#include "driverlib/fpu.h"
#include "utils/uartstdio.h"
#include <stdbool.h>
#include <stdbool.h>

#define UART_BASE UART0_BASE
// Bit positions for the magnetic switch, lamp (LED), and plug control
#define LED_LAMP   (1U << 1)   // PF1 - Lamp
#define PLUG_CTRL  (1U << 2)   // PF2 - Plug control
#define MAG_SWITCH (1U << 0)   // PB0 - Magnetic switch input


volatile int temp[43];
volatile int diff[43];
volatile unsigned int i = 0;
volatile unsigned int j = 0;
volatile unsigned int hh = 0;
volatile unsigned int hl = 0;
volatile unsigned int th = 0;
volatile unsigned int tl = 0;
volatile unsigned int checksum = 0;
volatile unsigned int check = 0;
volatile unsigned int dataok = 0;

// Function prototypes
void init_timer(void);
void duty_cycle(void);
void delayUs(uint32_t ui32Us);
void delayMs(int ui32Ms);
//void delay(int n);
void UART_Init(void);
void UART_SendString(const char *str);
void printmsg(const char *str);

// Global variables
uint32_t sys_clock;
uint32_t start = 0, end = 0, length = 0;


// UART init (already provided)
void UART_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)) {}
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {}

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

// UART send string
void UART_SendString(const char *str) {
    while (*str) {
        UARTCharPut(UART_BASE, *str++);
    }
}

// printmsg for debugging
void printmsg(const char *str) {
    UART_SendString(str);
}


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
    
    
    // Configure system clock at 40 MHz.
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
    sys_clock = SysCtlClockGet();

    // Enable the processor to respond to interrupts.
    IntMasterEnable();
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // Enable port B
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6); // Connect sensor at PB6
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00); // Off PB6 for 18 ms
    delayMs(18); // 18 ms delay
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0xff); // On PB6
    delayUs(40); // Delay 40 us
    init_timer(); // Timer initialization and make PB6 as an input timer

    TimerEnable(TIMER0_BASE, TIMER_BOTH);

    UART_Init();
    printmsg("UART Initialized. Hello, world!\n");

    int l, mul = 1;
    while (1) {
     
        // Process the sensor data
        hh = 0; hl = 0; th = 0; tl = 0; checksum = 0;
        mul = 1;
        if (i >= 42) {
            for (j = 1; j <= 8; j++) { // First 8-bit data is of first array index
                if (diff[j] == 1) { // Integer value is 41
                    for (l = 0; l < 8 - j; l++)
                        mul = mul * 2;
                    hh = hh + mul; // hh = 41 when above array
                }
                mul = 1; // hh is humidity integer
            }
            mul = 1;
            for (j = 9; j <= 16; j++) {
                if (diff[j] == 1) {
                    for (l = 0; l < 16 - j; l++)
                        mul = mul * 2;
                    hl = hl + mul; // hl is humidity after . with 0.1 multiple
                }
                mul = 1;
            }
            mul = 1;
            for (j = 17; j <= 24; j++) {
                if (diff[j] == 1) {
                    for (l = 0; l < 24 - j; l++)
                        mul = mul * 2;
                    th = th + mul; // th temp integer
                }
                mul = 1;
            }
            mul = 1;
            for (j = 25; j <= 32; j++) {
                if (diff[j] == 1) {
                    for (l = 0; l < 32 - j; l++)
                        mul = mul * 2;
                    tl = tl + mul; // tl after .
                }
                mul = 1;
            }
            mul = 1;
            for (j = 33; j <= 40; j++) {
                if (diff[j] == 1) {
                    for (l = 0; l < 40 - j; l++)
                        mul = mul * 2;
                    checksum = checksum + mul; // Last 8-bit (last 8 index of array)
                }
                mul = 1;
            }
            check = hh + hl + th + tl;
            if (check == checksum) { // Check parity
                dataok = 1;
                float humidity = hh + (0.1 * hl); // Humidity value in %
                float temperature = th + (0.1 * tl); // Temp value in Celsius

                // Create strings for humidity and temperature
                char humidityStr[20];
                char temperatureStr[20];

                // Convert float values to strings
                snprintf(humidityStr, sizeof(humidityStr), "Humidity: %.2f%%\n", humidity);
                snprintf(temperatureStr, sizeof(temperatureStr), "TEMP: %.2f\n", temperature);
                UART0_WriteString(temperatureStr);
                // Print the messages
                printmsg(temperatureStr);
                printmsg(humidityStr);
                delay(2000);
            } else {
                dataok = 0;
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

        // Wait for a command from UART
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
}
    
    void init_timer(void) {
      // Enable and configure Timer0 peripheral.
      SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

      // Initialize timer A to count up in edge time mode
      TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP));

      // Timer A records pos edge time
      TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);

      // Set the value that the timers count to max 0xffff
      TimerLoadSet(TIMER0_BASE, TIMER_A, 0xffff);

      // Configure the pin that the timer reads from (PB6)
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
      GPIOPinConfigure(GPIO_PB6_T0CCP0);
      GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_6);

      // Register an interrupt function to be called when timer A hits a Pos edge event
      IntRegister(INT_TIMER0A, duty_cycle); // Positive edge timer
      // Make sure the interrupt is cleared
      TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
      // Enable the indicated timer interrupt source.
      TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
      // The specified interrupt is enabled in the interrupt controller.
      IntEnable(INT_TIMER0A);
  }

  // When positive edge is hit record the values and find the difference, and found data is 0 or 1
  void duty_cycle(void) {
      TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
      start = TimerValueGet(TIMER0_BASE, TIMER_A);
      temp[i] = start;
      i += 1;
      if (i >= 3) {
          diff[i - 2] = temp[i - 1] - temp[i - 2]; // Timer interval when +ve edge found
          if (diff[i - 2] < 4000) // When timer interval is less than 4000 means data 0 read
              diff[i - 2] = 0;
          else
              diff[i - 2] = 1;
      }
  }

  void delayMs(int ui32Ms) {
      SysCtlDelay((ui32Ms * SysCtlClockGet() / 3 / 1000)); // ms delay
  }

  void delayUs(uint32_t ui32Us) {
      SysCtlDelay(ui32Us * (SysCtlClockGet() / 3 / 1000000)); // us delay
  }