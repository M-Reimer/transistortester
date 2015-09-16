#include "wait1000ms.h"
#include "lcd-routines.h"

void CheckPins(uint8_t HighPin, uint8_t LowPin, uint8_t TristatePin);
void GetResistance(uint8_t HighPin, uint8_t LowPin);
void ChargePin10ms(uint8_t PinToCharge, uint8_t ChargeDirection);
unsigned int ReadADC(uint8_t mux);		// read Routine for ADC
unsigned int W5msReadADC(uint8_t mux);		// wait 5ms and read than ADC
unsigned int W10msReadADC(uint8_t mux);		// wait 10ms and read than ADC
unsigned int W20msReadADC(uint8_t mux);		// wait 20ms and read then ADC
void ReadCapacity(uint8_t HighPin, uint8_t LowPin);	//capacity measurement
void ReadInductance(void);		//inductance measurement
uint16_t GetESR(uint8_t hipin, uint8_t lopin);	//get ESR of capacitor
void GetVloss(void);			//get Voltage loss of capacitor
void GetIr(uint8_t HighPin, uint8_t LowPin);	// get residual current of diode
void SerienDiodenAusgabe();		// output diodes as ->|-->|- with voltages
void UfAusgabe(uint8_t bcdchar);	// Output of the threshold voltage(s) Uf
void mVAusgabe(uint8_t nn);		// Output of the theshold voltage for Diode nn 
void RvalOut(uint8_t nrr);		// Output of the resistor value(s)
void ShowResistor(void);		// show one or two Resistors
void EntladePins(void);			// discharge capacitors
void RefVoltage(void);			// compensate the reference voltage for comparator 
void AutoCheck(uint8_t test_mode);	// check if self-test should be done 
unsigned int GetRLmultip(unsigned int cvolt);  // get C-Multiplikator for voltage cvolt
void Scale_C_with_vcc(void);		// scale capacity value for different VCC Voltages
void scale_intref_adc(void);		// get scale factors for ReadADC with internal reference
//uint8_t value_out(unsigned long vval,uint8_t pp);    // output 4 digits with (pp-1) digits after point
void DisplayValue(signed long vval,int8_t Expo,unsigned char Unit, unsigned char Digits); //output Digits characters with exponent and unit
void Display_mV(uint16_t val, uint8_t dd);		// show mV with dd digits
unsigned int compute_hfe(unsigned int lpx, unsigned int tpy);
void sleep_5ms(uint8_t xxx);		// set processor to sleep state for xxx times 5ms, xxx>200  (xxx-200)*100  
void PinLayout(char pin1, char pin2, char pin3); // show pin layout with EBC= or 123=
void PinLayoutLine(char pin1, char pin2, char pin3); // show pin layout with Pin 1=...
void Calibrate_UR(void);		// get reference voltages and port resistance
uint16_t get_log(uint16_t permil);	// get -1000*log(1-(permil/1000))
uint8_t ShortedProbes(uint8_t p1, uint8_t p2);	// check if two probes are shorted
uint8_t AllProbesShorted();		// check if all probes are shorted, return 3 if true
void mark_as_uncalibrated();		// mark in EEprom as uncalibrated device
uint8_t wait_for_key_ms(int max_time); // wait max_time ms units for key is pressed
void wait_for_key_5s_line2(void);	// wait 5 seconds and clear line 2
void i2lcd(int iw);			// output integer value to LCD
void u2lcd(uint16_t iw);		// output unsigned integer value to LCD
void EE_check_init(void);		// check calibration values and init the values if unset
void GetFrequency(uint8_t range);	// measure Frequency and display 
void function_menu();			// menu for selecting extra function
void show_vext();			// show external voltage (zener) 
void set_big_cap_corr();		// set the correction value for big capacitor measurement
void message_key_released(const unsigned char XX_str[]); // clear display, show message XX_str and wait for released key
void make_frequency(void);		// start counter 1 to make frequency at middle pin
void do_10bit_PWM(void);		// start counter 1 to make fixed frequency with PWM
void show_C_ESR();			// show big capacity values and ESR 
void show_Resis13();			// show Resistor value at TP1:TP3 
void show_Cap13(void);		// show one or two Resistors
void ReadBigCap(uint8_t HighPin, uint8_t LowPin);	//capacity measurement only for big capacitors
void CheckRotaryEncoder(void);				// check rotary puls generator
void check_rotary(void);			// check state of rotary switch	for menu function
void message2line(uint8_t number);	// write menue message number to lcd line
void switch_frequency(uint8_t freq_num);	// switch frequency and write actual frequency to LCD
void CalibrationCap(void);		// read voltage offsets with calibration capacitor
void set_contrast(void);		// set the contrast value of ST7565 controller
void ShowData(void);			// show software revision and calibration parameters
void Battery_check(void);		// check and report the battery state
void Bat_update(uint8_t tt);		// Update Battery voltage
void init_parts(void);			// initialize all parts to nothing found
void i2c_init(void);			// init the I2C interface
uint16_t Rnum2pins(uint8_t num);	// compute Pin-Numbers of the resistor number

#ifdef SamplingADC
extern uint16_t samplingADC(   // code is in sampling_ADC.S
   uint16_t what, 	// what to measure? see samplingADC_... defs below
   uint16_t *ptr,       // output pointer (note: ptr[0] will contain incorrect data; ptr[1]...ptr[n-1] will be valid)
   uint8_t n,              // number of samples (n=0 for 256)
   uint8_t Rport_1,	// port value for active part of step / inactive part of impulse
   uint8_t Rddr_1,		// ddr value for same
   uint8_t Rport_0,	// port values for inactive part of step / active part of impulse; note that for impulse response, Rport_1 must equal Rport_0 (we don't have enough time to toggle both port and ddr between impulses)
   uint8_t Rddr_0          // ddr value for same
   );
#define samplingADC_step      (1<<0)   // do step response, not impulse
#define samplingADC_slow4     (1<<1)   // only take a sample every 4 clockcycles
#define samplingADC_slow16    (1<<4)   // only take a sample every 16 clockcycles
#define samplingADC_cumul     (1<<2)   // don't overwrite, but add the samples to the array
#define samplingADC_twopulses (1<<3)   // send 2 impulses rather than one; inter-pulse time is in the upper 8 bits, should be in the range 4..15
extern int32_t sampling_cap(uint8_t HighPin, uint8_t LowPin, uint8_t hivolt);   // returns measured capacitance in 0.01 pF units; hivolt flag demands measurement at 5 V rather than at 0 V
#define sampling_cap_pre -14
void sampling_cap_calibrate(void);	// calibrate the sampling cap method
#endif
