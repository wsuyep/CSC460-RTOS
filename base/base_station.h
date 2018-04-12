#include <stdint.h>

uint8_t X    = 11; 
uint8_t Y    = 10; 

uint8_t X_servo    = 13; 
uint8_t Y_servo    = 12; 


// Joystick Axis for Rommba
int rx      = 0;
int ry      = 0;

//joystick for servo
int rx_servo = 0;
int ry_servo = 0;

int laser = 0;


void config();
uint16_t readADC(uint8_t channel);
void initADC();
void RommbaControl();


