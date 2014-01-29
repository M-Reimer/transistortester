// new code by K.-H. K�bbeler
// ReadCapacity tries to find the value of a capacitor by measuring the load time.
// first of all the capacitor is discharged.
// Then a series of up to 500 load pulses with 10ms duration each is done across the R_L (680Ohm)
// resistor.
// After each load pulse the voltage of the capacitor is measured without any load current.
// If voltage reaches a value of more than 300mV and is below 1.3V, the capacity can be
// computed from load time and voltage by a interpolating a build in table.
// If the voltage reaches a value of more than 1.3V with only one load pulse,
// another measurement methode is used:
// The build in 16bit counter can save the counter value at external events.
// One of these events can be the output change of a build in comparator.
// The comparator can compare the voltage of any of the ADC input pins with the voltage
// of the internal reference (1.3V or 1.1V).
// After setting up the comparator and counter properly, the load of capacitor is started 
// with connecting the positive pin with the R_H resistor (470kOhm) to VCC and immediately
// the counter is started. By counting the overflow Events of the 16bit counter and watching
// the counter event flag  the total load time of the capacitor until reaching the internal
// reference voltage can be measured.
// If any of the tries to measure the load time is successful,
// the following variables are set:
// cap.cval = value of the capacitor 
// cap.cval_uncorrected = value of the capacitor uncorrected
// cap.esr = serial resistance of capacitor,  0.01 Ohm units
// cap.cpre = units of cap.cval (-12==pF, -9=nF, -6=�F)
// ca   = Pin number (0-2) of the LowPin
// cb   = Pin number (0-2) of the HighPin

#include <avr/io.h>
#include <stdlib.h>
#include "Transistortester.h"


//=================================================================
void ReadCapacity(uint8_t HighPin, uint8_t LowPin) {
  // check if capacitor and measure the capacity value
  unsigned int tmpint;
  unsigned int adcv[4];
#ifdef INHIBIT_SLEEP_MODE
  unsigned int ovcnt16;
#endif
  uint8_t HiPinR_L, HiPinR_H;
  uint8_t LoADC;
  uint8_t ii;

#ifdef AUTO_CAL
  pin_combination = (HighPin * 3) + LowPin - 1;	// coded Pin combination for capacity zero offset
#endif

  LoADC = pgm_read_byte(&PinADCtab[LowPin]) | TXD_MSK;
  HiPinR_L = pgm_read_byte(&PinRLtab[HighPin]);	//R_L mask for HighPin R_L load
#if FLASEND > 0x3fff
  HiPinR_H = pgm_read_byte(&PinRHtab[HighPin]);	//R_H mask for HighPin R_H load
#else
  HiPinR_H = HiPinR_L + HiPinR_L;	//double for HighPin R_H load
#endif

#if DebugOut == 10
  lcd_line3();
  lcd_clear_line();
  lcd_line3();
  lcd_testpin(LowPin);
  lcd_data('C');
  lcd_testpin(HighPin);
  lcd_space();
#endif
  if(PartFound == PART_RESISTOR) {
#if DebugOut == 10
     lcd_data('R');
     wait_about2s();
#endif
     return;	//We have found a resistor already 
  }
  for (ii=0;ii<NumOfDiodes;ii++) {
     if ((diodes.Cathode[ii] == LowPin) && (diodes.Anode[ii] == HighPin) && (diodes.Voltage[ii] < 1500)) {
#if DebugOut == 10
        lcd_data('D');
        wait_about2s();
#endif
        return;
     }
  }
  
#if FLASHEND > 0x1fff
  unsigned int vloss;	// lost voltage after load pulse in 0.1% 
  cap.esr = 0;				// set ESR of capacitor to zero
  vloss = 0;				// set lost voltage to zero
#endif
  cap.cval = 0;				// set capacity value to zero
  cap.cpre = -12;			//default unit is pF
  EntladePins();			// discharge capacitor
  ADC_PORT = TXD_VAL;			// switch ADC-Port to GND
  R_PORT = 0;				// switch R-Port to GND
  ADC_DDR = LoADC;			// switch Low-Pin to output (GND)
//  R_DDR = HiPinR_L;			// switch R_L port for HighPin to output (GND)
  R_DDR = 0;				// set all R Ports to input (no current)
  adcv[0] = ReadADC(HighPin);		// voltage before any load 
// ******** should adcv[0] be measured without current???
  adcv[2] = adcv[0];			// preset to prevent compiler warning
  for (ovcnt16=0;ovcnt16<500;ovcnt16++) {
     R_PORT = HiPinR_L;			//R_L to 1 (VCC) 
     R_DDR = HiPinR_L;			//switch Pin to output, across R to GND or VCC
     wait10ms();			// wait exactly 10ms, do not sleep
     R_DDR = 0;				// switch back to input
     R_PORT = 0;			// no Pull up
     wait500us();			//wait a little time
     wdt_reset();
     // read voltage without current, is already charged enough?
     adcv[2] = ReadADC(HighPin);
     if (adcv[2] > adcv[0]) {
        adcv[2] -= adcv[0];		//difference to beginning voltage
     } else {
        adcv[2] = 0;			// voltage is lower or same as beginning voltage
     }
     if ((ovcnt16 == 126) && (adcv[2] < 75)) {
        // 300mV can not be reached well-timed 
        break;		// don't try to load any more
     }
     if (adcv[2] > 300) {
        break;		// probably 100mF can be charged well-timed 
     }
  }
  // wait 5ms and read voltage again, does the capacitor keep the voltage?
//  adcv[1] = W5msReadADC(HighPin) - adcv[0];
//  wdt_reset();
#if DebugOut == 10
  DisplayValue(ovcnt16,0,' ',4);
  DisplayValue(adcv[2],-3,'V',4);
#endif
  if (adcv[2] < 301) {
#if DebugOut == 10
     lcd_data('K');
     lcd_space();
     wait1s();
#endif
//     if (NumOfDiodes != 0) goto messe_mit_rh; /* ****************************** */
     goto keinC;		// was never charged enough, >100mF or shorted
  }
  //voltage is rised properly and keeps the voltage enough
  if ((ovcnt16 == 0 ) && (adcv[2] > 1300)) {
     goto messe_mit_rh;		// Voltage of more than 1300mV is reached in one pulse, too fast loaded
  }
  // Capacity is more than about 50�F
#ifdef NO_CAP_HOLD_TIME
  ChargePin10ms(HiPinR_H,0);		//switch HighPin with R_H 10ms auf GND, then currentless
  adcv[3] = ReadADC(HighPin) - adcv[0]; // read voltage again, is discharged only a little bit ?
  if (adcv[3] > adcv[0]) {
     adcv[3] -= adcv[0];		// difference to beginning voltage
  } else {
     adcv[3] = 0;			// voltage is lower to beginning voltage
  }
 #if DebugOut == 10
  lcd_data('U');
  lcd_data('3');
  lcd_data(':');
  u2lcd(adcv[3]);
  lcd_space();
  wait_about2s();
 #endif
  if ((adcv[3] + adcv[3]) < adcv[2]) {
 #if DebugOut == 10
     lcd_data('H');
     lcd_space();
     wait_about1s();
 #endif
     if (ovcnt16 == 0 )  {
        goto messe_mit_rh;		// Voltage of more than 1300mV is reached in one pulse, but not hold
     }
     goto keinC; //implausible, not yet the half voltage
  }
  cap.cval_uncorrected.dw = ovcnt16 + 1;
  cap.cval_uncorrected.dw *= GetRLmultip(adcv[2]);		// get factor to convert time to capacity from table
#else
  // wait the half the time which was required for loading
  adcv[3] = adcv[2];			// preset to prevent compiler warning
  for (tmpint=0;tmpint<=ovcnt16;tmpint++) {
     wait5ms();
     adcv[3] = ReadADC(HighPin);	// read voltage again, is discharged only a little bit ?
     wdt_reset();
  }
  if (adcv[3] > adcv[0]) {
     adcv[3] -= adcv[0];		// difference to beginning voltage
  } else {
     adcv[3] = 0;			// voltage is lower or same as beginning voltage
  }
  if (adcv[2] > adcv[3]) {
     // build difference to load voltage
     adcv[3] = adcv[2] - adcv[3];	// lost voltage during load time wait
  } else {
     adcv[3] = 0;			// no lost voltage
  }
#if FLASHEND > 0x1fff
  // compute equivalent parallel resistance from voltage drop
  if (adcv[3] > 0) {
     // there is any voltage drop (adcv[3]) !
     // adcv[2] is the loaded voltage.
     vloss = (unsigned long)(adcv[3] * 1000UL) / adcv[2];
  }
#endif
  if (adcv[3] > 100) {
     // more than 100mV is lost during load time
 #if DebugOut == 10
     lcd_data('L');
     lcd_space();
     wait_about1s();
 #endif
     if (ovcnt16 == 0 )  {
        goto messe_mit_rh;		// Voltage of more than 1300mV is reached in one pulse, but not hold
     }
     goto keinC;			// capacitor does not keep the voltage about 5ms
  }
  cap.cval_uncorrected.dw = ovcnt16 + 1;
  // compute factor with load voltage + lost voltage during the voltage load time
  cap.cval_uncorrected.dw *= GetRLmultip(adcv[2]+adcv[3]);	// get factor to convert time to capacity from table
#endif
   cap.cval = cap.cval_uncorrected.dw;	// set result to uncorrected
   cap.cpre = -9;		// switch units to nF 
   Scale_C_with_vcc();
   // cap.cval for this type is at least 40000nF, so the last digit will be never shown
   cap.cval *= (1000 - C_H_KORR);	// correct with C_H_KORR with 0.1% resolution, but prevent overflow
   cap.cval /= 100;
#if DebugOut == 10
   lcd_line3();
   lcd_clear_line();
   lcd_line3();
   lcd_testpin(LowPin);
   lcd_data('C');
   lcd_testpin(HighPin);
   lcd_space();
   DisplayValue(cap.cval,cap.cpre,'F',4);
   lcd_space();
   u2lcd(ovcnt16);
   wait_about3s();
#endif
   goto checkDiodes;

//==================================================================================
// Measurement of little capacity values
messe_mit_rh:
  //little capacity value, about  < 50 �F
  EntladePins();			// discharge capacitor
  //measure with the R_H (470kOhm) resistor 
  R_PORT = 0;		// R_DDR ist HiPinR_L
  ADC_DDR = (1<<TP1) | (1<<TP2) | (1<<TP3) | TXD_MSK;	//switch all Pins to output
  ADC_PORT = TXD_VAL;		//switch all ADC Pins to GND
  R_DDR = HiPinR_H;   		// switch R_H resistor port for HighPin to output (GND)
// setup Analog Comparator
  ADC_COMP_CONTROL = (1<<ACME);			//enable Analog Comparator Multiplexer
  ACSR =  (1<<ACBG) | (1<<ACI)  | (1<<ACIC);	// enable, 1.3V, no Interrupt, Connect to Timer1 
  ADMUX = (1<<REFS0) | HighPin;			// switch Mux to High-Pin
  ADCSRA = (1<<ADIF) | AUTO_CLOCK_DIV; //disable ADC
  wait200us();			//wait for bandgap to start up

// setup Counter1
  ovcnt16 = 0;
  TCCR1A = 0;			// set Counter1 to normal Mode
  TCNT1 = 0;			//set Counter to 0
  TI1_INT_FLAGS = (1<<ICF1) | (1<<OCF1B) | (1<<OCF1A) | (1<<TOV1);	// clear interrupt flags
#ifndef INHIBIT_SLEEP_MODE
  TIMSK1 = (1<<TOIE1) | (1<<ICIE1);	// enable Timer overflow interrupt and input capture interrupt
  unfinished = 1;
#endif
  R_PORT = HiPinR_H;           	// switch R_H resistor port for HighPin to VCC
  if(PartFound == PART_FET) {
     // charge capacitor with R_H resistor
     TCCR1B = (0<<ICNC1) | (1<<CS10);	//Start counter 1MHz or 8MHz without Noise Canceler
     ADC_DDR = (((1<<TP1) | (1<<TP2) | (1<<TP3) | TXD_MSK) & ~(1<<HighPin));	// release only HighPin ADC port
  } else {
     TCCR1B =  (0<<ICNC1) | (1<<CS10);	//start counter 1MHz or 8MHz without Noise Canceler
     ADC_DDR = LoADC;		// stay LoADC Pin switched to GND, charge capacitor with R_H slowly
  }
//******************************
#ifdef INHIBIT_SLEEP_MODE
  while(1) {
     // Wait, until  Input Capture is set
     ii = TI1_INT_FLAGS;	//read Timer flags
     if (ii & (1<<ICF1))  {
        break;
     }
     if((ii & (1<<TOV1))) {	// counter overflow, 65.536 ms @ 1MHz, 8.192ms @ 8MHz
        TI1_INT_FLAGS = (1<<TOV1);	// Reset OV Flag
        wdt_reset();
        ovcnt16++;
        if(ovcnt16 == (F_CPU/5000)) {
           break; 		//Timeout for Charging, above 12 s
        }
     }
  }
  TCCR1B = (0<<ICNC1) | (0<<ICES1) | (0<<CS10);  // stop counter
  TI1_INT_FLAGS = (1<<ICF1);		// Reset Input Capture
  tmpint = ICR1;		// get previous Input Capture Counter flag
// check actual counter, if an additional overflow must be added
  if((TCNT1 > tmpint) && (ii & (1<<TOV1))) {
     // this OV was not counted, but was before the Input Capture
     TI1_INT_FLAGS = (1<<TOV1);		// Reset OV Flag
     ovcnt16++;
  }
#else
  cli();		// disable interrupts to prevent wakeup Interrupts before sleeping
  set_sleep_mode(SLEEP_MODE_IDLE);
  while(unfinished) {
    sleep_enable();
    sei();		// enable interrupts after next instruction
    sleep_cpu();	// only enable interrupts during sleeping
    sleep_disable();
    cli();		// disable interrupts again
    wdt_reset();	// reset watch dog during waiting
    if(ovcnt16 == (F_CPU/5000)) {
       break; 		//Timeout for Charging, above 12 s
    }
  }
  sei();		// enable interrupts again
  TCCR1B = (0<<ICNC1) | (0<<ICES1) | (0<<CS10);  // stop counter
  tmpint = ICR1;		// get previous Input Capture Counter flag
  TIMSK1 = (0<<TOIE1) | (0<<ICIE1);	// disable Timer overflow interrupt and input capture interrupt
#endif

#if DebugOut == 11
  // test output for checking error free load time measurement
  // select a capacitor, which gives a load time of about 65536 clock tics
  if ((LowPin == 0) && (HighPin == 2)) {
    lcd_line3();
    lcd_clear_line();
    lcd_line3();
    if (ovcnt16 != 0) {
      DisplayValue(ovcnt16,0,',',4);
    }
    u2lcd(tmpint);
  }
#endif

//############################################################
  ADCSRA = (1<<ADEN) | (1<<ADIF) | AUTO_CLOCK_DIV; //enable ADC
  R_DDR = 0;			// switch R_H resistor port for input
  R_PORT = 0;			// switch R_H resistor port pull up for HighPin off
  adcv[2] = ReadADC(HighPin);   // get loaded voltage
  load_diff = adcv[2] + REF_C_KORR - ref_mv;	// build difference of capacitor voltage to Reference Voltage
//############################################################
  if (ovcnt16 >= (F_CPU/10000)) {
#if DebugOut == 10
     lcd_data('k');
     wait_about1s();
#endif
     goto keinC;	// no normal end
  }
//  cap.cval_uncorrected = CombineII2Long(ovcnt16, tmpint);
  cap.cval_uncorrected.w[1] = ovcnt16;
  cap.cval_uncorrected.w[0] = tmpint;

  cap.cpre = -12;			// cap.cval unit is pF 
  if (ovcnt16 > 65) {
     cap.cval_uncorrected.dw /= 100;	// switch to next unit
     cap.cpre += 2;			// set unit, prevent overflow
  }
  cap.cval_uncorrected.dw *= RHmultip;		// 708
  cap.cval_uncorrected.dw /= (F_CPU / 10000);	// divide by 100 (@ 1MHz clock), 800 (@ 8MHz clock)
  cap.cval = cap.cval_uncorrected.dw;		// set the corrected cap.cval
  Scale_C_with_vcc();
  if (cap.cpre == -12) {
#if COMP_SLEW1 > COMP_SLEW2
     if (cap.cval < COMP_SLEW1) {
        // add slew rate dependent offset
        cap.cval += (COMP_SLEW1 / (cap.cval+COMP_SLEW2 ));
     }
#endif
#ifdef AUTO_CAL
     // auto calibration mode, cap_null can be updated in selftest section
     tmpint = eeprom_read_byte(&c_zero_tab[pin_combination]);	// read zero offset
     if (cap.cval > tmpint) {
         cap.cval -= tmpint;		//subtract zero offset (pF)
     } else {
#if FLASHEND > 0x3fff
       if ((cap.cval+C_LIMIT_TO_UNCALIBRATED) < tmpint) {
         mark_as_uncalibrated();	// set in EEprom to uncalibrated
#if DebugOut == 11
         lcd_line3();
         lcd_testpin(LowPin);
         lcd_data('y');
         lcd_testpin(HighPin);
         lcd_space();
#endif
       }
#endif
         cap.cval = 0;			//unsigned long may not reach negativ value
     }
#else
     if (HighPin == TP2) cap.cval += TP2_CAP_OFFSET;	// measurements with TP2 have 2pF less capacity
     if (cap.cval > C_NULL) {
         cap.cval -= C_NULL;		//subtract constant offset (pF)
     } else {
         cap.cval = 0;			//unsigned long may not reach negativ value
     }
#endif
  }

#if DebugOut == 10
  R_DDR = 0;			// switch all resistor ports to input
  lcd_line4();
  lcd_clear_line();
  lcd_line4();
  lcd_testpin(LowPin);
  lcd_data('c');
  lcd_testpin(HighPin);
  lcd_space();
  DisplayValue(cap.cval,cap.cpre,'F',4);
  wait_about3s();
#endif
  R_DDR = HiPinR_L; 		//switch R_L for High-Pin to GND
#if F_CPU < 2000001
   if(cap.cval < 50)
#else 
   if(cap.cval < 25)
#endif 
     {
     // cap.cval can only be so little in pF unit, cap.cpre must not be testet!
#if DebugOut == 10
     lcd_data('<');
     lcd_space();
     wait_about1s();
#endif
      goto keinC;	//capacity to low, < 50pF @1MHz (25pF @8MHz)
   }
   // end low capacity 
checkDiodes:
   if((NumOfDiodes > 0)  && (PartFound != PART_FET)) {
#if DebugOut == 10
      lcd_data('D');
      lcd_space();
      wait_about1s();
#endif
      // nearly shure, that there is one or more diodes in reverse direction,
      // which would be wrongly detected as capacitor 
   } else {
      PartFound = PART_CAPACITOR;	//capacitor is found
      if ((cap.cpre > cap.cpre_max) || ((cap.cpre == cap.cpre_max) && (cap.cval > cap.cval_max))) {
         // we have found a greater one
         cap.cval_max = cap.cval;
         cap.cpre_max = cap.cpre;
#if FLASHEND > 0x1fff
         cap.v_loss = vloss;		// lost voltage in 0.01%
#endif
         cap.ca = LowPin;		// save LowPin
         cap.cb = HighPin;		// save HighPin
      }
   }

keinC:
  // discharge capacitor again
//  EntladePins();		// discharge capacitors
  //ready
  // switch all ports to input
  ADC_DDR =  TXD_MSK;		// switch all ADC ports to input
  ADC_PORT = TXD_VAL;		// switch all ADC outputs to GND, no pull up
  R_DDR = 0;			// switch all resistor ports to input
  R_PORT = 0; 			// switch all resistor outputs to GND, no pull up
  return;
 } // end ReadCapacity()


void Scale_C_with_vcc(void) {
   while (cap.cval > 100000) {
      cap.cval /= 10;
      cap.cpre ++;			// prevent overflow
   }
   cap.cval *= ADCconfig.U_AVCC;	// scale with measured voltage
   cap.cval /= U_VCC;			// Factors are computed for U_VCC
}
#ifndef INHIBIT_SLEEP_MODE
/* Interrupt Service Routine for timer1 Overflow */
 ISR(TIMER1_OVF_vect, ISR_BLOCK)
{
 if ((!(TI1_INT_FLAGS & (1<<ICF1)) && (unfinished !=0)) || ((TI1_INT_FLAGS & (1<<ICF1)) && (ICR1 < 250))) {
    // Capture Event not pending or (Capture Event pending and ICR1 < 250
    // the ICR1H is buffered and can not examined alone, we must read the ICR1L first (with ICR1 access) !
   ovcnt16++;				// count Overflow
 }
}
/* Interrupt Service Routine for timer1 capture event (Comparator) */
 ISR(TIMER1_CAPT_vect, ISR_BLOCK)
{
 unfinished = 0;			// clear unfinished flag
 // With unfinished set to 0, we prevent further counting the Overflow.s
 // Is already a Overflow detected?
 if((TI1_INT_FLAGS & (1<<TOV1))) {	// counter overflow, 65.536 ms @ 1MHz, 8.192ms @ 8MHz
   //there is already a Overflow detected, before or after the capture event?
   // ICR1H is buffered and can not examined alone, we must read the ICR1L first (with ICR1 access) !
   if (ICR1 < 250) {
     //  Yes, Input Capture Counter is low, Overflow has occured before the capture event
     ovcnt16++;				// count Overflow
   }
 }
}
#endif
