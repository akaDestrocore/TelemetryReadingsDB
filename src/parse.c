#include "parse.h"

/**
 * @brief  Creates a new database header in the file. 
 * @param fd: [in] File descriptor
 * @param  ppHeaderOut: [in] pointer to a pointer that will be set to the newly created header
 * @return file descriptor
 */
int parse_createDbHeader(int fd, Parse_DbHeader_t **ppHeaderOut) {
    Parse_DbHeader_t *pHeader = NULL;

    pHeader = calloc(1, sizeof(Parse_DbHeader_t));
    if (NULL == pHeader)
    {
        printf("Malloc failed to create db header\r\n");
        return STATUS_ERROR;
    }

    pHeader->version = 0x1;
    pHeader->count = 0;
    pHeader->magic = HEADER_MAGIC;
    pHeader->filesize = sizeof(Parse_DbHeader_t);

    *ppHeaderOut = pHeader;

    return STATUS_SUCCESS;
}

/**
 * @brief  Validates that the file is a valid database. 
 * @param fd: [in] File descriptor
 * @param  ppHeaderOut: [in] pointer to a pointer that will be set to the newly created header
 * @return 0 if success, -1 otherwise
 */
int parse_validateDbHeader(int fd, Parse_DbHeader_t **ppHeaderOut)
{
    Parse_DbHeader_t *pHeader = NULL;
    struct stat dbstat = {0};

    if (fd < 0)
    {
        printf("Got a bad FD from the user\r\n");
        return STATUS_ERROR;
    }

    pHeader = calloc(1, sizeof(Parse_DbHeader_t));
    if (NULL == pHeader)
    {
        printf("Malloc failed create a db header\r\n");
        return STATUS_ERROR;
    }

    if (read(fd, pHeader, sizeof(Parse_DbHeader_t)) != sizeof(Parse_DbHeader_t))
    {
    perror("read");
        free(pHeader);
        return STATUS_ERROR;
    }

    pHeader->version = ntohs(pHeader->version);
    pHeader->count = ntohs(pHeader->count);
    pHeader->magic = ntohl(pHeader->magic);
    pHeader->filesize = ntohl(pHeader->filesize);

    if (HEADER_MAGIC != pHeader->magic)
    {
        printf("Improper header magic\r\n");
        free(pHeader);
        return STATUS_ERROR;
    }

    if (1 != pHeader->version)
    {
        printf("Improper header version\r\n");
        free(pHeader);
        return STATUS_ERROR;
    }

    fstat(fd, &dbstat);
    if (pHeader->filesize != dbstat.st_size)
    {
        printf("Corrupted database\r\n");
        free(pHeader);
        return STATUS_ERROR;
    }

    *ppHeaderOut = pHeader;

    return STATUS_SUCCESS;
}

/**
 * @brief  Parse a sensor string and add it to the database
 * @param pDbhdr Pointer to the database header
 * @param pSensors Pointer to the sensors array
 * @param pAddString String containing the sensor data in the following format:
 *                  sensor_id,sensor_type,i2c_addr,timestamp,reading_value
 * @return 0 if success, -1 otherwise
 */
int parse_addSensor(Parse_DbHeader_t *pDbhdr, Parse_Sensor_t *pSensors, char *pAddString)
{
    char *pSensorId = NULL;
    char *pSensorType = NULL;
    char *pI2cAddrStr = NULL;
    char *pTimestampStr = NULL;
    char *pReadingStr = NULL;

    printf("%s\n", pAddString);

    // Parse format: sensor_id,sensor_type,i2c_addr,timestamp,reading_value
    // Example: "BNO055_01,BNO055,0x28,1701432000,25.5"

    pSensorId = strtok(pAddString, ",");
    pSensorType = strtok(NULL, ",");
    pI2cAddrStr = strtok(NULL, ",");
    pTimestampStr = strtok(NULL, ",");
    pReadingStr = strtok(NULL, ",");

    printf("%s %s %s %s %s\n", pSensorId, pSensorType, pI2cAddrStr, 
            pTimestampStr, pReadingStr);

    memset(&pSensors[pDbhdr->count - 1], 0, sizeof(Parse_Sensor_t));

    strncpy(pSensors[pDbhdr->count - 1].sensorId, pSensorId, 
            sizeof(pSensors[pDbhdr->count - 1].sensorId) - 1);
    strncpy(pSensors[pDbhdr->count - 1].sensorType, pSensorType, 
            sizeof(pSensors[pDbhdr->count - 1].sensorType) - 1);

    // Parse I2C address if any
    pSensors[pDbhdr->count - 1].i2cAddr = (unsigned char)strtol(pI2cAddrStr, NULL, 0);

    pSensors[pDbhdr->count - 1].timestamp = (time_t)atol(pTimestampStr);

    pSensors[pDbhdr->count - 1].readingValue = atof(pReadingStr);

    pSensors[pDbhdr->count - 1].flags = SENSOR_FLAG_ACTIVE | SENSOR_FLAG_CALIBRATED;
    strcpy(pSensors[pDbhdr->count - 1].location, "Unknown Location");
    pSensors[pDbhdr->count - 1].minThreshold = -100.0;
    pSensors[pDbhdr->count - 1].maxThreshold = 100.0;

    return STATUS_SUCCESS;
}


/**
 * @brief  Reads sensor data in the database
 * @param fd: [in] File descriptor
 * @param  pDbhdr: [in] Pointer to database header
 * @param ppSensorsOut: [out] Pointer to array of sensors
 * @return 0 if success, -1 otherwise
 */
int parse_readSensors(int fd, Parse_DbHeader_t *pDbhdr, Parse_Sensor_t **ppSensorsOut)
{
    int count = 0;
    int i = 0;
    Parse_Sensor_t *pSensors = NULL;
    unsigned int temp = 0;

    if (fd < 0)
    {
        printf("Got a bad FD from the user\r\n");
        return STATUS_ERROR;
    }

    count = pDbhdr->count;

    pSensors = calloc(count, sizeof(Parse_Sensor_t));
    if (NULL == pSensors)
    {
        printf("Malloc failed\r\n");
        return STATUS_ERROR;
    }

    read(fd, pSensors, count * sizeof(Parse_Sensor_t));

    for (i = 0; i < count; i++)
    {
        pSensors[i].timestamp = ntohl(pSensors[i].timestamp);

        temp = ntohl(*(unsigned int*)&pSensors[i].readingValue);
        pSensors[i].readingValue = *(float*)&temp;

        temp = ntohl(*(unsigned int*)&pSensors[i].minThreshold);
        pSensors[i].minThreshold = *(float*)&temp;

        temp = ntohl(*(unsigned int*)&pSensors[i].maxThreshold);
        pSensors[i].maxThreshold = *(float*)&temp;
    }

    *ppSensorsOut = pSensors;

    return STATUS_SUCCESS;
}


/**
 * @brief Output new database file to disk
 * @param fd: [in] File descriptor
 * @param  pDbhdr: [in] Pointer to database header
 * @param pSensors: [in] Pointer to sensor array
 * @return 0 if success, -1 otherwise
 */
void parse_outputFile(int fd, Parse_DbHeader_t *pDbhdr, Parse_Sensor_t *pSensors)
{
    int realCount = 0;
    int i = 0;
    Parse_Sensor_t tempSensor;
    unsigned int temp = 0;

    if (fd < 0)
    {
        printf("Got a bad FD from the user\r\n");
        return;
    }

    realCount = pDbhdr->count;

    pDbhdr->magic = htonl(pDbhdr->magic);
    pDbhdr->filesize = htonl(sizeof(Parse_DbHeader_t) + (sizeof(Parse_Sensor_t) * realCount));
    pDbhdr->count = htons(pDbhdr->count);
    pDbhdr->version = htons(pDbhdr->version);

    lseek(fd, 0, SEEK_SET);

    write(fd, pDbhdr, sizeof(Parse_DbHeader_t));

    for (i = 0; i < realCount; i++)
    {
        tempSensor = pSensors[i];
        tempSensor.timestamp = htonl(tempSensor.timestamp);
        
        temp = htonl(*(unsigned int*)&tempSensor.readingValue);
        tempSensor.readingValue = *(float*)&temp;

        temp = htonl(*(unsigned int*)&tempSensor.minThreshold);
        tempSensor.minThreshold = *(float*)&temp;

        temp = htonl(*(unsigned int*)&tempSensor.maxThreshold);
        tempSensor.maxThreshold = *(float*)&temp;

        write(fd, &tempSensor, sizeof(Parse_Sensor_t));
    }

    ftruncate(fd, sizeof(Parse_DbHeader_t) + (sizeof(Parse_Sensor_t) * realCount));
}
