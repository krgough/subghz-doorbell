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
#define THIS_NODE GATEWAYID
#define RING_PIN 4

// Uncomment these to use the LED
#define USE_LED
#define RING_LED 9

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

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
  sprintf(buff, "\nReceiving on %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  sprintf(buff, "This node is %d", THIS_NODE);
  Serial.println(buff);

#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)\n");
#endif

  delay(100);

  // Setup the pin that rings the bell
  pinMode(RING_PIN, OUTPUT);
  digitalWrite(RING_PIN, LOW);

#ifdef USE_LED
  pinMode(RING_LED, OUTPUT);
  digitalWrite(RING_LED, LOW);
#endif
}

const char* BELL_RING = "DING_DONG";

bool parse_rx_data(char* rx_data){
  //rx_data[10] = "\0";
  if (strcmp(BELL_RING, rx_data) == 0){
    Serial.println("DING_DONG received");
    return true;
  }
  //Serial.println((char *)rx_data);

  return false;
}

void ring_bell(){
  // Do some bell ringing thing for 500ms
  Serial.println("Ringing the bell, Ding Dong...");

#ifdef USE_LED
  digitalWrite(RING_LED, HIGH);
  delay(100);
  digitalWrite(RING_LED, LOW);
#endif  

  // This simulates the 500ms ring delay
  for (uint8_t i=0; i<10; i++)
  {
    digitalWrite(RING_PIN, HIGH);
    delay(25);
    digitalWrite(RING_PIN, LOW);
    delay(25);
  }
}

void loop() {

   // Put Radio into Rx mode
   radio.receiveDone();

   // Sleep for a while.  If a packet is received then we automatically
   // wake.  On wake check if packet is our wanted packet.
   LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);

   // On wake if received data then handle it
   if (radio.receiveDone() && (parse_rx_data((char *)radio.DATA)))
   {
       // Send an ACK
       radio.sendACK();
       radio.sleep();
       ring_bell();
   }
   else
   {
      // We are awake but there's no ring message so we sleep for 1000ms
      radio.sleep();
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
   }
}
