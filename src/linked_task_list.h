/*
 * linked_task_list.h
 *
 *  Created on: Mar 23, 2023
 *      Author: Kali Erickson, Isabella Rojas
 */

#ifndef LINKED_TASK_LIST_H_
#define LINKED_TASK_LIST_H_

/* Kernel includes. */
#include "stm32f4xx.h"
#include "../FreeRTOS_Source/include/FreeRTOS.h"
#include "../FreeRTOS_Source/include/task.h"

enum task_type {PERIODIC, APERIODIC};

struct dd_task {
	TaskHandle_t t_handle;
	enum task_type type;	//is this redeclaring task_type or?
	uint32_t task_id;
	uint32_t release_time;
	uint32_t absolute_deadline;
	uint32_t completion_time;
};

struct dd_task_node {
	struct dd_task task;	//again, should have struct here??
	struct dd_task_node *next_task;
};

//void listInsert(struct dd_task_node** head, struct dd_task new_task);
void listInsert(struct dd_task_node** head, struct dd_task_node** new_task);
void moveTask(struct dd_task_node** list_head, struct dd_task_node** new_list_head);
void printList(struct dd_task_node *list);
unsigned int getListLength(struct dd_task_node* list);
void frontBackSplit(struct dd_task_node* head, struct dd_task_node** left_half, struct dd_task_node** right_half);
void mergeSortByDeadline(struct dd_task_node** active_dd_tasks_ptr);
struct dd_task_node* mergeSortedLists(struct dd_task_node* left_half, struct dd_task_node* right_half);
int listLength(struct dd_task_node* head);

#endif /* LINKED_TASK_LIST_H_ */
