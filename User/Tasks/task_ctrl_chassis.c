/*
	底盘任务，用于控制底盘行为。
	
*/


/* Includes ------------------------------------------------------------------*/
/* Include 自身的头文件，OS头文件。*/
#include "task_debug.h"
#include "cmsis_os2.h"

/* Include Board相关的头文件。*/
#include "io.h"

/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
#include "mecanum.h"

/* Include Module相关的头文件。*/
#include "chassis.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TASK_CTRLCHASSIS_FREQ_HZ (50)
#define TASK_CTRLCHASSIS_INIT_DELAY (500)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


void Task_CtrlChassis(const void* argument) {
	uint32_t delay_tick = 1000U / TASK_CTRLCHASSIS_FREQ_HZ;
	Chassis_t chassis;
	/* 处理硬件相关的初始化。*/
	
	
	/* 初始化完成后等待一段时间后再开始任务。*/
	osDelay(TASK_CTRLCHASSIS_INIT_DELAY);
	while(1) {
		/* 任务主体。*/
		
		
		osDelayUntil(delay_tick);
	}
}