//
// Created by Willy on 2018-03-14.
//

#ifndef CSC460_KERNEL_H
#define CSC460_KERNEL_H

/********************************************************************************
*			DATA STRUCTURES
*********************************************************************************/
struct Queue
{
    struct ProcessDescriptor *head;
    struct ProcessDescriptor *tail;
};

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
} PD;

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

/**
  * This table contains ALL process descriptors. It doesn't matter what
  * state a task is in.
  */
static PD Process[MAXTHREAD];

static Queue SystemProcess;
static Queue PeriodicProcess;
static Queue RoundRobinProcess;

volatile static unsigned int NumSysTasks;
volatile static unsigned int NumPeriodTasks;
volatile static unsigned int NumRRTasks;

/**
  * The process descriptor of the currently RUNNING task.
  */
volatile static PD* cp;

/**
  * Since this is a "full-served" model, the kernel is executing using its own
  * stack. We can allocate a new workspace for this kernel stack, or we can
  * use the stack of the "main()" function, i.e., the initial C runtime stack.
  * (Note: This and the following stack pointers are used primarily by the
  *   context switching code, i.e., CSwitch(), which is written in assembly
  *   language.)
  */
volatile unsigned char *KernelSp;

/**
  * This is a "shadow" copy of the stack pointer of "Cp", the currently
  * running task. During context switching, we need to save and restore
  * it into the appropriate process descriptor.
  */
volatile unsigned char *CurrentSp;

/** index to next task to run */
volatile static unsigned int NextP;

/** 1 if kernel has been started; 0 otherwise. */
volatile static unsigned int KernelActive;

/** number of tasks created so far */
volatile static unsigned int Tasks;

#endif //CSC460_KERNEL_H
