#include <stdint.h>

uint8_t X    = 11; 
uint8_t Y    = 10; 


// Joystick Axis
int rx      = 0;
int ry      = 0;

void config();
uint16_t readADC(uint8_t channel);
void initADC();
void RommbaControl();


