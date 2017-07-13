#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h> 

/*
 * The watch is programmed using MCUdude's MicroCore library.
 * https://github.com/MCUdude/MicroCore
 * 
 * B.O.D. is set to disabled.
 * 
 * In order to save power the watch uses the 128kh internal clock.
 * 
 * To program it using an Arduino as ISP this line on the
 * Arduino as ISP sketch:
 *     #define SPI_CLOCK    (1000000/6)
 * should be changed to this:
 *     #define SPI_CLOCK     (128000/6)
 */

//uncomment to set the time in the watch
//must comment again and reload for the watch to work
//#define DOSETTIME

// Set your own pins with these defines !
#define DS1302_SCLK_PIN   PB2    // Arduino pin for the Serial Clock
#define DS1302_IO_PIN     PB3    // Arduino pin for the Data I/O
#define DS1302_CE_PIN     PB4    // Arduino pin for the Chip Enable



// Register names.
// Since the highest bit is always '1', 
// the registers start at 0x80
// If the register is read, the lowest bit should be '1'.
#define DS1302_SECONDS           0x80
#define DS1302_MINUTES           0x82
#define DS1302_HOURS             0x84
#define DS1302_DATE              0x86
#define DS1302_MONTH             0x88
#define DS1302_DAY               0x8A
#define DS1302_YEAR              0x8C
#define DS1302_ENABLE            0x8E
#define DS1302_TRICKLE           0x90
#define DS1302_CLOCK_BURST       0xBE
#define DS1302_CLOCK_BURST_WRITE 0xBE
#define DS1302_CLOCK_BURST_READ  0xBF
#define DS1302_RAMSTART          0xC0
#define DS1302_RAMEND            0xFC
#define DS1302_RAM_BURST         0xFE
#define DS1302_RAM_BURST_WRITE   0xFE
#define DS1302_RAM_BURST_READ    0xFF



// Defines for the bits, to be able to change 
// between bit number and binary definition.
// By using the bit number, using the DS1302 
// is like programming an AVR microcontroller.
// But instead of using "(1<<X)", or "_BV(X)", 
// the Arduino "bit(X)" is used.
#define DS1302_D0 0
#define DS1302_D1 1
#define DS1302_D2 2
#define DS1302_D3 3
#define DS1302_D4 4
#define DS1302_D5 5
#define DS1302_D6 6
#define DS1302_D7 7


// Bit for reading (bit in address)
#define DS1302_READBIT DS1302_D0 // READBIT=1: read instruction

// Bit for clock (0) or ram (1) area, 
// called R/C-bit (bit in address)
#define DS1302_RC DS1302_D6

// Seconds Register
#define DS1302_CH DS1302_D7   // 1 = Clock Halt, 0 = start

// Hour Register
#define DS1302_AM_PM DS1302_D5 // 0 = AM, 1 = PM
#define DS1302_12_24 DS1302_D7 // 0 = 24 hour, 1 = 12 hour

// Enable Register
#define DS1302_WP DS1302_D7   // 1 = Write Protect, 0 = enabled

// Trickle Register
#define DS1302_ROUT0 DS1302_D0
#define DS1302_ROUT1 DS1302_D1
#define DS1302_DS0   DS1302_D2
#define DS1302_DS1   DS1302_D2
#define DS1302_TCS0  DS1302_D4
#define DS1302_TCS1  DS1302_D5
#define DS1302_TCS2  DS1302_D6
#define DS1302_TCS3  DS1302_D7

//void setup(){
int main(void) {
  //disable some attiny functionality to save power
  ADCSRA &= ~(1<<ADEN); // disable analog to digital citcuits.
  wdt_disable();  // disable watchdog timer.

  /* uses binary coded decimals for each digit of the values
   * more info: https://en.wikipedia.org/wiki/Binary-coded_decimal
   *
   * The following table shows the encoding used for the values in each
   * register bit (from the DS1302 datasheet):
   * 
   * |            |                bit number                  |       |
   * |  register  |------+---+-------+---------+---+---+---+---| range |
   * |            | 7    | 6 |   5   |    4    | 3 | 2 | 1 | 0 |       |
   * |------------|------|---+-------+---------|---+---+---+---|-------|
   * | seconds    | halt |     seconds H       |   seconds L   | 0-59  |
   * | minutes    |  0   |     minutes H       |   minutes L   | 0-59  |
   * | hours (24) |  0   | 0 |     hour H      |     hour L    | 0-23  |
   * | '    '(12) |  1   | 0 | AM/PM | hour H  |     hour L    | 1-12  |
   * | date       |  0   | 0 |     date H      |     date L    | 1-31  |
   * | month      |  0   | 0 |   0   | month H |    month L    | 1-12  |
   * | week day   |  0   | 0 |   0   |    0    | 0 |    day    | 1-7   |
   * | year       |           year H           |     year L    | 0-99  |
   */
  #ifdef DOSETTIME
  // if(DS1302_read(DS1302_YEAR) < 10){ // defaults to 0
    DS1302_write(DS1302_YEAR,    0B00010111);
    DS1302_write(DS1302_MONTH,   0B00000111);
    DS1302_write(DS1302_DATE,    0B00000101);
    DS1302_write(DS1302_HOURS,   0B10100110);
    DS1302_write(DS1302_MINUTES, 0B00110010);

  // required to start the clock
    DS1302_write(DS1302_ENABLE,  0);
    DS1302_write(DS1302_SECONDS, 0); 
  // }
  #endif


	// // // read the actual registers:
	byte hourdata = DS1302_read(DS1302_HOURS);
	byte mindata  = DS1302_read(DS1302_MINUTES);
  // byte secdata  = DS1302_read(DS1302_SECONDS);

  byte monthdata  = DS1302_read(DS1302_MONTH);
  byte datedata = DS1302_read(DS1302_DATE);

 // //  // disable ds1302 communication:
  pinMode(DS1302_CE_PIN, OUTPUT);
  digitalWrite(DS1302_CE_PIN, LOW);

	// // // decode values of decimals stored as binary coded decimals:
	byte minutes  = ((mindata >> 4)*10 + (mindata & 0xf));
	byte hours    = ((bitRead(hourdata,4))*10 + (hourdata & 0xf)); // for 12h mode
  // byte hours  = ((hourdata >> 4)*10 + (hourdata & 0xf)); // for 24h mode
  // byte seconds  = ((secdata >> 4)*10+(secdata & 0xf));

  // show time for some time:
  for(int i = 0; i < 600; i++){
    for(byte currLed = 1; currLed <= 6; currLed++){
      if(bitRead(minutes, currLed - 1)){
      //if(currLed == 6){ // debug
        writeLed(0); //clear LEDs
        writeLed(currLed);
      }
    }
    for(byte currLed = 1; currLed <= 4; currLed++){
      if(bitRead(hours, currLed - 1)){
      //if(currLed == 1){ //debug
        writeLed(0); //clear LEDs
        writeLed(currLed + 6);
      }
    }
  }
  //delay(10);
  byte date  = ((datedata >> 4)*10 + (datedata & 0xf));
  byte month  = ((monthdata >> 4)*10 + (monthdata & 0xf));
  // show date for some time:
  for(int i = 0; i < 512; i++){
    for(byte currLed = 1; currLed <= 6; currLed++){
      if(bitRead(date, currLed - 1)){
      //if(currLed == 6){ // debug
        writeLed(0); //clear LEDs
        writeLed(currLed);
      }
    }
    for(int currLed = 1; currLed <= 4; currLed++){
      if(bitRead(month, currLed - 1)){
      //if(currLed == 1){ //debug
        writeLed(0); //clear LEDs
        writeLed(currLed + 6);
      }
    }
  }
  // clear the display:
  writeLed(0);
  // go to sleep:
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();
}

//void loop(){
//}

// Compact charlieplexing function
// Chaplex library was too big for the attiny13

// Takes a LED number and turns it on
// by changing the state and mode of the pins
// can handle a max of 12 LEDs, only 10 are implemented
void writeLed(byte ledNum){
  // the state and mode of each pin
  // are defined in a byte:
  // first 4 bits (right to left) define data direction
  // 1 = output, 0 = input
  // last 4 bits define data value
  // 1 = high, 0 = low
  // The ATTINY registers are then set to these values
  
  byte state;
  
  switch(ledNum){
    case 1:// M1
    state = 0B01001100;
    break;
    case 2:// M2
    state = 0B00100110;
    break;
    case 3:// M4
    state = 0B00010011;
    break;
    case 4:// M8
    state = 0B01000101;
    break;
    case 5:// M16
    state = 0B10001001;
    break;
    case 6:// M32
    state = 0B00011001;
    break;
    case 7:// H1
    state = 0B10001100;
    break;
    case 8:// H2
    state = 0B01000110;
    break;
    case 9:// H4
    state = 0B00100011;
    break;
    case 10:// H8
    state = 0B00010101;
    break;
    default:
    state = 0B00000000;
  }

  DDRB = state & 0x0f; // first 4 bytes set the direction
  PORTB = state >> 4; // last 4 bytes set the value
}

// --------------------------------------------------------
// DS1302_read
//
// This function reads a byte from the DS1302 
// (clock or ram).
//
// The address could be like "0x80" or "0x81", 
// the lowest bit is set anyway.
//
// This function may be called as the first function, 
// also the pinMode is set.
//
byte DS1302_read(int address)
{
  byte data;

  // set lowest bit (read bit) in address
  bitSet( address, DS1302_READBIT);  

  _DS1302_start();
  // the I/O-line is released for the data
  _DS1302_togglewrite( address, true);  
  data = _DS1302_toggleread();
  _DS1302_stop();

  return (data);
}


// --------------------------------------------------------
// DS1302_write
//
// This function writes a byte to the DS1302 (clock or ram).
//
// The address could be like "0x80" or "0x81", 
// the lowest bit is cleared anyway.
//
// This function may be called as the first function, 
// also the pinMode is set.
//
void DS1302_write( int address, byte data)
{
  // clear lowest bit (read bit) in address
  bitClear( address, DS1302_READBIT);   

  _DS1302_start();
  // don't release the I/O-line
  _DS1302_togglewrite( address, false); 
  // don't release the I/O-line
  _DS1302_togglewrite( data, false); 
  _DS1302_stop();  
}


// --------------------------------------------------------
// _DS1302_start
//
// A helper function to setup the start condition.
//
// An 'init' function is not used.
// But now the pinMode is set every time.
// That's not a big deal, and it's valid.
// At startup, the pins of the Arduino are high impedance.
// Since the DS1302 has pull-down resistors, 
// the signals are low (inactive) until the DS1302 is used.
void _DS1302_start( void)
{
  digitalWrite( DS1302_CE_PIN, LOW); // default, not enabled
  pinMode( DS1302_CE_PIN, OUTPUT);  

  digitalWrite( DS1302_SCLK_PIN, LOW); // default, clock low
  pinMode( DS1302_SCLK_PIN, OUTPUT);

  pinMode( DS1302_IO_PIN, OUTPUT);

  digitalWrite( DS1302_CE_PIN, HIGH); // start the session
  delayMicroseconds( 4);           // tCC = 4us
}


// --------------------------------------------------------
// _DS1302_stop
//
// A helper function to finish the communication.
//
void _DS1302_stop(void)
{
  // Set CE low
  digitalWrite( DS1302_CE_PIN, LOW);

  delayMicroseconds( 4);           // tCWH = 4us
}


// --------------------------------------------------------
// _DS1302_toggleread
//
// A helper function for reading a byte with bit toggle
//
// This function assumes that the SCLK is still high.
//
byte _DS1302_toggleread( void)
{
  byte i, data;

  data = 0;
  for( i = 0; i <= 7; i++)
  {
    // Issue a clock pulse for the next databit.
    // If the 'togglewrite' function was used before 
    // this function, the SCLK is already high.
    digitalWrite( DS1302_SCLK_PIN, HIGH);
    delayMicroseconds( 1);

    // Clock down, data is ready after some time.
    digitalWrite( DS1302_SCLK_PIN, LOW);
    delayMicroseconds( 1);        // tCL=1000ns, tCDD=800ns

    // read bit, and set it in place in 'data' variable
    bitWrite( data, i, digitalRead( DS1302_IO_PIN)); 
  }
  return( data);
}


// --------------------------------------------------------
// _DS1302_togglewrite
//
// A helper function for writing a byte with bit toggle
//
// The 'release' parameter is for a read after this write.
// It will release the I/O-line and will keep the SCLK high.
//
void _DS1302_togglewrite( byte data, byte release)
{
  int i;

  for( i = 0; i <= 7; i++)
  { 
    // set a bit of the data on the I/O-line
    digitalWrite( DS1302_IO_PIN, bitRead(data, i));  
    delayMicroseconds( 1);     // tDC = 200ns

    // clock up, data is read by DS1302
    digitalWrite( DS1302_SCLK_PIN, HIGH);     
    delayMicroseconds( 1);     // tCH = 1000ns, tCDH = 800ns

    if( release && i == 7)
    {
      // If this write is followed by a read, 
      // the I/O-line should be released after 
      // the last bit, before the clock line is made low.
      // This is according the datasheet.
      // I have seen other programs that don't release 
      // the I/O-line at this moment,
      // and that could cause a shortcut spike 
      // on the I/O-line.
      pinMode( DS1302_IO_PIN, INPUT);

      // For Arduino 1.0.3, removing the pull-up is no longer needed.
      // Setting the pin as 'INPUT' will already remove the pull-up.
      // digitalWrite (DS1302_IO, LOW); // remove any pull-up  
    }
    else
    {
      digitalWrite( DS1302_SCLK_PIN, LOW);
      delayMicroseconds( 1);       // tCL=1000ns, tCDD=800ns
    }
  }
}

