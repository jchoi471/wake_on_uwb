#include "ProtocolConsts.h"
#include "Adafruit_LSM9DS1.h"
#include <LSM6DSOSensor.h>

extern int32_t imu_buffer[IMU_BUFFER_SIZE][7];
extern LSM6DSOSensor AccGyr;

double get_time_us(){
    DW1000Time currentDWTime;
    DW1000.getSystemTimestamp(currentDWTime);
    uint64_t tmpTime_u64 = currentDWTime.getTimestamp();
    return tmpTime_u64 * TIME_UNIT * 1e6;
}

double get_elapsed_time_us(double startTime){
	double currentTime = get_time_us();
    return (startTime < currentTime) ? currentTime - startTime : currentTime + 17207401.0256 - startTime;
}

void print_uint64(uint64_t a){
	int r;
	int out[20]; int j = 0;
	while(a > 0){
		r = a % 10;
		a = (a - r) / 10;
		out[j++] = r;
	}
	for(int i = j - 1; i>= 0; i--){
		Serial.print(out[i]);
	}
}

int32_t scale_imu_measurement(double a) {
	return (int32_t)((max(min(a, 1000), -1000))*10000);
}

void add_imu_to_buffer(int32_t imu_buffer[][7], int &idx, int max_samples_in_buf){
	int32_t accelerometer[3];
    int32_t gyroscope[3];
    AccGyr.Get_X_Axes(accelerometer);
    AccGyr.Get_G_Axes(gyroscope);

	if (idx >= max_samples_in_buf) {
		idx = 0;
	}

	// The input of scale_imu_measurement must be in m/s^2 and dps
	if (idx < max_samples_in_buf){
		imu_buffer[idx][0] = scale_imu_measurement(((float)(accelerometer[0]))/1000*10);
		imu_buffer[idx][1] = scale_imu_measurement(((float)(accelerometer[1]))/1000*10);
		imu_buffer[idx][2] = scale_imu_measurement(((float)(accelerometer[2]))/1000*10);
		imu_buffer[idx][3] = scale_imu_measurement(((float)(gyroscope[0]))/1000);
		imu_buffer[idx][4] = scale_imu_measurement(((float)(gyroscope[1]))/1000);
		imu_buffer[idx][5] = scale_imu_measurement(((float)(gyroscope[2]))/1000);
		imu_buffer[idx][6] = (int32_t)(get_time_us()); // 0 ~ 18000
		Serial.print("imu meansurements:");
		Serial.print(idx);
		Serial.print(",");
		Serial.print(imu_buffer[idx][0]);
		Serial.print(",");
		Serial.print(imu_buffer[idx][1]);
		Serial.print(",");
		Serial.print(imu_buffer[idx][2]);
		Serial.print(",");
		Serial.print(imu_buffer[idx][3]);
		Serial.print(",");
		Serial.print(imu_buffer[idx][4]);
		Serial.print(",");
		Serial.println(imu_buffer[idx][5]);
		idx += 1;
	}
}


// buf should start from n_imu_idx
void embed_imus(uint8_t buf[], int32_t imu_measurements[][7], int n_imus){
	int n = (n_imus > MAX_IMU_EACH_PACKET_TWR_LOGIC) ? MAX_IMU_EACH_PACKET_TWR_LOGIC:n_imus;
	buf[0] = (uint8_t) (n & 0xFF);
	// Serial.print("n_imu is: ");
	// Serial.println(n_imus);

	int n_bytes = n_imus  * (3*ACC_MSG_LEN + 3*GYRO_MSG_LEN + IMU_TIME_MSG_LEN);
	// Serial.print("n_bytes is: ");
	// Serial.println(n_bytes);
	memcpy(&buf[1], imu_measurements, n_bytes);
}

void embed_imus(uint8_t buf[], int32_t imu_measurements[][7], int from_idx, 
	int to_idx){
	int n_imus = to_idx - from_idx;
	buf[0] = (uint8_t) (n_imus & 0xFF);
	// Serial.print("n_imu is: ");
	// Serial.println(n_imus);

	int n_bytes = n_imus  * (3*ACC_MSG_LEN + 3*GYRO_MSG_LEN + IMU_TIME_MSG_LEN);
	// Serial.print("n_bytes is: ");
	// Serial.println(n_bytes);
	// int offset = from_idx * 6 * ACC_MSG_LEN;
	memcpy(&buf[1], &imu_measurements[from_idx], n_bytes);
}
