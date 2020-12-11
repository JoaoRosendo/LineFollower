#include <avr/io.h>
#include <avr/interrupt.h>
#include "serial_printf.h"
#include <avr/eeprom.h>

volatile uint8_t nec_ok = 0;
volatile uint8_t  i, nec_state = 0;
volatile unsigned long ir_code;


ISR(TIMER2_OVF_vect) {                           // Timer1 interrupt service routine (ISR)
  nec_state = 0;                                 // Reset decoding process
  TCCR2B = 0;                                    // Disable Timer1 module
}

void setup_int0 (int T){
  if (T!=0) {
    EICRA=0;
    EICRA |=  (1<< ISC00);                           //set interrupt to occur on change
    EIMSK=0;
    EIMSK |= (1<<INT0);                             // Enable external interrupt (INT0)
  }
  else if (T==0) {
    EICRA &=  ~(1<< ISC00);                          
    EIMSK &= ~(1<<INT0);   
  }
}

ISR(INT0_vect) {
  unsigned int timer_value;
  if(nec_state != 0){
    timer_value = TCNT2;                         // Store Timer1 value
    TCNT2 = 0;                                   // Reset Timer1
  }
  switch(nec_state){
  case 0 :       
                                                  // Start receiving IR data (we're at the beginning of 9ms pulse)
    TCNT2  = 0;                                  // Reset Timer1
    TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);     // Enable Timer1 module with 1/1024 prescaler ( 2 ticks every 1 us)
    nec_state = 1;                               // Next state: end of 9ms pulse (start of 4.5ms space)
    i = 0;
    break;
  case 1 :                                      // End of 9ms pulse
    if((timer_value > 148) || (timer_value < 132)){         // Invalid interval ==> stop decoding and reset
      nec_state = 0;                             // Reset decoding process
      TCCR2B = 0;                                // Disable Timer1 module
    }
    else
      nec_state = 2;                             // Next state: end of 4.5ms space (start of 562µs pulse)
    break;
  case 2 :                                      // End of 4.5ms space
    if((timer_value > 78) || (timer_value < 62)){
      nec_state = 0;                             // Reset decoding process
      TCCR2B = 0;                                // Disable Timer1 module
    }
    else
      nec_state = 3;                             // Next state: end of 562µs pulse (start of 562µs or 1687µs space)
    break;
  case 3 :                                      // End of 562µs pulse
    if((timer_value > 10) || (timer_value < 6)){           // Invalid interval ==> stop decoding and reset
      TCCR2B = 0;                                // Disable Timer1 module
      nec_state = 0;                             // Reset decoding process
    }
    else
      nec_state = 4;                             // Next state: end of 562µs or 1687µs space
    break;
  case 4 :                                      // End of 562µs or 1687µs space
    if((timer_value > 28) || (timer_value < 6)){           // Time interval invalid ==> stop decoding
      TCCR2B = 0;                                // Disable Timer1 module
      nec_state = 0;                             // Reset decoding process
      break;
    }
    if( timer_value > 15) {                     // If space width > 1ms (short space)
      cli();
      ir_code |= (1 << (31 - i));                // Write 1 to bit (31 - i)
      sei();  
    }
    else{                                         // If space width < 1ms (long space)
      cli();
      ir_code &= ~(1 << (31 - i));               // Write 0 to bit (31 - i)
      sei();
    }
    i++;
    if(i > 31){                                  // If all bits are received
      nec_ok = 1;                                // Decoding process OK          
      setup_int0(0);                              // Disable external interrupt (INT0)  
      break;
    }
    nec_state = 3;  
    break;                             // Next state: end of 562µs pulse (start of 562µs or 1687µs space)
  }
}

void setup_timer2() {
  // Timer1 module configuration
  TCCR2A = 0;
  TCCR2B = 0;                                    // Disable Timer1 module
  TCNT2  = 0;                                    // Set Timer1 preload value to 0 (reset)
  TIMSK2 = 1;                                    // enable Timer1 overflow interrupt
}



int button_press()
{
   if(nec_ok){                       
      nec_ok = 0;                                // Reset decoding process
      nec_state = 0;
      TCCR2B = 0;                                // Disable Timer1 module
      ir_code = ir_code >> 16;
      //printf("HEX: %x ", ir_code);              // Display inverted command in hex format*/
      setup_int0(1);  
      return 1;                           // Enable external interrupt (INT0)
   }
   else
   {
     return 0;
   }
   
}

