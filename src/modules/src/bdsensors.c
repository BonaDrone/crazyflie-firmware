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

#include "console.h"
#include "cfassert.h"
#include "debug.h"

static CRTPPacket p;

static bool isInit = false;

static xSemaphoreHandle bdsensorsLock;

//Private functions
static void bdsensorsTask(void * prm);

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
		ledseqRun(SYS_LED, seq_testPassed);
		ledseqRun(LINK_LED, seq_testPassed);
		static float hardFloat = 1.5;

		xSemaphoreTake(bdsensorsLock, portMAX_DELAY);

        memcpy(&p.data[0], (char*)&hardFloat, 4);
		p.size=4;
		crtpSendPacket(&p);
		xSemaphoreGive(bdsensorsLock);




		/*
		 xSemaphoreTake(logLock, portMAX_DELAY);
		 if (p.channel==TOC_CH)
		 logTOCProcess(p.data[0]);
		 if (p.channel==CONTROL_CH)
		 logControlProcess();
		 xSemaphoreGive(logLock);
		 */
	}
}

