//
// Created by Willy on 2018-03-14.
//

#include "os.h"

#ifndef CSC460_KERNEL_H
#define CSC460_KERNEL_H

/********************************************************************************
*			DATA STRUCTURES
*********************************************************************************/
typedef void (*voidfuncptr) (void);      /* pointer to void f(void) */ 

typedef enum kernel_request_type
{
    NONE = 0,
    CREATE,
    NEXT,
    TERMINATE,
    SEND,
    RECEIVE,
    REPLY
} KERNEL_REQUEST_TYPE;

typedef enum {
    DEAD,
    READY,
    RUNNING,
    SENDBLOCKED,
    RECEIVEBLOCKED,
    REPLYBLOCKED
}PROCESS_STATES;

struct Queue
{
    struct ProcessDescriptor *head;
    struct ProcessDescriptor *tail;
};

typedef enum priority_level {
    SYSTEM = 3,
    PERIODIC = 2,
    RR = 1,
} PRIORITY_LEVEL;

struct IPCRequest
{
    PID pid;
    unsigned int v; //send message
    unsigned int r; //reply message
    MTYPE t; //messag type
    unsigned int status;//TODO: use this to indicate status 0:noop 1:sending 2:receiving 3:response
};

typedef struct ProcessDescriptor
{
    PID pid;    //process id
    unsigned char *sp;   /* stack pointer into the "workSpace" */
    unsigned char workSpace[WORKSPACE];
    PROCESS_STATES state;
    voidfuncptr  code;   /* function to be executed as a task */
    KERNEL_REQUEST_TYPE kernel_request; //request type
    struct Queue *senders_queue; //sender queue
    struct Queue *reply_queue;  //reply queue
    struct IPCRequest rps; //IPC request
    struct ProcessDescriptor *next;
    PRIORITY_LEVEL priority; //The priority of the task
    int arg;
    TICK wcet;
    TICK period;
    TICK offset;
} PD;

#endif //CSC460_KERNEL_H
