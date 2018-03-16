//
// Created by Willy on 2018-03-14.
//
#include "kernel.h"

/********************************************************************************
*			U T I L S
*********************************************************************************/


static unsigned int ItemsInQ(struct Queue * queue){
  unsigned int i = 0;
  PD *curr = queue->head;
  while(curr != NULL){
      i++;
      curr = curr->next;
  }
  return i;
}


void enqueue(struct Queue *queue, struct ProcessDescriptor *p){
    //printf("enqueue pid: %p\n",p);
    if(queue->head==NULL){
        queue->head = p;
        queue->tail = p;
        p->next = NULL;
    }else{
        queue->tail->next = p;
        queue->tail = p;
    }
}

/* delete the head from a queue */
static PD *dequeue(struct Queue *queue)
{
    struct ProcessDescriptor *p;

    if (queue->head == NULL) return NULL; /* empty queue, return NULL */

    p = queue->head;
    queue->head = p->next;

    if (queue->head == NULL) /* p is the last process of that queue */
        queue->tail = NULL;

    p->next = NULL;      /* make sure p->next is not pointing *
                        * to some other processes           */

    return p;
}

static void RemoveQ(struct Queue *queue, struct ProcessDescriptor *p)
{
    struct ProcessDescriptor *curr, *prev;

    if(queue->head ==NULL) return;     /* empty queue */
    if(queue->head == p){         /* head of queue */
        queue->head = p->next;
        p->next = NULL;
    } else {                      /* search remaining queue */
        prev = queue->head;
        curr = prev->next;
        while(curr != NULL && curr != p){
            prev = curr;
            curr = curr->next;
        }
        if(curr == p) {         /* found it */
            prev->next = p->next;
            p->next = NULL;
        }
    }
    if (queue->head == NULL) queue->tail = NULL;
}

static BOOL InQueue(struct Queue *queue, struct ProcessDescriptor *p){
    struct ProcessDescriptor *curr = queue->head;
    while(curr != NULL){
        if(curr == p) return 1;
        curr = curr->next;
    }
    return 0;
}

static void InitQueue(struct Queue *queue){
    queue->head = NULL;
    queue->tail = NULL;
}





