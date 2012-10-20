
/*########################################################################################
        Configuration
*/

/* Port , that is directly connected to the probes.
  This Port must have an ADC-Input  (ATmega8:  PORTC).
  The lower pins of this Port must be used for measurements.
  Please don't change the definitions of TP1, TP2 and TP3!
  The TPREF pin can be connected with a 2.5V precision voltage reference
*/

#define ADC_PORT PORTC
#define ADC_DDR DDRC
#define ADC_PIN PINC
#define TP1 PC0
#define TP2 PC1
#define TP3 PC2
// Port pin for 2.5V precision reference used for VCC check (optional)
#define TPREF PC4
// Port pin for Battery voltage measuring
#define TPBAT PC5


/*
  exact values of used resistors (Ohm).
  The standard value for R_L is 680 Ohm, for R_H 470kOhm.
  
  To calibrate your tester the resistor-values can be adjusted:
*/
  #define R_L_VAL 6800          // standard value 680 Ohm, multiplied by 10 for 0.1 Ohm resolution
//  #define R_L_VAL 6690          // this will be define a 669 Ohm
  #define R_H_VAL 47000         // standard value 470000 Ohm, multiplied by 10, divided by 100 
//  #define R_H_VAL 47900               // this will be define a 479000 Ohm, divided by 100 

#define R_DDR DDRB
#define R_PORT PORTB

/* Port for the Test resistors
  The Resistors must be connected to the lower 6 Pins of the Port in following sequence:
  RLx = 680R-resistor for Test-Pin x
  RHx = 470k-resistor for Test-Pin x

  RL1 an Pin 0
  RH1 an Pin 1
  RL2 an Pin 2
  RH2 an Pin 3
  RL3 an Pin 4
  RH3 an Pin 5
*/

#define ON_DDR DDRD
#define ON_PORT PORTD
#define ON_PIN_REG PIND
#define ON_PIN PD6      //Pin, must be switched to high to switch power on
#define RST_PIN PD7     //Pin, is swiched to low, if push button is pressed


// U_VCC defines the VCC Voltage of the ATmega in mV units

#define U_VCC 5000
// integer factors are used to change the ADC-value to mV resolution in ReadADC !

// With the option NO_CAP_HOLD_TIME you specify, that capacitor loaded with 680 Ohm resistor will not
// be tested to hold the voltage same time as load time.
// Otherwise (without this option) the voltage drop during load time is compensated to avoid displaying
// too much capacity for capacitors with internal parallel resistance.
// #define NO_CAP_HOLD_TIME


// U_SCALE can be set to 4 for better resolution of ReadADC function for resistor measurement
#define U_SCALE 4

// R_ANZ_MESS can be set to a higher number of measurements (up to 200) for resistor measurement
#define R_ANZ_MESS 190

//Watchdog
#define WDT_enabled
/* If you remove the "#define WDT_enabled" , the Watchdog will not be activated.
  This is only for Test or debugging usefull.
 For normal operation please activate the Watchdog !
*/

/*########################################################################################
End of configuration 
*/

#if R_ANZ_MESS < ANZ_MESS
 #undef R_ANZ_MESS
 #define R_ANZ_MESS ANZ_MESS
#endif
#if U_SCALE < 0
 // limit U_SCALE
 #undef U_SCALE
 #define U_SCALE 1
#endif
#if U_SCALE > 4
 // limit U_SCALE
 #undef U_SCALE
 #define U_SCALE 4
#endif

// the following definitions specify where to load external data from: EEprom or flash
#ifdef USE_EEPROM
 #define MEM_TEXT EEMEM
 #if E2END > 0X1FF
  #define MEM2_TEXT EEMEM
  #define MEM2_read_byte(a)  eeprom_read_byte(a)
  #define MEM2_read_word(a)  eeprom_read_word(a)
  #define lcd_fix2_string(a)  lcd_fix_string(a)
 #else
  #define MEM2_TEXT PROGMEM
  #define MEM2_read_byte(a)  pgm_read_byte(a)
  #define MEM2_read_word(a)  pgm_read_word(a)
  #define lcd_fix2_string(a)  lcd_pgm_string(a)
 #endif
 #define MEM_read_word(a)  eeprom_read_word(a)
 #define MEM_read_byte(a)  eeprom_read_byte(a)
#else
 #define MEM_TEXT PROGMEM
 #define MEM2_TEXT PROGMEM
 #define MEM_read_word(a)  pgm_read_word(a)
 #define MEM_read_byte(a)  pgm_read_byte(a)
 #define MEM2_read_byte(a)  pgm_read_byte(a)
 #define MEM2_read_word(a)  pgm_read_word(a)
 #define lcd_fix2_string(a)  lcd_pgm_string(a)
#endif

// RH_OFFSET : systematic offset of resistor measurement with RH (470k) 
// resolution is 0.1 Ohm, 7000 defines a offset of 700 Ohm
#define RH_OFFSET 3500 

// TP2_CAP_OFFSET is a additionally offset for TP2 capacity measurements in pF units
#define TP2_CAP_OFFSET 2

// CABLE_CAP defines the capacity (pF) of 12cm cable with clip at the terminal pins
#define CABLE_CAP 3
// select the right Processor Typ
#if defined(__AVR_ATmega48__)
 #define PROCESSOR_TYP 48
#elif defined(__AVR_ATmega48P__)
 #define PROCESSOR_TYP 48
#elif defined(__AVR_ATmega88__)
 #define PROCESSOR_TYP 88
#elif defined(__AVR_ATmega88P__)
 #define PROCESSOR_TYP 88
#elif defined(__AVR_ATmega168__)
 #define PROCESSOR_TYP 168
#elif defined(__AVR_ATmega168P__)
 #define PROCESSOR_TYP 168
#elif defined(__AVR_ATmega328__)
 #define PROCESSOR_TYP 328
#elif defined(__AVR_ATmega328P__)
 #define PROCESSOR_TYP 328
#else
 #define PROCESSOR_TYP 8
#endif
// automatic selection of right call type
#if FLASHEND > 0X1FFF
 #define ACALL call
#else
 #define ACALL rcall
#endif
// automatic selection of option and parameters for different AVR s
//----------------========----------
#if PROCESSOR_TYP == 48
//----------------========----------
  #define MCU_STATUS_REG MCUCR
  #define ADC_COMP_CONTROL ADCSRB
  #define TI1_INT_FLAGS TIFR1
  #define DEFAULT_BAND_GAP 1070
  #define DEFAULT_RH_FAKT  884      // mega328 1070 mV
// WITH_SELFTEST enables the selftest fuction 
  #ifdef WITH_SELFTEST
  #warning "ATmega48 does NOT support SELFTEST!"
  #undef WITH_SELFTEST
  #endif
// R_MESS activates the resistor measurement 
  #ifdef R_MESS
  #warning "ATmega48 does NOT support Resistor measuring!"
  #undef R_MESS
  #endif
// C_MESS activates the capacitor measurement 
  #ifdef C_MESS
  #warning "ATmega48 does NOT support Capacity measuring!"
  #undef C_MESS
  #endif
// WITH_UART activates the output of data with software UART 
  #ifdef WITH_UART
  #warning "no UART support with ATmega48!"
  #undef WITH_UART
  #endif
  #ifdef BAT_CHECK
  #warning "no BAT_CHECK support with ATmega48!"
  #undef BAT_CHECK
  #endif

  #define C_NULL 50 
  #define PIN_RM 190
  #define PIN_RP 220

//------------------========----------
#elif PROCESSOR_TYP == 88
//------------------========----------
  #define MCU_STATUS_REG MCUCR
  #define ADC_COMP_CONTROL ADCSRB
  #define TI1_INT_FLAGS TIFR1
  #define DEFAULT_BAND_GAP 1070
  #define DEFAULT_RH_FAKT  884      // mega328 1070 mV
// LONG_HFE  activates computation of current amplification factor with long variables
  #define LONG_HFE
// COMMON_COLLECTOR activates measurement of current amplification factor also in common collector circuit  (Emitter follower)
  #define COMMON_COLLECTOR

  #define PIN_RM 190
  #define PIN_RP 225
// CC0 defines the capacity of empty terminal pins 1 & 3 without cable
  #define CC0 35
// Slew rate correction  val += COMP_SLEW1 / (val + COMP_SLEW2)
  #define COMP_SLEW1 4000
  #define COMP_SLEW2 220
  #define C_NULL CC0+CABLE_CAP+(COMP_SLEW1 / (CC0 + CABLE_CAP + COMP_SLEW2))

//------------------=========----------
#elif PROCESSOR_TYP == 168
//------------------=========----------
  #define MCU_STATUS_REG MCUCR
  #define ADC_COMP_CONTROL ADCSRB
  #define TI1_INT_FLAGS TIFR1
  #define DEFAULT_BAND_GAP 1070
  #define DEFAULT_RH_FAKT  884      // mega328 1070 mV
// LONG_HFE  activates computation of current amplification factor with long variables
  #define LONG_HFE
// COMMON_COLLECTOR activates measurement of current amplification factor also in common collector circuit  (Emitter follower)
  #define COMMON_COLLECTOR
  #define MEGA168A 17
  #define MEGA168PA 18

// Pin resistor values of ATmega168
//  #define PIN_RM 196
//  #define PIN_RP 225
  #define PIN_RM 190
  #define PIN_RP 220
// CC0 defines the capacity of empty terminal pins 1 & 3 without cable
  #define CC0 36
// Slew rate correction  val += COMP_SLEW1 / (val + COMP_SLEW2)
  #define COMP_SLEW1 4000
  #define COMP_SLEW2 220
  #define C_NULL CC0+CABLE_CAP+(COMP_SLEW1 / (CC0 + CABLE_CAP + COMP_SLEW2))

//------------------=========----------
#elif PROCESSOR_TYP == 328
//------------------=========----------
  #define MCU_STATUS_REG MCUCR
  #define ADC_COMP_CONTROL ADCSRB
  #define TI1_INT_FLAGS TIFR1
  #define DEFAULT_BAND_GAP 1070
  #define DEFAULT_RH_FAKT  884      // mega328 1070 mV
// LONG_HFE  activates computation of current amplification factor with long variables
  #define LONG_HFE
// COMMON_COLLECTOR activates measurement of current amplification factor also in common collector circuit  (Emitter follower)
  #define COMMON_COLLECTOR

  #define PIN_RM 200
  #define PIN_RP 220
// CC0 defines the capacity of empty terminal pins 1 & 3 without cable
  #define CC0 36
// Slew rate correction  val += COMP_SLEW1 / (val + COMP_SLEW2)
  #define COMP_SLEW1 4000
  #define COMP_SLEW2 180
  #define C_NULL CC0+CABLE_CAP+(COMP_SLEW1 / (CC0 + CABLE_CAP + COMP_SLEW2))

//------------------=========----------
#else
//                   ATmega8
//------------------=========----------
  #define MCU_STATUS_REG MCUCSR
  #define ADC_COMP_CONTROL SFIOR
  #define TI1_INT_FLAGS TIFR
  #define DEFAULT_BAND_GAP 1298		//mega8 1298 mV
  #define DEFAULT_RH_FAKT  708      // mega8 1298 mV
// LONG_HFE  activates computation of current amplification factor with long variables
  #define LONG_HFE
// COMMON_COLLECTOR activates measurement of current amplification factor also in common collector circuit  (Emitter follower)
  #define COMMON_COLLECTOR

  #define PIN_RM 196
  #define PIN_RP 240
// CC0 defines the capacity of empty terminal pins 1 & 3 without cable
  #define CC0 27
// Slew rate correction  val += COMP_SLEW1 / (val + COMP_SLEW2)
  #define COMP_SLEW1 0
  #define COMP_SLEW2 33
  #define C_NULL CC0+CABLE_CAP+(COMP_SLEW1 / (CC0 + CABLE_CAP + COMP_SLEW2))
#endif

#ifndef REF_R_KORR
 #define REF_R_KORR 0
#endif
#ifndef REF_C_KORR
 #define REF_C_KORR 0
#endif
#ifndef C_MESS
  // undef WITH_AUTO_REF if no capacity measurement
  #undef WITH_AUTO_REF
#endif

#define LONG_WAIT_TIME 14000
#define SHORT_WAIT_TIME 5000

#ifdef POWER_OFF
// if POWER OFF function is selected, wait 10s
// if POWER_OFF with parameter > 2, wait only 3s before repeating
 #if (POWER_OFF+0) > 2
  #define OFF_WAIT_TIME SHORT_WAIT_TIME
 #else
  #define OFF_WAIT_TIME LONG_WAIT_TIME
 #endif
#else
// if POWER OFF function is not selected, wait 3s before repeat measurement
 #define OFF_WAIT_TIME  SHORT_WAIT_TIME
#endif

//**********************************************************
// defines for the selection of a correctly  ADC-Clock 
// will match for 1MHz, 2MHz, 4MHz, 8MHz and 16MHz
// ADC-Clock can be 125000 or 250000 
// 250 kHz is out of the full accuracy specification!
// clock divider is 4, when CPU_Clock==1MHz and ADC_Clock==250kHz
// clock divider is 128, when CPU_Clock==16MHz and ADC_Clock==125kHz
#define F_ADC 125000
//#define F_ADC 250000
#if F_CPU/F_ADC == 4
 #define AUTO_CLOCK_DIV (1<<ADPS1) 
#endif
#if F_CPU/F_ADC == 8
 #define AUTO_CLOCK_DIV (1<<ADPS1) | (1<<ADPS0)
#endif
#if F_CPU/F_ADC == 16
 #define AUTO_CLOCK_DIV (1<<ADPS2)
#endif
#if F_CPU/F_ADC == 32
 #define AUTO_CLOCK_DIV (1<<ADPS2) | (1<<ADPS0)
#endif
#if F_CPU/F_ADC == 64
 #define AUTO_CLOCK_DIV (1<<ADPS2) | (1<<ADPS1)
#endif
#if F_CPU/F_ADC == 128
 #define AUTO_CLOCK_DIV (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0)
#endif

#ifndef PIN_RP
  #define PIN_RP  220           //estimated internal resistance PORT to VCC
                                //will only be used, if not set before in config.h
#endif
#ifndef PIN_RM
  #define PIN_RM  190           //estimated internal resistance PORT to GND
                                //will only be used, if not set before in config.h
#endif

//**********************************************************

// defines for the WITH_UART option
/*
With define SWUART_INVERT you can specify, if the software-UART operates normal or invers.
In der normalen Betriebsart sendet der UART in der �blichen Logik (Low = 0; High = 1).
n the normal mode the UART sends with usual logik level (Low = 0; High = 1).
You can use this mode for direct connection to a �C, or a level converter like MAX232.

With invers mode the UART sends with invers logik (Low = 1, High = 0).
This is the level of a standard RS232 port of a PC.
In most cases the output of the software UART can so be connected to the RxD of a PC.
The specification say, that level -3V to 3V is unspecified, but in most cases it works.
Is a simple but unclean solution.

Is SWUART_INVERT defined, the UART works is inverse mode
*/
//#define SWUART_INVERT

#define TxD PC3	//TxD-Pin of Software-UART; must be at Port C !
#ifdef WITH_UART
 #define TXD_MSK (1<<TxD)
#else
 #define TXD_MSK 0
#endif

#ifdef SWUART_INVERT
  #define TXD_VAL 0
#else
  #define TXD_VAL TXD_MSK
#endif

#ifdef __AVR_ATmega8__
 // 2.54V reference voltage + korrection (fix for ATmega8)
 #ifdef AUTO_CAL
  #define ADC_internal_reference (2560 + (int8_t)eeprom_read_byte((uint8_t *)&RefDiff))
 #else
  #define ADC_internal_reference (2560 + REF_R_KORR)
 #endif
#else
 #ifdef AUTO_CAL
  #define ADC_internal_reference (ref_mv + (int8_t)eeprom_read_byte((uint8_t *)&RefDiff))
 #else
  #define ADC_internal_reference (ref_mv + REF_R_KORR)
 #endif
#endif

#undef AUTO_RH
#ifdef C_MESS
 #ifdef WITH_AUTO_REF
  #define AUTO_RH
 #else
  #ifdef AUTO_CAL
   #define AUTO_RH
  #endif
 #endif
#endif

#undef CHECK_CALL
#ifdef WITH_SELFTEST
 // AutoCheck Function is needed
 #define CHECK_CALL
#endif

#ifdef AUTO_CAL
  // AutoCheck Function is needed
  #define CHECK_CALL
  #define RR680PL resis680pl
  #define RR680MI resis680mi
#else
  #define RR680PL (R_L_VAL + PIN_RP)
  #define RR680MI (R_L_VAL + PIN_RM)
#endif

#ifndef ESR_ZERO
 //define a default zero value for ESR measurement (0.01 Ohm units)
 #define ESR_ZERO 23
#endif

