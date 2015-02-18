//****************************************************************

// OSBSS PIR motion sensor datalogger - v0.01
// Last edited on February 17, 2015

//****************************************************************

#include <PowerSaver.h>
#include <DS3234lib3.h>
#include <SdFat.h>
#include <EEPROM.h>

PowerSaver chip; // declare object for PowerSaver class
DS3234 RTC;      // declare object for DS3234 class
SdFat sd;        // declare object for SdFat class
SdFile file;     // declare object for SdFile class

#define POWER 4        // pin 4 supplies power to microSD card breakout
#define RTCPOWER 6     // pin 6 supplies power to DS3234 RTC breakout
#define LED 7          // pin 7 controls LED
int SDcsPin = 9;       // pin 9 is micrSD card breakout CS pin
int state = 0;         // this variable stores the state of PIR sensor's output (either 1 or 0)

char filename[15] = "log.csv";    // Filename. Format: "12345678.123". Cannot be more than 8 characters in length, contain spaces or start with a number

ISR(PCINT0_vect)  // Interrupt Service Routine for PCINT0 vector (pin 8)
{
  asm("nop");  // do nothing
}

// Main code ****************************************************************
void setup()
{
  Serial.begin(19200);
  pinMode(POWER, OUTPUT);
  pinMode(RTCPOWER, OUTPUT);
  pinMode(LED, OUTPUT);
  
  digitalWrite(POWER, HIGH);     // turn on SD card
  digitalWrite(RTCPOWER, HIGH);     // turn on RTC
  delay(1);                      // give some delay to ensure SD card is turned on properly
  if(!sd.init(SPI_FULL_SPEED, SDcsPin))  // initialize SD card on the SPI bus
  {
    delay(10);
    SDcardError();
  }
  else
  {
    delay(10);
    file.open(filename, O_CREAT | O_APPEND | O_WRITE);  // open file in write mode and append data to the end of file
    delay(1);
    String time = RTC.timeStamp();    // get date and time from RTC
    file.println();
    file.print("Date/Time,Occupancy");    // Print header to file
    file.println();
    PrintFileTimeStamp();
    file.close();    // close file - very important
                     // give some delay by blinking status LED to wait for the file to properly close
    digitalWrite(LED, HIGH);
    delay(10);
    digitalWrite(LED, LOW);
    delay(10);    
  }
  chip.sleepInterruptSetup();  // set up sleep mode and interrupts
}

void loop()
{
  digitalWrite(POWER, LOW);  // turn off SD card
  digitalWrite(RTCPOWER, LOW);     // turn off RTC
  delay(1);
  chip.turnOffADC();  // turn off ADC to save power
  chip.turnOffSPI();  // turn off SPI bus to save power
  //chip.turnOffWDT();  // turn off WatchDog timer. This doesn't work for Pro Mini (Rev 11); only works for Arduino Uno
  chip.turnOffBOD();

  chip.goodNight();    // put the processor in power-down mode
  
  // code will resume from here once the processor wakes up ============== //
  chip.turnOnADC();   // turn on ADC once the processor wakes up
  chip.turnOnSPI();   // turn on SPI bus once the processor wakes up
  
  digitalWrite(POWER, HIGH);  // turn on SD card
  digitalWrite(RTCPOWER, HIGH);     // turn on RTC
  delay(1);    // give some delay to ensure SD card is turned on properly
  
  printToSD(); // print data to SD card
}

void printToSD()
{
  pinMode(SDcsPin, OUTPUT);
  if(!sd.init(SPI_FULL_SPEED, SDcsPin))    // very important - reinitialize SD card on the SPI bus
  {
    delay(10);
    SDcardError();
  }
  else
  {
    delay(10);
    file.open(filename, O_WRITE | O_AT_END);  // open file in write mode
    delay(1);
    String time = RTC.timeStamp();    // get date and time from RTC
    SPCR = 0;  // reset SPI control register
    
    state = digitalRead(8); // read current state of PIR. 1 = movement detected, 0 = no movement.
    
    if(state == 1)
    {
      file.print(time);
      file.print(",");
      file.print("0");
      file.println();
      file.print(time);
      file.print(",");
      file.print("1");
    }
    else if(state == 0)
    {
      file.print(time);
      file.print(",");
      file.print("1");
      file.println();
      file.print(time);
      file.print(",");
      file.print("0");
    }
    file.println();
    PrintFileTimeStamp();
    file.close();    // close file - very important
                     // give some delay by blinking status LED to wait for the file to properly close
    digitalWrite(LED, HIGH);
    delay(10);
    digitalWrite(LED, LOW);
    delay(10);  
  }
}

// file timestamps  ****************************************************************
void PrintFileTimeStamp() // Print timestamps to data file. Format: year, month, day, hour, min, sec
{ 
  file.timestamp(T_WRITE, RTC.year, RTC.month, RTC.day, RTC.hour, RTC.minute, RTC.second);    // date modified
  file.timestamp(T_ACCESS, RTC.year, RTC.month, RTC.day, RTC.hour, RTC.minute, RTC.second);    // date accessed
}

// SD card Error response  ****************************************************************
void SDcardError()
{
    for(int i=0;i<3;i++)   // blink LED 3 times to indicate SD card read/write error
    {
      digitalWrite(LED, HIGH);
      delay(50);
      digitalWrite(LED, LOW);
      delay(150);
    }
}

//****************************************************************
