/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "ble_bridged_device.h"
#include "ble_connectivity_manager.h"
#include "bridged_device_data_provider.h"

#define BT_UUID_BT_HOME_ATC_MITHERMOMETER_VAL \
	BT_UUID_128_ENCODE(0x0000fcd2, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb)

#define BT_UUID_BT_HOME_ATC_MITHERMOMETER BT_UUID_DECLARE_128(BT_UUID_BT_HOME_ATC_MITHERMOMETER_VAL)

class BtHomeAtcMiThDataProvider : public Nrf::BLEBridgedDeviceProvider {
public:
	explicit BtHomeAtcMiThDataProvider(UpdateAttributeCallback updateCallback, InvokeCommandCallback commandCallback)
		: Nrf::BLEBridgedDeviceProvider(updateCallback, commandCallback) {}
	~BtHomeAtcMiThDataProvider() = default;

	void Init() override;
	void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
			       size_t dataSize) override;
	CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;
	const bt_uuid *GetServiceUuid() override;
	int ParseDiscoveredData(bt_gatt_dm *discoveredData) override;

private:
	static void NotifyTemperatureAttributeChange(intptr_t context);
	static void NotifyHumidityAttributeChange(intptr_t context);
	static void NotifyBatteryAttributeChange(intptr_t context);

	int16_t mTemperatureValue{};
	uint16_t mHumidityValue{};
	uint8_t mBatteryValue{};
};
