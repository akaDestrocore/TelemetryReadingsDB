# Network Telemetry Database Project

A basic networked database system for telemetry with client-server architecture:

## Core Components

- **Server (`telemetry_srv`)**: 
  - Poll-based architecture handling multiple simultaneous clients
  - State machine for connection management (NEW → HANDSHAKE → MSG)
  - Persistent database storage
  - Clean signal handling for graceful shutdown

- **Client (`telemetry_cli`)**:
  - Command-line interface for database operations
  - Implements handshaking protocol
  - Supports add/list/delete operations

## Technical Implementation

- **Network Protocol**:
  - Custom binary protocol with version negotiation
  - State-based message handling
  - Connection-per-request model

### Usage Examples

```bash
# Server
$ ./bin/telemetry_srv 
Filepath is a required argument
Usage: ./bin/telemetry_srv -n -f <database file>
Options:
	-n          create new database file
	-f <file>   (required) database file path
	-p <port>   (required) port to listen on
$ ./bin/telemetry_srv -f ./telemetry_db.db -n -p 8080
  Listening on: 0.0.0.0:8080

  CONNECT CLIENT:
  ./bin/telemetry_cli -p 8080 -h 127.0.0.1
```

Client side:
```bash
# Client
$ ./bin/telemetry_cli 
Usage: ./bin/telemetry_cli -f -n <database file>
         -h             - (required) host to connect to
         -p             - (required) port to connect to
         -a             - add new sensor data with the given string format 'sensor_id,sensor_type,i2c_addr(if any),timestamp,reading_value'
         -l             - list all sensor etries in the database
         -d <name>      - delete sensor entry from the database with the given ID
root@destrocore:/home/destrocore/WORKSPACE/VS_CODE_PROJECTS/C_CODE/TelemetryReadingsDB# ./bin/telemetry_cli -p 8080 -h 127.0.0.1 -a "TM100_01,TM100,-,1701432000,5.2"
Server connected!
Sensor added succesfully.
root@destrocore:/home/destrocore/WORKSPACE/VS_CODE_PROJECTS/C_CODE/TelemetryReadingsDB# ./bin/telemetry_cli -p 8080 -h 127.0.0.1 -l
Server connected!
Sent sensor list request to server
Received sensor list response from server. Count: 1

Sensor 0:
  ID: TM100_01
  Type: TM100
  I2C Address: 0x00
  Timestamp: (null)  Reading: 5.20
  Flags: 0x05 ACTIVE CALIBRATED
  Location: Unknown Location
  Thresholds: Min=-100.00, Max=100.00
root@destrocore:/home/destrocore/WORKSPACE/VS_CODE_PROJECTS/C_CODE/TelemetryReadingsDB# ./bin/telemetry_cli -p 8080 -h 127.0.0.1 -a "BNO055_01,BNO055,0x28,1701432000,25.5"
Server connected!
Sensor added succesfully.
root@destrocore:/home/destrocore/WORKSPACE/VS_CODE_PROJECTS/C_CODE/TelemetryReadingsDB# ./bin/telemetry_cli -p 8080 -h 127.0.0.1 -d "BNO055_01"
Server connected!
Sent delete request to server. Sensor ID: BNO055_01
Successfully deleted sensor 'BNO055_01' from database
root@destrocore:/home/destrocore/WORKSPACE/VS_CODE_PROJECTS/C_CODE/TelemetryReadingsDB# ./bin/telemetry_cli -p 8080 -h 127.0.0.1 -l
Server connected!
Sent sensor list request to server
Received sensor list response from server. Count: 1

Sensor 0:
  ID: TM100_01
  Type: TM100
  I2C Address: 0x00
  Timestamp: (null)  Reading: 5.20
  Flags: 0x05 ACTIVE CALIBRATED
  Location: Unknown Location
  Thresholds: Min=-100.00, Max=100.00
```

Server side:
```bash
#Server
New connection from 127.0.0.1:38948
Client connected in slot 0 with fd 5
Client promoted to STATE_MSG
Adding sensor: TM100_01,TM100,-,1701432000,5.2
TM100_01,TM100,-,1701432000,5.2
TM100_01 TM100 - 1701432000 5.2
Client disconnected
New connection from 127.0.0.1:50984
Client connected in slot 0 with fd 5
Client promoted to STATE_MSG
Listing all sensors
Client disconnected
New connection from 127.0.0.1:39644
Client connected in slot 0 with fd 5
Client promoted to STATE_MSG
Adding sensor: BNO055_01,BNO055,0x28,1701432000,25.5
BNO055_01,BNO055,0x28,1701432000,25.5
BNO055_01 BNO055 0x28 1701432000 25.5
Client disconnected
New connection from 127.0.0.1:53728
Client connected in slot 0 with fd 5
Client promoted to STATE_MSG
Deleting sensor: BNO055_01
Removing sensor at index 1
Client disconnected
New connection from 127.0.0.1:39276
Client connected in slot 0 with fd 5
Client promoted to STATE_MSG
Listing all sensors
Client disconnected
```
