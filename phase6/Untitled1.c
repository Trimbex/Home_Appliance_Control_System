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

#define UART_BASE UART0_BASE


volatile int temp[43];
volatile int diff[43];
volatile unsigned int i=0;
volatile unsigned int j=0;
volatile unsigned int hh = 0;
volatile unsigned int hl = 0;
volatile unsigned int th = 0;
volatile unsigned int tl = 0;
volatile unsigned int checksum = 0;
volatile unsigned int check = 0;
volatile unsigned int dataok = 0 ;

// function prototypes
void init_timer(void);
void duty_cycle(void);
void delayUs(uint32_t ui32Us);
void delayMs(int ui32Ms);
void delay(int n);
// Function prototypes
void UART_Init(void);
void UART_SendString(const char *str);
void printmsg(const char *str);
void resetDHT(void);

// global variables
uint32_t sys_clock;
uint32_t start = 0, end = 0, length = 0;

// Simple millisecond delay using loops
void delay(int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 3160; j++) {
            // Just a delay
        }
    }
}


// UART init (already provided)
void UART_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)) {}
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {}

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


int main(void)
{
// Configure system clock at 40 MHz.
SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
sys_clock = SysCtlClockGet();

// Enable the processor to respond to interrupts.
IntMasterEnable();
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); //enable port B
GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6); //connect sensor at PB6
GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00); //off PB6 for 18 ms
delayMs(18) ; //18ms delay
GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0xff); //on PB6
delayUs(40) ; //delay 40us
init_timer(); // timer initialisation and make PB6 as a input timer

TimerEnable(TIMER0_BASE, TIMER_BOTH);

UART_Init();
    printmsg("UART Initialized. Hello, world!\n");

int l,mul=1;

while(1) {
    // Reset DHT11 and start new reading
    resetDHT();
    
    // Wait for enough readings
    delayMs(50);
    
    // Process readings if we have enough data
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
            mul=1; //hh is humidity integer
        }
        mul=1;
        for (j = 9; j <= 16; j++)
        {
            if(diff[j]==1)
            {
                for(l=0;l<16-j;l++)
                    mul=mul*2;
                hl=hl+mul; //hl is humidity after . with 0.1 multiple
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
            float humidity=hh+(0.1*hl); //humidity value in %
            float temperature=th+(0.1*tl); //temp value in celsious

            // Create strings for humidity and temperature
            char humidityStr[20];
            char temperatureStr[20];

            // Convert float values to strings
            snprintf(humidityStr, sizeof(humidityStr), "Humidity: %.2f%%\n", humidity);
            snprintf(temperatureStr, sizeof(temperatureStr), "Temperature: %.2f°C\n", temperature);

            // Print the messages
            printmsg(temperatureStr);
            printmsg(humidityStr);
        } else {
            printmsg("Checksum Error\n");
        }
    }
}

}

void resetDHT(void) {
    // Reset all variables
    i = 0;
    j = 0;
    hh = 0;
    hl = 0;
    th = 0;
    tl = 0;
    checksum = 0;
    check = 0;
    dataok = 0;
    
    // Generate start signal
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
    delayMs(18);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0xff);
    delayUs(40);
    
    // Reconfigure pin for timer input
    init_timer();
    TimerEnable(TIMER0_BASE, TIMER_BOTH);
}

void init_timer(void)
{
// Enable and configure Timer0 peripheral.
SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

// Initialize timer A to count up in edge time mode
TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP ));

// Timer A records pos edge time
TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);

//set the value that the timers count to max 0xffff
TimerLoadSet(TIMER0_BASE, TIMER_A, 0xffff);

//Configure the pin that the timer reads from (PB6)
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
GPIOPinConfigure(GPIO_PB6_T0CCP0);
GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_6);

// Registers a interrupt function to be called when timer A hits a Pos edge event
IntRegister(INT_TIMER0A, duty_cycle); //positive edge timer
// Makes sure the interrupt is cleared
TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
// Enable the indicated timer interrupt source.
TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
// The specified interrupt is enabled in the interrupt controller.
IntEnable(INT_TIMER0A);
}

//When positive edge is hit record the values and find the difference, and found data is 0 or 1
void duty_cycle(void)
{
TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
start = TimerValueGet(TIMER0_BASE, TIMER_A);
temp[i] = start;
i += 1;
if (i>=3)
  {
    diff[i-2]=temp[i-1]-temp[i-2]; //timer interval when +ve edge found
    if(diff[i-2]<4000) //when timer interval is less than 4000 means data 0 read
    diff[i-2]=0;
    else
    diff[i-2]=1;
  }
}

void delayMs(int ui32Ms)
{
  SysCtlDelay((ui32Ms * SysCtlClockGet() /3/ 1000)); //ms delay
}

void delayUs(uint32_t ui32Us)
{
  SysCtlDelay(ui32Us * (SysCtlClockGet() / 3 / 1000000)); //us delay
}