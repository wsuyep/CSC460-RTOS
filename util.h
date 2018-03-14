//
// Created by Willy on 2018-03-14.
//

#ifndef CSC460_UTIL_H
#define CSC460_UTIL_H

int  Task_GetArg(void);

PID  Task_Pid(void);

PD *getProcess(PID id);

void enqueue(struct Queue *queue, struct ProcessDescriptor *p);

static struct ProcessDescriptor *dequeue(struct ProcessQueue *queue);

static void RemoveQ(struct ProcessQueue *queue, struct ProcessDescriptor *p);

static Boolean InQueue(struct ProcessQueue *queue, struct ProcessDescriptor *p);

#endif //CSC460_UTIL_H
