// Sample RFM69 sender/node sketch, with ACK and optional encryption, and Automatic Transmission Control
// Sends periodic messages of increasing length to gateway (id=1)
// If gateway is present it should ACK the message.
// **********************************************************************************
// Copyright Felix Rusu 2018, http://www.LowPowerLab.com/contact
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://www.github.com/lowpowerlab/rfm69
//#include <SPIFlash.h>      //get it here: https://www.github.com/lowpowerlab/spiflash
#include <LowPower.h>      //get library from: https://github.com/lowpowerlab/lowpower

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE ************
//*********************************************************************************************
// Address IDs are 10bit, meaning usable ID range is 1..1023
// Address 0 is special (broadcast), messages to address 0 are received by all *listening* nodes (ie. active RX mode)
// Gateway ID should be kept at ID=1 for simplicity, although this is not a hard constraint
//*********************************************************************************************
#define NODEID        2    // keep UNIQUE for each node on same network
#define NETWORKID     100  // keep IDENTICAL on all nodes that talk to each other
#define GATEWAYID     1    // "central" node

//*********************************************************************************************
// Frequency should be set to match the radio module hardware tuned frequency,
// otherwise if say a "433mhz" module is set to work at 915, it will work but very badly.
// Moteinos and RF modules from LowPowerLab are marked with a colored dot to help identify their tuned frequency band,
// see this link for details: https://lowpowerlab.com/guide/moteino/transceivers/
// The below examples are predefined "center" frequencies for the radio's tuned "ISM frequency band".
// You can always set the frequency anywhere in the "frequency band", ex. the 915mhz ISM band is 902..928mhz.
//*********************************************************************************************
//#define FREQUENCY   RF69_433MHZ
#define FREQUENCY   RF69_868MHZ
//#define FREQUENCY     RF69_915MHZ
//#define FREQUENCY_EXACT 916000000 // you may define an exact frequency/channel in Hz
// ENCRYPT key should be defined in config.h and NOT aved to github
//#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*********************************************************************************************
//Auto Transmission Control - dials down transmit power to save battery
//Usually you do not need to always transmit at max output power
//By reducing TX power even a little you save a significant amount of battery power
//This setting enables this gateway to work with remote nodes that have ATC enabled to
//dial their power down to only the required level (ATC_RSSI)
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -80
//*********************************************************************************************
#define SERIAL_BAUD   115200

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

// Keith's global vars
#define THIS_NODE NODEID
#define BUTTON_PIN 3

#define USE_LED
#define LED 9

void setup() {
  Serial.begin(SERIAL_BAUD);
  radio.initialize(FREQUENCY,THIS_NODE,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif

#ifdef ENCRYPTKEY
  radio.encrypt(ENCRYPTKEY);
#endif

#ifdef FREQUENCY_EXACT
  radio.setFrequency(FREQUENCY_EXACT); //set frequency to some custom frequency
#endif
  
//Auto Transmission Control - dials down transmit power to save battery (-100 is the noise floor, -90 is still pretty good)
//For indoor nodes that are pretty static and at pretty stable temperatures (like a MotionMote) -90dBm is quite safe
//For more variable nodes that can expect to move or experience larger temp drifts a lower margin like -70 to -80 would probably be better
//Always test your ATC mote in the edge cases in your own environment to ensure ATC will perform as you expect
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  char buff[50];
  sprintf(buff, "\nTransmitting on %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  sprintf(buff, "This node is %d", THIS_NODE);
  Serial.println(buff);

#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)\n");
#endif
  delay(100);

  // Configure BUTTON_PIN as input.
  // This may consume some current?
  pinMode(BUTTON_PIN, INPUT_PULLUP);

#ifdef USE_LED
  pinMode(LED, OUTPUT);

  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
#endif
  
}

int send_ding_dong(){
  // Send the "ding_dong" message until we get an ACK
  // or we timeout
  uint32_t send_time;
  uint32_t timeout = 10;
  uint8_t max_attempts = 101; // 10ms * 10 = 1010ms (i.e. just longer than our Gateway sleep period)
  
  for (uint8_t i=0; i <= max_attempts; i++)
  {
    radio.send(GATEWAYID, "DING_DONG", 9, true);
    send_time = millis();
    while(millis() - send_time < timeout)
    {
      if (radio.ACKReceived(GATEWAYID)) return i + 1;
    }
  }
  return -1;
}

bool debounce(){
  // Simple debouce.
  // We read the pin every 10ms and count up
  // If we get 5 lows in 5 reads then we assume
  // its a valid button push

  uint8_t count = 0;
  for (uint8_t i=0; i<5; i++)
  {
    if (digitalRead(BUTTON_PIN) == LOW) count++;
    if (count >= 4) return true;
  }
  return false;
}

bool button_pushed = false;
void loop() {

  // Put Radio to sleep
  radio.sleep();
 
  // Go to sleep for at least 8s (might change this to forever)
  delay(10);  // short delay to allow all serial writes to complete
 
  // Setup the interrupt
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_pushed_isr, FALLING);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 

  // Disable external pin interrupt on wake up pin.
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));

  // debounce de-asserts button_pushed if we think it's a false trigger
  button_pushed = debounce();

  Serial.println("Going round again...");

  // If the bell button was pushed then ring the bell
  // Otherwise loop and go to sleep again
  // Repeatedly Tx for 1s or until ACK
  if (button_pushed){
    digitalWrite(LED, HIGH);
    button_pushed = false;
    Serial.print("Tx now: ");
    int attempts = send_ding_dong();
    Serial.println(attempts);
    digitalWrite(LED, LOW);
  }
}

void button_pushed_isr(){
  button_pushed = true;
}
