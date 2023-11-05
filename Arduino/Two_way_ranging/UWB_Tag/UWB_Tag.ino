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
#include "RangingContainer.h"
#include "Adafruit_LSM9DS1.h"
#include <SdFat.h>
#include <time.h>
#include<TimeLib.h>
#include "RTClib.h"
#include<Wire.h>

#define VBATPIN A2
#define INIT_RTC_ALWAYS 0
#define USB_CONNECTION 0
#define INITIATOR 0
#define DEBUG_PRINT 0
#define DEBUG_CIR 0

// connection pins
#define OUR_UWB_FEATHER 1
#define AUS_UWB_FEATHER 0

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
byte rx_resp_msg[MAX_RESP_LEN] = {RESP_MSG_TYPE, 0x02, 0, 0, 0, 0, 0};
byte tx_final_msg[MAX_FINAL_LEN] = {FINAL_MSG_TYPE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

Ranging thisRange;

byte rx_packet[128]; //stores a packet received by the dw1000 sensor
uint8_t myAcc[1000];

typedef enum states{STATE_IDLE, STATE_POLL, STATE_RESP_EXPECTED, STATE_FINAL_SEND, STATE_TWR_DONE, STATE_RESP_SEND, STATE_FINAL_EXPECTED, STATE_OTHER_POLL_EXPECTED, STATE_RESP_PENDING, STATE_DIST_EST_EXPECTED, STATE_DIST_EST_SEND, STATE_TIGHT_LOOP,
STATE_RECEIVE, STATE_PRESYNC, STATE_SYNC, STATE_ANCHOR, STATE_TAG, STATE_FIRST_START, STATE_OBLIVION, STATE_ACK_EXPECTED,
STATE_RECEIVED_WIFI_DATA, STATE_SEND_UWB_DATA, UWB_DATA_EXPECTED, STATE_RECEIVE_UWB_DATA} STATES;
volatile uint8_t current_state = STATE_IDLE;
unsigned long silenced_at =0;
long randNumber;
int currentSlots = 8;
int myDevID = INITIATOR+2;

//Timer for implementing timeouts
#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1024

void startTimer(int frequencyHz);
void setTimerFrequency(int frequencyHz);

void TC3_Handler();

#define MAX_TIMEOUTS 2
volatile boolean timeout_established[MAX_TIMEOUTS];
volatile boolean timeout_triggered[MAX_TIMEOUTS];
volatile boolean timeout_overflow[MAX_TIMEOUTS];
volatile uint64_t timeout_time[MAX_TIMEOUTS];

//Time
RTC_PCF8523 rtc;

typedef struct DeviceRespTs {
  int deviceID;
  uint64_t respRxTime;
};

#define MAX_DEVICES_TOGETHER 20
DeviceRespTs deviceRespTs[MAX_DEVICES_TOGETHER];
int currentDeviceIndex = 0;

//Function to receive a packet using the dw1000 sensor
void receiver(uint16_t rxtoval=0 ) {
  received = false;
  DW1000.newReceive();
  DW1000.setDefaults();
  // we cannot don't need to restart the receiver manually
  DW1000.receivePermanently(false);
  if (rxtoval>0) {
    DW1000.setRxTimeout(rxtoval);
  } else {
    DW1000.setRxTimeout(rxtoval);
  }
  DW1000.startReceive();
}

//Function to print the CIR data
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
    
    strcpy(long_buff, "CIR");
    DW1000.getAccMem(myAcc, 0, endtap + 1); //myAcc will contain 16 bit real imaginary pairs
    
    for (int i = starttap ; i < endtap; i++) {
      RealData = myAcc[(i * 4) + 2] << 8 | myAcc[(i * 4) + 1];
      ImaginaryData = myAcc[(i * 4) + 4] << 8 | myAcc[(i * 4) + 3];
   sprintf(buff, "[%d,%d,%d,d%d]", packet_count, RealData, ImaginaryData, i + 1);
    strcat(long_buff, buff);
  }
  Serial.println(long_buff);
}


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
  //Setting up the RTC Clock
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  
  if (! rtc.initialized()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("Setting new time");
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
     rtc.adjust(DateTime(2020, 10, 17, 19, 40, 0));
  }

//In production, INIT_RTC_ALWAYS should be 0.
//Only turn this to 1 when testing
#if (INIT_RTC_ALWAYS == 1)
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
#endif

  Serial.println("Current Time");
  DateTime now = rtc.now();
  Serial.print(now.year());
  Serial.print("/");
  Serial.print(now.month());
  Serial.print("/");
  Serial.print(now.day());
  Serial.print(" ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  delay(1000);

  randomSeed(analogRead(0));
  Serial.println(F("Peer-peer ranging protocol"));
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
  receiver(0);
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
  Serial.println("Send complete");
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
uint16_t recvd_poll_seq = 0;
uint16_t recvd_resp_seq = 0;
uint16_t recvd_final_seq = 0;

volatile int unlock_waiting = 0;
long loop_counter = 0;
volatile long waiting = 0;
DW1000Time currentDWTime;
uint64_t currentTime;
uint64_t final_sent_time;
uint64_t init_time;
double elapsed_time;
double TIME_UNIT = 1.0 / (128*499.2e6); // seconds


double start_time_us = 0, current_time_us = 0;


void loop() {
  if (RxTimeout == true) {
    RxTimeout = false;
    current_state = STATE_IDLE;
    #if (INITIATOR==0)
      receiver(0);
    #endif
  }
    
  Serial.println("============");
  delay(2000);
  seq++;
  Serial.print("seq: ");
  Serial.println(seq);
  Serial.print("current state: ");
  Serial.println(current_state);
  
  switch(current_state) {

    //For initiator - Send uwb info to the phone that it should turn its wifi on
    //Follows directly from case: "STATE_RECEIVED_WIFI_DATA"
    case STATE_SEND_UWB_DATA: {
      Serial.println("initiator: STATE_SEND_UWB_DATA");
      seq++;
      currentDeviceIndex = 0;
      tx_poll_msg[SRC_IDX] = myDevID; // 3 for initiator
      tx_poll_msg[DST_IDX] = BROADCAST_ID;
      tx_poll_msg[SEQ_IDX] = seq & 0xFF;
      tx_poll_msg[SEQ_IDX + 1] = seq >> 8;

      currentTime = get_time_u64();
      
      generic_send(tx_poll_msg, sizeof(tx_poll_msg), POLL_MSG_POLL_TX_TS_IDX, SEND_DELAY_FIXED);
      
      // reset the state
      current_state = STATE_IDLE;

      while(!sendComplete);
      sendComplete = false;
      
      Serial.println("Sent the UWB data");
      break;
    }

    //For tag (phone) - keep UWB communication open. If data is received, change state to STATE_RECEIVE_UWB_DATA
    case UWB_DATA_EXPECTED: {
      Serial.println("phone: UWB_DATA_EXPECTED");
      if (received) {
        received = false;
        current_state = STATE_RECEIVE_UWB_DATA;
      } else {
        received = false;
        receiver(TYPICAL_RX_TIMEOUT);
      }
      break; 
    }
    
    //For tag (phone) - receive UWB data from the initiator that it should turn its wifi on
    case STATE_RECEIVE_UWB_DATA: {
      Serial.println("phone: STATE_RECEIVE_UWB_DATA");

      received = false;
      show_packet(rx_packet, DW1000.getDataLength());

      Serial.println("rx_packet[DST_IDX]: " + rx_packet[DST_IDX]);
      Serial.println("myDevID: " + myDevID);
      Serial.println("BROADCAST_ID: " + BROADCAST_ID);
      
      //Check that the response is from the tag
      if ((rx_packet[DST_IDX] == myDevID || rx_packet[DST_IDX] == BROADCAST_ID)) {
        Serial.println("the response matches the tag devID: ");
      
        // turn the phone's wifi on

        // turn the phone's wifi off


      }
      // return to original state
      current_state = STATE_IDLE;
      received = false;
      receiver(TYPICAL_RX_TIMEOUT);

      Serial.println("==================");
      break;
    }
    //For initiator - Sends to state poll
    //For tag - Waits for poll message from initiator
    case STATE_IDLE: {
      if (RxTimeout == true) {
        RxTimeout = false;
        #if (INITIATOR==0)
          receiver(0);
        #endif
      }
      if (received) {
        received = false;
        
        thisRange.initialize();
        current_state = STATE_RECEIVE_UWB_DATA;
        #if (DEBUG_PRINT==1)
        Serial.println("******************");
        Serial.println("Going to resp send");
        #endif
        
      }
      #if (INITIATOR==1)
        //Randomly begin the POLL process.
        waiting=0;
        received = false;
        sendComplete = false;
        //Switch to POLL state
        current_state = STATE_SEND_UWB_DATA;
        unlock_waiting = 0;
        // Serial.println("INIT is 1");
        // Serial.println("switching state to STATE POLL");
      #endif
      break;
    }
    //For Initiator - Creates and broadcasts a poll message to the tag
    case STATE_POLL: {
      Serial.println("STATE POLL");
      //Send POLL here
      seq++;
      // Serial.println(seq);
      currentDeviceIndex = 0;
      tx_poll_msg[SRC_IDX] = myDevID;
      tx_poll_msg[DST_IDX] = BROADCAST_ID;
      tx_poll_msg[SEQ_IDX] = seq & 0xFF;
      tx_poll_msg[SEQ_IDX + 1] = seq >> 8;

      currentTime = get_time_u64();
      
      generic_send(tx_poll_msg, sizeof(tx_poll_msg), POLL_MSG_POLL_TX_TS_IDX, SEND_DELAY_FIXED);
      current_state = STATE_RESP_EXPECTED;
      int i = 0;

      while(!sendComplete);

      current_time_us = get_time_us();
      sendComplete = false;

      receiver(TYPICAL_RX_TIMEOUT);
      DW1000.getSystemTimestamp(currentDWTime);
      init_time = currentDWTime.getTimestamp();

      break;
    }
    //For tag - Sends a response to the poll message received from the initiator
    case STATE_RESP_SEND: {
      Serial.println("STATE_RESP_SEND");
      //retrieve sequence number from the poll message
      seq = rx_packet[SEQ_IDX] +  ((uint16_t)rx_packet[SEQ_IDX+1] << 8);
      #if (DEBUG_PRINT==1)  
        Serial.print("Preparing to send response for ");
        Serial.println(seq);
      #endif

      uint64_t PollTxTime_64=0L;
      //get the timestamp of the poll message
      any_msg_get_ts(&rx_packet[POLL_MSG_POLL_TX_TS_IDX], &PollTxTime_64);
      thisRange.PollTxTime = DW1000Time((int64_t)PollTxTime_64);
      DW1000Time rxTS; 
      DW1000.getReceiveTimestamp(rxTS);
      thisRange.PollRxTime = rxTS;

      //Prepare response message
      rx_resp_msg[DST_IDX] = rx_packet[SRC_IDX];
      rx_resp_msg[SRC_IDX] = myDevID;
      
      //Send response
      generic_send(rx_resp_msg, sizeof(rx_resp_msg), POLL_MSG_POLL_TX_TS_IDX, SEND_DELAY_FIXED);

      while(!sendComplete); //Wait for the send to complete
      
      sendComplete = false;
      DW1000Time txTS; 
      DW1000.getTransmitTimestamp(txTS);
      thisRange.RespTxTime = txTS;
      receiver(60);
      current_state = STATE_FINAL_EXPECTED;
      break;
    }

    //For initiator - Waits for a response from the tag
    case STATE_RESP_EXPECTED: {
      Serial.println("STATE_RESP_EXPECTED");
      if (received) {
        received = false;
        show_packet(rx_packet, DW1000.getDataLength());
        //Check that the response is from the tag
        if ((rx_packet[DST_IDX] == myDevID || rx_packet[DST_IDX] == BROADCAST_ID)) {
          recvd_resp_seq = rx_packet[SEQ_IDX] +  ((uint16_t)rx_packet[SEQ_IDX+1] << 8);
          deviceRespTs[currentDeviceIndex].deviceID = rx_packet[SRC_IDX];
          DW1000Time rxTS;
          DW1000.getReceiveTimestamp(rxTS);
          deviceRespTs[currentDeviceIndex].respRxTime = rxTS.getTimestamp();
          current_state = STATE_FINAL_SEND;
                
        } else {
          received = false;
          receiver(TYPICAL_RX_TIMEOUT);
        }
      }

      if (unlock_waiting == 1)
      {
        waiting++;
        if (waiting > 50000)
        {
          waiting=0;
          current_state = STATE_FINAL_SEND;
        }
        if (timeout_triggered[1] == true) {
          current_state = STATE_FINAL_SEND;
        }
      }
      break;
    }
    //For initiator - Sends a final message to the tag
    case STATE_FINAL_SEND: {
      Serial.println("STATE_FINAL_SEND");
      //Create final message
      tx_final_msg[SRC_IDX] = myDevID;
      tx_final_msg[DST_IDX] = BROADCAST_ID;
      tx_final_msg[SEQ_IDX] = recvd_resp_seq & 0xFF;
      tx_final_msg[SEQ_IDX + 1] = recvd_resp_seq >> 8;
      
      currentTime = get_time_u64();
      int i=0;

      tx_final_msg[FINAL_MSG_RESP_RX_TS_IDX + (i*FINAL_MSG_ONE_RESP_ENTRY)] = deviceRespTs[i].deviceID;
      any_msg_set_ts(&tx_final_msg[FINAL_MSG_RESP_RX_TS_IDX + 1 + (i*FINAL_MSG_ONE_RESP_ENTRY)], deviceRespTs[i].respRxTime);

      generic_send(tx_final_msg, MAX_FINAL_LEN, FINAL_MSG_FINAL_TX_TS_IDX, SEND_DELAY_FIXED);
      while(!sendComplete); //Wait for the send to complete
      
      sendComplete = false;
      DW1000.getSystemTimestamp(currentDWTime);
      final_sent_time = currentDWTime.getTimestamp();

      current_state = STATE_ACK_EXPECTED;
      receiver(TYPICAL_RX_TIMEOUT);
      break;
    }
    //Doesn't do anything but leads to occaisonal wrong distance measurements whn removed
    case STATE_ACK_EXPECTED: {
       //Serial.println("State: ACK EXPECTED");
      if (received) {
        received = false;
        if (rx_packet[0] == TWR_DONE_TYPE) {
           //Serial.println("ACK: receive a TWR DONE packet!");
          recvd_resp_seq = rx_packet[SEQ_IDX] +  ((uint16_t)rx_packet[SEQ_IDX+1] << 8);
          if(recvd_resp_seq == seq){
            //Serial.println("Recieved ACK!");
            current_state = STATE_POLL;
            break;
          }
        }
      }

      DW1000.getSystemTimestamp(currentDWTime);
      currentTime = currentDWTime.getTimestamp();
      elapsed_time = (currentTime - final_sent_time) * TIME_UNIT * 1000;
      if(elapsed_time >= TYPICAL_RX_TIMEOUT){
          current_state = STATE_POLL;
          break;
      }
      start_time_us = get_time_us();
      break;
    }
    /* Dont delete, TBC*/
    case STATE_FINAL_EXPECTED: {
      Serial.println("STATE_FINAL_EXPECTED");
      Serial.println("RECEIVED: " + String(received));
      if(received) {
        received = false;
        Serial.println("rx_packet[DST_IDX]: " + String(rx_packet[DST_IDX]));
        Serial.println("myDevID: " + String(myDevID));
        Serial.println("BROADCAST_ID: " + String(BROADCAST_ID));


        if (rx_packet[DST_IDX]==myDevID || rx_packet[DST_IDX] == BROADCAST_ID) {
          DW1000Time rxTS;
          DW1000.getReceiveTimestamp(rxTS);
          thisRange.FinalRxTime = rxTS;
          int i = 0;
          uint64_t RespRxTime_64=0L;
          any_msg_get_ts(&rx_packet[FINAL_MSG_RESP_RX_TS_IDX + (i*FINAL_MSG_ONE_RESP_ENTRY)+1], &RespRxTime_64);
          thisRange.RespRxTime = DW1000Time((int64_t)RespRxTime_64);

          uint64_t FinalTxTime_64=0L;
          any_msg_get_ts(&rx_packet[FINAL_MSG_FINAL_TX_TS_IDX], &FinalTxTime_64);
          seq = rx_packet[SEQ_IDX];
          receiver(0); //Enable the receiver quickly to allow the next POLL to work
          thisRange.FinalTxTime = DW1000Time((int64_t)FinalTxTime_64);
          //thisRange.printAll();
          //Serial.println("-------");
          Serial.print("Distance is: ");
          Serial.println(thisRange.calculateRange());
          int dist = thisRange.calculateRange();
          //thisRange.printAll();
          current_state = STATE_IDLE;
        }
      }
      break;
    }
    //*/
    case STATE_OBLIVION: {
      //Do nothing!
    }
  }
  if(DEBUG_CIR==1){
    print_CIR();
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

//Timer Functions
void setTimerFrequency(int frequencyHz) {
  int compareValue = (CPU_HZ / (TIMER_PRESCALER_DIV * frequencyHz)) - 1;
  
  TcCount16* TC = (TcCount16*) TC3;
  // Make sure the count is in a proportional position to where it was
  // to prevent any jitter or disconnect when changing the compare value.
  TC->COUNT.reg = map(TC->COUNT.reg, 0, TC->CC[0].reg, 0, compareValue);
  TC->CC[0].reg = compareValue;
  //Serial.println(TC->COUNT.reg);
  //Serial.println(TC->CC[0].reg);
  while (TC->STATUS.bit.SYNCBUSY == 1);
}

void startTimer(int frequencyHz) {
  
  REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3) ;
  
  while ( GCLK->STATUS.bit.SYNCBUSY == 1 ); // wait for sync

  TcCount16* TC = (TcCount16*) TC3;
  
  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use the 16-bit timer
  TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use match mode so that the timer counter resets when the count matches the compare register
  TC->CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Set prescaler to 1024
  TC->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  setTimerFrequency(frequencyHz);

  // Enable the compare interrupt
  TC->INTENSET.reg = 0;
  TC->INTENSET.bit.MC0 = 1;

  
    NVIC_SetPriority(TC3_IRQn, 3);
    NVIC_EnableIRQ(TC3_IRQn);  
    

  TC->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
}


void TC3_Handler() 
{
  DW1000Time currectTS;
  uint64_t currentUWBTime;
  for(int i=0;i<MAX_TIMEOUTS;i++) {
    if (timeout_established[i]) {
      DW1000.getSystemTimestamp(currectTS);
      currentUWBTime = currectTS.getTimestamp();
      break; //Any timeout if established will populate the currentUWBTime
    }
  }
  for(int i=0;i<MAX_TIMEOUTS;i++) {
    if (timeout_established[i]) {
      if(currentUWBTime > timeout_time[i]) {
        timeout_established[i] = false;
        timeout_time[i] = INFINITE_TIME;
        timeout_overflow[i] = false;
        timeout_triggered[i] = true;
      } else if (timeout_overflow[i] == true && currentUWBTime > (timeout_time[i] - 2^40)) {
        timeout_established[i] = false;
        timeout_time[i] = INFINITE_TIME;
        timeout_overflow[i] = false;
        timeout_triggered[i] = true;
      }
    }
  }
}

void set_timeout(int whichTO, uint32_t delayTime) {
  DW1000Time currectTS;
  uint64_t currentUWBTime;
  DW1000.getSystemTimestamp(currectTS);
  currentUWBTime = currectTS.getTimestamp();
  DW1000Time deltaTime = DW1000Time(delayTime, DW1000Time::MILLISECONDS);
  timeout_time[whichTO] = (currectTS + deltaTime).getTimestamp();
  if (timeout_time[whichTO] > 2^40) {
    timeout_overflow[whichTO] = true;
  } else {
    timeout_overflow[whichTO] = false;
  }
}

double get_time_us(){
    DW1000.getSystemTimestamp(currentDWTime);
    currentTime = currentDWTime.getTimestamp();
    return currentTime * TIME_UNIT * 1e6;
}

uint64_t get_time_u64(){
    DW1000.getSystemTimestamp(currentDWTime);
    return currentDWTime.getTimestamp();
}


//Utility functions
void dateTime(uint16_t* date, uint16_t* time_) {
  DateTime now = rtc.now();
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time_ = FAT_TIME(now.hour(), now.minute(), now.second());
  printDateTime();
}

void printDateTime()
{
  DateTime now = rtc.now();
  Serial.print(now.year());
  Serial.print("/");
  Serial.print(now.month());
  Serial.print("/");
  Serial.print(now.day());
  Serial.print(" ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());
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
