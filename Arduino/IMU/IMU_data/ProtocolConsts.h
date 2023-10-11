#ifndef PROTOCOL_CONSTS_H
#define PROTOCOL_CONSTS_H

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

#define BROADCAST_ID 0xFF

/* Return code */
#define NO_ERROR 0x00
#define ERROR_GENERAL 0x01
#define ERROR_INVALID_DISTANCE 0x02
#define RET_INSUFFICIENT_DISTANCE_MEASUREMENTS 0x10
#define RET_PROXIMITY_CHECK_FAIL 0x11
#define RET_PROXIMITY_CHECK_SUCCESS 0x12

/* Define the format of the packet. */
/* Handshaking request. */
#define MSG_TYPE_IDX 0
#define SRC_IDX 1
#define DST_IDX 2
#define SEQ_IDX 3
#define ACK_IDX 5

/* Handshaking reply + handshaking PCACK. */
#define SESSION_KEY_IDX 7

/* Handshaking RINGACK. */
#define HS_RINGACK_STATUS_IDX 7

/* Pattern info*/
#define PAT_N_ALL_IMU_COUNT_IDX 7
#define PAT_FROM_IMU_IDX 9
#define PAT_TO_IMU_IDX 11
#define PAT_INFO_COMPLETE_BIT_IDX 13
#define PAT_N_IMU_IDX 14
#define PAT_IMU_MEASUREMENTS_IDX 15


#define PREV_RX_TS_IDX 7
#define CURRENT_TX_TS_IDX 12
#define RESERVED_MASK_IDX 17

#define SKETCH_N_IMU_IDX 48


/* Length of each type of message. */
#define SHORT_MSG_LEN 2
#define IMU_MSG_LEN 4
#define ACC_MSG_LEN 4
#define GYRO_MSG_LEN 4
#define IMU_TIME_MSG_LEN 4 
#define BEACON_MSG_LEN 512
#define TS_LEN 5
#define SESSION_KEY_LEN 32

#define RESP_DURATION 2000

/* Message types.*/
#define BEACON_TYPE 0x01
#define HS_REQUEST_TYPE 0x02
#define HS_REPLY_TYPE 0x03
#define HS_PCACK_TYPE 0x04
#define HS_RINGACK_TYPE 0x05
#define TWR_BEACON_TYPE 0x06
#define PAT_REQUEST_TYPE 0x07
#define PAT_REPLY_TYPE 0x08
#define PAT_SHORTBEACON_TYPE 0x09
#define PAT_INFO_START_TYPE 0x0A
#define PAT_INFO_ACK_TYPE 0x0B
#define PAT_INFO_TYPE 0x0C
#define PAT_INFO_IMU_ACK_TYPE 0x0D

/*Output message types. */
#define OUTPUT_TWR_CIR_TYPE 0x01
#define OUTPUT_TWR_ONLY_TYPE 0x02
#define OUTPUT_CIR_VERIFIER_TYPE 0x03
#define OUTPUT_IMU_ONLY_TYPE 0x04
#define OUTPUT_TWR_IMU_TYPE 0x05

/* Return code */
#define NO_ERROR 0x00
#define ERROR_GENERAL 0x01
#define ERROR_INVALID_DISTANCE 0x02
#define RET_INSUFFICIENT_DISTANCE_MEASUREMENTS 0x10
#define RET_PROXIMITY_CHECK_FAIL 0x11
#define RET_PROXIMITY_CHECK_SUCCESS 0x12
#define RET_IMU_INFO_COMPLETE 0x13
#define RET_IMU_INFO_NOTCOMPLETE 0x14
#define RET_RECEIVE_IMU_ACK 0x15
#define RET_RECEIVE_WRONG_ACK 0x16


#define MAX_POLL_LEN 12
#define MAX_RESP_LEN 8
#define MAX_FINAL_LEN 44
#define MAX_REPORT_LEN 10
// (56 if imu not included)
#define PROBE_LEN 125//125    //4 byte info + 5 byte transmission time + 6*TRXNUM + (5,8,10)*9*2 [IMU] + 2

typedef enum {STATE_IDLE, STATE_SEND, STATE_RECEIVE,
	/*Login device, handshaking. */
	STATE_HS_SEND_REQUEST, STATE_HS_REPLY_EXPECTED, STATE_HS_SEND_PCACK, STATE_HS_RINGACK_EXPECTED,
	/*Ring, handshaking. */
	STATE_HS_REQUEST_EXPECTED, STATE_HS_SEND_REPLY, STATE_HS_PCACK_EXPECTED, STATE_HS_SEND_RINGACK,
    /*Ring/Login device, two way ranging. */
    STATE_TWR_SEND, STATE_TWR_RECEIVE,
    /*Login device, pattern recognition*/
	STATE_PAT_SEND_REQUEST, STATE_PAT_REPLY_EXPECTED, STATE_PAT_SEND_SHORTBEACON,
	STATE_PAT_INFO_EXPECTED,
    /*Ring, pattern recognition*/
    STATE_PAT_SEND_REPLY, STATE_PAT_SHORTBEACON_EXPECTED, STATE_PAT_SEND_INFO_START,
    STATE_PAT_INFO_ACK_EXPECTED, STATE_PAT_SEND_INFO, STATE_PAT_INFO_IMU_ACK_EXPECTED,
    STATE_ALL_COMPLETE
} STATES;

typedef enum send_modes{SEND_IMMEDIATE, SEND_DELAY_FIXED, SEND_DELAY_BOARDID, SEND_SHORT_DELAY_BOARDID, SEND_LONG_DELAY_BOARDID, SEND_DELAY_GIVEN} SEND_MODES;


#define DEBUG_FLAG 0
#define TIME_UNIT 1/128/499.2e6

#define OUR_UWB_FEATHER 1
#define AUS_UWB_FEATHER 0


#define YIFENG_TEST 0


/* Configurations */
#define FIXED_DELAY 6000
#define WAIT_BEFORE_POLL 2
#define TEN_SECOND_TIMEOUT_US 10e6
#define FIVE_SECOND_TIMEOUT_US 5e6
#define ONE_SECOND_TIMEOUT_US 1e6
#define STANDARD_TIMEOUT_US 60000
#define ONE_MSEC 0x3CF00
#define MAX_SEQ_NUMBER 65535
#define IMU_BUFFER_SIZE 100
#define MAX_IMU_EACH_PACKET_TWR_LOGIC 16
#define MAX_IMU_EACH_PACKET_TYPICAL 19
#define MAX_IMU_SAMPLES 100
#define MAX_CIR_VERIFIER 3

#define MAX_IMU_SAMPLES_IN_BUFFER_TWRCIR 16
#define MAX_IMU_SAMPLES_IN_BUFFER_APP MAX_IMU_SAMPLES

const uint16_t deca_timeout_us = 40000;
const int response_expect_timeout_us = 60000;
const int my_index = 1;
const int imu_rate = 100;
const byte my_session_key[32] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
};


#endif
