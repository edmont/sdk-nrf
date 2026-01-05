# BTHome ATC MiThermometer Implementation for Matter Bridge

## Overview
This implementation adds support for BTHome-compatible ATC MiThermometer devices (Xiaomi Mijia LYWSD03MMC) to the Nordic Matter Bridge application. These devices broadcast temperature, humidity, and battery data using the BTHome v2 protocol over BLE advertisements.

## Device Specification
- **Device**: ATC MiThermometer (Xiaomi Mijia LYWSD03MMC)
- **Protocol**: BTHome v2 (https://bthome.io/)
- **Service UUID**: 0xFCD2 (128-bit: 0000fcd2-0000-1000-8000-00805f9b34fb)
- **Data Format**: BLE Service Data advertisements (passive broadcast)

### BTHome v2 Data Format
The ATC MiThermometer broadcasts data in BLE advertisements using the BTHome v2 format:
- **Temperature** (Object ID 0x02): sint16, factor 0.01, °C
- **Humidity** (Object ID 0x03): uint16, factor 0.01, %
- **Battery** (Object ID 0x01): uint8, factor 1, %

## Matter Device Type Mapping
The BTHome service is mapped to the following Matter device types:
- **Temperature Sensor** (0x0302) - with PowerSource cluster for battery reporting
- **Humidity Sensor** (0x0307)

## Implementation Details

This implementation follows the Matter Bridge architecture documented in the Nordic Connect SDK. It adds a new BLE bridged device service by creating a data provider that implements the `BLEBridgedDeviceProvider` interface and integrating it into the bridge factory system.

### Files Modified/Created

#### 1. `src/ble/data_providers/bt_home_atc_mith_data_provider.h`
**New file** - Header file defining the BTHome data provider class.

Key features:
- Extends `Nrf::BLEBridgedDeviceProvider` to integrate with the Matter Bridge architecture
- Defines BTHome service UUID (0xFCD2) using Zephyr's UUID macros
- Declares interface methods required by the BridgedDeviceDataProvider pattern:
  - `Init()` - Initialize the provider
  - `NotifyUpdateState()` - Notify Matter stack of attribute changes
  - `UpdateState()` - Handle Matter attribute writes
  - `GetServiceUuid()` - Return the BTHome service UUID
  - `ParseDiscoveredData()` - Process discovered GATT data (minimal for BTHome)
- Stores sensor data in private member variables:
  - `mTemperatureValue` (int16_t) - Temperature in °C × 100
  - `mHumidityValue` (uint16_t) - Humidity in % × 100
  - `mBatteryValue` (uint8_t) - Battery percentage (0-100%)
- Declares static notification callbacks for asynchronous Matter updates

#### 2. `src/ble/data_providers/bt_home_atc_mith_data_provider.cpp`
**New file** - Implementation of the BTHome data provider.

Key implementation details:
- **BTHome v2 Object IDs**: Defines constants for temperature (0x02), humidity (0x03), and battery (0x01)
- **Init()**: Empty implementation - BTHome uses advertisements, not GATT characteristics
- **GetServiceUuid()**: Returns the BTHome service UUID for BLE scanning
- **NotifyUpdateState()**: Invokes the update callback to notify Matter stack of data changes
- **UpdateState()**: Handles Matter attribute write requests (only supports NodeLabel updates)
- **ParseDiscoveredData()**: Minimal implementation - BTHome data comes from advertisements, not GATT discovery
- **Notification Callbacks**: Three static methods that schedule Matter attribute updates:
  - `NotifyTemperatureAttributeChange()` - Updates TemperatureMeasurement cluster on Temperature Sensor endpoint
  - `NotifyHumidityAttributeChange()` - Updates RelativeHumidityMeasurement cluster on Humidity Sensor endpoint
  - `NotifyBatteryAttributeChange()` - Updates PowerSource cluster on Temperature Sensor endpoint (converts 0-100% to 0-200 half-percent units)

**Important Architecture Note**: BTHome devices broadcast sensor data in BLE advertisements (Service Data) rather than through GATT characteristics. The actual advertisement parsing would be handled by the BLE Connectivity Manager. This data provider focuses on the Matter integration layer, providing the interface between parsed BTHome data and the Matter Data Model.

#### 3. `src/ble/ble_bridged_device_factory.h`
**Modified** - Added BTHome service UUID to the factory enumeration.

Changes:
- Added `BtHomeAtcMiThService = 0xfcd2` to the `ServiceUuid` enum
- Included `bt_home_atc_mith_data_provider.h` header to make the provider available

This enum is used throughout the bridge application to identify and map BLE services to Matter device types.

#### 4. `src/ble/ble_bridged_device_factory.cpp`
**Modified** - Integrated BTHome into the device factory system.

Changes:
- **BleServiceToMatterDeviceType()**: Added `BtHomeAtcMiThService` case that maps to two device types:
  - TemperatureSensor
  - HumiditySensor
  
  This creates two Matter endpoints when a BTHome device is added to the bridge.

- **GetDataProviderFactory()**: Added factory method for creating `BtHomeAtcMiThDataProvider` instances:
  ```cpp
  { ServiceUuid::BtHomeAtcMiThService,
    [](UpdateAttributeCallback updateClb, InvokeCommandCallback commandClb) {
      return chip::Platform::New<BtHomeAtcMiThDataProvider>(updateClb, commandClb);
    } 
  }
  ```

The factory pattern allows the bridge to dynamically create the appropriate data provider when a BTHome device is discovered.

#### 5. `src/app_task.cpp`
**Modified** - Registered BTHome UUID for BLE scanning.

Changes:
- Included `ble/data_providers/bt_home_atc_mith_data_provider.h` header (conditional on `CONFIG_BRIDGED_DEVICE_BT`)
- Added `sUuidBtHomeAtcMiTh` constant pointing to the BTHome service UUID
- Added the BTHome UUID to the `sUuidServices` array:
  ```cpp
  const bt_uuid *sUuidServices[] = { sUuidLbs, sUuidEs, sUuidBtHomeAtcMiTh };
  ```

This allows the BLE Connectivity Manager to scan for and recognize BTHome devices during BLE discovery.

#### 6. `CMakeLists.txt`
**Modified** - Added BTHome data provider to the build system.

Changes:
- Added `src/ble/data_providers/bt_home_atc_mith_data_provider.cpp` to the build sources (conditional on `CONFIG_BRIDGED_DEVICE_BT`)
  
This ensures the BTHome implementation is compiled and linked into the Matter Bridge application.

#### 7. `Kconfig`
**Modified** - Updated BLE scan configuration.

Changes:
- Increased `CONFIG_BT_SCAN_UUID_CNT` from 2 to 3

This configuration option controls how many different BLE service UUIDs the bridge can simultaneously scan for. The three services are:
1. LED Button Service (LBS) - 0xbcd1
2. Environmental Sensor Service (ESS) - 0x181a  
3. BTHome ATC MiThermometer Service - 0xfcd2

#### 8. `src/bridged_device_types/temperature_sensor.h`
**Modified** - Added PowerSource cluster support for battery reporting.

Changes:
- Added PowerSource cluster handler methods:
  - `HandleReadPowerSource()` - Handles reads of PowerSource cluster attributes
  - `GetBatteryPercentRemaining()` - Returns battery percentage in half-percent units (0-200)
  - `GetBatteryChargeLevel()` - Returns battery charge level (0=OK, 1=Warning, 2=Critical)
- Added PowerSource cluster metadata:
  - `GetPowerSourceClusterRevision()` - Returns cluster revision 2
  - `GetPowerSourceFeatureMap()` - Returns feature map (0 = no optional features)
- Added private setters and member variables:
  - `SetBatteryPercentRemaining()` - Updates battery percentage
  - `SetBatteryChargeLevel()` - Updates battery charge level
  - `mBatteryPercentRemaining` - Stores battery level (default: 200 = 100%)
  - `mBatteryChargeLevel` - Stores charge status (default: 0 = OK)

#### 9. `src/bridged_device_types/temperature_sensor.cpp`
**Modified** - Implemented PowerSource cluster functionality.

Changes:
- **PowerSource Cluster Attributes Declaration**: Added `powerSourceAttrs` with:
  - Status (ENUM8) - Power source status
  - Order (INT8U) - Power source priority (0 = primary)
  - Description (CHAR_STRING) - Human-readable description ("Battery")
  - BatPercentRemaining (INT8U) - Battery percentage in half-percent units (0-200)
  - BatChargeLevel (ENUM8) - Charge level indicator (0/1/2)
  - FeatureMap (BITMAP32) - Cluster feature bitmap

- **Cluster List Update**: Added PowerSource cluster to `bridgedTemperatureClusters`:
  ```cpp
  DECLARE_DYNAMIC_CLUSTER(Clusters::PowerSource::Id, powerSourceAttrs, 
                          ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr)
  ```

- **HandleRead() Update**: Added PowerSource case to route reads to `HandleReadPowerSource()`

- **HandleReadPowerSource() Implementation**: Handles all PowerSource attribute reads:
  - Status: Returns current charge level
  - Order: Returns 0 (primary power source)
  - Description: Returns "Battery" as CharSpan
  - BatPercentRemaining: Returns battery percentage (0-200, where 200 = 100%)
  - BatChargeLevel: Returns charge level enum (0=OK, 1=Warning, 2=Critical)
  - ClusterRevision: Returns 2
  - FeatureMap: Returns 0

- **HandleAttributeChange() Update**: Added PowerSource case to handle battery updates:
  - BatPercentRemaining: Converts incoming data and calls `SetBatteryPercentRemaining()`

- **Namespace Organization**: Moved `using namespace` declarations before anonymous namespace to resolve PowerSource attribute compilation errors

**Battery Format**: Matter PowerSource cluster uses half-percent units (0-200) for battery reporting, where:
- 0 = 0% (empty)
- 100 = 50%
- 200 = 100% (full)

This allows for 0.5% precision in battery level reporting.

#### 10. `prj.conf`
**Modified** - Extended BLE scan timeout.

Changes:
- Added `CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS=15000` to increase scan duration from default 10s to 15s

This provides more time for BTHome devices to be discovered during BLE scanning, improving reliability of device detection.

## Usage

### Scanning for BTHome Devices
```
matter_bridge scan
```

This will list all available BLE devices, including BTHome devices with UUID 0xfcd2.

### Adding a BTHome Device
```
matter_bridge add <device_index> "Living Room Sensor"
```

This will create two Matter endpoints:
- One for the temperature sensor
- One for the humidity sensor

### Viewing Bridged Devices
```
matter_bridge list
```

This shows all bridged devices with their endpoint IDs and types.

## Technical Notes

### Advertisement-Based vs GATT-Based
BTHome devices differ from traditional BLE sensors:
- **Traditional sensors** (like ESS): Require GATT connection, characteristic discovery, and subscriptions
- **BTHome sensors**: Broadcast data in advertisements, no connection required

This implementation handles both patterns:
- The `ParseDiscoveredData()` method returns success without GATT operations
- Actual data parsing happens in the advertisement handler (BLE Connectivity Manager)
- The data provider focuses on converting BTHome data to Matter format

### Future Enhancements
Potential improvements for production use:
1. **Advertisement Parser**: Implement BTHome v2 advertisement parsing in BLE Connectivity Manager
2. **Encryption Support**: Add support for BTHome encrypted advertisements (AES-CCM)
3. **Additional Sensors**: Support other BTHome sensor types (pressure, PM2.5, etc.)
4. **Device Type ID**: Parse and use BTHome device type ID for device identification
5. **Packet ID**: Implement packet ID filtering to avoid duplicate data processing

### BTHome Specification Reference
- Format: https://bthome.io/format/
- UUID: Sponsored by Allterco Robotics (Shelly)
- License: https://bthome.io/images/License_Statement_-_BTHOME.pdf

## Testing

To test the implementation:

1. **Prepare the ATC MiThermometer**:
   - Flash the device with BTHome-compatible firmware (e.g., [ATC_MiThermometer custom firmware](https://github.com/pvvx/ATC_MiThermometer))
   - Configure it to broadcast in BTHome v2 format
   - Ensure the device is advertising

2. **Build the Matter Bridge with Bluetooth LE support**:
   
   **Important**: The default Matter Bridge configuration uses simulated devices. You must enable Bluetooth LE bridged devices to use BTHome sensors.
   
   ```bash
   cd nrf/applications/matter_bridge
   west build -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_BRIDGED_DEVICE_BT=y
   ```
   
   Alternatively, use menuconfig:
   ```bash
   west build -b nrf7002dk/nrf5340/cpuapp -t menuconfig
   # Navigate to: Bridged Device implementation → Select "Bluetooth LE Bridged Device"
   west build
   ```

3. **Flash and Run**:
   ```bash
   west flash
   ```

4. **Connect to UART console** and verify BT commands are available:
   ```
   uart:~$ matter_bridge
   ```
   
   You should see `scan`, `add`, `remove`, and `list` subcommands. If `scan` is not listed, `CONFIG_BRIDGED_DEVICE_BT` was not enabled.

5. **Scan for BTHome Devices**:
   ```
   uart:~$ matter_bridge scan
   ```
   
   Expected output should show the BTHome device with UUID 0xfcd2:
   ```
   Scanning for 30 s ...
   Scan result:
   ---------------------------------------------------------------------
   | Index |      Address      |                   UUID
   ---------------------------------------------------------------------
   | 0     | A4:C1:38:XX:XX:XX | 0xfcd2 (BTHome ATC MiThermometer)
   ```

6. **Add the BTHome Device**:
   ```
   uart:~$ matter_bridge add 0 "Kitchen Temp"
   ```
   
   Expected output:
   ```
   I: Added device to dynamic endpoint 3 (index=0)
   I: Added device to dynamic endpoint 4 (index=1)
   I: Created 0x302 device type on the endpoint 3
   I: Created 0x307 device type on the endpoint 4
   ```

7. **Verify Device Creation**:
   ```
   uart:~$ matter_bridge list
   ```
   
   Expected output:
   ```
   Bridged devices list:
   ---------------------------------------------------------------------
   | Endpoint ID |        Name        |           Type
   ---------------------------------------------------------------------
   | 3           | Kitchen Temp       | TemperatureSensor (0x0302)
   | 4           | Kitchen Temp       | HumiditySensor (0x0307)
   ---------------------------------------------------------------------
   Total: 2 device(s)
   ```
   
   **Note**: Endpoint 3 (Temperature Sensor) includes both the TemperatureMeasurement cluster and the PowerSource cluster for battery reporting. The battery information is not shown as a separate endpoint because PowerSource is a cluster attribute, not a distinct device type in Matter.

8. **Commission the Bridge** to a Matter network and verify the temperature and humidity sensors appear in your Matter controller (e.g., Google Home, Apple Home, Home Assistant).

9. **Verify Battery Reporting**: Use a Matter controller or chip-tool to read the PowerSource cluster on endpoint 3:
   ```bash
   # Read battery percentage (in half-percent units, 0-200)
   chip-tool powersource read bat-percent-remaining <node-id> 3
   
   # Read battery charge level (0=OK, 1=Warning, 2=Critical)
   chip-tool powersource read bat-charge-level <node-id> 3
   ```

## Battery Reporting Details

### PowerSource Cluster Integration
Battery information from BTHome devices is exposed through the Matter PowerSource cluster on the Temperature Sensor endpoint (endpoint 3). This follows the Matter specification pattern where power source information is a cluster on the device that uses it, rather than a separate endpoint.

### Supported Attributes
The PowerSource cluster implementation includes:

| Attribute | ID | Type | Description | Value |
|-----------|-----|------|-------------|-------|
| Status | 0x0000 | ENUM8 | Power source status | Same as BatChargeLevel |
| Order | 0x0001 | UINT8 | Power source priority | 0 (primary) |
| Description | 0x0002 | String | Human-readable name | "Battery" |
| BatPercentRemaining | 0x000C | UINT8 | Battery % (0-200) | 0-200 (200 = 100%) |
| BatChargeLevel | 0x000E | ENUM8 | Charge level | 0=OK, 1=Warning, 2=Critical |
| FeatureMap | 0xFFFC | BITMAP32 | Cluster features | 0 (no optional features) |
| ClusterRevision | 0xFFFD | UINT16 | Cluster revision | 2 |

### Battery Percentage Format
Matter's PowerSource cluster uses **half-percent units** for battery reporting:
- Range: 0-200
- Resolution: 0.5%
- Formula: `Matter_Value = BTHome_Percentage * 2`
- Examples:
  - BTHome: 100% → Matter: 200
  - BTHome: 50% → Matter: 100
  - BTHome: 25.5% → Matter: 51

### Charge Level Mapping
The charge level enum provides quick battery status assessment:
- **0 (OK)**: Battery is healthy (typically > 20%)
- **1 (Warning)**: Battery is low (typically 10-20%)
- **2 (Critical)**: Battery is critically low (typically < 10%)

The conversion from BTHome battery percentage to charge level can be implemented based on application requirements.

## Compliance

This implementation follows the Matter Bridge application architecture as documented in:
- `/home/edmo-nordic/repos/ncs/nrf/applications/matter_bridge/doc/matter_bridge_description.rst`
- `/home/edmo-nordic/repos/ncs/nrf/applications/matter_bridge/doc/adding_ble_bridged_device_service.rst`

The code adheres to Nordic Semiconductor coding standards and uses the existing Matter Bridge framework patterns.
