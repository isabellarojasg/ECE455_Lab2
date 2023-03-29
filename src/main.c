
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "linked_task_list.h"
#include "stm32f4_discovery.h"
/* Kernel includes. */
#include "stm32f4xx.h"
#include "../FreeRTOS_Source/include/FreeRTOS.h"
#include "../FreeRTOS_Source/include/queue.h"
#include "../FreeRTOS_Source/include/semphr.h"
#include "../FreeRTOS_Source/include/task.h"
#include "../FreeRTOS_Source/include/timers.h"


/*-----------------------------------------------------------*/
#define mainQUEUE_LENGTH 16

/*
 * TODO: Implement this function for any hardware specific clock configuration
 * that was not already performed before main() was called.
 */
static void prvSetupHardware( void );

//enum task_type {PERIODIC, APERIODIC};
enum message_type {NEW_TASK, COMPLETED_TASK, ACTIVE_LIST_REQ, COMP_LIST_REQ, OD_LIST_REQ};

struct dds_message {
	enum message_type type;
	struct dd_task task;
};

static void dds_task(void *pvParameters);
static void generator_task1(void *pvParameters);
static void generator_task2(void *pvParameters);
static void generator_task3(void *pvParameters);
static void monitor_task(void *pvParameters);
static void usd_task1(void *pvParameters);
static void usd_task2(void *pvParameters);
static void usd_task3(void *pvParameters);
static struct dd_task_node* get_active_dd_task_list();
static struct dd_task_node* get_completed_dd_task_list();
static struct dd_task_node* get_overdue_dd_task_list();
static void task1_timer_callback(TimerHandle_t xTimer);
static void task2_timer_callback(TimerHandle_t xTimer);
static void task3_timer_callback(TimerHandle_t xTimer);

TaskHandle_t generator_handle1;
TaskHandle_t generator_handle2;
TaskHandle_t generator_handle3;

QueueHandle_t xQueue_DDS_Messages;
QueueHandle_t xQueue_List_Response;

TimerHandle_t xTimer_Task1_Generator;
TimerHandle_t xTimer_Task2_Generator;
TimerHandle_t xTimer_Task3_Generator;

uint8_t active_list	= 0;
uint8_t completed_list = 1;
uint8_t overdue_list = 2;
uint32_t curr_task_id = 0;
/*-----------------------------------------------------------*/

int main(void)
{

	/* Initialize LEDs */

	/* Configure the system ready to run the demo.  The clock configuration
	can be done here if it was not done before main() was called. */
	prvSetupHardware();

	/* Create the queues */
	xQueue_DDS_Messages = xQueueCreate(mainQUEUE_LENGTH,	/* The number of items the queue can hold. */
									sizeof( struct dds_message ) );	/* The size of each item the queue holds. */
	xQueue_List_Response = xQueueCreate(mainQUEUE_LENGTH,	/* The number of items the queue can hold. */
												sizeof(struct dd_task_node* ) );	/* The size of each item the queue holds. */

	/* Add to the registry, for the benefit of kernel aware debugging. */
	vQueueAddToRegistry( xQueue_DDS_Messages, "DDS Message Queue" );	//holds messages for DDS to schedule
	vQueueAddToRegistry( xQueue_List_Response, "List Response Queue" );	//holds copies of the current states of the DDS task lists

	//TODO:set period of timer
	xTimer_Task1_Generator = xTimerCreate("Task1 Generation Timer", pdMS_TO_TICKS(500), pdTRUE, 0, task1_timer_callback);
	xTimer_Task2_Generator = xTimerCreate("Task2 Generation Timer", pdMS_TO_TICKS(500), pdTRUE, 0, task2_timer_callback);
	xTimer_Task3_Generator = xTimerCreate("Task3 Generation Timer", pdMS_TO_TICKS(500), pdTRUE, 0, task3_timer_callback);


	xTaskCreate( dds_task, "Deadline Driven Scheduler", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
	xTaskCreate( generator_task1, "Deadline-Driven Task Generator 1", configMINIMAL_STACK_SIZE, NULL, 4, &generator_handle1);
	xTaskCreate( generator_task2, "Deadline-Driven Task Generator 2", configMINIMAL_STACK_SIZE, NULL, 4, &generator_handle2);
	xTaskCreate( generator_task3, "Deadline-Driven Task Generator 3", configMINIMAL_STACK_SIZE, NULL, 4, &generator_handle3);
	//xTaskCreate( monitor_task, "Monitor Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

	/* Start the tasks and timer running. */
	xTimerStart(xTimer_Task1_Generator, portMAX_DELAY);
	xTimerStart(xTimer_Task2_Generator, portMAX_DELAY);
	xTimerStart(xTimer_Task3_Generator, portMAX_DELAY);
	vTaskStartScheduler();

	return 0;
}

/*-----------------------------------------------------------*/

//Creates the dd_task struct
void release_dd_task(struct dd_task new_task){
	struct dds_message new_message;
	new_message.type = NEW_TASK;
	new_message.task = new_task;
	printf("r_dd_t id: %u\n", new_task.task_id);
	xQueueSend(xQueue_DDS_Messages, &new_message, 1000);
}

void complete_dd_task(uint32_t task_id){
	struct dd_task comp_task;
	comp_task.task_id = task_id;
	struct dds_message new_message;
	new_message.type = COMPLETED_TASK;
	new_message.task = comp_task;
	xQueueSend(xQueue_DDS_Messages, &new_message, 1000);
}

static void dds_task(void *pvParameters){
	struct dd_task_node* active_dd_tasks = NULL;
	struct dd_task_node* completed_dd_tasks = NULL;
	struct dd_task_node* overdue_dd_tasks = NULL;
	struct dds_message message;
	BaseType_t xStatus;

	for(;;){
		xStatus = xQueueReceive(xQueue_DDS_Messages, &message, portMAX_DELAY); //check new_task queue
		if(xStatus == pdPASS){
			switch(message.type){
				case NEW_TASK:
					message.task.release_time = xTaskGetTickCount();	//assign release time
					struct dd_task_node* new_task_node = (struct dd_task_node*)pvPortMalloc(sizeof(struct dd_task_node));
					new_task_node->task = message.task;
					new_task_node->next_task = NULL;
					printf("msg:\nid: %u\nabs: %u\n", message.task.task_id, message.task.absolute_deadline);

					if(active_dd_tasks) vTaskPrioritySet(active_dd_tasks->task.t_handle, 1);
					listInsert(&active_dd_tasks, &new_task_node);	//add to active task list

					//sort active list and set priorities of user tasks
					mergeSortByDeadline(&active_dd_tasks);
					vTaskPrioritySet(active_dd_tasks->task.t_handle, 2);
					break;
				case COMPLETED_TASK:
					active_dd_tasks->task.completion_time = xTaskGetTickCount();	//assign completion time. TODO: covert to ms??
					moveTask(&active_dd_tasks, &completed_dd_tasks);	//remove from active list and add to completed list todo: should this move according to id rather than just move the one @ the front??

					//remove according to id or???? shouldn't it just always be the one at the front of the list?
					vTaskDelete(completed_dd_tasks->task.t_handle);

					//sort active list and set priorities of user tasks
					if(active_dd_tasks){
						vTaskPrioritySet(active_dd_tasks->task.t_handle, 1); //not necessary if assuming that task at front of active list is always the one that completed
						mergeSortByDeadline(&active_dd_tasks);
						vTaskPrioritySet(active_dd_tasks->task.t_handle, 2);
					}
					break;
				case ACTIVE_LIST_REQ:
					xQueueSend(xQueue_List_Response, &active_dd_tasks, 500);
					break;
				case COMP_LIST_REQ:
					xQueueSend(xQueue_List_Response, &completed_dd_tasks, 500);
					break;
				case OD_LIST_REQ:
					xQueueSend(xQueue_List_Response, &overdue_dd_tasks, 500);
					break;
				default:
					//error
					printf("error\n");
			}
		}
	}
}

static void task1_timer_callback(TimerHandle_t xTimer){
	BaseType_t xYieldRequired = xTaskResumeFromISR(generator_handle1);
	portYIELD_FROM_ISR(xYieldRequired);
}

static void task2_timer_callback(TimerHandle_t xTimer){
	BaseType_t xYieldRequired = xTaskResumeFromISR(generator_handle2);
	portYIELD_FROM_ISR(xYieldRequired);
}

static void task3_timer_callback(TimerHandle_t xTimer){
	BaseType_t xYieldRequired = xTaskResumeFromISR(generator_handle3);
	portYIELD_FROM_ISR(xYieldRequired);
}


static void generator_task1(void *pvParameters){
	struct dd_task new_task;
	TaskHandle_t* usd_task_handle = (TaskHandle_t*)pvPortMalloc(sizeof(TaskHandle_t));
	uint32_t task_period = pdMS_TO_TICKS(500);

	for(;;){
		new_task.task_id = curr_task_id++;
		xTaskCreate( usd_task1, "User-Defined Task", configMINIMAL_STACK_SIZE, (void *) new_task.task_id, 1, usd_task_handle);
		new_task.t_handle = *usd_task_handle;
		new_task.absolute_deadline = xTaskGetTickCount() + task_period;
		release_dd_task(new_task);
		vTaskSuspend(NULL);
	}

}

static void generator_task2(void *pvParameters){
	struct dd_task new_task;
	TaskHandle_t* usd_task_handle= (TaskHandle_t*)pvPortMalloc(sizeof(TaskHandle_t));
	uint32_t task_period = pdMS_TO_TICKS(500);

	for(;;){
		new_task.task_id = curr_task_id++;
		xTaskCreate( usd_task2, "User-Defined Task", configMINIMAL_STACK_SIZE, (void *) new_task.task_id, 1, usd_task_handle);
		new_task.t_handle = *usd_task_handle;
		new_task.absolute_deadline = xTaskGetTickCount() + task_period;
		release_dd_task(new_task);
		vTaskSuspend(NULL);
	}

}

static void generator_task3(void *pvParameters){
	struct dd_task new_task;
	TaskHandle_t* usd_task_handle = (TaskHandle_t*)pvPortMalloc(sizeof(TaskHandle_t));
	uint32_t task_period = pdMS_TO_TICKS(500);

	for(;;){
		new_task.task_id = curr_task_id++;
		xTaskCreate( usd_task3, "User-Defined Task", configMINIMAL_STACK_SIZE, (void *) new_task.task_id, 1, usd_task_handle);
		new_task.t_handle = *usd_task_handle;
		new_task.absolute_deadline = xTaskGetTickCount() + task_period;
		release_dd_task(new_task);
		vTaskSuspend(NULL);
	}

}

static void monitor_task(void *pvParameters){
	struct dd_task_node *listCopy;
	for(;;){
		listCopy = get_active_dd_task_list();
		printList(listCopy);
		printf("active list len: %u\n", getListLength(listCopy));
		listCopy = get_completed_dd_task_list();
		printList(listCopy);
		printf("comp list len: %u\n", getListLength(listCopy));
		listCopy = get_overdue_dd_task_list();
		printList(listCopy);
		printf("od list len: %u\n", getListLength(listCopy));
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

static void usd_task1(void *pvParameters){
	for(;;){
		for(int i = 0; i < pdMS_TO_TICKS(95); i++);
		printf("done usdt1\n");
		complete_dd_task((uint32_t) pvParameters);
	}

}

static void usd_task2(void *pvParameters){
	for(;;){
		for(int i = 0; i < pdMS_TO_TICKS(150); i++);
		printf("done usdt2\n");
		complete_dd_task((uint32_t) pvParameters);
	}
}

static void usd_task3(void *pvParameters){
	for(;;){
		for(int i = 0; i < pdMS_TO_TICKS(250); i++);
		printf("done usdt3\n");
		complete_dd_task((uint32_t) pvParameters);
	}
}

static struct dd_task_node* get_active_dd_task_list(){
	struct dd_task_node* listCopy;
	struct dds_message new_msg;
	new_msg.type = ACTIVE_LIST_REQ;
	xQueueSend(xQueue_DDS_Messages, &new_msg, 1000);
	xQueueReceive(xQueue_List_Response, &listCopy, 1000);
	return listCopy;
}

static struct dd_task_node* get_completed_dd_task_list(){
	struct dd_task_node* listCopy;
	struct dds_message new_msg;
	new_msg.type = COMP_LIST_REQ;
	xQueueSend(xQueue_DDS_Messages, &new_msg, 1000);
	xQueueReceive(xQueue_List_Response, &listCopy, 1000);
	return listCopy;
}

static struct dd_task_node* get_overdue_dd_task_list(){
	struct dd_task_node* listCopy;
	struct dds_message new_msg;
	new_msg.type = OD_LIST_REQ;
	xQueueSend(xQueue_DDS_Messages, &new_msg, 1000);
	xQueueReceive(xQueue_List_Response, &listCopy, 1000);
	return listCopy;
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* The malloc failed hook is enabled by setting
	   configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.
	Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.  pxCurrentTCB can be
	inspected in the debugger if the task name passed into this function is
	corrupt. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* The idle task hook is enabled by setting configUSE_IDLE_HOOK to 1 in
	FreeRTOSConfig.h.
	This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amount of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
			/* By now, the kernel has allocated everything it is going to, so
			if there is a lot of heap remaining unallocated then
			the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
			reduced accordingly. */
	}
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Ensure all priority bits are assigned as preemption priority bits.
	http://www.freertos.org/RTOS-Cortex-M3-M4.html */
	NVIC_SetPriorityGrouping( 0 );

	/* TODO: Setup the clocks, etc. here, if they were not configured before
	main() was called. */
}
