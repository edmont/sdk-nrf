/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_home_atc_mith_data_provider.h"

#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace Nrf;

static const bt_uuid *sServiceUuid = BT_UUID_BT_HOME_ATC_MITHERMOMETER;

/* BTHome v2 Object IDs */
#define BTHOME_OBJ_ID_TEMPERATURE 0x02  /* sint16, factor 0.01, °C */
#define BTHOME_OBJ_ID_HUMIDITY    0x03  /* uint16, factor 0.01, % */
#define BTHOME_OBJ_ID_BATTERY     0x01  /* uint8, factor 1, % */

const bt_uuid *BtHomeAtcMiThDataProvider::GetServiceUuid()
{
	return sServiceUuid;
}

void BtHomeAtcMiThDataProvider::Init()
{
	/* Do nothing - BTHome uses advertisements, not GATT characteristics */
}

void BtHomeAtcMiThDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
						  void *data, size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, clusterId, attributeId, data, dataSize);
	}
}

CHIP_ERROR BtHomeAtcMiThDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
						  uint8_t *buffer)
{
	if (clusterId != Clusters::BridgedDeviceBasicInformation::Id) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	switch (attributeId) {
	case Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id:
		/* Node label is just updated locally and there is no need to propagate the information to the end
		 * device. */
		break;
	default:
		return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
	}

	return CHIP_NO_ERROR;
}

int BtHomeAtcMiThDataProvider::ParseDiscoveredData(bt_gatt_dm *discoveredData)
{
	/* BTHome devices broadcast their data in BLE advertisements via Service Data.
	 * The data is already parsed from advertisements and doesn't require GATT discovery.
	 * This function is called after connection, but BTHome devices typically don't 
	 * require a connection - they broadcast data passively.
	 * 
	 * For now, we return success as the actual data will be received via advertisements
	 * which are handled by the BLE Connectivity Manager and parsed separately.
	 */
	
	LOG_INF("BTHome device connected - data will be received via advertisements");
	return 0;
}

void BtHomeAtcMiThDataProvider::NotifyTemperatureAttributeChange(intptr_t context)
{
	BtHomeAtcMiThDataProvider *provider = reinterpret_cast<BtHomeAtcMiThDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::TemperatureMeasurement::Id,
				    Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id,
				    &provider->mTemperatureValue, sizeof(provider->mTemperatureValue));
}

void BtHomeAtcMiThDataProvider::NotifyHumidityAttributeChange(intptr_t context)
{
	BtHomeAtcMiThDataProvider *provider = reinterpret_cast<BtHomeAtcMiThDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::RelativeHumidityMeasurement::Id,
				    Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id,
				    &provider->mHumidityValue, sizeof(provider->mHumidityValue));
}

void BtHomeAtcMiThDataProvider::NotifyBatteryAttributeChange(intptr_t context)
{
	BtHomeAtcMiThDataProvider *provider = reinterpret_cast<BtHomeAtcMiThDataProvider *>(context);

	/* Convert battery percentage (0-100) to half-percent units (0-200) as required by Matter */
	uint16_t batteryLevel = provider->mBatteryValue * 2;

	provider->NotifyUpdateState(Clusters::PowerSource::Id,
				    Clusters::PowerSource::Attributes::BatPercentRemaining::Id,
				    &batteryLevel, sizeof(batteryLevel));
}
