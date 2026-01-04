# BTHome ATC MiThermometer - Implementation Summary

## Overview

This implementation adds support for **BTHome v2** protocol to the Nordic Matter Bridge application, specifically targeting **ATC MiThermometer** devices (Xiaomi Mijia LYWSD03MMC with custom firmware). 

BTHome is an open-source BLE advertisement format for broadcasting sensor data, sponsored by Allterco Robotics (Shelly). The implementation follows the Matter Bridge architecture documented in the Nordic Connect SDK, adding a new BLE bridged device service through a dedicated data provider.

## What is BTHome?

BTHome devices broadcast sensor data in BLE advertisements using Service Data (UUID 0xFCD2), eliminating the need for GATT connections. This is ideal for battery-powered sensors as it minimizes power consumption.

**Data broadcasted by ATC MiThermometer:**
- Temperature (Object ID 0x02): sint16, 0.01°C resolution
- Humidity (Object ID 0x03): uint16, 0.01% resolution  
- Battery (Object ID 0x01): uint8, 1% resolution

## Implementation Details

### Files Changed

```
 applications/matter_bridge/CMakeLists.txt                                            |   3 +
 applications/matter_bridge/Kconfig                                                   |   2 +-
 applications/matter_bridge/src/app_task.cpp                                          |   4 +
 applications/matter_bridge/src/ble/ble_bridged_device_factory.cpp                    |   6 ++
 applications/matter_bridge/src/ble/ble_bridged_device_factory.h                      |   4 +-
 applications/matter_bridge/src/ble/data_providers/bt_home_atc_mith_data_provider.cpp | 114 +++++++++++++++++++++
 applications/matter_bridge/src/ble/data_providers/bt_home_atc_mith_data_provider.h   |  39 +++++++
 7 files changed, 170 insertions(+), 2 deletions(-)
```

### Component Overview

#### 1. **Data Provider** (New Files)
- `bt_home_atc_mith_data_provider.h` - Class definition
- `bt_home_atc_mith_data_provider.cpp` - Implementation

**Purpose**: Implements the `BLEBridgedDeviceProvider` interface to integrate BTHome devices into the Matter Bridge architecture.

**Key Methods**:
- `GetServiceUuid()` - Returns BTHome UUID (0xFCD2) for scanning
- `ParseDiscoveredData()` - Minimal GATT discovery (BTHome uses advertisements)
- `NotifyUpdateState()` - Propagates sensor data to Matter Data Model
- `UpdateState()` - Handles Matter attribute writes (NodeLabel only)
- Notification callbacks for temperature, humidity, and battery updates

**Data Storage**:
- `mTemperatureValue` (int16_t) - Temperature × 100
- `mHumidityValue` (uint16_t) - Humidity × 100  
- `mBatteryValue` (uint8_t) - Battery percentage

#### 2. **Factory Integration** (Modified)
- `ble_bridged_device_factory.h` - Added `BtHomeAtcMiThService = 0xfcd2` enum
- `ble_bridged_device_factory.cpp` - Added service-to-device mapping and provider factory

**Service Mapping**: BTHome service → Temperature Sensor + Humidity Sensor (2 endpoints)

**Provider Factory**: Lambda function to instantiate `BtHomeAtcMiThDataProvider`

#### 3. **Application Integration** (Modified)
- `app_task.cpp` - Added BTHome UUID to `sUuidServices[]` array for BLE scanning
- `CMakeLists.txt` - Added data provider source file to build
- `Kconfig` - Increased `BT_SCAN_UUID_CNT` from 2 to 3

### Architecture Highlights

**Advertisement-Based Design**: Unlike traditional BLE sensors (ESS, LBS) that use GATT characteristics and subscriptions, BTHome devices broadcast data in BLE advertisements. This implementation provides the Matter integration layer while the actual advertisement parsing would be handled by the BLE Connectivity Manager.

**Multi-Endpoint**: Each BTHome device creates two Matter endpoints:
- Endpoint N: Temperature Sensor (0x0302)
- Endpoint N+1: Humidity Sensor (0x0307)

**Matter Cluster Mapping**:
- Temperature → `TemperatureMeasurement::MeasuredValue` (°C × 100)
- Humidity → `RelativeHumidityMeasurement::MeasuredValue` (% × 100)
- Battery → `PowerSource::BatPercentRemaining` (% × 2, half-percent units)

## Usage Example

### Scanning for BTHome Devices
```
uart:~$ matter_bridge scan
```

Output:
```
Scan result:
---------------------------------------------------------------------
| Index |      Address      |                   UUID
---------------------------------------------------------------------
| 0     | A4:C1:38:XX:XX:XX | 0xfcd2 (BTHome)
| 1     | C7:44:0F:XX:XX:XX | 0xbcd1 (Led Button Service)
---------------------------------------------------------------------
```

### Adding a BTHome Device
```
uart:~$ matter_bridge add 0 "Living Room Sensor"
```

Output:
```
I: Added device to dynamic endpoint 3 (index=0)
I: Added device to dynamic endpoint 4 (index=1)
I: Created 0x302 device type on the endpoint 3
I: Created 0x307 device type on the endpoint 4
```

### Viewing Bridged Devices
```
uart:~$ matter_bridge list
```

Output:
```
Bridged devices list:
---------------------------------------------------------------------
| Endpoint ID |        Name            |           Type
---------------------------------------------------------------------
| 3           | Living Room Sensor     | TemperatureSensor (0x0302)
| 4           | Living Room Sensor     | HumiditySensor (0x0307)
---------------------------------------------------------------------
Total: 2 device(s)
```

### Removing a Device
```
uart:~$ matter_bridge remove 3
```

This removes both endpoints (temperature and humidity) associated with the device.

## Building and Testing

### Prerequisites
- **Hardware**: Nordic development kit (e.g., nRF7002 DK, nRF5340 DK)
- **BTHome Device**: ATC MiThermometer with [custom firmware](https://github.com/pvvx/ATC_MiThermometer) configured for BTHome v2

### Build Instructions

**Important**: You must enable Bluetooth bridged devices support instead of the default simulated devices.

#### Option 1: Using menuconfig
```bash
cd nrf/applications/matter_bridge
west build -b nrf7002dk/nrf5340/cpuapp -t menuconfig
```

Navigate to:
- `Bridged Device implementation` → Select `Bluetooth LE Bridged Device`

Save and build:
```bash
west build
west flash
```

#### Option 2: Using command line configuration
```bash
cd nrf/applications/matter_bridge
west build -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_BRIDGED_DEVICE_BT=y
west flash
```

#### Option 3: Using prj.conf overlay
Create a file `prj_bt.conf` with:
```
CONFIG_BRIDGED_DEVICE_BT=y
```

Then build:
```bash
west build -b nrf7002dk/nrf5340/cpuapp -- -DOVERLAY_CONFIG=prj_bt.conf
west flash
```

### Verifying the Build

After flashing, connect to the UART console and verify the `scan` command is available:

```
uart:~$ matter_bridge
```

You should see `scan` listed among the available subcommands. If you see "Please specify a subcommand" without `scan` listed, it means `CONFIG_BRIDGED_DEVICE_BT` was not enabled.

### Testing Steps
1. Power on the ATC MiThermometer (ensure BTHome v2 broadcasting is enabled)
2. Connect to the Matter Bridge UART console
3. Scan for devices using `matter_bridge scan`
4. Add the BTHome device using `matter_bridge add <index> "Name"`
5. Verify endpoints were created using `matter_bridge list`
6. Commission the bridge to a Matter network
7. Verify temperature and humidity sensors appear in your Matter controller

### Expected Behavior
- BTHome devices appear with UUID 0xfcd2 during scanning
- Adding a BTHome device creates two Matter endpoints
- Temperature and humidity values update based on BTHome advertisements
- Battery level is reported via the PowerSource cluster

## Future Enhancements

This implementation provides the foundation for BTHome support. Potential enhancements include:

1. **Advertisement Parser**: Implement BTHome v2 service data parsing in BLE Connectivity Manager to extract sensor values from advertisements
2. **Encryption Support**: Add AES-CCM decryption for encrypted BTHome advertisements
3. **Additional Sensors**: Extend support to other BTHome object types:
   - Pressure (0x04)
   - Illuminance (0x05)
   - CO2 (0x12)
   - PM2.5 (0x0D)
   - And [many more](https://bthome.io/format/)
4. **Packet ID Filtering**: Use BTHome packet ID (0x00) to filter duplicate advertisements
5. **Connection-less Mode**: Optimize for passive scanning without establishing BLE connections
6. **Device Type ID**: Parse and utilize BTHome device type ID (0xF0) for better device identification
7. **Firmware Version**: Extract and display firmware version from BTHome data (0xF1, 0xF2)

## Technical Notes

### BTHome vs Traditional BLE Sensors

| Aspect | BTHome | Traditional (ESS/LBS) |
|--------|--------|----------------------|
| **Data Transport** | BLE Advertisements | GATT Characteristics |
| **Connection Required** | No | Yes |
| **Power Consumption** | Very Low | Higher |
| **Update Mechanism** | Passive broadcast | Subscribe/Notify |
| **Discovery** | Service Data UUID | GATT Discovery |

### Matter Bridge Architecture

This implementation follows the documented pattern for adding BLE services:
1. Create data provider extending `BLEBridgedDeviceProvider`
2. Register service UUID in factory enum
3. Map service to Matter device types
4. Add provider factory method
5. Register UUID for scanning

See: `nrf/applications/matter_bridge/doc/adding_ble_bridged_device_service.rst`

## References

- **BTHome Specification**: https://bthome.io/format/
- **BTHome GitHub**: https://github.com/Bluetooth-Devices/bthome-ble
- **ATC MiThermometer Firmware**: https://github.com/pvvx/ATC_MiThermometer
- **Matter Bridge Documentation**: `nrf/applications/matter_bridge/doc/`
- **Nordic Connect SDK**: https://developer.nordicsemi.com/nRF_Connect_SDK/

## License

This implementation follows the same license as the Nordic Connect SDK:
- SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

BTHome UUID (0xFCD2) is sponsored by Allterco Robotics and free to use under the [BTHome license](https://bthome.io/images/License_Statement_-_BTHOME.pdf).

---

**Full technical details**: See `BTHOME_IMPLEMENTATION.md` in the repository root.
