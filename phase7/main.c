#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
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

// Pin definitions
#define LED_LAMP    (1U << 1)   // PF1 - Lamp
#define PLUG_CTRL   (1U << 2)   // PF2 - Plug control
#define MAG_SWITCH  (1U << 0)   // PB0 - Magnetic switch input
#define PUSH_BUTTON (1U << 1)   // PB1 - Push button for lamp control
#define BUZZER      (1U << 4)   // PF4 - Buzzer

// Constants
#define TEMP_LIMIT  32.0        // Temperature threshold in Celsius
#define UART_BASE   UART0_BASE

// Global variables
volatile float temperature = 0.0;
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

// System clock variable
uint32_t sys_clock;

// Function prototypes
void UART_Init(void);
void UART_SendString(const char *str);
void printmsg(const char *str);
void UART0_Init(void);
void UART0_WriteString(const char *str);
char UART0_ReadChar(void);
void init_gpio(void);
void init_timer(void);
void resetDHT(void);
void duty_cycle(void);
void delayMs(int ui32Ms);
void delayUs(uint32_t ui32Us);

/**
 * UART initialization function
 */
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

/**
 * Sends a string via UART.
 *
 * @param str Pointer to the null-terminated string.
 */
void UART_SendString(const char *str) {
    while (*str) {
        UARTCharPut(UART_BASE, *str++);
    }
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

/**
 * Debug function to print a message via UART.
 *
 * @param str Pointer to the null-terminated string.
 */
void printmsg(const char *str) {
    UART_SendString(str);
}

/**
 * Initialize GPIO for peripherals like the lamp, buzzer, and switches.
 */
void init_gpio(void) {
    // Enable clock for PORTF and PORTB
    SYSCTL_RCGCGPIO_R |= (1U << 5) | (1U << 1);
    while ((SYSCTL_PRGPIO_R & ((1U << 5) | (1U << 1))) == 0) {}

    // Configure lamp (LED), plug control, and buzzer as outputs on PORTF
    GPIO_PORTF_DIR_R |= (LED_LAMP | PLUG_CTRL | BUZZER);

    // Configure magnetic switch as input on PORTB
    GPIO_PORTB_DIR_R &= ~MAG_SWITCH;

    // Configure push button on PB1 as input
    GPIO_PORTB_DIR_R &= ~PUSH_BUTTON;

    // Enable digital function for relevant pins
    GPIO_PORTF_DEN_R |= (LED_LAMP | PLUG_CTRL | BUZZER);
    GPIO_PORTB_DEN_R |= (MAG_SWITCH | PUSH_BUTTON);

    // Enable pull-up resistor for the magnetic switch and button
    GPIO_PORTB_PUR_R |= (MAG_SWITCH | PUSH_BUTTON);

    // Unlock PF4 if necessary
    GPIO_PORTF_LOCK_R = 0x4C4F434B;  // Unlock GPIOCR register
    GPIO_PORTF_CR_R |= BUZZER;       // Allow changes to PF4

    // Ensure the buzzer is off initially
    GPIO_PORTF_DATA_R &= ~BUZZER;
}

/**
 * Timer initialization function for DHT11 communication.
 */
void init_timer(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    // Configure Timer0A for edge time capture mode
    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP);
    TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 0xFFFF);

    // Configure the pin used for Timer0A (PB6)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinConfigure(GPIO_PB6_T0CCP0);
    GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_6);

    // Register and enable Timer0A interrupt
    IntRegister(INT_TIMER0A, duty_cycle);
    TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
    TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
    IntEnable(INT_TIMER0A);
}

/**
 * Function to reset the DHT11 sensor and prepare it for data collection.
 */
void resetDHT(void) {
    i = j = hh = hl = th = tl = checksum = check = dataok = 0;

    // Generate start signal
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
    delayMs(18);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0xFF);
    delayUs(40);

    // Reconfigure pin for timer input
    init_timer();
    TimerEnable(TIMER0_BASE, TIMER_BOTH);
}

/**
 * Timer interrupt handler to process edge events and calculate timing differences.
 */
void duty_cycle(void) {
    uint32_t start = 0;
    TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
    start = TimerValueGet(TIMER0_BASE, TIMER_A);
    temp[i] = start;
    i += 1;

    if (i >= 3) {
        diff[i - 2] = temp[i - 1] - temp[i - 2];
        diff[i - 2] = (diff[i - 2] < 4000) ? 0 : 1;
    }
}

/**
 * Millisecond delay function.
 *
 * @param ui32Ms Number of milliseconds to delay.
 */
void delayMs(int ui32Ms) {
    SysCtlDelay((ui32Ms * sys_clock / 3 / 1000));
}

/**
 * Microsecond delay function.
 *
 * @param ui32Us Number of microseconds to delay.
 */
void delayUs(uint32_t ui32Us) {
    SysCtlDelay(ui32Us * (sys_clock / 3 / 1000000));
}

/**
 * Main application entry point.
 */
int main(void) {
    // Initialize system clock to 40 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
    sys_clock = SysCtlClockGet();

    UART_Init();
    init_gpio();
    printmsg("System Initialized\n");

    uint8_t prev_mag_switch_state = 0xFF;
    uint8_t app_lamp_state = 0;
    uint8_t button_state = 0;
    uint8_t prev_button_state = 0xFF;
    int l,mul=1;
    while (1) {
        resetDHT();
        delayMs(50);

        if (i >= 42) {
// Process the sensor data
          hh = 0; hl = 0; th = 0; tl = 0; checksum = 0;
          mul = 1;
          for (j = 1; j <= 8; j++) //first 8bit data is of first array index
          { // array {0,0,1,0,1,0,0,1}
              if(diff[j]==1) //integer value is 41
              {
                  for(l=0;l<8-j;l++)
                      mul=mul*2;
                  hh=hh+mul; //hh=41 when above array
              }
              mul=1;
          }
          mul=1;
          for (j = 9; j <= 16; j++)
          {
              if(diff[j]==1)
              {
                  for(l=0;l<16-j;l++)
                      mul=mul*2;
                  hl=hl+mul;
              }
              mul=1;
          }
          mul=1;
          for (j = 17; j <= 24; j++)
          {
              if(diff[j]==1)
              {
                  for(l=0;l<24-j;l++)
                      mul=mul*2;
                  th=th+mul; //th temp integer
              }
              mul=1;
          }
          mul=1;
          for (j = 25; j <= 32; j++)
          {
              if(diff[j]==1)
              {
                  for(l=0;l<32-j;l++)
                      mul=mul*2;
                  tl=tl+mul; //tl after .
              }
              mul=1;
          }
          mul=1;
          for (j = 33; j <= 40; j++)
          {
              if(diff[j]==1)
              {
                  for(l=0;l<40-j;l++)
                      mul=mul*2;
                  checksum=checksum+mul; //last 8bit(last 8 index of array)
              } //convert last 8 index into one integer
              mul=1;
          }
          check = hh+hl+th+tl;
          if (check == checksum) //check parity//last 8 index combined integer//addition of 32 index integer
          {
              dataok = 1;
              float temperature=th+(0.1*tl); //temp value in celsious

              // Create strings for temperature
              char temperatureStr[20];

              // Convert float values to strings
              snprintf(temperatureStr, sizeof(temperatureStr), "TEMP: %.2f\n", temperature);

              // Print the messages
              printmsg(temperatureStr);
              // *Buzzer Control Logic Starts Here*
              if (temperature >= TEMP_LIMIT) {
                  GPIO_PORTF_DATA_R |= BUZZER;  // Activate buzzer
                  printmsg("Temperature limit exceeded! Buzzer ON.\n");
              } else {
                  GPIO_PORTF_DATA_R &= ~BUZZER; // Deactivate buzzer
              }
              // *Buzzer Control Logic Ends Here*
          } else {
              printmsg("Checksum Error\n");
          }
        }

        // Handle UART commands
        if (!(UART0_FR_R & 0x10)) {
            char command = UART0_ReadChar();
            if (command == 'T') app_lamp_state ^= 1;
            if (command == 'P') GPIO_PORTF_DATA_R ^= PLUG_CTRL;
        }

        // Read button state
        button_state = (GPIO_PORTB_DATA_R & PUSH_BUTTON) ? 1 : 0;
        if (prev_button_state == 1 && button_state == 0) app_lamp_state ^= 1;
        prev_button_state = button_state;

        // Update lamp state
        if (app_lamp_state) GPIO_PORTF_DATA_R |= LED_LAMP;
        else GPIO_PORTF_DATA_R &= ~LED_LAMP;

        // Read and process magnetic switch state
        uint8_t mag_switch_state = (GPIO_PORTB_DATA_R & MAG_SWITCH) ? 1 : 0;
        if (mag_switch_state != prev_mag_switch_state) {
            UART0_WriteString(mag_switch_state ? "OPEN\n" : "CLOSED\n");
            prev_mag_switch_state = mag_switch_state;
        }
    }
}
