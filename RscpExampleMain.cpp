/*
	Based on RscpExample from E3DC for S10
	Modified and extended from Thomas Bella <thomas@bella.network>.

	This program fetches the current data from an E3DC S10 device over the RSCP TCP/IP protocol
	and deliveres the fetched data into a json file which can be simply parsed by other programs.

	Copyright (c) 2018 Thomas Bella <thomas@bella.network>

	MIT Licence

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "RscpProtocol.h"
#include "RscpTags.h"
#include "SocketConnection.h"
#include "AES.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include "settings.h"

#define AES_KEY_SIZE        32
#define AES_BLOCK_SIZE      32

#ifndef SERVER_IP
printf("SERVER_IP is not defined. Check settings.h within source code")
exit(EXIT_FAILURE);
#endif

static int iSocket = -1;
static int iAuthenticated = 0;
static AES aesEncrypter;
static AES aesDecrypter;
static uint8_t ucEncryptionIV[AES_BLOCK_SIZE];
static uint8_t ucDecryptionIV[AES_BLOCK_SIZE];
static char TAG_EMS_OUT_SERIAL_NUMBER[17];
static bool gotData = false;
static uint8_t gotDataFailed = 0;
static uint16_t printStats = 0;

using json = nlohmann::json;
json mainJSONObject;

using namespace std;

int createRequestExample(SRscpFrameBuffer * frameBuffer) {
	RscpProtocol protocol;
	SRscpValue rootValue;
	// The root container is create with the TAG ID 0 which is not used by any device.
	protocol.createContainerValue(&rootValue, 0);

	//---------------------------------------------------------------------------------------------------------
	// Create a request frame
	//---------------------------------------------------------------------------------------------------------
	if(iAuthenticated == 0)
	{
		printf("\nRequest authentication\n");
		// authentication request
		SRscpValue authenContainer;
		protocol.createContainerValue(&authenContainer, TAG_RSCP_REQ_AUTHENTICATION);
		protocol.appendValue(&authenContainer, TAG_RSCP_AUTHENTICATION_USER, E3DC_USER);
		protocol.appendValue(&authenContainer, TAG_RSCP_AUTHENTICATION_PASSWORD, E3DC_PASSWORD);
		// append sub-container to root container
		protocol.appendValue(&rootValue, authenContainer);
		// free memory of sub-container as it is now copied to rootValue
		protocol.destroyValueData(authenContainer);
	}
	else
	{
		protocol.appendValue(&rootValue, TAG_INFO_REQ_TIME);

		// Only get special results once because they do not change in time
		if (strlen(TAG_EMS_OUT_SERIAL_NUMBER) == 0) {
			protocol.appendValue(&rootValue, TAG_INFO_REQ_SERIAL_NUMBER);
		}

		// request power data information
		protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_PV);
		protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_BAT);
		protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_HOME);
		protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_GRID);
		protocol.appendValue(&rootValue, TAG_EMS_REQ_POWER_ADD);
		protocol.appendValue(&rootValue, TAG_EMS_REQ_AUTARKY);
		protocol.appendValue(&rootValue, TAG_EMS_REQ_SELF_CONSUMPTION);
		protocol.appendValue(&rootValue, TAG_EMS_REQ_COUPLING_MODE);
		protocol.appendValue(&rootValue, TAG_EMS_REQ_GET_POWER_SETTINGS);
		protocol.appendValue(&rootValue, TAG_EMS_REQ_GET_IDLE_PERIODS);

		// request battery information
		SRscpValue batteryContainer;
		protocol.createContainerValue(&batteryContainer, TAG_BAT_REQ_DATA);
		protocol.appendValue(&batteryContainer, TAG_BAT_INDEX, (uint8_t)0);
		protocol.appendValue(&batteryContainer, TAG_BAT_REQ_RSOC);
		protocol.appendValue(&batteryContainer, TAG_BAT_REQ_MODULE_VOLTAGE);
		protocol.appendValue(&batteryContainer, TAG_BAT_REQ_CURRENT);
		protocol.appendValue(&batteryContainer, TAG_BAT_REQ_CHARGE_CYCLES);
		protocol.appendValue(&batteryContainer, TAG_BAT_REQ_TRAINING_MODE);
		protocol.appendValue(&rootValue, batteryContainer);
		protocol.destroyValueData(batteryContainer);

		// PVI (PV MPP-Tracker / Strings)
		SRscpValue PVIContainer;
		protocol.createContainerValue(&PVIContainer, TAG_PVI_REQ_DATA);
		protocol.appendValue(&PVIContainer, TAG_PVI_INDEX, (uint8_t)0);
		protocol.appendValue(&PVIContainer, TAG_PVI_REQ_ON_GRID);
		protocol.appendValue(&PVIContainer, TAG_PVI_REQ_SYSTEM_MODE);

		if (PVI_TRACKER == 2)
			protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_POWER, (uint8_t)1);
		protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_POWER, (uint8_t)0);
		if (PVI_TRACKER == 2)
			protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_VOLTAGE, (uint8_t)1);
		protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_VOLTAGE, (uint8_t)0);
		if (PVI_TRACKER == 2)
			protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_CURRENT, (uint8_t)1);
		protocol.appendValue(&PVIContainer, TAG_PVI_REQ_DC_CURRENT, (uint8_t)0);

		protocol.appendValue(&rootValue, PVIContainer);
		protocol.destroyValueData(PVIContainer);

		// PM
		SRscpValue PMContainer;
		protocol.createContainerValue(&PMContainer, TAG_PM_REQ_DATA);
		protocol.appendValue(&PMContainer, TAG_PM_INDEX, (uint8_t)0);
		protocol.appendValue(&PMContainer, TAG_PM_REQ_DEVICE_STATE);
		protocol.appendValue(&PMContainer, TAG_PM_REQ_ACTIVE_PHASES);
		protocol.appendValue(&PMContainer, TAG_PM_REQ_POWER_L1);
		protocol.appendValue(&PMContainer, TAG_PM_REQ_POWER_L2);
		protocol.appendValue(&PMContainer, TAG_PM_REQ_POWER_L3);
		protocol.appendValue(&PMContainer, TAG_PM_REQ_VOLTAGE_L1);
		protocol.appendValue(&PMContainer, TAG_PM_REQ_VOLTAGE_L2);
		protocol.appendValue(&PMContainer, TAG_PM_REQ_VOLTAGE_L3);
		protocol.appendValue(&rootValue, PMContainer);
		protocol.destroyValueData(PMContainer);
	}

	// create buffer frame to send data to the S10
	protocol.createFrameAsBuffer(frameBuffer, rootValue.data, rootValue.length, true); // true to calculate CRC on for transfer
	// the root value object should be destroyed after the data is copied into the frameBuffer and is not needed anymore
	protocol.destroyValueData(rootValue);

	return 0;
}

int handleResponseValue(RscpProtocol *protocol, SRscpValue *response) {

	// check if any of the response has the error flag set and react accordingly
	if(response->dataType == RSCP::eTypeError) {
		// handle error for example access denied errors
		uint32_t uiErrorCode = protocol->getValueAsUInt32(response);
		printf("Tag 0x%08X received error code %u.\n", response->tag, uiErrorCode);
		return -1;
	}

	// check the SRscpValue TAG to detect which response it is
	switch(response->tag) {
		case TAG_RSCP_AUTHENTICATION:
		{
			// It is possible to check the response->dataType value to detect correct data type
			// and call the correct function. If data type is known,
			// the correct function can be called directly like in this case.
			uint8_t ucAccessLevel = protocol->getValueAsUChar8(response);
			if(ucAccessLevel > 0) {
				iAuthenticated = 1;
			}
			printf("RSCP authentitication level %i\n", ucAccessLevel);
			break;
		}
		case TAG_EMS_POWER_PV:
		{
			// response for TAG_EMS_REQ_POWER_PV
			int32_t iPower = protocol->getValueAsInt32(response);
			mainJSONObject["power"]["pv"] = iPower;
			break;
		}
		case TAG_EMS_POWER_BAT:
		{
			// response for TAG_EMS_REQ_POWER_BAT
			int32_t iPower = protocol->getValueAsInt32(response);
			mainJSONObject["power"]["bat"] = iPower;
			break;
		}
		case TAG_EMS_POWER_HOME:
		{
			// response for TAG_EMS_REQ_POWER_HOME
			int32_t iPower = protocol->getValueAsInt32(response);
			mainJSONObject["power"]["home"] = iPower;
			break;
		}
		case TAG_EMS_POWER_GRID:
		{
			// response for TAG_EMS_REQ_POWER_GRID
			int32_t iPower = protocol->getValueAsInt32(response);
			mainJSONObject["power"]["grid"] = iPower;
			break;
		}
		case TAG_EMS_POWER_ADD:
		{
			// response for TAG_EMS_REQ_POWER_ADD
			int32_t iPower = protocol->getValueAsInt32(response);
			mainJSONObject["power"]["add"] = iPower;
			break;
		}
		case TAG_EMS_AUTARKY:
		{
			// response for TAG_EMS_REQ_AUTARKY
			float iPower = protocol->getValueAsFloat32(response);
			mainJSONObject["meta"]["autarky"] = iPower;
			break;
		}
		case TAG_EMS_SELF_CONSUMPTION:
		{
			// response for TAG_EMS_SELF_CONSUMPTION
			float iPower = protocol->getValueAsFloat32(response);
			mainJSONObject["meta"]["consumption"] = iPower;
			break;
		}
		case TAG_EMS_COUPLING_MODE:
		{
			// response for TAG_EMS_COUPLING_MODE
			uint8_t iPower = protocol->getValueAsUChar8(response);
			mainJSONObject["meta"]["operation_mode_desc"] = "0: DC / 1: DC-MultiWR / 2: AC / 3: HYBRID / 4: ISLAND";
			mainJSONObject["meta"]["operation_mode"] = iPower;
			break;
		}
		case TAG_INFO_TIME:
		{
			// response for TAG_INFO_REQ_TIME
			gotData = true;
			int32_t unixTimestamp = protocol->getValueAsInt32(response);
			if (unixTimestamp == 0) {
				exit(EXIT_FAILURE);
			}
			mainJSONObject["meta"]["timestamp"] = unixTimestamp;
			break;
		}
		case TAG_INFO_SERIAL_NUMBER:
		{
			std::string serialNumber = protocol->getValueAsString(response);
			strcpy(TAG_EMS_OUT_SERIAL_NUMBER, serialNumber.c_str());
			mainJSONObject["meta"]["serial_number"] = TAG_EMS_OUT_SERIAL_NUMBER;
			break;
		}
		case TAG_PM_DATA:
		{
			// resposne for TAG_PM_REQ_DATA
			uint8_t ucPMIndex = 0;
			std::vector<SRscpValue> PMData = protocol->getValueAsContainer(response);

			for (size_t i = 0; i < PMData.size(); ++i) {
				if(PMData[i].dataType == RSCP::eTypeError) {
					// handle error for example access denied errors
					uint32_t uiErrorCode = protocol->getValueAsUInt32(&PMData[i]);
					printf("Tag 0x%08X received error code %u.\n", PMData[i].tag, uiErrorCode);
					return -1;
				}
				// check each battery sub tag
				switch(PMData[i].tag) {
				case TAG_PM_INDEX:
				{
					ucPMIndex = protocol->getValueAsUChar8(&PMData[i]);
					break;
				}
				case TAG_PM_DEVICE_STATE:
				{
					// response for TAG_PM_REQ_DEVICE_STATE
					bool devState = protocol->getValueAsBool(&PMData[i]);
					mainJSONObject["pm"]["lm0_state"] = devState;
					break;
				}
				case TAG_PM_ACTIVE_PHASES:
				{
					// response for TAG_PM_REQ_ACTIVE_PHASES
					int32_t activePhases = protocol->getValueAsInt32(&PMData[i]);
					mainJSONObject["pm"]["active_phases"] = activePhases;
					break;
				}
				case TAG_PM_POWER_L1:
				{
					// response for TAG_PM_POWER_L1
					double iPower = protocol->getValueAsDouble64(&PMData[i]);
					mainJSONObject["pm"]["power1"] = iPower;
					break;
				}
				case TAG_PM_POWER_L2:
				{
					// response for TAG_PM_POWER_L2
					double iPower = protocol->getValueAsDouble64(&PMData[i]);
					mainJSONObject["pm"]["power2"] = iPower;
					break;
				}
				case TAG_PM_POWER_L3:
				{
					// response for TAG_PM_POWER_L3
					double iPower = protocol->getValueAsDouble64(&PMData[i]);
					mainJSONObject["pm"]["power3"] = iPower;
					break;
				}

				case TAG_PM_VOLTAGE_L1:
				{
					// response for TAG_PM_VOLTAGE_L1
					float iPower = protocol->getValueAsFloat32(&PMData[i]);
					mainJSONObject["pm"]["voltage1"] = iPower;
					break;
				}
				case TAG_PM_VOLTAGE_L2:
				{
					// response for TAG_PM_VOLTAGE_L2
					float iPower = protocol->getValueAsFloat32(&PMData[i]);
					mainJSONObject["pm"]["voltage2"] = iPower;
					break;
				}
				case TAG_PM_VOLTAGE_L3:
				{
					// response for TAG_PM_VOLTAGE_L3
					float iPower = protocol->getValueAsFloat32(&PMData[i]);
					mainJSONObject["pm"]["voltage3"] = iPower;
					break;
				}

				// ...
				default:
					// default behaviour
					printf("Unknown PM tag %08X\n", response->tag);
					break;
				}
			}
			protocol->destroyValueData(PMData);
			break;
		}
		case TAG_BAT_DATA:
		{
			// resposne for TAG_BAT_REQ_DATA
			uint8_t ucBatteryIndex = 0;
			std::vector<SRscpValue> batteryData = protocol->getValueAsContainer(response);
			for (size_t i = 0; i < batteryData.size(); ++i) {
				if(batteryData[i].dataType == RSCP::eTypeError) {
					// handle error for example access denied errors
					uint32_t uiErrorCode = protocol->getValueAsUInt32(&batteryData[i]);
					printf("Tag 0x%08X received error code %u.\n", batteryData[i].tag, uiErrorCode);
					return -1;
				}
				// check each battery sub tag
				switch(batteryData[i].tag) {
					case TAG_BAT_INDEX:
					{
						ucBatteryIndex = protocol->getValueAsUChar8(&batteryData[i]);
						break;
					}
					case TAG_BAT_RSOC:
					{
						// response for TAG_BAT_REQ_RSOC
						float fSOC = protocol->getValueAsFloat32(&batteryData[i]);
						mainJSONObject["battery"]["charge"] = fSOC;
						break;
					}
					case TAG_BAT_MODULE_VOLTAGE:
					{
						// response for TAG_BAT_REQ_MODULE_VOLTAGE
						float fVoltage = protocol->getValueAsFloat32(&batteryData[i]);
						mainJSONObject["battery"]["voltage"] = fVoltage;
						break;
					}
					case TAG_BAT_CURRENT:
					{
						// response for TAG_BAT_REQ_CURRENT
						float fVoltage = protocol->getValueAsFloat32(&batteryData[i]);
						mainJSONObject["battery"]["current"] = fVoltage;
						break;
					}
					case TAG_BAT_CHARGE_CYCLES:	{ // response for TAG_BAT_CHARGE_CYCLES
						uint32_t bCycles = protocol->getValueAsUInt32(&batteryData[i]);
						mainJSONObject["battery"]["cycles"] = bCycles;
						break;
					}
					case TAG_BAT_TRAINING_MODE:
					{
						uint32_t trainingMode = protocol->getValueAsUChar8(&batteryData[i]);
						mainJSONObject["battery"]["training"] = trainingMode;
						mainJSONObject["battery"]["training_desc"] = "0 - Nicht im Training / 1 - Trainingmodus Entladen / 2 - Trainingmodus Laden";
						break;
					}

					// ...
					default:
						// default behaviour
						printf("Unknown battery tag %08X\n", response->tag);
						break;
				}
			}
			protocol->destroyValueData(batteryData);
			break;
		}
		case TAG_EMS_GET_IDLE_PERIODS:
		{
			uint8_t ucPVIIndex = 0;
			std::vector<SRscpValue> EMSIdlePeriods = protocol->getValueAsContainer(response);
			/*
							6: {
								active: false,
								day: 6,
								end_hour: 21,
								end_minute: 0,
								start_hour: 1,
								start_minute: 0,
								type: 0
								},
							*/

			for (size_t i = 0; i < EMSIdlePeriods.size(); ++i)
			{
				uint8_t idlePeriodDay;
				uint8_t idlePeriodType;
				uint8_t idlePeriodStartHour;
				uint8_t idlePeriodStartMinute;
				uint8_t idlePeriodEndHour;
				uint8_t idlePeriodEndMinute;
				bool idlePeriodActive;

				if (EMSIdlePeriods[i].dataType == RSCP::eTypeError)
				{
					// handle error for example access denied errors
					uint32_t uiErrorCode = protocol->getValueAsUInt32(&EMSIdlePeriods[i]);
					printf("Tag 0x%08X received error code %u.\n", EMSIdlePeriods[i].tag, uiErrorCode);
					return -1;
				}
				switch (EMSIdlePeriods[i].tag)
				{
					case TAG_PVI_INDEX:
					{
						ucPVIIndex = protocol->getValueAsUChar8(&EMSIdlePeriods[i]);
						break;
					}
					case TAG_EMS_IDLE_PERIOD:
					{
						std::vector<SRscpValue> container = protocol->getValueAsContainer(&EMSIdlePeriods[i]);
						for (size_t n = 0; n < container.size(); n++)
						{
							if (container[n].dataType == RSCP::eTypeError)
							{
								// handle error for example access denied errors
								uint32_t uiErrorCode = protocol->getValueAsUInt32(&container[n]);
								printf("Tag 0x%08X received error code %u.\n", container[n].tag, uiErrorCode);
								return -1;
							}
							switch (container[n].tag) {
								case TAG_EMS_IDLE_PERIOD_TYPE:
								{
									idlePeriodType = protocol->getValueAsUChar8(&container[n]);
									break;
								}
								case TAG_EMS_IDLE_PERIOD_ACTIVE:
								{
									idlePeriodActive = protocol->getValueAsBool(&container[n]);
									break;
								}
								case TAG_EMS_IDLE_PERIOD_DAY:
								{
									idlePeriodDay = protocol->getValueAsUChar8(&container[n]);
									break;
								}
								case TAG_EMS_IDLE_PERIOD_START:
								{
									std::vector<SRscpValue> container2 = protocol->getValueAsContainer(&container[n]);
									for (size_t o = 0; o < container2.size(); o++)
									{
										if (container2[o].dataType == RSCP::eTypeError)
										{
											// handle error for example access denied errors
											uint32_t uiErrorCode = protocol->getValueAsUInt32(&container2[o]);
											printf("Tag 0x%08X received error code %u.\n", container2[o].tag, uiErrorCode);
											return -1;
										}
										else if (container2[o].tag == TAG_EMS_IDLE_PERIOD_HOUR)
										{
											idlePeriodStartHour = protocol->getValueAsUChar8(&container2[o]);
										}
										else if (container2[o].tag == TAG_EMS_IDLE_PERIOD_MINUTE)
										{
											idlePeriodStartMinute = protocol->getValueAsUChar8(&container2[o]);
										}
									}
									protocol->destroyValueData(container2);
									break;
								}
								case TAG_EMS_IDLE_PERIOD_END:
								{
									std::vector<SRscpValue> container2 = protocol->getValueAsContainer(&container[n]);
									for (size_t o = 0; o < container2.size(); o++)
									{
										if (container2[o].dataType == RSCP::eTypeError)
										{
											// handle error for example access denied errors
											uint32_t uiErrorCode = protocol->getValueAsUInt32(&container2[o]);
											printf("Tag 0x%08X received error code %u.\n", container2[o].tag, uiErrorCode);
											return -1;
										}
										else if (container2[o].tag == TAG_EMS_IDLE_PERIOD_HOUR)
										{
											idlePeriodEndHour = protocol->getValueAsUChar8(&container2[o]);
										}
										else if (container2[o].tag == TAG_EMS_IDLE_PERIOD_MINUTE)
										{
											idlePeriodEndMinute = protocol->getValueAsUChar8(&container2[o]);
										}
									}
									protocol->destroyValueData(container2);
									break;
								}
								default:
								{
									printf("Found unknown tag 0x%08X on iteration {%i}{%i}\n", container[n].tag, i, n);
									break;
								}
							}
						}
						protocol->destroyValueData(container);
						break;
					}
					default:
					{
						printf("Unknown EMS Idle Period tag 0x%08X -> 0x%08X\n", response->tag, EMSIdlePeriods[i].tag);
						break;
					}
				}

				std::string name;
				if (idlePeriodType == 0) {
					name = "charge";
				} else {
					name = "discharge";
				}
				name = std::to_string(idlePeriodDay) + "_" + name;

				mainJSONObject["idle_block"][name]["day"] = idlePeriodDay;
				mainJSONObject["idle_block"][name]["active"] = idlePeriodActive;
				mainJSONObject["idle_block"][name]["type"] = idlePeriodType;
				mainJSONObject["idle_block"][name]["start"] = std::to_string(idlePeriodStartHour) + ":" + std::to_string(idlePeriodStartMinute);
				mainJSONObject["idle_block"][name]["end"] = std::to_string(idlePeriodEndHour) + ":" + std::to_string(idlePeriodEndMinute);
			}
			protocol->destroyValueData(EMSIdlePeriods);
			break;
		}
		case TAG_EMS_GET_POWER_SETTINGS:
		{
			uint8_t ucPVIIndex = 0;
			std::vector<SRscpValue> PWRSettings = protocol->getValueAsContainer(response);
			for (size_t i = 0; i < PWRSettings.size(); ++i)
			{
				if (PWRSettings[i].dataType == RSCP::eTypeError)
				{
					// handle error for example access denied errors
					uint32_t uiErrorCode = protocol->getValueAsUInt32(&PWRSettings[i]);
					printf("Tag 0x%08X received error code %u.\n", PWRSettings[i].tag, uiErrorCode);
					return -1;
				}
				switch (PWRSettings[i].tag)
				{
					case TAG_PVI_INDEX:
					{
						ucPVIIndex = protocol->getValueAsUChar8(&PWRSettings[i]);
						break;
					}
					case TAG_EMS_POWER_LIMITS_USED:
					{
						bool powerLimitsUsed = protocol->getValueAsBool(&PWRSettings[i]);
						mainJSONObject["pwrs"]["limits_used"] = powerLimitsUsed;
						break;
					}
					case TAG_EMS_MAX_CHARGE_POWER:
					{
						uint8_t maxChargePower = protocol->getValueAsUInt32(&PWRSettings[i]);
						mainJSONObject["pwrs"]["max_charge_power"] = maxChargePower;
						break;
					}
					case TAG_EMS_MAX_DISCHARGE_POWER:
					{
						uint8_t maxDischargePower = protocol->getValueAsUInt32(&PWRSettings[i]);
						mainJSONObject["pwrs"]["max_discharge_power"] = maxDischargePower;
						break;
					}
					case TAG_EMS_DISCHARGE_START_POWER:
					{
						uint8_t dischargeStartPower = protocol->getValueAsUInt32(&PWRSettings[i]);
						mainJSONObject["pwrs"]["discharge_start_power"] = dischargeStartPower;
						break;
					}
					case TAG_EMS_POWERSAVE_ENABLED:
					{
						uint8_t powersaveEnabled = protocol->getValueAsUChar8(&PWRSettings[i]);
						mainJSONObject["pwrs"]["powersave_enabled"] = powersaveEnabled;
						break;
					}
					case TAG_EMS_WEATHER_REGULATED_CHARGE_ENABLED:
					{
						uint8_t weatherRegulatedCharge = protocol->getValueAsUChar8(&PWRSettings[i]);
						mainJSONObject["pwrs"]["weather_regulated_charge"] = weatherRegulatedCharge;
						break;
					}
					default: {
						break;
					}
				}
			}
			protocol->destroyValueData(PWRSettings);
			break;
		}
		case TAG_PVI_DATA:
		{
			// resposne for TAG_PVI_REQ_DATA
			uint8_t ucPVIIndex = 0;
			std::vector<SRscpValue> PVIData = protocol->getValueAsContainer(response);
			for (size_t i = 0; i < PVIData.size(); ++i)
			{
				if (PVIData[i].dataType == RSCP::eTypeError)
				{
					// handle error for example access denied errors
					uint32_t uiErrorCode = protocol->getValueAsUInt32(&PVIData[i]);
					printf("Tag 0x%08X received error code %u.\n", PVIData[i].tag, uiErrorCode);
					return -1;
				}
				// check each battery sub tag
				switch (PVIData[i].tag)
				{
					case TAG_PVI_INDEX:
					{
						ucPVIIndex = protocol->getValueAsUChar8(&PVIData[i]);
						break;
					}
					case TAG_PVI_SYSTEM_MODE:
					{
						uint8_t systemMode = protocol->getValueAsUChar8(&PVIData[i]);
						mainJSONObject["pvi"]["system_mode"] = systemMode;
						mainJSONObject["pvi"]["system_mode_desc"] = "IdleMode = 0, / NormalMode = 1, / GridChargeMode = 2, / BackupPowerMode = 3";
						break;
					}
					case TAG_PVI_ON_GRID:
					{
						// response for TAG_PVI_REQ_ON_GRID
						bool onGrid = protocol->getValueAsBool(&PVIData[i]);
						mainJSONObject["pvi"]["on_grid"] = onGrid;
						break;
					}
					case TAG_PVI_DC_POWER:
					{
						int index = -1;
						float TAG_OUT_PVI_DC_POWER = 0;
						std::vector<SRscpValue> container = protocol->getValueAsContainer(&PVIData[i]);
						for (size_t n = 0; n < container.size(); n++)
						{
							if ((container[n].tag == TAG_PVI_INDEX))
							{
								index = protocol->getValueAsUInt16(&container[n]);
							}
							else if ((container[n].tag == TAG_PVI_VALUE))
							{
								TAG_OUT_PVI_DC_POWER = protocol->getValueAsFloat32(&container[n]);
								if (index == 0)
								{
									mainJSONObject["pvi"]["dc0_power"] = TAG_OUT_PVI_DC_POWER;
								}
								else if (index == 1)
								{
									mainJSONObject["pvi"]["dc1_power"] = TAG_OUT_PVI_DC_POWER;
								}
							}
						}
						protocol->destroyValueData(container);
						break;
					}
					case TAG_PVI_DC_VOLTAGE:
					{
						int index = -1;
						float TAG_OUT_PVI_DC_VOLTAGE = 0;
						std::vector<SRscpValue> container = protocol->getValueAsContainer(&PVIData[i]);
						for (size_t n = 0; n < container.size(); n++)
						{
							if ((container[n].tag == TAG_PVI_INDEX))
							{
								index = protocol->getValueAsUInt16(&container[n]);
							}
							else if ((container[n].tag == TAG_PVI_VALUE))
							{
								TAG_OUT_PVI_DC_VOLTAGE = protocol->getValueAsFloat32(&container[n]);
								if (index == 0)
								{
									mainJSONObject["pvi"]["dc0_voltage"] = TAG_OUT_PVI_DC_VOLTAGE;
								}
								if (index == 1)
								{
									mainJSONObject["pvi"]["dc1_voltage"] = TAG_OUT_PVI_DC_VOLTAGE;
								}
							}
						}
						protocol->destroyValueData(container);
						break;
					}
					case TAG_PVI_DC_CURRENT:
					{
						int index = -1;
						float TAG_OUT_PVI_DC_CURRENT = 0;
						std::vector<SRscpValue> container = protocol->getValueAsContainer(&PVIData[i]);
						for (size_t n = 0; n < container.size(); n++)
						{
							if ((container[n].tag == TAG_PVI_INDEX))
							{
								index = protocol->getValueAsUInt16(&container[n]);
							}
							else if ((container[n].tag == TAG_PVI_VALUE))
							{
								TAG_OUT_PVI_DC_CURRENT = protocol->getValueAsFloat32(&container[n]);
								if (index == 0)
								{
									mainJSONObject["pvi"]["dc0_current"] = TAG_OUT_PVI_DC_CURRENT;
								}
								if (index == 1)
								{
									mainJSONObject["pvi"]["dc1_current"] = TAG_OUT_PVI_DC_CURRENT;
								}
							}
						}
						protocol->destroyValueData(container);
						break;
					}
					// ...
					default:
						// default behaviour
						printf("Unknown PVI tag %08X\n", response->tag);
						break;
				}
			}
			protocol->destroyValueData(PVIData);
			break;
		}
		// ...
		default:
			// default behavior
			printf("Unknown tag %08X\n", response->tag);
			break;
	}

}

static int processReceiveBuffer(const unsigned char * ucBuffer, int iLength)
{
	RscpProtocol protocol;
	SRscpFrame frame;

	int iResult = protocol.parseFrame(ucBuffer, iLength, &frame);
	if(iResult < 0) {
		// check if frame length error occured
		// in that case the full frame length was not received yet
		// and the receive function must get more data
		if(iResult == RSCP::ERR_INVALID_FRAME_LENGTH) {
			return 0;
		}
		// otherwise a not recoverable error occured and the connection can be closed
		else {
			return iResult;
		}
	}

	int iProcessedBytes = iResult;

	// process each SRscpValue struct seperately
	for(unsigned int i; i < frame.data.size(); i++) {
		handleResponseValue(&protocol, &frame.data[i]);
	}

	// Write data to json file if data was correctly received
	if (gotData) {
		std::ofstream o(TARGET_FILE);
		o << std::setw(4) << mainJSONObject << std::endl;
		gotData = false;
		gotDataFailed = 0;
	} else {
		// Increase failure counter
		++gotDataFailed;

		// If failure appeared multiple times in a row we have a basic communication problem with E3DC
		// Exit application. Start script should restart this program shortly after exit
		if (gotDataFailed > 10) {
			printf("Failed to receive data multiple times. Exiting with error");
			exit(EXIT_FAILURE);
		}
	}

	// destroy frame data and free memory
	protocol.destroyFrameData(frame);

	// returned processed amount of bytes
	return iProcessedBytes;
}

static void receiveLoop(bool & bStopExecution)
{
	//--------------------------------------------------------------------------------------------------------------
	// RSCP Receive Frame Block Data
	//--------------------------------------------------------------------------------------------------------------
	// setup a static dynamic buffer which is dynamically expanded (re-allocated) on demand
	// the data inside this buffer is not released when this function is left
	static int iReceivedBytes = 0;
	static std::vector<uint8_t> vecDynamicBuffer;

	// check how many RSCP frames are received, must be at least 1
	// multiple frames can only occur in this example if one or more frames are received with a big time delay
	// this should usually not occur but handling this is shown in this example
	int iReceivedRscpFrames = 0;
	while(!bStopExecution && ((iReceivedBytes > 0) || iReceivedRscpFrames == 0))
	{
		// check and expand buffer
		if((vecDynamicBuffer.size() - iReceivedBytes) < 4096) {
			// check maximum size
			if(vecDynamicBuffer.size() > RSCP_MAX_FRAME_LENGTH) {
				// something went wrong and the size is more than possible by the RSCP protocol
				printf("Maximum buffer size exceeded %i\n", vecDynamicBuffer.size());
				bStopExecution = true;
				break;
			}
			// increase buffer size by 4096 bytes each time the remaining size is smaller than 4096
			vecDynamicBuffer.resize(vecDynamicBuffer.size() + 4096);
		}
		// receive data
		int iResult = SocketRecvData(iSocket, &vecDynamicBuffer[0] + iReceivedBytes, vecDynamicBuffer.size() - iReceivedBytes);
		if(iResult < 0)
		{
			// check errno for the error code to detect if this is a timeout or a socket error
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				// receive timed out -> continue with re-sending the initial block
				printf("Response receive timeout (retry)\n");
				break;
			}
			// socket error -> check errno for failure code if needed
			printf("Socket receive error. errno %i\n", errno);
			bStopExecution = true;
			break;
		}
		else if(iResult == 0)
		{
			// connection was closed regularly by peer
			// if this happens on startup each time the possible reason is
			// wrong AES password or wrong network subnet (adapt hosts.allow file required)
			printf("Connection closed by peer\n");
			bStopExecution = true;
			break;
		}
		// increment amount of received bytes
		iReceivedBytes += iResult;

		// process all received frames
		while (!bStopExecution)
		{
			// round down to a multiple of AES_BLOCK_SIZE
			int iLength = ROUNDDOWN(iReceivedBytes, AES_BLOCK_SIZE);
			// if not even 32 bytes were received then the frame is still incomplete
			if(iLength == 0) {
				break;
			}
			// resize temporary decryption buffer
			std::vector<uint8_t> decryptionBuffer;
			decryptionBuffer.resize(iLength);
			// initialize encryption sequence IV value with value of previous block
			aesDecrypter.SetIV(ucDecryptionIV, AES_BLOCK_SIZE);
			// decrypt data from vecDynamicBuffer to temporary decryptionBuffer
			aesDecrypter.Decrypt(&vecDynamicBuffer[0], &decryptionBuffer[0], iLength / AES_BLOCK_SIZE);

			// data was received, check if we received all data
			int iProcessedBytes = processReceiveBuffer(&decryptionBuffer[0], iLength);
			if(iProcessedBytes < 0) {
				// an error occured;
				printf("Error parsing RSCP frame: %i\n", iProcessedBytes);
				// stop execution as the data received is not RSCP data
				bStopExecution = true;
				break;

			}
			else if(iProcessedBytes > 0) {
				// round up the processed bytes as iProcessedBytes does not include the zero padding bytes
				iProcessedBytes = ROUNDUP(iProcessedBytes, AES_BLOCK_SIZE);
				// store the IV value from encrypted buffer for next block decryption
				memcpy(ucDecryptionIV, &vecDynamicBuffer[0] + iProcessedBytes - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
				// move the encrypted data behind the current frame data (if any received) to the front
				memcpy(&vecDynamicBuffer[0], &vecDynamicBuffer[0] + iProcessedBytes, vecDynamicBuffer.size() - iProcessedBytes);
				// decrement the total received bytes by the amount of processed bytes
				iReceivedBytes -= iProcessedBytes;
				// increment a counter that a valid frame was received and
				// continue parsing process in case a 2nd valid frame is in the buffer as well
				iReceivedRscpFrames++;
			}
			else {
				// iProcessedBytes is 0
				// not enough data of the next frame received, go back to receive mode if iReceivedRscpFrames == 0
				// or transmit mode if iReceivedRscpFrames > 0
				break;
			}
		}
	}
}

void process_mem_usage(double &vm_usage, double &resident_set)
{
	vm_usage = 0.0;
	resident_set = 0.0;

	// the two fields we want
	unsigned long vsize;
	long rss;
	{
		std::string ignore;
		std::ifstream ifs("/proc/self/stat", std::ios_base::in);
		ifs >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> vsize >> rss;
	}

	long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
	vm_usage = vsize / 1024.0;
	resident_set = rss * page_size_kb;
}

static void mainLoop(void)
{
	RscpProtocol protocol;
	bool bStopExecution = false;

	while(!bStopExecution)
	{
		//--------------------------------------------------------------------------------------------------------------
		// RSCP Transmit Frame Block Data
		//--------------------------------------------------------------------------------------------------------------
		SRscpFrameBuffer frameBuffer;
		memset(&frameBuffer, 0, sizeof(frameBuffer));

		// create an RSCP frame with requests to some example data
		createRequestExample(&frameBuffer);

		// check that frame data was created
		if(frameBuffer.dataLength > 0)
		{
			// resize temporary encryption buffer to a multiple of AES_BLOCK_SIZE
			std::vector<uint8_t> encryptionBuffer;
			encryptionBuffer.resize(ROUNDUP(frameBuffer.dataLength, AES_BLOCK_SIZE));
			// zero padding for data above the desired length
			memset(&encryptionBuffer[0] + frameBuffer.dataLength, 0, encryptionBuffer.size() - frameBuffer.dataLength);
			// copy desired data length
			memcpy(&encryptionBuffer[0], frameBuffer.data, frameBuffer.dataLength);
			// set continues encryption IV
			aesEncrypter.SetIV(ucEncryptionIV, AES_BLOCK_SIZE);
			// start encryption from encryptionBuffer to encryptionBuffer, blocks = encryptionBuffer.size() / AES_BLOCK_SIZE
			aesEncrypter.Encrypt(&encryptionBuffer[0], &encryptionBuffer[0], encryptionBuffer.size() / AES_BLOCK_SIZE);
			// save new IV for next encryption block
			memcpy(ucEncryptionIV, &encryptionBuffer[0] + encryptionBuffer.size() - AES_BLOCK_SIZE, AES_BLOCK_SIZE);

			// send data on socket
			int iResult = SocketSendData(iSocket, &encryptionBuffer[0], encryptionBuffer.size());
			if(iResult < 0) {
				printf("Socket send error %i. errno %i\n", iResult, errno);
				bStopExecution = true;
			}
			else {
				// go into receive loop and wait for response
				receiveLoop(bStopExecution);
			}
		}
		// free frame buffer memory
		protocol.destroyFrameData(&frameBuffer);

		// Print periodic statistics about memory consumption (yeah, looks like we could have a memory-leak)
		if (printStats >= 300) {
			// Get current memory consumption
			double vm, rss;
			process_mem_usage(vm, rss);

			printStats = 0;
			mainJSONObject["prog"]["mem_vm"] = vm;
			mainJSONObject["prog"]["mem_rss"] = rss;
		} else {
			++printStats;
		}

		// main loop sleep / cycle time before next request
		sleep(FETCH_INTERVAL);
	}
}

int main(int argc, char *argv[])
{
	// endless application which re-connections to server on connection lost
	while(true)
	{
		// connect to server
		printf("Connecting to server %s:%i\n", SERVER_IP, SERVER_PORT);
		iSocket = SocketConnect(SERVER_IP, SERVER_PORT);
		if(iSocket < 0) {
			printf("Connection failed\n");
			sleep(1);
			continue;
		}
		printf("Connected successfully\n");

		// reset authentication flag
		iAuthenticated = 0;

		// create AES key and set AES parameters
		{
			// initialize AES encryptor and decryptor IV
			memset(ucDecryptionIV, 0xff, AES_BLOCK_SIZE);
			memset(ucEncryptionIV, 0xff, AES_BLOCK_SIZE);

			// limit password length to AES_KEY_SIZE
			int iPasswordLength = strlen(AES_PASSWORD);
			if(iPasswordLength > AES_KEY_SIZE)
				iPasswordLength = AES_KEY_SIZE;

			// copy up to 32 bytes of AES key password
			uint8_t ucAesKey[AES_KEY_SIZE];
			memset(ucAesKey, 0xff, AES_KEY_SIZE);
			memcpy(ucAesKey, AES_PASSWORD, iPasswordLength);

			// set encryptor and decryptor parameters
			aesDecrypter.SetParameters(AES_KEY_SIZE * 8, AES_BLOCK_SIZE * 8);
			aesEncrypter.SetParameters(AES_KEY_SIZE * 8, AES_BLOCK_SIZE * 8);
			aesDecrypter.StartDecryption(ucAesKey);
			aesEncrypter.StartEncryption(ucAesKey);
		}

		// enter the main transmit / receive loop
		mainLoop();

		// close socket connection
		SocketClose(iSocket);
		iSocket = -1;
	}

	return 0;
}
