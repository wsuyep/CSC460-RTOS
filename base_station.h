#include <stdint.h>

uint8_t X    = 10; 
uint8_t Y    = 11; 


// Joystick Axis
int x      = 0;
int y      = 0;

void config();
uint16_t readADC(uint8_t channel);
void initADC();
void RommbaControl();


