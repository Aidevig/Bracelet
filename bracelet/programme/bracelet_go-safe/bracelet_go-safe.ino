
/**************************************************************************/
/*!
    Initialisation
*/
/**************************************************************************/

#include <SPI.h>
#include "Adafruit_BLE_UART.h"
#include <Wire.h>

#define ADAFRUITBLE_REQ 10
#define ADAFRUITBLE_RDY 3
#define ADAFRUITBLE_RST 8

Adafruit_BLE_UART uart = Adafruit_BLE_UART(ADAFRUITBLE_REQ, ADAFRUITBLE_RDY, ADAFRUITBLE_RST);

//  VARIABLES

int pulseStep = 120;              // Pulse frequency step that triggers an alert (BPM)
int pulseTime = 60;               // Time above pulseStep to trigger an alert (sec)
int pulseCount = 0;               // if this count is greater than the pulseTime an automatic alert is triggered
int notifTime = 60;               // Delay in which the device is waiting for the user to confirm an alert 
int timeout = 0;                  // Counter
int soundStep = 650;              // Sound step that triggers an alert
char notif = 'B';                 // Char sends when alert is triggered manually via the button
unsigned char heartRate = 0;      // HeartRate detected by the pulse sensor
boolean btnState = LOW;           // Initial state of the button
boolean btnValue;                 //

/**************************************************************************/
/*!
    This function is called whenever data arrives on the RX channel
    If the RX received begins by a 'V', the device vibrate for 1000ms
*/
/**************************************************************************/
void rxCallback(uint8_t *buffer, uint8_t len)
{
  if(buffer[0]=='V'){
    digitalWrite(6, HIGH);
    delay(1000);
    digitalWrite(6, LOW);
  }
}

/**************************************************************************/
/*!
    This function is called whenever the button is pressed
    Sends the char notif and the heart rate when the button is pressed
    Waits 10s before the next notification to avoid excessive signals
*/
/**************************************************************************/
void notify()
{
  uart.print(String(notif) + heartRate);   // Sends data through uart characteristic
  notif = 'B';                             // Resets the notif to the initial value
  delay(10000);                            // Waits 10 s before processing the next notification
}


/**************************************************************************/
/*!
    This function is called whenever an alert is triggered by the pulse or the sound sensor
    Two short vibrations notify the user of the detected alert
    The timeout is then the period of time during which the alert is in the pending state, 
    waiting for the user to confirm the alert
*/
/**************************************************************************/
void askForNotif(char c)
{
  digitalWrite(6, HIGH);
  delay(500);
  digitalWrite(6, LOW);
  delay(250);
  digitalWrite(6, HIGH);
  delay(500);
  digitalWrite(6, LOW);

  notif = c;
  timeout = notifTime * 2;
}

/**************************************************************************/
/*!
    This function sends an analog value and its type i.e askForNotif
    If the value i.e the heart rate is greater than the pulse step there are two cases
        Case 1 : the pulseCount is greater than 2, meaning we processed twice a dangerous heart rate, a notification is sent and the pulse count is reset to 0
        Otherwise, the pulseCount is incremented 
*/
/**************************************************************************/

void processData(int value)
{
  if(value >= pulseStep) {
    if(pulseCount >= pulseTime / 30) {
      if(timeout<=0){
        askForNotif('C');
        pulseCount =  0;
      }
    } else {
      pulseCount ++;
    }
  } else {
    pulseCount = 0;
  }
}
  
/**************************************************************************/
/*!
    Configure the Arduino and start advertising with the radio
*/
/**************************************************************************/
void setup(void)
{
  pinMode(6, OUTPUT);
  pinMode(2, INPUT);
  pinMode(A2, INPUT);
  Wire.begin();
  uart.setRXcallback(rxCallback);
  uart.setDeviceName("GO-SAFE"); /* 7 characters max! */
  uart.begin(); 
}

/**************************************************************************/
/*!
    Constantly checks for new events on the nRF8001
*/
/**************************************************************************/
void loop()
{
  uart.pollACI();

  Wire.requestFrom(0xA0 >> 1, 1);    // request 1 bytes from slave device 
  while(Wire.available()) {          // slave may send less than requested
    heartRate = Wire.read();         // receive heart rate value (a byte)
    processData(heartRate);          // process heartRate
  }

  for(int i = 0; i<25; i++){
    
    if (timeout<=0 && analogRead(A2) >= soundStep) {
      askForNotif('S'); 
    }

    btnValue = digitalRead(2);

    if(btnValue == HIGH && btnState == LOW){
      btnState = HIGH;
      notify();
    }
    if(btnValue == LOW){
      btnState = LOW;
    }

    delay(20);
  }

  if(timeout <= 0){
    notif = 'B';
  } else {
    timeout --; 
  }
}
