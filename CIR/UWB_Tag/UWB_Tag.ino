/*
 * Copyright (c) 2020 by Ashutosh Dhekne <dhekne@gatech.edu>
 * Peer-peer protocol
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file PeerProtocol_test01.ino
 * 
 *  
 */

#include <SPI.h>
#include <math.h>
#include "genericFunctions.h"
#include "Adafruit_LSM9DS1.h"
#include <time.h>
#include<TimeLib.h>
#include "RTClib.h"
#include<Wire.h>


#define INITIATOR 0
Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();

#define DEBUG_PRINT 0
#define DEBUG_CIR 0
// connection pins
#define OUR_UWB_FEATHER 1
#define NCIR_FULL 1016
#define RX_TIME_FP_INDEX_OFFSET 5
int packet_count = 1;
int packet_type = 0;


#if(OUR_UWB_FEATHER==1)
const uint8_t PIN_RST = 9; // reset pin
const uint8_t PIN_IRQ = 17; // irq pin
const uint8_t PIN_SS = 19; // spi select pin
#endif


// DEBUG packet sent status and count
volatile boolean received = false;
volatile boolean error = false;
volatile int16_t numReceived = 0; // todo check int type
volatile boolean sendComplete = false;
volatile boolean RxTimeout = false;
String message;

byte tx_poll_msg[MAX_POLL_LEN] = {POLL_MSG_TYPE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


byte rx_packet[128];
uint8_t myAcc[1000];

typedef enum states{STATE_IDLE, STATE_POLL} STATES;
volatile uint8_t current_state = STATE_IDLE;
unsigned long silenced_at =0;
#define SILENCE_PERIOD 120
long randNumber;
int currentSlots = 8;
int myDevID = INITIATOR+2;

#define MAX_TIMEOUTS 2
volatile boolean timeout_established[MAX_TIMEOUTS];
volatile boolean timeout_triggered[MAX_TIMEOUTS];
volatile boolean timeout_overflow[MAX_TIMEOUTS];
volatile uint64_t timeout_time[MAX_TIMEOUTS];

//Function to receive a packet
void receiver(uint16_t rxtoval=0 ) {
  received = false;
  DW1000.newReceive();
  DW1000.setDefaults();
  // we cannot don't need to restart the receiver manually
  DW1000.receivePermanently(false);
  if (rxtoval>0) {
    DW1000.setRxTimeout(rxtoval);
  } else {
    //Serial.print("Resetting Timeout to  ");
    //Serial.println(rxtoval);
    DW1000.setRxTimeout(rxtoval);
  }
  DW1000.startReceive();
  //Serial.println("Started Receiver");
}

void print_CIR()
{
    char buff[140];
    char long_buff[1400];
  
    byte firstPath[2];
    DW1000.readBytes(RX_TIME, RX_TIME_FP_INDEX_OFFSET, firstPath, 2);
    uint16_t firstpath = uint16_t(firstPath[1]<<8 | firstPath[0]);
    uint16_t firstPath_integer = (firstpath & 0xFFC0) >> 6;
    uint16_t firstPath_fraction = (firstpath & 0x003F);
    float RX_POWER = DW1000.getReceivePower();
    float FP_POWER = DW1000.getFirstPathPower();
    
    uint8_t myAcc[4 * NCIR_FULL + 6];
    int starttap = 720;
    int endtap = 816; 
    int16_t RealData = 0;
    int16_t ImaginaryData = 0;
    
    sprintf(long_buff, "CIR %d %d ", firstPath_fraction, firstPath_integer);
    DW1000.getAccMem(myAcc, 0, endtap + 1); //myAcc will contain 16 bit real imaginary pairs
    
    for (int i = starttap ; i < endtap; i++) {
      RealData = myAcc[(i * 4) + 2] << 8 | myAcc[(i * 4) + 1];
      ImaginaryData = myAcc[(i * 4) + 4] << 8 | myAcc[(i * 4) + 3];
   sprintf(buff, "[%d,%d,%d,%d],", packet_count, RealData, ImaginaryData, i + 1);
    strcat(long_buff, buff);
  }
  Serial.println(long_buff);
}

//Setup the board
void setup() {
  analogReadResolution(10);
  // DEBUG monitoring
  Serial.begin(115200);
  while(!Serial)
  {
    delay(10);
    #if(USB_CONNECTION==0)
      break;
    #endif
  }
  Serial.print("Waiting...");
  delay(5000);
  Serial.print("Should see this...");
  randomSeed(analogRead(0));
  Serial.println("Free memory: ");
  Serial.println(freeMemory());
  // initialize the driver
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);
  Serial.println(F("DW1000 initialized ..."));
  // general configuration
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(6);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();
  Serial.println(F("Committed configuration ..."));
  // DEBUG chip info and registers pretty printed
  char msg[128];
  DW1000.getPrintableDeviceIdentifier(msg);
  Serial.print("Device ID: "); Serial.println(msg);
  DW1000.getPrintableExtendedUniqueIdentifier(msg);
  Serial.print("Unique ID: "); Serial.println(msg);
  DW1000.getPrintableNetworkIdAndShortAddress(msg);
  Serial.print("Network ID & Device Address: "); Serial.println(msg);
  DW1000.getPrintableDeviceMode(msg);
  Serial.print("Device mode: "); Serial.println(msg);
  // attach callback for (successfully) received messages
  DW1000.attachReceivedHandler(handleReceived);
  DW1000.attachReceiveTimeoutHandler(handleRxTO);
  DW1000.attachReceiveFailedHandler(handleError);
  DW1000.attachErrorHandler(handleError);
  DW1000.attachSentHandler(handleSent);
  // start reception
  
  #if (INITIATOR==0)
    receiver(0);
    
  #endif
  current_state = STATE_IDLE;

  for(int i=0;i<MAX_TIMEOUTS;i++) {
    timeout_established[i]=false;
    timeout_triggered[i]=false;
    timeout_overflow[i]=false;
    timeout_time[i]=0;
  } 
}

void handleSent() {
  // status change on sent success
  sendComplete = true;
  //Serial.println("Send complete");
}


void handleReceived() {
  // status change on reception success
  
  DW1000.getData(rx_packet, DW1000.getDataLength());
  //Serial.println("Received something...");
  received = true;
  //show_packet(rx_packet, DW1000.getDataLength());
}

void handleError() {
  error = true;
}

void handleRxTO() {
  current_state = STATE_IDLE;
  RxTimeout = true;
  #if (DEBUG_PRINT==1)
  Serial.println("Rx Timeout");
  Serial.println("State: ");
  Serial.println(current_state);
  #endif
}

uint16_t seq = 0;

void loop() {

      if (received) {
        received = false;
        
        print_CIR();
        seq = rx_packet[SEQ_IDX] +  ((uint16_t)rx_packet[SEQ_IDX+1] << 8);
        Serial.print("Received: ");
        Serial.println(seq);
        receiver(60);
      }
}


void show_packet(byte packet[], int num) {
  #if (DEBUG_PRINT==1)
  for (int i=0;i<num;i++) {
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }
  #endif
  
}


#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__


int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
