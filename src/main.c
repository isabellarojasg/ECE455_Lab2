/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved
    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.
    This file is part of the FreeRTOS distribution.
    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.
    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html
    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************
    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wwrong?".  Have you
    defined configASSERT()?
    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.
    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.
    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.
    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.
    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.
    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.
    1 tab == 4 spaces!
*/

/*
FreeRTOS is a market leading RTOS from Real Time Engineers Ltd. that supports
31 architectures and receives 77500 downloads a year. It is professionally
developed, strictly quality controlled, robust, supported, and free to use in
commercial products without any requirement to expose your proprietary source
code.
This simple FreeRTOS demo does not make use of any IO ports, so will execute on
any Cortex-M3 of Cortex-M4 hardware.  Look for TODO markers in the code for
locations that may require tailoring to, for example, include a manufacturer
specific header file.
This is a starter project, so only a subset of the RTOS features are
demonstrated.  Ample source comments are provided, along with web links to
relevant pages on the http://www.FreeRTOS.org site.
Here is a description of the project's functionality:
The main() Function:
main() creates the tasks and software timers described in this section, before
starting the scheduler.
The Queue Send Task:
The queue send task is implemented by the prvQueueSendTask() function.
The task uses the FreeRTOS vTaskDelayUntil() and xQueueSend() API functions to
periodically send the number 100 on a queue.  The period is set to 200ms.  See
the comments in the function for more details.
http://www.freertos.org/vtaskdelayuntil.html
http://www.freertos.org/a00117.html
The Queue Receive Task:
The queue receive task is implemented by the prvQueueReceiveTask() function.
The task uses the FreeRTOS xQueueReceive() API function to receive values from
a queue.  The values received are those sent by the queue send task.  The queue
receive task increments the ulCountOfItemsReceivedOnQueue variable each time it
receives the value 100.  Therefore, as values are sent to the queue every 200ms,
the value of ulCountOfItemsReceivedOnQueue will increase by 5 every second.
http://www.freertos.org/a00118.html
An example software timer:
A software timer is created with an auto reloading period of 1000ms.  The
timer's callback function increments the ulCountOfTimerCallbackExecutions
variable each time it is called.  Therefore the value of
ulCountOfTimerCallbackExecutions will count seconds.
http://www.freertos.org/RTOS-software-timer.html
The FreeRTOS RTOS tick hook (or callback) function:
The tick hook function executes in the context of the FreeRTOS tick interrupt.
The function 'gives' a semaphore every 500th time it executes.  The semaphore
is used to synchronise with the event semaphore task, which is described next.
The event semaphore task:
The event semaphore task uses the FreeRTOS xSemaphoreTake() API function to
wait for the semaphore that is given by the RTOS tick hook function.  The task
increments the ulCountOfReceivedSemaphores variable each time the semaphore is
received.  As the semaphore is given every 500ms (assuming a tick frequency of
1KHz), the value of ulCountOfReceivedSemaphores will increase by 2 each second.
The idle hook (or callback) function:
The idle hook function queries the amount of free FreeRTOS heap space available.
See vApplicationIdleHook().
The malloc failed and stack overflow hook (or callback) functions:
These two hook functions are provided as examples, but do not contain any
functionality.
*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "stm32f4_discovery.h"
/* Kernel includes. */
#include "stm32f4xx.h"
#include "../FreeRTOS_Source/include/FreeRTOS.h"
#include "../FreeRTOS_Source/include/queue.h"
#include "../FreeRTOS_Source/include/semphr.h"
#include "../FreeRTOS_Source/include/task.h"
#include "../FreeRTOS_Source/include/timers.h"



/*-----------------------------------------------------------*/
#define mainQUEUE_LENGTH 1

/*
 * TODO: Implement this function for any hardware specific clock configuration
 * that was not already performed before main() was called.
 */
static void prvSetupHardware( void );

enum task_type {PERIODIC, APERIODIC};
enum message_type {NEW_TASK, COMPLETED_TASK, ACTIVE_LIST_REQ, COMP_LIST_REQ, OD_LIST_REQ};

struct dd_task {
	TaskHandle_t t_handle;
	enum task_type type;	//is this redeclaring task_type or?
	uint32_t task_id;
	uint32_t release_time;
	uint32_t absolute_deadline;
	uint32_t completion_time;
};

struct dd_task_list {
	struct dd_task task;	//again, should have struct here??
	struct dd_task_list *next_task;
};

struct dds_message {
	enum message_type type;
	struct dd_task task;
};

/*
 * The queue send and receive tasks as described in the comments at the top of
 * this file.
 */
static void dds_task(void *pvParameters);
static void generator_task(void *pvParameters);
static void monitor_task(void *pvParameters);
static void usd_task(void *pvParameters);
static struct dd_task_list* get_active_dd_task_list();
static struct dd_task_list* get_completed_dd_task_list();
static struct dd_task_list* get_overdue_dd_task_list();
static void task1_timer_callback(TimerHandle_t xTimer);

void listInsert(struct dd_task_list* head, struct dd_task new_task);
void printList(struct dd_task_list *list);
unsigned int getListLength(struct dd_task_list* list);

TaskHandle_t generator_handle;

QueueHandle_t xQueue_DDS_Messages;
QueueHandle_t xQueue_List_Response;

TimerHandle_t xTimer_Task1_Generator;

uint8_t active_list	= 0;
uint8_t completed_list = 1;
uint8_t overdue_list = 2;
/*-----------------------------------------------------------*/

int main(void)
{

	/* Initialize LEDs */

	/* Configure the system ready to run the demo.  The clock configuration
	can be done here if it was not done before main() was called. */
	prvSetupHardware();


	/* Create the queue used by the queue send and queue receive tasks.
	http://www.freertos.org/a00116.html */
	xQueue_DDS_Messages = xQueueCreate(mainQUEUE_LENGTH,	/* The number of items the queue can hold. */
									sizeof( uint16_t ) );	/* The size of each item the queue holds. */
	xQueue_List_Response = xQueueCreate(mainQUEUE_LENGTH,	/* The number of items the queue can hold. */
												sizeof( uint16_t ) );	/* The size of each item the queue holds. */

	/* Add to the registry, for the benefit of kernel aware debugging. */
	vQueueAddToRegistry( xQueue_DDS_Messages, "DDS Message Queue" );	//holds messages for DDS to schedule
	vQueueAddToRegistry( xQueue_List_Response, "List Response Queue" );	//holds copies of the current states of the DDS task lists

	//TODO:set period of timer
	xTimer_Task1_Generator = xTimerCreate("Task1 Generation Timer", pdMS_TO_TICKS(500), pdFALSE, 0, task1_timer_callback);

	xTaskCreate( dds_task, "Deadline Driven Scheduler", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate( generator_task, "Deadline-Driven Task Generator", configMINIMAL_STACK_SIZE, NULL, 3, generator_handle);
	xTaskCreate( monitor_task, "Monitor Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
//	xTaskCreate( get_active_dd_task_list, "Get active task list", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
//	xTaskCreate( get_completed_dd_task_list, "Get completed task list", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
//	xTaskCreate( get_overdue_dd_task_list, "Get overdue task list", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

	/* Start the tasks and timer running. */
	vTaskStartScheduler();
	xTimerStart(xTimer_Task1_Generator, portMAX_DELAY);

	return 0;
}

/*-----------------------------------------------------------*/

//Creates the dd_task struct
void release_dd_task(struct dd_task new_task){
	struct dds_message new_message;
	new_message.type = NEW_TASK;
	new_message.task = new_task;
	xQueueSend(xQueue_DDS_Messages, &new_message, 1000);
}

void complete_dd_task(uint32_t task_id){

}

static void dds_task(void *pvParameters){
	struct dd_task_list* active_dd_tasks = NULL;
	struct dd_task_list* completed_dd_tasks = NULL;
	struct dd_task_list* overdue_dd_tasks = NULL;
	struct dds_message message;

	for(;;){
		xQueueReceive(xQueue_DDS_Messages, &message, 1000); //check new_task queue
		switch(message.type){
		case NEW_TASK:
			message.task.release_time = xTaskGetTickCount();	//assign release time
			listInsert(active_dd_tasks, message.task);	//add to active task list
			vTaskPrioritySet(active_dd_tasks, 1);
			//sort active list and set priorities of user tasks









			vTaskPrioritySet(active_dd_tasks, 2);

			break;
		case COMPLETED_TASK:
			//assign completion time
			//remove from active list
			//add to completed list
			//sort active list and set priorities of user tasks
			break;
		case ACTIVE_LIST_REQ:
			break;
		case COMP_LIST_REQ:
			break;
		case OD_LIST_REQ:
			break;
		default:
			//error
			printf("error\n");
		}
	}
}

static void task1_timer_callback(TimerHandle_t xTimer){
	BaseType_t xYieldRequired = xTaskResumeFromISR(generator_handle);
	portYIELD_FROM_ISR(xYieldRequired);
}


static void generator_task(void *pvParameters){
	struct dd_task new_task;
	TaskHandle_t* usd_task_handle = pvPortMalloc(sizeof(TaskHandle_t));
	uint32_t curr_task_id = 0;
	uint32_t task_period = pdMS_TO_TICKS(500);

	for(;;){
		new_task.task_id = curr_task_id++;
		xTaskCreate( usd_task, "User-Defined Task", configMINIMAL_STACK_SIZE, (void *) new_task.task_id, 1, usd_task_handle);
		new_task.t_handle = *usd_task_handle;
		new_task.absolute_deadline = xTaskGetTickCount() + task_period;
		release_dd_task(new_task);
		vTaskSuspend(NULL);
	}

}

static void monitor_task(void *pvParameters){
	struct dd_task_list *listCopy;
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

static void usd_task(void *pvParameters){
	for(int i = 0; i < pdMS_TO_TICKS(500); i++);
	printf("done usdt\n");
	complete_dd_task((uint32_t) pvParameters);
}

static struct dd_task_list* get_active_dd_task_list(){
	struct dd_task_list* listCopy;
	xQueueSend(xQueue_DDS_Messages, &active_list, 1000);
	xQueueReceive(xQueue_List_Response, &listCopy, 1000);
	return listCopy;
}

static struct dd_task_list* get_completed_dd_task_list(){
	struct dd_task_list* listCopy;
	xQueueSend(xQueue_DDS_Messages, &completed_list, 1000);
	xQueueReceive(xQueue_List_Response, &listCopy, 1000);
	return listCopy;
}

static struct dd_task_list* get_overdue_dd_task_list(){
	struct dd_task_list* listCopy;
	xQueueSend(xQueue_DDS_Messages, &overdue_list, 1000);
	xQueueReceive(xQueue_List_Response, &listCopy, 1000);
	return listCopy;
}

void printList(struct dd_task_list *list){
	struct dd_task_list *curr_task = list;
	while(curr_task){
		printf("task_id: %u", curr_task->task.task_id);
		curr_task = curr_task->next_task;
	}
}

unsigned int getListLength(struct dd_task_list *list){
	struct dd_task_list *curr_task = list;
	unsigned int count = 0;
	while(curr_task){
		count++;
		curr_task = curr_task->next_task;
	}
}


//insert front
void listInsert(struct dd_task_list* head, struct dd_task new_task){
	if (head == NULL) {
		head = pvPortMalloc(sizeof(struct dd_task_list));
		head->task = new_task;
		head->next_task = NULL;
		return;
	}
	struct dd_task_list* temp = pvPortMalloc(sizeof(struct dd_task_list));
	temp->task = head->task;
	temp->next_task = head->next_task;
	head->task = new_task;
	head->next_task = temp;
}

struct dd_task_list* mergeLists(struct dd_task_list* left_half, struct dd_task_list* right_half){
	struct dd_task_list* result = NULL;
	if (left_half == NULL){
		return right_half;
	} else if (right_half == NULL){
		return left_half;
	}
	
	if (left_half->task->deadline <= right_half->task->deadline){
		
	}else {
		result = right_half;
		result->next = mergeLists(left_half,right_half->next)
	}
	
}

void mergeSortByDeadline(struct dd_task_list** active_dd_tasks_ptr) {
    struct dd_task_list* active_dd_tasks = *active_dd_tasks_ptr;
    struct dd_task_list* left_half;
    struct dd_task_list* right_half;
    int list_size = listLength(active_dd_tasks);
 
    if (list_size <= 1) {
        return;
    }

    int middle = list_size / 2;
    struct dd_task_list* current = active_dd_tasks;
    int i = 0;

    // Split the list into two halves
    while (current != NULL) {
        if (i < middle) {
            left_half = listInsert(left_half, current->task);
        } else {
            right_half = listInsert(right_half, current->task);
        }
        current = current->next;
        i++;
    }

    // Recursively sort the two halves
    mergeSortByDeadline(&left_half);
    mergeSortByDeadline(&right_half);

    // Merge the sorted halves
    *active_dd_tasks_ptr = mergeLists(left_half, right_half);
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
