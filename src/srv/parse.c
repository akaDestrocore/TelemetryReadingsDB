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
 * @return STATUS_SUCCESS or STATUS_ERROR
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
 * @param ppSensors Pointer to pointer of sensors array
 * @param pAddString String containing the sensor data in the following format:
 *                  sensor_id,sensor_type,i2c_addr,timestamp,reading_value
 * @return STATUS_SUCCESS or STATUS_ERROR
 */
int parse_addSensor(Parse_DbHeader_t *pDbhdr, Parse_Sensor_t **ppSensors, char *pAddString)
{
    char *pSensorId = NULL;
    char *pSensorType = NULL;
    char *pI2cAddrStr = NULL;
    char *pTimestampStr = NULL;
    char *pReadingStr = NULL;
    Parse_Sensor_t *pNewSensors = NULL;

    printf("%s\n", pAddString);

    // Parse format: sensor_id,sensor_type,i2c_addr,timestamp,reading_value
    // Example: "BNO055_01,BNO055,0x28,1701432000,25.5"

    pSensorId = strtok(pAddString, ",");
    pSensorType = strtok(NULL, ",");
    pI2cAddrStr = strtok(NULL, ",");
    pTimestampStr = strtok(NULL, ",");
    pReadingStr = strtok(NULL, ",");

    if (pSensorId == NULL || pSensorType == NULL || pI2cAddrStr == NULL ||
        pTimestampStr == NULL || pReadingStr == NULL) {
        printf("Invalid format for sensor data\r\n");
        return STATUS_ERROR;
    }

    printf("%s %s %s %s %s\n", pSensorId, pSensorType, pI2cAddrStr, 
            pTimestampStr, pReadingStr);

    // Increment count first
    pDbhdr->count++;

    // Reallocate the sensors array to fit the new sensor
    pNewSensors = realloc(*ppSensors, pDbhdr->count * sizeof(Parse_Sensor_t));
    if (NULL == pNewSensors && pDbhdr->count > 0)
    {
        printf("Realloc failed to expand sensors array\r\n");
        pDbhdr->count--;
        return STATUS_ERROR;
    }

    *ppSensors = pNewSensors;

    // Initialize the new sensor entry
    memset(&(*ppSensors)[pDbhdr->count - 1], 0, sizeof(Parse_Sensor_t));

    strncpy((*ppSensors)[pDbhdr->count - 1].sensorId, pSensorId, 
            sizeof((*ppSensors)[pDbhdr->count - 1].sensorId) - 1);
    strncpy((*ppSensors)[pDbhdr->count - 1].sensorType, pSensorType, 
            sizeof((*ppSensors)[pDbhdr->count - 1].sensorType) - 1);

    // Parse I2C address if any
    (*ppSensors)[pDbhdr->count - 1].i2cAddr = (unsigned char)strtol(pI2cAddrStr, NULL, 0);

    (*ppSensors)[pDbhdr->count - 1].timestamp = (time_t)atol(pTimestampStr);

    (*ppSensors)[pDbhdr->count - 1].readingValue = atof(pReadingStr);

    (*ppSensors)[pDbhdr->count - 1].flags = SENSOR_FLAG_ACTIVE | SENSOR_FLAG_CALIBRATED;
    strcpy((*ppSensors)[pDbhdr->count - 1].location, "Unknown Location");
    (*ppSensors)[pDbhdr->count - 1].minThreshold = -100.0;
    (*ppSensors)[pDbhdr->count - 1].maxThreshold = 100.0;

    // Update filesize
    pDbhdr->filesize = sizeof(Parse_DbHeader_t) + (sizeof(Parse_Sensor_t) * pDbhdr->count);

    return STATUS_SUCCESS;
}

/**
 * @brief Remove a sensor from the database
 * @param pDbhdr: Pointer to the database header
 * @param ppSensors: Pointer to pointer of sensors array (for reallocation)
 * @param pRemove: Pointer to the string containing the sensor ID to be removed
 * @return STATUS_SUCCESS or STATUS_ERROR
 */
int parse_removeSensor(Parse_DbHeader_t *pDbhdr, Parse_Sensor_t **ppSensors, char *pRemove) {
    int sensorIndex = -1;
    Parse_Sensor_t *pNewSensors = NULL;

    sensorIndex = parse_findSensor(pDbhdr, *ppSensors, pRemove);

    if (-1 == sensorIndex)
    {
        printf("Sensor '%s' not found\n", pRemove);
        return STATUS_ERROR;
    }

    printf("Removing sensor at index %d\n", sensorIndex);

    // Determine how many sensors need to be moved
    int sensors_to_move = pDbhdr->count - sensorIndex - 1;

    // Shift all subsequent sensors up one position
    if (sensors_to_move > 0)
    {
        memmove(&(*ppSensors)[sensorIndex], &(*ppSensors)[sensorIndex + 1],
                sensors_to_move * sizeof(Parse_Sensor_t));
    }

    pDbhdr->count--;

    // Reallocate the array to the new smaller size
    if (pDbhdr->count > 0)
    {
        pNewSensors = realloc(*ppSensors, pDbhdr->count * sizeof(Parse_Sensor_t));
        if (NULL == pNewSensors)
        {
            // Revert count decrement
            pDbhdr->count++;
            printf("Failed to resize sensors array\n");
            return STATUS_ERROR;
        }
        *ppSensors = pNewSensors;
    }
    else
    {
        // If count is 0, free the array
        free(*ppSensors);
        *ppSensors = NULL;
    }

    // Update filesize
    pDbhdr->filesize = sizeof(Parse_DbHeader_t) + (sizeof(Parse_Sensor_t) * pDbhdr->count);

    return STATUS_SUCCESS;
}


void parse_listSensors(Parse_DbHeader_t *pDbhdr, Parse_Sensor_t *pSensors)
{
    int i = 0;

    for (i = 0; i < pDbhdr->count; i++)
    {
        printf("Sensor %d\n", i);
        printf("\tID: %s\n", pSensors[i].sensorId);
        printf("\tType: %s\n", pSensors[i].sensorType);
        printf("\tI2C Address: 0x%02X\n", pSensors[i].i2cAddr);
        printf("\tTimestamp: %s", ctime(&pSensors[i].timestamp));
        printf("\tReading: %.2f\n", pSensors[i].readingValue);
        printf("\tFlags: 0x%02X", pSensors[i].flags);

        if (pSensors[i].flags & SENSOR_FLAG_ACTIVE)
        {
            printf(" ACTIVE");
        }
        if (pSensors[i].flags & SENSOR_FLAG_ERROR)
        {
            printf(" ERROR");
        }
        if (pSensors[i].flags & SENSOR_FLAG_CALIBRATED)
        {
            printf(" CALIBRATED");
        }

        printf("\n");
        printf("\tLocation: %s\n", pSensors[i].location);
        printf("\tThresholds: Min=%.2f, Max=%.2f\n", 
                pSensors[i].minThreshold, pSensors[i].maxThreshold);
    }
}

/**
 * @brief  Reads sensor data in the database
 * @param fd: [in] File descriptor
 * @param  pDbhdr: [in] Pointer to database header
 * @param ppSensorsOut: [out] Pointer to array of sensors
 * @return STATUS_SUCCESS or STATUS_ERROR
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
 * @brief Find a sensor by ID
 * @param pDbhdr: Pointer to the database header
 * @param pSensors: Pointer to the array of sensors
 * @param pSensorId: Sensor ID to search for
 * @return Index of sensor if found, -1 otherwise
 */
int parse_findSensor(Parse_DbHeader_t *pDbhdr, Parse_Sensor_t *pSensors, const char *pSensorId) {
    int i = 0;

    for (i = 0; i < pDbhdr->count; i++)
    {
        if (0 == strcmp(pSensorId, pSensors[i].sensorId))
        {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Output database to disk
 * @param fd: [in] File descriptor
 * @param  pDbhdr: [in] Pointer to database header
 * @param pSensors: [in] Pointer to sensor array
 * @return STATUS_SUCCESS or STATUS_ERROR
 */
int parse_outputFile(int fd, Parse_DbHeader_t *pDbhdr, Parse_Sensor_t *pSensors)
{
    int realCount = 0;
    int i = 0;
    Parse_Sensor_t tempSensor;
    unsigned int temp = 0;

    if (fd < 0)
    {
        printf("Got a bad FD from the user\r\n");
        return STATUS_ERROR;
    }

    realCount = pDbhdr->count;

    // Convert header to network byte order
    pDbhdr->magic = htonl(pDbhdr->magic);
    pDbhdr->filesize = htonl(sizeof(Parse_DbHeader_t) + (sizeof(Parse_Sensor_t) * realCount));
    pDbhdr->count = htons(pDbhdr->count);
    pDbhdr->version = htons(pDbhdr->version);

    // Position file pointer at beginning
    lseek(fd, 0, SEEK_SET);

    write(fd, pDbhdr, sizeof(Parse_DbHeader_t));

    // Write each sensor
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

    // Truncate file to exact size
    ftruncate(fd, sizeof(Parse_DbHeader_t) + (sizeof(Parse_Sensor_t) * realCount));

    // Convert header back to host byte order
    pDbhdr->magic = ntohl(pDbhdr->magic);
    pDbhdr->version = ntohs(pDbhdr->version);
    pDbhdr->count = ntohs(pDbhdr->count);
    pDbhdr->filesize = ntohl(pDbhdr->filesize);

    return STATUS_SUCCESS;
}
