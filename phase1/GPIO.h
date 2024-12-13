#define porta 0
#define portb 1
#define portc 2
#define portd 3
#define porte 4
#define portf 5

#include <stdbool.h>

bool readpin(int x, int pin);

int readport(int port);

void writeport(int value, int port);