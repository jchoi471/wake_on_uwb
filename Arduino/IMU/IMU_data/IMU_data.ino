/*
 * Copyright (c) 2021 by Yifeng Cao <ycao361@gatech.edu>
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
 * @file DWM1000-TWR.ino
 * 
 *  
 */

#include <SPI.h>
#include <math.h>
#include <DW1000.h>
#include <DW1000Constants.h>
#include "genericFunctions.h"
#include "Adafruit_LSM9DS1.h"
#include <SdFat.h>
#include <time.h>
#include<TimeLib.h>
#include<Wire.h>
// #include <Adafruit_ISM330DHCX.h>
// #include <Adafruit_LIS3MDL.h>

TwoWire dev_i2c(&sercom3, SDA, SCL);  
LSM6DSOSensor AccGyr(&dev_i2c);

// Adafruit_LIS3MDL lis3mdl;
// Adafruit_ISM330DHCX ism330dhcx;

// PIN Macro
#define VBATPIN A2

//Timer for implementing timeouts
#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1024

//Other Macros
#define DEBUG_PRINT 0


#if(OUR_UWB_FEATHER==1)
const uint8_t PIN_RST = 9; // reset pin
const uint8_t PIN_IRQ = 17; // irq pin
const uint8_t PIN_SS = 19; // spi select pin
#endif

#if(AUS_UWB_FEATHER==1)
const uint8_t PIN_RST = 2; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
const uint8_t PIN_SS = 4; // spi select pin
#endif

int dst_index = 2;
int imu_start_idx = 0;

long randNumber;


// uint8_t myAcc[1000];
int32_t imu_buffer[IMU_BUFFER_SIZE][7] = {0};
int n_imu_samples = 0;

//Time
double prev_imu_time_us = 0;
double elapsed_imu_time_us = 0;


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
  imu_setup();
  Serial.print("Waiting...");
  delay(5000);
  Serial.print("Should see this...");
  randomSeed(analogRead(0));
  Serial.println("Free memory: ");
  Serial.println(freeMemory());
}



void loop() {
  elapsed_imu_time_us = get_elapsed_time_us(prev_imu_time_us);
   if (elapsed_imu_time_us > 1e6 / imu_rate) {
    
     add_imu_to_buffer(imu_buffer, n_imu_samples,MAX_IMU_SAMPLES_IN_BUFFER_TWRCIR);
     // Serial.print("n_imu_samples=");
     // Serial.println(n_imu_samples);
     prev_imu_time_us = get_time_us();
   }
}

void imu_setup(){
  dev_i2c.begin();
  
  // Initialize I2C bus.
  LSM6DSOSensor AccGyr(&dev_i2c);
  AccGyr.begin();

  AccGyr.Set_X_ODR(416); // Acc 416Hz
  AccGyr.Set_X_FS(4); // Acc range: 4g  
  AccGyr.Set_G_ODR(416); // Gyro 416Hz
  AccGyr.Set_G_FS(500); // Gyro range: 500 dps
  AccGyr.Enable_X(); 
  AccGyr.Enable_G();
}

//Utility functions




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
