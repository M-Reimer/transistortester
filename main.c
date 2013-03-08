
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <math.h>

#define MAIN_C
#include "Transistortester.h"

#ifndef INHIBIT_SLEEP_MODE
  // prepare sleep mode
  EMPTY_INTERRUPT(TIMER2_COMPA_vect);
  EMPTY_INTERRUPT(ADC_vect);
#endif

//begin of transistortester program
int main(void) {
  //switch on
  ON_DDR = (1<<ON_PIN);			// switch to output
#ifdef PULLUP_DISABLE
  ON_PORT = (1<<ON_PIN); 		// switch power on 
#else
  ON_PORT = (1<<ON_PIN)|(1<<RST_PIN); 	// switch power on , enable internal Pullup for Start-Pin
#endif
  uint8_t tmp;
  //ADC-Init
  ADCSRA = (1<<ADEN) | AUTO_CLOCK_DIV;	//prescaler=8 or 64 (if 8Mhz clock)
#ifdef __AVR_ATmega8__
// #define WDRF_HOME MCU_STATUS_REG
 #define WDRF_HOME MCUCSR
#else
 #define WDRF_HOME MCUSR
#endif
  tmp = (WDRF_HOME & (1<<WDRF));	// save Watch Dog Flag
  WDRF_HOME &= ~(1<<WDRF);	 	//reset Watch Dog flag
  wdt_disable();			// disable Watch Dog
#ifndef INHIBIT_SLEEP_MODE
  // switch off unused Parts
  PRR = (1<<PRTWI) | (1<<PRTIM0) | (1<<PRSPI) | (1<<PRUSART0);
  DIDR0 = (1<<ADC5D) | (1<<ADC4D) | (1<<ADC3D);	
  TCCR2A = (0<<WGM21) | (0<<WGM20);		// Counter 2 normal mode
 #if F_CPU <= 1000000UL
  TCCR2B = (1<<CS22) | (0<<CS21) | (1<<CS20);	//prescaler 128, 128us @ 1MHz
  #define T2_PERIOD 128
 #endif 
 #if F_CPU == 2000000UL
  TCCR2B = (1<<CS22) | (1<<CS21) | (0<<CS20);	//prescaler 256, 128us @ 2MHz
  #define T2_PERIOD 128
 #endif 
 #if F_CPU == 4000000UL
  TCCR2B = (1<<CS22) | (1<<CS21) | (0<<CS20);	//prescaler 256, 64us @ 2MHz
  #define T2_PERIOD 64
 #endif 
 #if F_CPU >= 8000000UL
  TCCR2B = (1<<CS22) | (1<<CS21) | (1<<CS20);	//prescaler 1024, 128us @ 8MHz, 64us @ 16MHz
  #define T2_PERIOD (1024 / (F_CPU / 1000000UL));	/* set to 128 or 64 us */
 #endif 
  sei();				// enable interrupts
#endif
  lcd_init();				//initialize LCD
	
//  ADC_PORT = TXD_VAL;
//  ADC_DDR = TXD_MSK;
  if(tmp) { 
     // check if  Watchdog-Event 
     // this happens, if the Watchdog is not reset for 2s
     // can happen, if any loop in the Program doen't finish.
     lcd_line1();
     lcd_fix_string(TestTimedOut);	//Output Timeout
     wait_about3s();				//wait for 3 s
     ON_PORT = 0;			//shut off!
//     ON_DDR = (1<<ON_PIN);		//switch to GND
     return 0;
  }
  LCDLoadCustomChar(LCD_CHAR_DIODE1);	//Custom-Character Diode symbol
  lcd_fix_customchar(DiodeIcon1);	//load Character  >|
  LCDLoadCustomChar(LCD_CHAR_DIODE2);	//Custom-Character 
  lcd_fix_customchar(DiodeIcon2);	//load Character  |<
  LCDLoadCustomChar(LCD_CHAR_CAP);	//Custom-Character  Capacitor symbol
  lcd_fix_customchar(CapIcon);		//load Character  ||
  LCDLoadCustomChar(LCD_CHAR_RESIS1);	//Custom-Character Resistor symbol
  lcd_fix_customchar(ResIcon1);		//load Character  [
  LCDLoadCustomChar(LCD_CHAR_RESIS2);	//Custom-Character 
  lcd_fix_customchar(ResIcon2);		//load Character  ]
  
#ifdef LCD_CYRILLIC
  //if kyrillish LCD-Characterset, load  Omega- and �-Character
  LCDLoadCustomChar(LCD_CHAR_OMEGA);	//Custom-Character
  //load Omega-Character to LCD
  lcd_fix_customchar(CyrillicOmegaIcon);
  LCDLoadCustomChar(LCD_CHAR_U);	//Custom-Character
  //load �-Character to LCD 
  lcd_fix_customchar(CyrillicMuIcon);
#endif
#ifdef PULLUP_DISABLE
 #ifdef __AVR_ATmega8__
  SFIOR = (1<<PUD);		// disable Pull-Up Resistors mega8
 #else
  MCUCR = (1<<PUD);		// disable Pull-Up Resistors mega168 family
 #endif
#endif

//  DIDR0 = 0x3f;			//disable all Input register of ADC

#if POWER_OFF+0 > 1
  // tester display time selection
  display_time = OFF_WAIT_TIME;		// LONG_WAIT_TIME for single mode, else SHORT_WAIT_TIME
  if (!(ON_PIN_REG & (1<<RST_PIN))) {
     // if power button is pressed ...
     wait_about300ms();			// wait to catch a long key press
     if (!(ON_PIN_REG & (1<<RST_PIN))) {
        // check if power button is still pressed
        display_time = LONG_WAIT_TIME;	// ... set long time display anyway
     }
  }
#else
  #define display_time OFF_WAIT_TIME
#endif

  empty_count = 0;
  mess_count = 0;


//*****************************************************************
//Entry: if start key is pressed before shut down
start:
  PartFound = PART_NONE;	// no part found
  NumOfDiodes = 0;		// Number of diodes = 0
  PartReady = 0;
  PartMode = 0;
  WithReference = 0;		// no precision reference voltage
  lcd_clear();
  ADC_DDR = TXD_MSK;		//activate Software-UART 
  ResistorsFound = 0;		// no resistors found
  cap.ca = 0;
  cap.cb = 0;
#ifdef WITH_UART
  uart_newline();		// start of new measurement
#endif
  ADCconfig.RefFlag = 0;
  Calibrate_UR();		// get Ref Voltages and Pin resistance
  lcd_line1();	//1. row 
  
  ADCconfig.U_Bandgap = ADC_internal_reference;	// set internal reference voltage for ADC

#ifdef BAT_CHECK
  // Battery check is selected
  ReadADC(TPBAT);	//Dummy-Readout
  trans.uBE[0] = W5msReadADC(TPBAT); 	//with 5V reference
  lcd_fix_string(Bat_str);		//output: "Bat. "
 #ifdef BAT_OUT
  // display Battery voltage
  // The divisor to get the voltage in 0.01V units is ((10*33)/133) witch is about 2.4812
  // A good result can be get with multiply by 4 and divide by 10 (about 0.75%).
//  cap.cval = (trans.uBE[0]*4)/10+((BAT_OUT+5)/10); // usually output only 2 digits
//  DisplayValue(cap.cval,-2,'V',2);		// Display 2 Digits of this 10mV units
  cap.cval = (trans.uBE[0]*4)+BAT_OUT;		// usually output only 2 digits
  DisplayValue(cap.cval,-3,'V',2);		// Display 2 Digits of this 10mV units
  lcd_space();
 #endif
 #if (BAT_POOR > 12000)
   #warning "Battery POOR level is set very high!"
 #endif
 #if (BAT_POOR < 2500)
   #warning "Battery POOR level is set very low!"
 #endif
 #if (BAT_POOR > 5300)
  // use .8 V difference to Warn-Level
  #define WARN_LEVEL (((unsigned long)(BAT_POOR+800)*(unsigned long)33)/133)
 #elif (BAT_POOR > 2900)
  // less than 5.4 V only .4V difference to Warn-Level
  #define WARN_LEVEL (((unsigned long)(BAT_POOR+400)*(unsigned long)33)/133)
 #elif (BAT_POOR > 1300)
  // less than 2.9 V only .2V difference to Warn-Level
  #define WARN_LEVEL (((unsigned long)(BAT_POOR+200)*(unsigned long)33)/133)
 #else
  // less than 1.3 V only .1V difference to Warn-Level
  #define WARN_LEVEL (((unsigned long)(BAT_POOR+100)*(unsigned long)33)/133)
 #endif
 #define POOR_LEVEL (((unsigned long)(BAT_POOR)*(unsigned long)33)/133)
  // check the battery voltage
  if (trans.uBE[0] <  WARN_LEVEL) {
     //Vcc < 7,3V; show Warning 
     if(trans.uBE[0] < POOR_LEVEL) {	
        //Vcc <6,3V; no proper operation is possible
        lcd_fix_string(BatEmpty);	//Battery empty!
        wait_about2s();
        PORTD = 0;			//switch power off
        return 0;
     }
     lcd_fix_string(BatWeak);		//Battery weak
  } else { // Battery-voltage OK
     lcd_fix_string(OK_str); 		// "OK"
  }
#else
  lcd_fix_string(VERSION_str);		// if no Battery check, Version .. in row 1
#endif
#ifdef WDT_enabled
  wdt_enable(WDTO_2S);		//Watchdog on
#endif

//  wait_about1s();			// add more time for reading batterie voltage
  // begin tests
#ifdef AUTO_RH
  RefVoltage();			//compute RHmultip = f(reference voltage)
#endif
#if FLASHEND > 0x1fff
  if (WithReference) {
     /* 2.5V precision reference is checked OK */
     if ((mess_count == 0) && (empty_count == 0)) {
         /* display VCC= only first time */
         lcd_line2();
         lcd_fix_string(VCC_str);		// VCC=
         DisplayValue(ADCconfig.U_AVCC,-3,'V',3);	// Display 3 Digits of this mV units
//         lcd_space();
//         DisplayValue(RRpinMI,-1,LCD_CHAR_OMEGA,4);
         wait_about1s();
     }
  }
#endif
#ifdef WITH_VEXT
  // show the external voltage
  while (!(ON_PIN_REG & (1<<RST_PIN))) {
     lcd_line2();
     lcd_clear_line();
     lcd_line2();
     lcd_fix_string(Vext_str);		// Vext=
     trans.uBE[1] = W5msReadADC(TPext);	// read external voltage 
     DisplayValue(trans.uBE[1]*10,-3,'V',3);	// Display 3 Digits of this mV units
     wait_about300ms();
  }
#endif

  lcd_line2();			//LCD position row2, column 1
  lcd_fix_string(TestRunning);		//String: testing...
#ifndef DebugOut
  lcd_line2();			//LCD position row 2, column 1
#endif
  EntladePins();		// discharge all capacitors!
  if(PartFound == PART_CELL) {
    lcd_clear();
    lcd_fix_string(Cell_str);	// display "Cell!"
    goto end2;
  }

#ifdef CHECK_CALL
  AutoCheck();			//check, if selftest should be done
#endif
     
  // check all 6 combinations for the 3 pins 
//         High  Low  Tri
  CheckPins(TP1, TP2, TP3);
  CheckPins(TP2, TP1, TP3);

  CheckPins(TP1, TP3, TP2);
  CheckPins(TP3, TP1, TP2);

  CheckPins(TP2, TP3, TP1);
  CheckPins(TP3, TP2, TP1);
  
  //separate check if is is a capacitor
  if(((PartFound == PART_NONE) || (PartFound == PART_RESISTOR) || (PartFound == PART_DIODE)) ) {
     EntladePins();		// discharge capacities
     //measurement of capacities in all 3 combinations
     cap.cval_max = 0;		// set max to zero
     cap.cpre_max = -12;	// set max to pF unit
     ReadCapacity(TP3, TP1);
     ReadCapacity(TP3, TP2);
     ReadCapacity(TP2, TP1);

#if FLASHEND > 0x1fff
     ReadInductance();			// measure inductance
#endif
  }
  //All checks are done, output result to display
  lcd_clear();
  if(PartFound == PART_DIODE) {
     if(NumOfDiodes == 1) {		//single Diode
        lcd_fix_string(Diode);		//"Diode: "
#if FLASHEND > 0x1fff
        // enough memory to sort the pins
 #if EBC_STYLE == 321
        // the higher test pin number is left side
        if (diodes[0].Anode > diodes[0].Cathode) {
           lcd_testpin(diodes[0].Anode);
           lcd_fix_string(AnKat);	//"->|-"
           lcd_testpin(diodes[0].Cathode);
        } else {
           lcd_testpin(diodes[0].Cathode);
           lcd_fix_string(KatAn);	//"-|<-"
           lcd_testpin(diodes[0].Anode);
        }
 #else
        // the higher test pin number is right side
        if (diodes[0].Anode < diodes[0].Cathode) {
           lcd_testpin(diodes[0].Anode);
           lcd_fix_string(AnKat);	//"->|-"
           lcd_testpin(diodes[0].Cathode);
        } else {
           lcd_testpin(diodes[0].Cathode);
           lcd_fix_string(KatAn);	//"-|<-"
           lcd_testpin(diodes[0].Anode);
        }
 #endif
#else
        // too less memory to sort the pins
        lcd_testpin(diodes[0].Anode);
        lcd_fix_string(AnKat);		//"->|-"
        lcd_testpin(diodes[0].Cathode);
#endif
        UfAusgabe(0x70);
        lcd_fix_string(GateCap_str);	//"C="
        ReadCapacity(diodes[0].Cathode,diodes[0].Anode);	// Capacity opposite flow direction
        DisplayValue(cap.cval,cap.cpre,'F',3);
        goto end;
     } else if(NumOfDiodes == 2) { // double diode
        lcd_data('2');
        lcd_fix_string(Dioden);		//"diodes "
        if(diodes[0].Anode == diodes[1].Anode) { //Common Anode
           lcd_testpin(diodes[0].Cathode);
           lcd_fix_string(KatAn);	//"-|<-"
           lcd_testpin(diodes[0].Anode);
           lcd_fix_string(AnKat);	//"->|-"
           lcd_testpin(diodes[1].Cathode);
           UfAusgabe(0x01);
           goto end;
        } else if(diodes[0].Cathode == diodes[1].Cathode) { //Common Cathode
           lcd_testpin(diodes[0].Anode);
           lcd_fix_string(AnKat);	//"->|-"
	   lcd_testpin(diodes[0].Cathode);
           lcd_fix_string(KatAn);	//"-|<-"
           lcd_testpin(diodes[1].Anode);
           UfAusgabe(0x01);
           goto end;
          }
        else if ((diodes[0].Cathode == diodes[1].Anode) && (diodes[1].Cathode == diodes[0].Anode)) {
           //Antiparallel
           lcd_testpin(diodes[0].Anode);
           lcd_fix_string(AnKat);	//"->|-"
           lcd_testpin(diodes[0].Cathode);
           lcd_fix_string(AnKat);	//"->|-"
           lcd_testpin(diodes[1].Cathode);
           UfAusgabe(0x01);
           goto end;
        }
     } else if(NumOfDiodes == 3) {
        //Serial of 2 Diodes; was detected as 3 Diodes 
        trans.b = 3;
        trans.c = 3;
        /* Check for any constallation of 2 serial diodes:
          Only once the pin No of anyone Cathode is identical of another anode.
          two diodes in series is additionally detected as third big diode.
        */
        if(diodes[0].Cathode == diodes[1].Anode)
          {
           trans.b = 0;
           trans.c = 1;
          }
        if(diodes[0].Anode == diodes[1].Cathode)
          {
           trans.b = 1;
           trans.c = 0;
          }
        if(diodes[0].Cathode == diodes[2].Anode)
          {
           trans.b = 0;
           trans.c = 2;
          }
        if(diodes[0].Anode == diodes[2].Cathode)
          {
           trans.b = 2;
           trans.c = 0;
          }
        if(diodes[1].Cathode == diodes[2].Anode)
          {
           trans.b = 1;
           trans.c = 2;
          }
        if(diodes[1].Anode == diodes[2].Cathode)
          {
           trans.b = 2;
           trans.c = 1;
          }
#if DebugOut == 4
	lcd_line3();
        lcd_testpin(diodes[0].Anode);
        lcd_data(':');
        lcd_testpin(diodes[0].Cathode);
        lcd_space();
        lcd_string(utoa(diodes[0].Voltage, outval, 10));
        lcd_space();
        lcd_testpin(diodes[1].Anode);
        lcd_data(':');
        lcd_testpin(diodes[1].Cathode);
        lcd_space();
        lcd_string(utoa(diodes[1].Voltage, outval, 10));
	lcd_line4();
        lcd_testpin(diodes[2].Anode);
        lcd_data(':');
        lcd_testpin(diodes[2].Cathode);
        lcd_space();
        lcd_string(utoa(diodes[2].Voltage, outval, 10));
        lcd_line1();
#endif
        if((trans.b<3) && (trans.c<3)) {
           lcd_data('3');
           lcd_fix_string(Dioden);	//"Diodes "
           lcd_testpin(diodes[trans.b].Anode);
           lcd_fix_string(AnKat);	//"->|-"
           lcd_testpin(diodes[trans.b].Cathode);
           lcd_fix_string(AnKat);	//"->|-"
           lcd_testpin(diodes[trans.c].Cathode);
           UfAusgabe( (trans.b<<4)|trans.c);
           goto end;
        }
     }
     // end (PartFound == PART_DIODE)
  } else if (PartFound == PART_TRANSISTOR) {
    if(PartReady != 0) {
       if((trans.hfe[0]>trans.hfe[1])) {
          //if the amplification factor was higher at first testr: swap C and E !
          tmp = trans.c;
          trans.c = trans.e;
          trans.e = tmp;
       } else {
          trans.hfe[0] = trans.hfe[1];
          trans.uBE[0] = trans.uBE[1];
       }
    }

    if(PartMode == PART_MODE_NPN) {
       lcd_fix_string(NPN_str);		//"NPN "
    } else {
       lcd_fix_string(PNP_str);		//"PNP "
    }
    if( NumOfDiodes > 2) {	//Transistor with protection diode
#ifdef EBC_STYLE
 #if EBC_STYLE == 321
       // Layout with 321= style
       if (((PartMode == PART_MODE_NPN) && (trans.c < trans.e)) || ((PartMode != PART_MODE_NPN) && (trans.c > trans.e)))
 #else
       // Layout with EBC= style
       if(PartMode == PART_MODE_NPN)
 #endif
#else
       // Layout with 123= style
       if (((PartMode == PART_MODE_NPN) && (trans.c > trans.e)) || ((PartMode != PART_MODE_NPN) && (trans.c < trans.e)))
#endif
       {
          lcd_fix_string(AnKat);	//"->|-"
       } else {
          lcd_fix_string(KatAn);	//"-|<-"
       }
    }
    PinLayout('E','B','C'); 		//  EBC= or 123=...
    lcd_line2(); //2. row 
    lcd_fix_string(hfe_str);		//"B="  (hFE)
    DisplayValue(trans.hfe[0],0,0,3);
    lcd_space();

    lcd_fix_string(Uf_str);		//"Uf="
    DisplayValue(trans.uBE[0],-3,'V',3);
    goto end;
    // end (PartFound == PART_TRANSISTOR)
  } else if (PartFound == PART_FET) {	//JFET or MOSFET
    if(PartMode&1) {
       lcd_data('P');			//P-channel
    } else {
       lcd_data('N');			//N-channel
    }
    lcd_data('-');

    tmp = PartMode/2;
    if (tmp == (PART_MODE_N_D_MOS/2)) {
       lcd_data('D');			// N-D
    }
    if (tmp == (PART_MODE_N_E_MOS/2)) {
       lcd_data('E');			// N-E
    }
    if (tmp == (PART_MODE_N_JFET/2)) {
       lcd_fix_string(jfet_str);	//"JFET"
    } else {
       lcd_fix_string(mosfet_str);	//"-MOS "
    }
    PinLayout('S','G','D'); 		//  SGD= or 123=...
    if((NumOfDiodes > 0) && (PartMode < PART_MODE_N_D_MOS)) {
       //MOSFET with protection diode; only with enhancement-FETs
#ifdef EBC_STYLE
 #if EBC_STYLE == 321
       // layout with 321= style
       if (((PartMode&1) && (trans.c > trans.e)) || ((!(PartMode&1)) && (trans.c < trans.e)))
 #else
       // Layout with SGD= style
       if (PartMode&1) /* N or P MOS */
 #endif
#else
       // layout with 123= style
       if (((PartMode&1) && (trans.c < trans.e)) || ((!(PartMode&1)) && (trans.c > trans.e)))
#endif
       {
          lcd_data(LCD_CHAR_DIODE1);	//show Diode symbol >|
       } else {
          lcd_data(LCD_CHAR_DIODE2);	//show Diode symbol |<
       }
    }
    lcd_line2();			//2. Row
    if(PartMode < PART_MODE_N_D_MOS) {	//enhancement-MOSFET
	//Gate capacity
       lcd_fix_string(GateCap_str);		//"C="
       ReadCapacity(trans.b,trans.e);	//measure capacity
       DisplayValue(cap.cval,cap.cpre,'F',3);
       lcd_fix_string(vt_str);		// "Vt="
    } else {
       lcd_data('I');
       lcd_data('=');
       DisplayValue(trans.uBE[1],-5,'A',2);
       lcd_fix_string(Vgs_str);		// " Vgs="
    }
    //Gate-threshold voltage
    DisplayValue(gthvoltage,-3,'V',3);
    goto end;
    // end (PartFound == PART_FET)
  } else if (PartFound == PART_THYRISTOR) {
    lcd_fix_string(Thyristor);		//"Thyristor"
    goto gakAusgabe;
  } else if (PartFound == PART_TRIAC) {
    lcd_fix_string(Triac);		//"Triac"
    goto gakAusgabe;
  }
  else if(PartFound == PART_RESISTOR) {
    if (ResistorsFound == 1) { // single resistor
       lcd_testpin(resis[0].rb);  	//Pin-number 1
       lcd_fix_string(Resistor_str);
       lcd_testpin(resis[0].ra);		//Pin-number 2
    } else { // R-Max suchen
       ii = 0;
       if (resis[1].rx > resis[0].rx)
          ii = 1;
       if (ResistorsFound == 2) {
          ii = 2;
       } else {
          if (resis[2].rx > resis[ii].rx) {
             ii = 2;
          }
       }
       char x = '1';
       char y = '3';
       char z = '2';
   
       if (ii == 1) {
          // x = '1';
          y = '2';
          z = '3';
       }
       if (ii == 2) {
          x = '2';
          y = '1';
          z = '3';
       }
       lcd_data(x);
       lcd_fix_string(Resistor_str);    // -[=]-
       lcd_data(y);
       lcd_fix_string(Resistor_str);    // -[=]-
       lcd_data(z);
    }
    lcd_line2(); //2. row 
    if (ResistorsFound == 1) {
       RvalOut(0);
#if FLASHEND > 0x1fff
       if (resis[0].lx != 0) {
	  // resistor have also Inductance
          lcd_fix_string(Lis_str);	// "L="
          DisplayValue(resis[0].lx,resis[0].lpre,'H',3);	// output inductance
       }
#endif
    } else {
       // output resistor values in right order
       if (ii == 0) {
          RvalOut(1);
          RvalOut(2);
       }
       if (ii == 1) {
          RvalOut(0);
          RvalOut(2);
       }
       if (ii == 2) {
          RvalOut(0);
          RvalOut(1);
       }
    }
    goto end;

  } // end (PartFound == PART_RESISTOR)

//capacity measurement is wanted
  else if(PartFound == PART_CAPACITOR) {
//     lcd_fix_string(Capacitor);
     lcd_testpin(cap.ca);		//Pin number 1
     lcd_fix_string(CapZeich);		// capacitor sign
     lcd_testpin(cap.cb);		//Pin number 2
#if FLASHEND > 0x1fff
//     wait_about1s();
//     GetEPR();				// get EPR of capacitor
//     if (cap.epr != 0) {
//        lcd_fix_string(EPR_str);	// "  EPR="
//        DisplayValue(cap.epr,-1,LCD_CHAR_OMEGA,2);
//     }
#endif
     lcd_line2(); 			//2. row 
     DisplayValue(cap.cval_max,cap.cpre_max,'F',4);
#if FLASHEND > 0x1fff
     GetESR();				// get ESR of capacitor
#endif
     goto end;
  }
  if(NumOfDiodes == 0) { //no diodes are found
     lcd_fix_string(TestFailed1); 	//"Kein,unbek. oder"
     lcd_line2(); //2. row 
     lcd_fix_string(TestFailed2); 	//"defektes "
     lcd_fix_string(Bauteil);		//"Bauteil"
  } else {
     lcd_fix_string(Bauteil);		//"Bauteil"
     lcd_fix_string(Unknown); 		//" unbek."
     lcd_line2(); //2. row 
     lcd_fix_string(OrBroken); 		//"oder defekt "
     lcd_data(NumOfDiodes + '0');
     lcd_fix_string(AnKat);		//"->|-"
  }
  empty_count++;
  mess_count = 0;
  goto end2;


gakAusgabe:
  lcd_line2(); //2. row 
  PinLayout(Cathode_char,'G','A'); 	// CGA= or 123=...
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 end:
  empty_count = 0;		// reset counter, if part is found
  mess_count++;			// count measurements

 end2:
  ADC_DDR = (1<<TPREF) | TXD_MSK; 	// switch pin with reference to GND, release relay
  while(!(ON_PIN_REG & (1<<RST_PIN)));	//wait ,until button is released
  wait_about200ms();
// wait 14 seconds or 5 seconds (if repeat function)
  for(gthvoltage = 0;gthvoltage<display_time;gthvoltage+=10) {
     if(!(ON_PIN_REG & (1<<RST_PIN))) {
        // If the key is pressed again... 
        // goto start of measurement 
        goto start;
     }
     wdt_reset();
     wait_about10ms();
  }
#ifdef POWER_OFF
 #if POWER_OFF > 127
  #define POWER2_OFF 255
 #else
  #define POWER2_OFF POWER_OFF*2
 #endif
 #if POWER_OFF+0 > 1
  if ((empty_count < POWER_OFF) && (mess_count < POWER2_OFF)) {
     goto start;			// repeat measurement POWER_OFF times
  }
 #endif
  // only one Measurement requested, shut off
//  MCUSR = 0;
  ON_PORT &= ~(1<<ON_PIN);		//switch off power
  //never ending loop 
  while(1) {
     if(!(ON_PIN_REG & (1<<RST_PIN))) {
        // The statement is only reached if no auto off equipment is installed
        goto start;
     }
     wdt_reset();
     wait_about10ms();
  }
#else
  goto start;	// POWER_OFF not selected, repeat measurement
#endif
  return 0;
}   // end main


//******************************************************************
// output of flux voltage for 1-2 diodes in row 2
// bcdnum = Numbers of both Diodes:
// higher 4 Bit  number of first Diode
// lower 4 Bit  number of second Diode (Structure diodes[nn])
// if number >= 3  no output is done
void UfAusgabe(uint8_t bcdnum) {

   lcd_line2(); 				//2. row
   lcd_fix_string(Uf_str);			//"Uf="
   mVAusgabe(bcdnum >> 4);
   mVAusgabe(bcdnum & 0x0f);
}
void mVAusgabe(uint8_t nn) {
   if (nn < 3) {
      // Output in mV units
      DisplayValue(diodes[nn].Voltage,-3,'V',3);
      lcd_space();
   }
}

void RvalOut(uint8_t ii) {	
   // output of resistor value

   DisplayValue(resis[ii].rx,-1,LCD_CHAR_OMEGA,4);
   lcd_space();
 }

//******************************************************************
#include "CheckPins.c"

void ChargePin10ms(uint8_t PinToCharge, uint8_t ChargeDirection) {
   //Load the specified pin to the specified direction with 680 Ohm for 10ms.
   //Will be used by discharge of MOSFET Gates or to load big capacities.
   //Parameters:
   //PinToCharge: specifies the pin as mask for R-Port
   //ChargeDirection: 0 = switch to GND (N-Kanal-FET), 1= switch to VCC(P-Kanal-FET)

   if(ChargeDirection&1) {
      R_PORT |= PinToCharge;	//R_PORT to 1 (VCC) 
   } else {
      R_PORT &= ~PinToCharge; // or 0 (GND)
   }
   R_DDR |= PinToCharge;		//switch Pin to output, across R to GND or VCC
   wait_about10ms();			// wait about 10ms
   // switch back Input, no current
   R_DDR &= ~PinToCharge;	// switch back to input
   R_PORT &= ~PinToCharge;	// no Pull up
}

// first discharge any charge of capacitors
void EntladePins() {
  uint8_t adc_gnd;		// Mask of ADC-outputs, which can be directly connected to GND
  unsigned int adcmv[3];	// voltages of 3 Pins in mV
  unsigned int clr_cnt;		// Clear Counter
  uint8_t lop_cnt;		// loop counter
// max. time of discharge in ms  (10000/20) == 10s
#define MAX_ENTLADE_ZEIT  (10000/20)

  for(lop_cnt=0;lop_cnt<10;lop_cnt++) {
     adc_gnd = TXD_MSK;		// put all ADC to Input
     ADC_DDR = adc_gnd;
     ADC_PORT = TXD_VAL;		// ADC-outputs auf 0
     R_PORT = 0;			// R-outputs auf 0
     R_DDR = (2<<(PC2*2)) | (2<<(PC1*2)) | (2<<(PC0*2)); // R_H for all Pins to GND
     adcmv[0] = W5msReadADC(PC0);	// which voltage has Pin 1?
     adcmv[1] = ReadADC(PC1);	// which voltage has Pin 2?
     adcmv[2] = ReadADC(PC2);	// which voltage has Pin 3?
     if ((PartFound == PART_CELL) || (adcmv[0] < CAP_EMPTY_LEVEL) & (adcmv[1] < CAP_EMPTY_LEVEL) & (adcmv[2] < CAP_EMPTY_LEVEL)) {
        ADC_DDR = TXD_MSK;		// switch all ADC-Pins to input
        R_DDR = 0;			// switch all R_L Ports (and R_H) to input
        return;			// all is discharged
     }
     // all Pins with voltage lower than 1V can be connected directly to GND (ADC-Port)
     if (adcmv[0] < 1000) {
        adc_gnd |= (1<<PC0);	//Pin 1 directly to GND
     }
     if (adcmv[1] < 1000) {
        adc_gnd |= (1<<PC1);	//Pin 2 directly to GND
     }
     if (adcmv[2] < 1000) {
        adc_gnd |= (1<<PC2);	//Pin 3 directly to  GND
     }
     ADC_DDR = adc_gnd;		// switch all selected ADC-Ports at the same time

     // additionally switch the leaving Ports with R_L to GND.
     // since there is no disadvantage for the already directly switched pins, we can
     // simply switch all  R_L resistors to GND
     R_DDR = (1<<(PC2*2)) | (1<<(PC1*2)) | (1<<(PC0*2));	// Pins across R_L resistors to GND
     for(clr_cnt=0;clr_cnt<MAX_ENTLADE_ZEIT;clr_cnt++) {
        wdt_reset();
        adcmv[0] = W20msReadADC(PC0);	// which voltage has Pin 1?
        adcmv[1] = ReadADC(PC1);	// which voltage has Pin 2?
        adcmv[2] = ReadADC(PC2);	// which voltage has Pin 3?
        if (adcmv[0] < 1300) {
           ADC_DDR |= (1<<PC0);	// below 1.3V , switch directly with ADC-Port to GND
        }
        if (adcmv[1] < 1300) {
           ADC_DDR |= (1<<PC1);	// below 1.3V, switch directly with ADC-Port to GND
        }
        if (adcmv[2] < 1300) {
           ADC_DDR |= (1<<PC2);	// below 1.3V, switch directly with ADC-Port to GND
        }
        if ((adcmv[0] < (CAP_EMPTY_LEVEL+2)) && (adcmv[1] < (CAP_EMPTY_LEVEL+2)) && (adcmv[2] < (CAP_EMPTY_LEVEL+2))) {
           break;
        }
     }
     if (clr_cnt == MAX_ENTLADE_ZEIT) {
        PartFound = PART_CELL;	// mark as Battery
        // there is charge on capacitor, warn later!
     }
     for(adcmv[0]=0;adcmv[0]<clr_cnt;adcmv[0]++) {
        // for safety, discharge 5% of discharge  time
        wait1ms();
     }
  } // end for lop_cnt
 }



#ifdef AUTO_RH
void RefVoltage(void) {
// RefVoltage interpolates table RHtab corresponding to voltage ref_mv .
// RHtab contain the factors to get capacity from load time with 470k for
// different Band gab reference voltages.
// for remember:
//resistor     470000 Ohm      1000 1050 1100 1150 1200 1250 1300 1350 1400  mV
//uint16_t RHTAB[] MEM_TEXT = { 954, 903, 856, 814, 775, 740, 707, 676, 648};

#define Ref_Tab_Abstand 50		// displacement of table is 50mV
#define Ref_Tab_Beginn 1000		// begin of table is 1000mV

  unsigned int referenz;
  unsigned int y1, y2;
  uint8_t tabind;
  uint8_t tabres;

  #ifdef AUTO_CAL
  referenz = ref_mv + (int16_t)eeprom_read_word((uint16_t *)(&ref_offset));
  #else
  referenz = ref_mv + REF_C_KORR;
  #endif
  if (referenz >= Ref_Tab_Beginn) {
     referenz -= Ref_Tab_Beginn;
  } else  {
     referenz = 0;		// limit to begin of table
  }
  tabind = referenz / Ref_Tab_Abstand;
  tabres = referenz % Ref_Tab_Abstand;
  tabres = Ref_Tab_Abstand-tabres;
  if (tabind > 7) {
     tabind = 7;		// limit to end of table
  }
  // interpolate the table of factors
  y1 = MEM_read_word(&RHtab[tabind]);
  y2 = MEM_read_word(&RHtab[tabind+1]);
  // RHmultip is the interpolated factor to compute capacity from load time with 470k
  RHmultip = ((y1 - y2) * tabres + (Ref_Tab_Abstand/2)) / Ref_Tab_Abstand + y2;
 }
#endif

#ifdef LCD_CLEAR
void lcd_clear_line(void) {
 // writes 20 spaces to LCD-Display, Cursor must be positioned to first column
 unsigned char ll;
 for (ll=0;ll<20;ll++) {
    lcd_space();
 }
}
#endif

/* ************************************************************************
 *   display of values and units
 * ************************************************************************ */


/*
 *  display value and unit
 *  - max. 4 digits excluding "." and unit
 *
 *  requires:
 *  - value
 *  - exponent of factor related to base unit (value * 10^x)
 *    e.g: p = 10^-12 -> -12
 *  - unit character (0 = none)
 *  digits = 2, 3 or 4
 */
void DisplayValue(unsigned long Value, int8_t Exponent, unsigned char Unit, unsigned char digits)
{
  char OutBuffer[15];
  unsigned int      Limit;
  unsigned char     Prefix;		/* prefix character */
  uint8_t           Offset;		/* exponent of offset to next 10^3 step */
  uint8_t           Index;		/* index ID */
  uint8_t           Length;		/* string length */


  Limit = 100;				/* scale value down to 2 digits */
  if (digits == 3) Limit = 1000;
  if (digits == 4) Limit = 10000;
  while (Value >= Limit)
  {
    Value += 5;				/* for automagic rounding */
    Value = Value / 10;			/* scale down by 10^1 */
    Exponent++;				/* increase exponent by 1 */
  }


  /*
   *  determine prefix
   */
  Length = Exponent + 12;
  if (Length <  0) Length = 0;		/* Limit to minimum prefix */
  if (Length > 18) Length = 18;		/* Limit to maximum prefix */
  Index = Length / 3;
  Offset = Length % 3;
    if (Offset > 0)
    {
      Index++;				/* adjust index for exponent offset, take next prefix */
      Offset = 3 - Offset;		/* reverse value (1 or 2) */
    }
  Prefix = MEM_read_byte((uint8_t *)(&PrefixTab[Index]));   /* look up prefix in table */
  /*
   *  display value
   */

  /* convert value into string */
  utoa((unsigned int)Value, OutBuffer, 10);
  Length = strlen(OutBuffer);

  /* position of dot */
  Exponent = Length - Offset;		/* calculate position */

  if (Exponent <= 0)			/* we have to prepend "0." */
  {
    /* 0: factor 10 / -1: factor 100 */
//    lcd_data('0');
    lcd_data('.');
    if (Exponent < 0) lcd_data('0');	/* extra 0 for factor 100 */
  }

  if (Offset == 0) Exponent = -1;	/* disable dot if not needed */

  /* adjust position to array or disable dot if set to 0 */
//  Exponent--;

  /* display value and add dot if requested */
  Index = 0;
  while (Index < Length)		/* loop through string */
  {
    lcd_data(OutBuffer[Index]);		/* display char */
    Index++;				/* next one */
    if (Index == Exponent) {
      lcd_data('.');			/* display dot */
    }
  }

  /* display prefix and unit */
  if (Prefix != 0) lcd_data(Prefix);
  if (Unit) lcd_data(Unit);
}

#ifndef INHIBIT_SLEEP_MODE
/* set the processor to sleep state */
/* wake up will be done with compare match interrupt of counter 2 */
void sleep_5ms(uint16_t pause){
// pause is the delay in 5ms units
uint8_t t2_offset;
#define RESTART_DELAY_US (RESTART_DELAY_TICS/(F_CPU/1000000UL))
// for 8 MHz crystal the Restart delay is 16384/8 = 2048us

while (pause > 0)
  {
 #if 3000 > RESTART_DELAY_US
   if (pause > 1)
     {
      // Startup time is too long with 1MHz Clock!!!!
      t2_offset =  (10000 - RESTART_DELAY_US) / T2_PERIOD;	/* set to 10ms above the actual counter */
      pause -= 2;
     } else {
      t2_offset =  (5000 - RESTART_DELAY_US) / T2_PERIOD;	/* set to 5ms above the actual counter */
      pause = 0;
     }
   
   OCR2A = TCNT2 + t2_offset;	/* set the compare value */
   TIMSK2 = (0<<OCIE2B) | (1<<OCIE2A) | (0<<TOIE2); /* enable output compare match A interrupt */ 
   set_sleep_mode(SLEEP_MODE_PWR_SAVE);
//   set_sleep_mode(SLEEP_MODE_IDLE);
   sleep_mode();
// wake up after output compare match interrupt
 #else
   // restart delay ist too long, use normal delay of 5ms
   wait5ms();
 #endif
   wdt_reset();
  }
TIMSK2 = (0<<OCIE2B) | (0<<OCIE2A) | (0<<TOIE2); /* disable output compare match A interrupt */ 
}
#endif

// show the Pin Layout of the device 
void PinLayout(char pin1, char pin2, char pin3) {
// pin1-3 is EBC or SGD or CGA
#ifndef EBC_STYLE
   // Layout with 123= style
   lcd_fix_string(N123_str);		//" 123="
   for (ii=0;ii<3;ii++) {
       if (ii == trans.e)  lcd_data(pin1);	// Output Character in right order
       if (ii == trans.b)  lcd_data(pin2);
       if (ii == trans.c)  lcd_data(pin3);
   }
#else
 #if EBC_STYLE == 321
   // Layout with 321= style
   lcd_fix_string(N321_str);		//" 321="
   ii = 3;
   while (ii != 0) {
       ii--;
       if (ii == trans.e)  lcd_data(pin1);	// Output Character in right order
       if (ii == trans.b)  lcd_data(pin2);
       if (ii == trans.c)  lcd_data(pin3);
   }
 #else 
   // Layout with EBC= style
   lcd_space();
   lcd_data(pin1);
   lcd_data(pin2);
   lcd_data(pin3);
   lcd_data('=');
   lcd_testpin(trans.e);
   lcd_testpin(trans.b);
   lcd_testpin(trans.c);
 #endif
#endif
}

#ifdef CHECK_CALL
 #include "AutoCheck.c"
#endif
