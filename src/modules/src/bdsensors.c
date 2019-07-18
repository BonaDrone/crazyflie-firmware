/*
 * bd_sensors.c
 *
 *  Created on: Jul 15, 2019
 *      Author: bitcraze
 */

#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

/* FreeRtos includes */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "config.h"
#include "crtp.h"
#include "bdsensors.h"
#include "crc.h"
#include "worker.h"
#include "num.h"
#include "ledseq.h"
#include "pm.h"

#include "console.h"
#include "cfassert.h"
#include "debug.h"

enum cmd_message{
	HardFloat = 0,
	BatteryLevel = 1
};

static CRTPPacket p;

static bool isInit = false;

static xSemaphoreHandle bdsensorsLock;

//Private functions
static void bdsensorsTask(void * prm);
static void bdsensorsProcess(int cmd_sensor);


void bdsensorsInit(void) {

	if (isInit)
		return;

	// Big lock that protects the log datastructures
	bdsensorsLock = xSemaphoreCreateMutex();

	//Start the log task
	xTaskCreate(bdsensorsTask, BD_SENSORS_TASK_NAME, BD_SENSORS_TASK_STACKSIZE,
			NULL, BD_SENSORS_TASK_PRI, NULL);

	isInit = true;
    ledseqRun(LINK_LED, seq_testPassed);

}

void bdsensorsTask(void * prm) {
	crtpInitTaskQueue(CRTP_PORT_BD);

	while (1) {
		crtpReceivePacketBlock(CRTP_PORT_BD, &p);

		xSemaphoreTake(bdsensorsLock, portMAX_DELAY);
		bdsensorsProcess(p.data[0]);
		xSemaphoreGive(bdsensorsLock);

	}
}

/*
 * This function evaluates the message received from the Bonadrone Platform and send the adequate
 * input: cmd_sensor  sensor to be read
 */
void bdsensorsProcess(int cmd_sensor){

	switch (cmd_sensor)
	{
	case HardFloat:
	{
		float hardFloat = hardFloat = 1.5;
		ledseqRun(SYS_LED, seq_testPassed);
		memcpy(&p.data[1], (char*)&hardFloat, 4);  //as in line 145 from info.c
		//(char*) makes the pointer behave like a char pointer
		p.size=5;
		crtpSendPacket(&p);
		break;
	}
	case BatteryLevel:
	{
		float battery = pmGetBatteryVoltage();
		ledseqRun(LINK_LED, seq_testPassed);
		memcpy(&p.data[1], (char*)&battery, 4);
		p.size=5;
		crtpSendPacket(&p);
		break;
	}
	default:
		break;
	}

}


