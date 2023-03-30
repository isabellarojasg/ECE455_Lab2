/*
 * linked_task_list.c
 *
 *  Created on: Mar 23, 2023
 *      Author: Kali Erickson, Isabella Rojas
 *      References: https://www.geeksforgeeks.org/merge-sort-for-linked-list/ (accessed: Mar. 28, 2023)
 */

#include "linked_task_list.h"

void printList(struct dd_task_node *list){
	struct dd_task_node *curr_task = list;
	printf("list:\n");
	while(curr_task){
		printf("task_id: %u\n", (curr_task->task.task_id));
		curr_task = curr_task->next_task;
	}
}

unsigned int getListLength(struct dd_task_node *head){
	unsigned int length = 0;
	struct dd_task_node* current_node = head;
	while (current_node!= NULL){
		length ++;
		current_node = current_node->next_task;
	}
	return length;
}


////insert front
//void insertFront(struct dd_task_node** head, struct dd_task_node** new_task){
//	if ((*head) == NULL) {
//		(*head) = (*new_task);
//	//	printList(*head);
//		return;
//	}
//	(*new_task)->next_task = *head;
//	*head = *new_task;
//	//printList(*head);
//}

//insert back
void listInsert(struct dd_task_node** head, struct dd_task_node** new_task){
	if ((*head) == NULL) {
		(*head) = (*new_task);
	//	printList(*head);
		return;
	}
	struct dd_task_node *lastNode = *head;

    while(lastNode->next_task != NULL){
    	lastNode = lastNode->next_task;
    }

    lastNode->next_task = *new_task;
}


//move task from front of one list to front of other list
void moveTask(struct dd_task_node** list_head, struct dd_task_node** new_list_head){
	struct dd_task_node* temp = *list_head;
	*list_head = (*list_head)->next_task;
	temp->next_task = *new_list_head;
	*new_list_head = temp;
}


//Merge two sorted lists
struct dd_task_node* mergeSortedLists(struct dd_task_node* left_half, struct dd_task_node* right_half){

	struct dd_task_node* result = NULL;
	if (left_half == NULL){
		return right_half;
	} else if (right_half == NULL){
		return left_half;
	}

	if (left_half->task.absolute_deadline <= right_half->task.absolute_deadline){
		result = left_half;
		result->next_task = mergeSortedLists(left_half->next_task,right_half);
	}else {
		result = right_half;
		result->next_task = mergeSortedLists(left_half,right_half->next_task);
	}
	return result;
}

void frontBackSplit(struct dd_task_node* head,
                    struct dd_task_node** left_half, struct dd_task_node** right_half)
{
    struct dd_task_node* fast;
    struct dd_task_node* slow;
    slow = head;
    fast = head->next_task;

    /* Advance 'fast' two nodes, and advance 'slow' one node */
    while (fast != NULL) {
        fast = fast->next_task;
        if (fast != NULL) {
            slow = slow->next_task;
            fast = fast->next_task;
        }
    }

    /* 'slow' is before the midpoint in the list, so split it in two
    at that point. */
    *left_half = head;
    *right_half = slow->next_task;
    slow->next_task = NULL;
}

//Using merge sort to sort the Linked List
void mergeSortByDeadline(struct dd_task_node** active_dd_tasks_ptr) {
    struct dd_task_node* active_dd_tasks = *active_dd_tasks_ptr;
    struct dd_task_node* left_half = NULL;
    struct dd_task_node* right_half = NULL;

    if(active_dd_tasks->next_task == NULL)
    	return;

    // Split the list into two halves
    frontBackSplit(active_dd_tasks, &left_half, &right_half);

    // Recursively sort the two halves
    mergeSortByDeadline(&left_half);
    mergeSortByDeadline(&right_half);

    // Merge the sorted halves
    *active_dd_tasks_ptr = mergeSortedLists(left_half, right_half);
}

void sort_active_dd_tasks(struct dd_task_node **active_dd_tasks) {
    int i, j;
    struct dd_task_node *temp;

    unsigned int l = getListLength(*active_dd_tasks);
    for (i = 0; i < l; i++) {
        for (j = 0; j < l - i - 1; j++) {
            if (active_dd_tasks[j]->task.release_time > active_dd_tasks[j+1]->task.release_time) {
                // swap nodes
                temp = active_dd_tasks[j];
                active_dd_tasks[j] = active_dd_tasks[j+1];
                active_dd_tasks[j+1] = temp;
            }
        }
    }
}
