#ifndef _PARSE_H
#define _PARSE_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "common.h"
#include <sys/stat.h>
#include <arpa/inet.h>


#define HEADER_MAGIC 0x53454E53

typedef enum {
    SENSOR_FLAG_ACTIVE      = 0x01,
    SENSOR_FLAG_ERROR       = 0x02,
    SENSOR_FLAG_CALIBRATED  = 0x04
} Sensor_Flag_t;

typedef struct
{
  unsigned int magic;
  unsigned short version;
  unsigned short count;
  unsigned int filesize;
} Parse_DbHeader_t;

typedef struct
{
  char sensorId[64];
  char sensorType[32];
  unsigned char i2cAddr;
  time_t timestamp;
  float readingValue;
  unsigned char flags;
  char location[128];
  float minThreshold;
  float maxThreshold;
} Parse_Sensor_t;

// create database header
int parse_createDbHeader(int fd, Parse_DbHeader_t **ppHeaderOut);
// validate if header is valid
int parse_validateDbHeader(int fd, Parse_DbHeader_t **ppHeaderOut);
// add new sensor to database
int parse_addSensor(Parse_DbHeader_t *pDbhdr, Parse_Sensor_t *pSensors, char *pAddString);
// read sensors in database
int parse_readSensors(int fd, Parse_DbHeader_t *pDbhdr, Parse_Sensor_t **ppSensorsOut);
// write database to file
void parse_outputFile(int fd, Parse_DbHeader_t *pDbhdr, Parse_Sensor_t *pSensors);

#endif /* _PARSE_H */