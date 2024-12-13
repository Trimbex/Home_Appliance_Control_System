#include "GPIO.h"
#include "tm4c123gh6pm.h"
#include <stdbool.h>


bool readpin(int x, int pin){

  switch (x){
    
  case 0:
    return GPIO_PORTA_DATA_R & (1<<(pin-1));
    
  case 1:
    return GPIO_PORTB_DATA_R & (1<<(pin-1));
    
  case 2:
    return GPIO_PORTC_DATA_R & (1<<(pin-1));
    
  case 3:
    return GPIO_PORTD_DATA_R & (1<<(pin-1));
    
  case 4:
    return GPIO_PORTE_DATA_R & (1<<(pin-1));
    
  case 5:
    return GPIO_PORTF_DATA_R & (1<<(pin-1));
  }
}

int readport(int port){

  switch (port){
    
  case 0:
    return GPIO_PORTA_DATA_R;
    
  case 1:
    return GPIO_PORTB_DATA_R;
    
  case 2:
    return GPIO_PORTC_DATA_R;
    
  case 3:
    return GPIO_PORTD_DATA_R;
    
  case 4:
    return GPIO_PORTE_DATA_R;
    
  case 5:
    return GPIO_PORTF_DATA_R;
  
  default:
    return 0;
    
  }
}

void writeport(int value, int port){

  switch (port){
    
  case 0:
    GPIO_PORTA_DATA_R = value;
    break;
    
  case 1:
    GPIO_PORTB_DATA_R = value;
    break;
    
  case 2:
    GPIO_PORTC_DATA_R = value;
    break;
    
  case 3:
    GPIO_PORTD_DATA_R = value;
    break;
    
  case 4:
    GPIO_PORTE_DATA_R = value;
    break;
    
  case 5:
    GPIO_PORTF_DATA_R = value;
    break;
  }
}