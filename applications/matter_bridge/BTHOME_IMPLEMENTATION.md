# BTHome ATC MiThermometer Implementation for Matter Bridge

## Overview
This implementation adds support for BTHome-compatible ATC MiThermometer devices (Xiaomi Mijia LYWSD03MMC) to the Nordic Matter Bridge application. These devices broadcast temperature, humidity, and battery data using the BTHome v2 protocol over BLE advertisements.

**Key Innovation**: Unlike traditional BLE sensors that require GATT connections, BTHome devices are **advertisement-only** - they broadcast data in BLE Service Data without supporting connections or pairing. This implementation includes special handling for advertisement-only devices in the Matter Bridge architecture.

## Device Specification
- **Device**: ATC MiThermometer (Xiaomi Mijia LYWSD03MMC)
- **Protocol**: BTHome v2 (https://bthome.io/)
- **Service UUID**: 0xFCD2 (128-bit: 0000fcd2-0000-1000-8000-00805f9b34fb)
- **Data Format**: BLE Service Data advertisements (passive broadcast)
- **Connection Support**: None - advertisement-only operation
- **Pairing Support**: Not required - no security needed for advertisements

### BTHome v2 Data Format
The ATC MiThermometer broadcasts data in BLE advertisements using the BTHome v2 format:
- **Temperature** (Object ID 0x02): sint16, factor 0.01, °C
- **Humidity** (Object ID 0x03): uint16, factor 0.01, %
- **Battery** (Object ID 0x01): uint8, factor 1, %

**Important**: The BTHome UUID (0xFCD2) is broadcast in **Service Data** (BT_DATA_SVC_DATA16), not in the Service UUID list. This required custom advertisement parsing logic.

## Matter Device Type Mapping
The BTHome service is mapped to the following Matter device types:
- **Temperature Sensor** (0x0302)
- **Humidity Sensor** (0x0307)
- **Power Source** (for battery reporting)

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
  - `mBatteryValue` (uint8_t) - Battery percentage
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
  - `NotifyTemperatureAttributeChange()` - Updates TemperatureMeasurement cluster
  - `NotifyHumidityAttributeChange()` - Updates RelativeHumidityMeasurement cluster  
  - `NotifyBatteryAttributeChange()` - Updates PowerSource cluster (converts percentage to half-percent units)

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

2. **Build the Matter Bridge**:
   ```bash
   cd nrf/applications/matter_bridge
   west build -b nrf7002dk/nrf5340/cpuapp
   ```

3. **Flash and Run**:
   ```bash
   west flash
   ```

4. **Connect to UART console** and wait for the bridge to initialize

5. **Scan for BTHome Devices**:
   ```
   uart:~$ matter_bridge scan
   ```
   
   Expected output should show the BTHome device with UUID 0xfcd2:
   ```
   Scan result:
   ---------------------------------------------------------------------
   | Index |      Address      |                   UUID
   ---------------------------------------------------------------------
   | 0     | A4:C1:38:XX:XX:XX | 0xfcd2 (BTHome)
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

8. **Commission the Bridge** to a Matter network and verify the temperature and humidity sensors appear in your Matter controller (e.g., Google Home, Apple Home, Home Assistant).

## Technical Challenges and Solutions

### Challenge 1: Service Data vs Service UUID Detection

**Problem**: The Nordic bt_scan library's `bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, ...)` only matches devices advertising Service UUIDs in the BLE advertisement packet. BTHome devices broadcast their UUID (0xFCD2) in **Service Data** (BT_DATA_SVC_DATA16), not as a Service UUID.

**Symptoms**: 
- BTHome devices visible in nRF Connect app
- `matter_bridge scan` shows empty results
- Filter match callback never triggered

**Solution**: 
1. Added `FilterNoMatch` callback to `BLEConnectivityManager` to handle devices that don't match Service UUID filters
2. Implemented custom advertisement parsing using `bt_data_parse()` with a lambda callback
3. Manually search advertisement data for BT_DATA_SVC_DATA16 with UUID 0xFCD2
4. Add matching devices to scan results

**Code Location**: `ble_connectivity_manager.cpp:FilterNoMatch()`

### Challenge 2: Advertisement-Only Architecture (No Connections)

**Problem**: Matter Bridge architecture assumes all BLE devices support GATT connections. Calling `bt_conn_le_create()` on BTHome devices results in:
```
E: pairing failed (peer reason 0x5)
E: Security failed: level 1 err 5
I: Disconnected: A4:C1:38:88:03:10 (public) (reason 22)
```

**Root Cause**: BTHome devices are advertisement-only and don't support BLE connections or pairing.

**Solution**:
1. Modified `BleBridgedDeviceFactory::CreateDevice()` to detect BTHome service UUID
2. For BTHome devices, skip `BLEConnectivityManager::Connect()` call
3. Directly invoke `BluetoothDeviceConnected(true, context)` to trigger endpoint creation
4. Matter endpoints created successfully without establishing BLE connection

**Code Location**: `ble_bridged_device_factory.cpp:428-445`

### Challenge 3: Callback Signature Mismatch

**Problem**: Initial attempt called callback with wrong signature:
```cpp
BluetoothDeviceConnected(contextPtr.get(), 0);  // Wrong!
```

**Root Cause**: Callback signature is `(bool success, void *context)`, not `(void *context, int error)`.

**Solution**: Corrected to:
```cpp
BluetoothDeviceConnected(true, contextPtr.get());  // Correct
```

**Result**: Matter endpoints created successfully after device scan.

### Challenge 4: UUID Display Name

**Problem**: Scan results showed "Unknown" for BTHome UUID.

**Solution**: Added BTHome case to `GetUuidString()` function to display "BTHome" instead of "Unknown".

**Code Location**: `ble_bridged_device_factory.cpp:500-501`

## Current Limitations

### Sensor Data Not Updated
The current implementation successfully:
- ✅ Detects BTHome devices via Service Data parsing
- ✅ Creates Matter endpoints without GATT connection
- ✅ Bypasses pairing for advertisement-only devices

However, sensor values (temperature, humidity, battery) are not yet updated because:
- Advertisement data parsing is not implemented
- No mechanism to monitor ongoing advertisements from bridged devices
- Notification callbacks are prepared but not called with real data

**Next Steps for Full Functionality**:
1. Implement advertisement monitoring for bridged BTHome devices
2. Parse BTHome v2 packet format from Service Data payload:
   ```
   Byte 0: Device Info (encryption, version, etc.)
   Byte 1+: Object ID + Value pairs
   ```
3. Extract temperature, humidity, and battery values
4. Call provider notification callbacks:
   - `NotifyTemperatureAttributeChange()`
   - `NotifyHumidityAttributeChange()`
   - `NotifyBatteryAttributeChange()`

## Compliance

This implementation follows the Matter Bridge application architecture as documented in:
- `/home/edmo-nordic/repos/ncs/nrf/applications/matter_bridge/doc/matter_bridge_description.rst`
- `/home/edmo-nordic/repos/ncs/nrf/applications/matter_bridge/doc/adding_ble_bridged_device_service.rst`

The code adheres to Nordic Semiconductor coding standards and uses the existing Matter Bridge framework patterns.
