// show_Resis_Cap.c , new code by K.-H. K�bbeler

#include <avr/io.h>
#include <stdlib.h>
#include "Transistortester.h"


#if FLASHEND > 0x3fff
//=================================================================
// selection of different functions

/* ****************************************************************** */
/* show_Resis13 measures the resistance of a part connected to TP1 and TP3 */
/* if RMETER_WITH_L is configured, inductance is also measured */
/* ****************************************************************** */

#if 1



void show_Resis13(void) {
  uint8_t key_pressed;
  message_key_released(RESIS_13_str_RL);	// "1-|=|-3 .."
#ifdef POWER_OFF
  uint8_t times;
  for (times=0;times<250;) 
#else
  while (1)		/* wait endless without the POWER_OFF option */
#endif
  {
        init_parts();		// set all parts to nothing found 
        GetResistance(TP3, TP1);
        GetResistance(TP1, TP3);

// display formats:
// |....|....|....|

// nothing found:
// 1-RR-3      [RL]
// ?

// only resistance:
// 1-RR-3      [RL]
// 12.34kO

// resistance and inductance
// 1-RR-LL-3   [RL]
// 12.34kO L=12.3uH
// 334.5 kHz Q=12.3    <--- only  if resonance detected

// resistance and inductance measured through resonance, rather tight format to fit on 3 lines
// 1-RR-LL-3 12.3kO
// 12.3uH if 22.0nF
// 334.5 kHz Q=12.3

// same case but on bigger screens:  <---- not yet implemented!
// 1-RR-LL-3
// 12.34kO
// 12.3uH if 22.0nF
// 334.5 kHz Q=12.3

        if (ResistorsFound != 0) {
#ifdef RMETER_WITH_L
	   ReadInductance();	// measure inductance, possible only with R<2.1k
 #ifdef SamplingADC
           sampling_lc(0,2);
           lcd_clear();
           if (inductor_lpre != 0 || lc_lx!=0) lcd_MEM_string(RESIS_13_str_RL);
           else lcd_MEM_string(RESIS_13_str_R);
 #else
           lcd_clear();
//           int ss = strlen(RESIS_13_str);
           if (inductor_lpre != 0) lcd_MEM_string(RESIS_13_str_RL);
           else lcd_MEM_string(RESIS_13_str_R);
 #endif
 #ifdef SamplingADC
           if (lc_lx==0) {
 #endif
              lcd_line2();
              RvalOut(1);		// show Resistance, probably ESR
 #ifdef SamplingADC
           } else {
              lcd_set_cursor(0,10);
              RvalOut(1);		// show Resistance, probably ESR
              lcd_line2();
              DisplayValue(lc_lx,lc_lpre,'H',3);	// output inductance
              lcd_MEM2_string(iF_str);		// " iF "
              DisplayValue(lc_cpar,-12,'F',3);	        // show parallel capacitance
              goto skip_inductor;
           }
           if (inductor_lpre != 0) {
              // resistor has also inductance
              lcd_MEM_string(Lis_str);		// "L="
              DisplayValue(inductor_lx,inductor_lpre,'H',3);        // output inductance
           }
           if (lc_fx) {
skip_inductor:
              lcd_next_line_wait(0);
              DisplayValue(lc_fx,lc_fpre,'H',4);
              lcd_MEM2_string(zQ_str);		// "z Q="
              DisplayValue(lc_qx, lc_qpre,' ',3);
           }
 #endif
#else		/* without Inductance measurement, only show resistance */
           lcd_line2();
           inductor_lpre = -1;		// prevent ESR measurement because Inductance is not tested
           RvalOut(1);			// show Resistance, no ESR
#endif
        } else {		/* no resistor found */
#ifdef RMETER_WITH_L
           lcd_clear();
           lcd_MEM_string(RESIS_13_str_R);
#endif
           lcd_line2();
           lcd_data('?');		// too big
        }
#if defined(POWER_OFF) && defined(BAT_CHECK)
     Bat_update(times);
#endif
     key_pressed = wait_for_key_ms(1000);
#ifdef WITH_ROTARY_SWITCH
     if ((key_pressed != 0) || (rotary.incre > 3)) break;
#else
     if (key_pressed != 0) break;
#endif
#if defined(POWER_OFF)
     times = Pwr_mode_check(times);	// no time limit with DC_Pwr_mode
#endif
  }  /* end for times */
  lcd_clear();
} /* end show_Resis13() */

#endif

/* ****************************************************************** */
/* show_Cap13 measures the capacity of a part connected to TP1 and TP3 */
/* ****************************************************************** */
#if (LCD_LINES > 2)
 #define SCREEN_TIME 1000
#else
 #define SCREEN_TIME 2000	/* line 2 is multi use, wait longer to read */
#endif
void show_Cap13(void) {
  uint8_t key_pressed;
  message_key_released(CAP_13_str);	// 1-||-3 at the beginning of line 1
  lcd_set_cursor(0,LCD_LINE_LENGTH-3);
  lcd_MEM2_string(CMETER_13_str);	// "[C]" at the end of line 1
#ifdef POWER_OFF
  uint8_t times;
  for (times=0;times<250;) 
#else
  while (1)		/* wait endless without the POWER_OFF option */
#endif
  {
     init_parts();		// set all parts to nothing found 
//     PartFound = PART_NONE;
//     NumOfDiodes = 0;
//     cap.cval_max = 0;		// clear cval_max for update of vloss
//     cap.cpre_max = -12;	// set to pF unit
     cap.v_loss = 0;		// clear vloss  for low capacity values (<25pF)!
     ReadCapacity(TP3, TP1);
#ifdef SamplingADC
     if (cap.cpre==-12 && cap.cval<100) {
        // if below 100 pF, try the alternative measuring method for small capacitors
        cap.cval=sampling_cap(TP3,TP1,0);
        cap.cpre=sampling_cap_pre;
     }
#endif
     lcd_line2();		// overwrite old Capacity value 
     if (cap.cpre < 0) {
        // a cap is detected
        DisplayValue(cap.cval,cap.cpre,'F',4);	// display capacity
        lcd_spaces(8 - _lcd_column);
        PartFound = PART_CAPACITOR;	// GetESR should check the Capacity value
        cap.esr = GetESR(TP3,TP1);
        if ( cap.esr < 65530) {
           // ESR is measured
           lcd_MEM_string(&ESR_str[1]);		// show also "ESR="
           DisplayValue(cap.esr,-2,LCD_CHAR_OMEGA,2); // and ESR value
	   lcd_clear_line();		// clear to end of line 2
           lcd_set_cursor(0,4);
           lcd_MEM2_string(Resistor_str);   // "-[=]- .."
           lcd_testpin(TP3);		// add the TP3
        } else {		// no ESR known
	   lcd_clear_line();		// clear to end of line, overwrite old ESR
           lcd_set_cursor(0,4);		// clear ESR resistor
           lcd_testpin(TP3);		// write the TP3
           lcd_spaces(5);			// overwrite ESR resistor symbol
        }
        GetVloss();                        // get Voltage loss of capacitor
 #if (LCD_LINES > 2)
        lcd_line3();
        if (cap.v_loss != 0) {
           lcd_MEM_string(&VLOSS_str[1]);  // "Vloss="
           DisplayValue(cap.v_loss,-1,'%',2);
        }
        lcd_clear_line();		// clear to end of line
 #else
        if (cap.v_loss != 0) {
           key_pressed = wait_for_key_ms(SCREEN_TIME);
  #ifdef WITH_ROTARY_SWITCH
//           if ((key_pressed != 0) || (rotary.incre > 3)) break;
  #else
//           if (key_pressed != 0) break;
  #endif
           lcd_line2();
           lcd_MEM_string(&VLOSS_str[1]);  // "Vloss="
           DisplayValue(cap.v_loss,-1,'%',2);
           lcd_clear_line();		// clear to end of line
        }
 #endif
     } else { /* no cap detected */
       lcd_data('?');
       lcd_clear_line();		// clear to end of line 2
 #if (LCD_LINES > 2)
       lcd_line3();	
       lcd_clear_line();	// clear old Vloss= message
 #endif
     }
#if defined(POWER_OFF) && defined(BAT_CHECK)
     Bat_update(times);
#endif
     key_pressed = wait_for_key_ms(SCREEN_TIME);
#ifdef WITH_ROTARY_SWITCH
     if ((key_pressed != 0) || (rotary.incre > 3)) break;
#else
     if (key_pressed != 0) break;
#endif
#if defined(POWER_OFF)
     times = Pwr_mode_check(times);	// no time limit with DC_Pwr_mode
#endif
  }  /* end for times */
  lcd_clear();		// clear to end of line
} /* end show_Cap13() */
#endif

#if defined(POWER_OFF) && defined(BAT_CHECK)
// monitor Battery in line 4 or line2, if a two line display 
void Bat_update(uint8_t tt) {
  if((tt % 16) == 0) {
 #if (LCD_LINES > 3)
     lcd_line4();
     Battery_check();
 #else
     wait_about1s();
     lcd_line2();
     Battery_check();
     wait_about2s();
 #endif
  }
};	/* end Bat_update() */
#endif
#if defined(POWER_OFF) 
uint8_t Pwr_mode_check(uint8_t tt) {
 if ((tt == 15) && (DC_Pwr_mode == 1)) return(0);  // when DC_Mode, next cycle start with 0
 return(tt + 1);	// otherwise increase
};
#endif
