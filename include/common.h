#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>

#define     PORT            8080
#define     BUFF_SIZE       4096
#define     PROTOCOL_VER    100

typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERROR = -1
} Status_e;

typedef enum {
    MSG_HANDSHAKE_REQ,
    MSG_HANDSHAKE_RESP,
    MSG_SENSOR_LIST_REQ,
    MSG_SENSOR_LIST_RESP,
    MSG_SENSOR_ADD_REQ,
    MSG_SENSOR_ADD_RESP,
    MSG_SENSOR_DEL_REQ,
    MSG_SENSOR_DEL_RESP,
    MSG_ERROR
} DbProtocol_e;

typedef struct {
    DbProtocol_e type;
    uint16_t len;   // number of subelements
} DbProtocolHdr_t;

typedef struct {
    uint16_t version;
} DbProtocolVer_Req_t;

typedef struct {
    uint16_t version;
} DbProtocolVer_Resp_t;

typedef struct {
    uint8_t data[1024];
} DbProtocol_SensorAddReq_t;

typedef struct {
    char sensorId[64];
    char sensorType[32];
    unsigned char i2cAddr;
    uint32_t timestamp;
    float readingValue;
    unsigned char flags;
    char location[128];
    float minThreshold;
    float maxThreshold;
} DbProtocol_SensorListResp_t;

#endif /* _COMMON_H */