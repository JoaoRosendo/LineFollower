#include <avr/interrupt.h> 
#include <avr/io.h>

extern uint8_t IR[5];
extern uint8_t MUXSELECTOR;
extern int AVRG;
extern long Motor_speed;
extern long P, I, D, previous_P; 
extern float Kp;
extern float Ki;
extern float Kd;

ISR(ADC_vect);
void ADC_init();
void AVRG_IR();
void PID();