// new code by K.-H. K�bbeler

#include <avr/io.h>
#include <stdlib.h>
#include "Transistortester.h"


//=================================================================
// selection of different functions

#define MAX_FUNC 4
#ifdef WITH_MENU
void function_menu() {
  uint8_t ii;
  uint8_t ll;
  uint8_t func_number;

  func_number = 0xff;
  for (ll=0;ll<((MAX_FUNC+1)*20);ll++) {
     func_number++;
     if (func_number > MAX_FUNC) func_number = 0;
     message_key_released(SELECTION_str);
     lcd_line2();
     if (func_number == 2) {
 #ifdef WITH_VEXT
        lcd_MEM_string(VOLTAGE_str);
 #else
        continue;
 #endif
     }
     if (func_number == MAX_FUNC) {
 #ifdef POWER_OFF
        lcd_MEM_string(OFF_str);
 #else
        continue;
 #endif
     }
     if (func_number == 0) lcd_MEM2_string(TESTER_str);
     if (func_number == 1) lcd_MEM2_string(FREQ_str);
     if (func_number == 3) lcd_MEM2_string(F_GEN_str);
     ii = wait_for_key_ms(SHORT_WAIT_TIME);
     if (ii >= 30) {
        if (func_number == 0) break;		// return to TransistorTester
        if (func_number == 1) GetFrequency();
        if (func_number == 2) show_vext();
        if (func_number == 3) {
           make_frequency();
        }
        if (func_number == MAX_FUNC) {
           ON_PORT &= ~(1<<ON_PIN);              //switch off power
           wait_for_key_ms(0); //never ending loop 
        }
        // don't increase function number for easier selection the same function
        func_number--;	
     } /* end if (ii >= 30) */
  } /* end for ll */
  return;
 } // end function_menu()

void show_vext() {
 #ifdef WITH_VEXT
 
  uint8_t key_pressed;
  uint8_t times;
  unsigned int Vext;
  // show the external voltage
  message_key_released(VOLTAGE_str);
  for (times=0;times<250;times++) {
     lcd_line2();
     lcd_clear_line();
     lcd_line2();
     lcd_MEM_string(Vext_str);          // Vext=
     ADC_DDR = 0;               //deactivate Software-UART
     Vext = W5msReadADC(TPext); // read external voltage 
//     ADC_DDR = TXD_MSK;               //activate Software-UART 
//#ifdef WITH_UART
//     uart_newline();          // start of new measurement
//#endif
  #if EXT_NUMERATOR <= (0xffff/U_VCC)
     DisplayValue(Vext*EXT_NUMERATOR/EXT_DENOMINATOR,-3,'V',3); // Display 3 Digits of this mV units
  #else
     DisplayValue((unsigned long)Vext*EXT_NUMERATOR/EXT_DENOMINATOR,-3,'V',3);  // Display 3 Digits of this mV units
  #endif
     key_pressed = wait_for_key_ms(800);
     if (key_pressed != 0) break;
  }  /* end for times */
 #endif  /* WITH_VEXT */
} /* end show_vext() */

/* *************************************************** */
/* make frequency starts a frequency generator at TP2 with 680 Ohm resistor */
/* some predefined frequencies can be switched with the key */
/* a long key press returns to the selection menu */
/* *************************************************** */
void make_frequency() {
#define MAX_FREQ_NR 18
  uint8_t times;
  uint8_t key_pressed;
  uint8_t freq_nr;

  message_key_released(F_GEN_str);	// display f-Generator and wait for key released
  // OC1B is connected with 680 Ohm resistor to TP2 (middle test pin) 
  TCCR1A = (0<<COM1B1) | (1<<COM1B0) | (0<<WGM11) | (0<<WGM10); // CTC mode, count to OCR1A
  TIMSK1 = 0;		// no interrupt used
  OCR1A = 1;		// highest frequency
  OCR1B	= 0;		// toggle OC1B at this count
  TIFR1 = (1<<OCF1A) | (1<<OCF1A) | (1<<TOV1);	// reset interrupt flags
  TCCR1C = 0;
  TCCR1B = (0<<WGM13) | (1<<WGM12) | (0<<CS12) | (0<<CS11) | (0<<CS10); // set counter mode to 9 PWM
  R_PORT = 0;		// set all resistor port outputs to GND
  R_DDR = (1<<PIN_RL1) | (1<<PIN_RL3);		// set TP1 and TP3 to output
  ADC_PORT = TXD_VAL;
  ADC_DDR = (1<<TP1) | TXD_MSK;			//connect TP1 to GND
  DDRB  |= (1<<DDB2);	// set output enable
  TCCR1B = (0<<WGM13) | (1<<WGM12) | (0<<CS12) | (0<<CS11) | (1<<CS10); // no clock divide
  key_pressed = 1;
  freq_nr = MAX_FREQ_NR;
  for (times=0; times<240; times++) {
     if(key_pressed >= 30) break;
     if (key_pressed != 0) {
       times = 0;	// reset counter
       freq_nr++;
       if (freq_nr > MAX_FREQ_NR) freq_nr = 0;
       lcd_line2();
       lcd_clear_line();	// clear line 2 for next frequency
       lcd_line2();
       if (freq_nr == 0) {
 #define F_TIM1 (F_CPU)
          TCCR1B = (0<<WGM13) | (1<<WGM12) | (0<<CS12) | (0<<CS11) | (1<<CS10); // no clock divide
          DisplayValue(2000000UL,0,'H',6);
          OCR1A = ((F_TIM1+2000000) / (2*2000000UL)) - 1;
       }
       // 1333.333kHz
       if (freq_nr == 1) {
          DisplayValue(1000000UL,0,'H',6);
          OCR1A = ((F_TIM1+1000000) / (2*1000000UL)) - 1;
       }
       // 800kHz, 666.666kHz
       if (freq_nr == 2) {
          DisplayValue(500000UL,0,'H',6);
          OCR1A = ((F_TIM1+500000) / (2*500000UL)) - 1;
       }
       // 444.444kHz, 400kHz, 362.636kHz, 333.333kHz, 307.692kHz 285.714kHz
       if (freq_nr == 3) {
          DisplayValue(250000UL,0,'H',6);
          OCR1A = ((F_TIM1+250000) / (2*250000UL)) - 1;
       }
       // 235.294kHz, 222.222kHz, 210.526kHz, 200kHz, 190.476kHz, 181.818kHz, 173.913kHz, 166.666kHz
       // 160kHz, 153.846kHz, 148.148kHz, 142.857kHz, 137.931kHz, 133.333kHz, 129.032kHz, 125kHz,
       // (33) 121.212kHz, 117.647kHz, 114.285kHz, 111.111kHz, 108.108kHz, 105.263kHz , 102.564kHz 
       if (freq_nr == 4) {
#undef F_TIM1
#define F_TIM1 (F_CPU)
#undef DIVIDER
#define DIVIDER ((F_TIM1+153600) / (2*153600UL))
          OCR1A = DIVIDER - 1;
          DisplayValue(((F_TIM1 * 50UL) + (DIVIDER / 2)) / DIVIDER,-2,'H',6);
       }
       if (freq_nr == 5) {
          DisplayValue(100000UL,0,'H',6);
          OCR1A = ((F_TIM1+100000) / (2*100000UL)) - 1;
       }
       if (freq_nr == 6) {
          DisplayValue(50000UL,0,'H',6);
          OCR1A = ((F_TIM1+50000) / (2*50000UL)) - 1;
       }
       if (freq_nr == 7) {
          DisplayValue(25000UL,0,'H',6);
          OCR1A = ((F_TIM1+25000) / (2*25000UL)) - 1;
       }
       if (freq_nr == 8) {
          DisplayValue(10000UL,0,'H',6);
          OCR1A = ((F_TIM1+10000) / (2*10000UL)) - 1;
       }
       if (freq_nr == 9) {
          DisplayValue(5000UL,0,'H',6);
          OCR1A = ((F_TIM1+5000) / (2*5000UL)) - 1;
       }
       if (freq_nr == 10) {
          DisplayValue(2500UL,0,'H',6);
          OCR1A = ((F_TIM1+2500) / (2*2500UL)) - 1;
       }
       if (freq_nr == 11) {
          DisplayValue(1000UL,0,'H',6);
          OCR1A = ((F_TIM1+1000) / (2*1000UL)) - 1;
       }
       if (freq_nr == 12) {
#undef DIVIDER
#define DIVIDER  ((F_TIM1+443) / (2*443UL))
          OCR1A = (DIVIDER - 1);
          DisplayValue(((F_TIM1 * 50UL) + (DIVIDER / 2)) / DIVIDER,-2,'H',6);
       }
       if (freq_nr == 13) {
#undef DIVIDER
#define DIVIDER  ((F_TIM1+442) / (2*442UL))
          OCR1A = DIVIDER - 1;
          DisplayValue(((F_TIM1 * 50UL) + (DIVIDER / 2)) / DIVIDER,-2,'H',6);
       }
       if (freq_nr == 14) {
#undef DIVIDER
#define DIVIDER  ((F_TIM1+440) / (2*440UL))
          OCR1A = DIVIDER - 1;
          DisplayValue(((F_TIM1 * 50UL) + (DIVIDER / 2)) / DIVIDER,-2,'H',6);
       }
       if (freq_nr == 15) {
#undef F_TIM1
#define F_TIM1 (F_CPU)
#undef DIVIDER
#define DIVIDER ((F_TIM1+250) / (2*250UL))
          OCR1A = DIVIDER - 1;
          DisplayValue(((F_TIM1 * 50UL) + (DIVIDER / 2)) / DIVIDER,-2,'H',6);
       }
// please use clock divider to build frequencies lower than 250 Hz (DIVIDER=64000 with 16MHz clock)
       if (freq_nr == 16) {
#undef F_TIM1
#define F_TIM1 (F_CPU/64)
#undef DIVIDER
#define DIVIDER ((F_TIM1+100) / (2*100UL))
          TCCR1B = (0<<WGM13) | (1<<WGM12) | (0<<CS12) | (1<<CS11) | (1<<CS10); // divide clock by 64
          OCR1A = DIVIDER - 1;
          DisplayValue(((F_TIM1 * 500UL) + (DIVIDER / 2)) / DIVIDER,-3,'H',6);
       }
       if (freq_nr == 17) {
#undef DIVIDER
#define DIVIDER ((F_TIM1+50) / (2*50UL))
//          TCCR1B = (0<<WGM13) | (1<<WGM12) | (0<<CS12) | (1<<CS11) | (1<<CS10); // divide clock by 64
          OCR1A = DIVIDER - 1;
          DisplayValue(((F_TIM1 * 500UL) + (DIVIDER / 2)) / DIVIDER,-3,'H',6);
       }
       if (freq_nr == 18) {
#undef F_TIM1
#define F_TIM1 (F_CPU/64)
#undef DIVIDER
#define DIVIDER ((F_TIM1+10) / (2*10UL))
//          TCCR1B = (0<<WGM13) | (1<<WGM12) | (0<<CS12) | (1<<CS11) | (1<<CS10); // divide clock by 64
          OCR1A = DIVIDER - 1;
          DisplayValue(((F_TIM1 * 500UL) + (DIVIDER / 2)) / DIVIDER,-3,'H',6);
       }
       lcd_data('z');		// append the z to get Hz unit
     } /* end if key_pressed != 0 */
     key_pressed = wait_for_key_ms(1000);
  } /* end for times */
  TCCR1B = 0;		// stop counter
  R_DDR = 0;		// switch resistor ports to Input
  DDRB  &= ~(1<<DDB2);	// disable output 

} /* end make frequency */

#endif  /* WITH_MENU */
