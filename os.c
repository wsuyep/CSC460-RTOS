#include <avr/io.h>
#include <avr/interrupt.h>

#include "os.h"
#include "kernel.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcd/lcd_drv.h"
#include <util/delay.h>
#include "avr_console.h"

#define Disable_Interrupt()		asm volatile ("cli"::)
#define Enable_Interrupt()		asm volatile ("sei"::)

/********************************************************************************
*			G L O B A L    V A R I A B L E S
*********************************************************************************/
static PD Process[MAXTHREAD];

static struct Queue SystemProcess;
static struct Queue PeriodicProcess;
static struct Queue RoundRobinProcess;
static struct Queue DeadPool;

volatile static unsigned int NumSysTasks;
volatile static unsigned int NumPeriodTasks;
volatile static unsigned int NumRRTasks;
volatile static unsigned int Tasks;

volatile static PD* cp;

volatile unsigned char *KernelSp;
volatile unsigned char *CurrentSp;

volatile static unsigned int NextP;

volatile static unsigned int KernelActive;

TICK ticks = 0;
unsigned int counter = 0;

extern void Enter_Kernel();
extern void Exit_Kernel();    /* this is the same as CSwitch() */

/* Prototype */
void Task_Terminate(void);
static void Next_Kernel_Request();

// The calling task gets its initial "argument" when it was created.
int Task_GetArg(){
    return cp->arg;
}

// It returns the calling task's PID.
PID Task_Pid(){
    return cp->pid;
}

int getProcess(PID id){
    for(int i=0;i<MAXTHREAD;i++){
        if(Process[i].pid == id){
            return i;
        }
    }
    return -1;
}

void OS_Abort(unsigned int error){
  cli();
  while(1){
    printf("Error with error code: %d\n", error);
  }
}
/********************************************************************************
*			OS (Kernel methods)
*********************************************************************************/

void setupTimer(){
    cli();
    //clear timer config
    TCCR4A = 0;
    TCCR4B = 0;

    //set to CTC
    TCCR4B |=(1<<WGM42);

    //set prescaller to 256
    TCCR4B |=(1<<CS42);

    //set Top value
    OCR4A = 625;

    //Enable interupt A for timer 3
    TIMSK4 |= (1<<OCIE4A);

    //set timer to 0
    TCNT4 = 0;

    sei();

}

static void PutBackToReadyQueue(PD* p){
   p->state = READY;
   counter++;
   switch(p->priority){
      case SYSTEM:
         counter++;
         enqueue(&SystemProcess,p);
         counter++;
         break;
      case PERIODIC:
         enqueue(&PeriodicProcess,p);
         break;
      case RR:
         enqueue(&RoundRobinProcess,p);
         break;
      default:
         //OS_Abort(2);
         break;
   }
}

static PD *GetFirstNonBlockProcess(struct Queue *queue){
    PD *curr = queue->head;
    while(curr != NULL){
        if(curr->state == READY){
           RemoveQ(queue,curr);
           return curr; 
        }
        curr = curr->next;
    }
    return NULL;
}

static PD *GetFirstNonBlockPeriodicProcess(){
    unsigned int readyCount = 0;
    PD * curr = PeriodicProcess.head;
    PD * readyTask = NULL;
    while(curr != NULL){
      if((ticks - curr->offset)%curr->period == 0){
        readyTask = curr;
        readyCount ++;
      }
      curr = curr->next;
    }
    
    if(readyCount > 1){
      OS_Abort(1);
    }else if (readyCount == 1){
      return readyTask;
    }
    return NULL;
}
static void Dispatch()
{
     /* find the next READY task
       * Note: if there is no READY task, then this will loop forever!.
       */
    while(1){
      counter++;
      cp = GetFirstNonBlockProcess(&SystemProcess);
      if(cp){
        counter++;
        printf("%d.Found system task: %d\n",counter,cp->pid);
        break;
      }
      cp = GetFirstNonBlockPeriodicProcess();
      if(cp) {
        counter++;
        printf("%d.Found periodic task: %d\n",counter, cp->pid);
        break;
      }
      cp = GetFirstNonBlockProcess(&RoundRobinProcess);
      if(cp) {
        counter++;
        printf("%d.Found RR task: %d\n",counter,cp->pid);
        break;
      }
    }
    CurrentSp = cp ->sp;
    cp->state = RUNNING;
}

void Kernel_Create_Task_At( PD *p, voidfuncptr f )
{   
   unsigned char *sp;

   //Changed -2 to -1 to fix off by one error.
   sp = (unsigned char *) &(p->workSpace[WORKSPACE-1]);

   //Clear the contents of the workspace
   memset(&(p->workSpace),0,WORKSPACE);

   //Store terminate at the bottom of stack to protect against stack underrun.
   *(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
   *(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;
   *(unsigned char *)sp-- = 0x00;

   //Place return address of function at bottom of stack
   *(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
   *(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;
   *(unsigned char *)sp-- = 0x00;

   //Place stack pointer at top of stack
   sp = sp - 34;
   
   p->sp = sp;		/* stack pointer into the "workSpace" */
   p->code = f;		/* function to be executed as a task */
   p->kernel_request = NONE;
   InitQueue(p->senders_queue);
   InitQueue(p->reply_queue);
   p->rps.pid = 0;
   p->rps.v = 0;
   p->rps.r = 0;
   p->rps.t = 0;
   p->rps.status=0;
   Tasks++;
   p->next =NULL;
   p->state = READY;
   PutBackToReadyQueue(p);
}

/*================
  * RTOS  API  and Stubs
  *================
  */
void OS_Init() 
{
   int x;
   Tasks = 0;
   KernelActive = 0;
   NextP = 0;

   InitQueue(&SystemProcess);
   InitQueue(&PeriodicProcess);
   InitQueue(&RoundRobinProcess);
   InitQueue(&DeadPool);

   for (x = 0; x < MAXTHREAD; x++) {
      memset(&(Process[x]),0,sizeof(PD));
      Process[x].state = DEAD;
      Process[x].pid = x+1;
      Process[x].next = &(Process[x+1]);
      enqueue(&DeadPool,&(Process[x]));
   }
}

void OS_Start() 
{   
   if ( (! KernelActive) && (Tasks > 0)) {
       Disable_Interrupt();
      /* we may have to initialize the interrupt vector for Enter_Kernel() here. */

      /* here we go...  */
      KernelActive = 1;
      Next_Kernel_Request();
      /* NEVER RETURNS!!! */
   }
}

/**
  * This internal kernel function is the "main" driving loop of this full-served
  * model architecture. Basically, on OS_Start(), the kernel repeatedly
  * requests the next user task's next system call and then invokes the
  * corresponding kernel function on its behalf.
  *
  * This is the main loop of our kernel, called by OS_Start().
  */
static void Next_Kernel_Request()
{
   Dispatch();  /* select a new task to run */

   while(1) {
      cp->kernel_request = NONE; /* clear its request */

      /* activate this newly selected task */
      CurrentSp = cp->sp;
      Exit_Kernel();    /* or CSwitch() */

      /* if this task makes a system call, it will return to here! */

      /* save the cp's stack pointer */
      cp->sp = (unsigned char *)CurrentSp;

      switch(cp->kernel_request){
         case NEXT:
         case NONE:
            /* NONE could be caused by a timer interrupt */
            counter++;
            switch(cp->state){
              case RUNNING:
                printf("RUNNING\n");
                break;
              case READY:
                printf("READY\n");
                break;
              case NONE:
                printf("DEAD\n");
                break;
              default:
                printf("BLOCKED\n");
                break;
            }
            //if(cp->state != RUNNING) OS_Abort(4);
            PutBackToReadyQueue((PD *)cp);//TODO: put it back to ready queue
            Dispatch();
            break;
         case TERMINATE:
            /* deallocate all resources used by this task */
            cp->state = DEAD;
            Dispatch();
            break;
         default:
            /* Houston! we have a problem here! */
            OS_Abort(3);
            break;
      }
   }
}

PID Task_Create_System(void (*f)(void), int arg){
   if (Tasks == MAXTHREAD || DeadPool.head == NULL) return 0;
   Disable_Interrupt();
   PD *new_p = dequeue(&DeadPool);
   new_p->priority = SYSTEM;
   new_p->arg = arg;
   Kernel_Create_Task_At( new_p, f );
   Enable_Interrupt();
   return new_p->pid;
}

PID Task_Create_RR(void (*f)(void), int arg){
   if (Tasks == MAXTHREAD || DeadPool.head == NULL) return 0;
   Disable_Interrupt();
   PD *new_p = dequeue(&DeadPool);
   new_p->priority = RR;
   new_p->arg = arg;
   Kernel_Create_Task_At( new_p, f );
   Enable_Interrupt();
   return new_p->pid;
}

PID Task_Create_Period(void (*f)(void), int arg, TICK period, TICK wcet, TICK offset){
   if (Tasks == MAXTHREAD || DeadPool.head == NULL) return 0;
   Disable_Interrupt();
   PD *new_p = dequeue(&DeadPool);
   new_p->priority = PERIODIC;
   new_p->arg = arg;
   new_p->wcet = wcet;
   new_p->period = period;
   new_p->offset = offset;
   Kernel_Create_Task_At( new_p, f );
   Enable_Interrupt();
   return new_p->pid;
}

/**
  * The calling task gives up its share of the processor voluntarily.
  */
void Task_Next()
{
   if (KernelActive) {
      Disable_Interrupt();
      cp ->kernel_request = NEXT;
      Enter_Kernel();
   }
}

/**
  * The calling task terminates itself.
  */
void Task_Terminate()
{
   if (KernelActive) {
      Disable_Interrupt();
      cp -> kernel_request = TERMINATE;
      Enter_Kernel();
      /* never returns here! */
   }
}


/********************************************************************************
*			IPC SRR
*********************************************************************************/

/****IPC*****/
//client thread
void Msg_Send(PID id, MTYPE t, unsigned int *v){
     PID receiver;
     //assign PD by id to receiver
     receiver = getProcess(id);
     if(receiver==NULL){
         return;
     }

     if(Process[receiver].state==RECEIVEBLOCKED){
         printf("receive block\n");
         Process[receiver].rps.pid = Task_Pid();
         //message sent, receiver picks up message v
         Process[receiver].rps.v = *v;
         //message type
         Process[receiver].rps.t = t;
         //client thread becomes reply blocks
         cp->state = REPLYBLOCKED;
         Process[receiver].state=READY;
         enqueue(Process[receiver].reply_queue, (PD *)cp);
         PutBackToReadyQueue(&Process[receiver]);
         Task_Next();
     }else{ /*receiver thread is not ready */
        //the client thread becomes sendblock
         printf("sender block\n");
         cp->state = SENDBLOCKED;
         //add receiver to the sender queue
         
         enqueue(Process[receiver].senders_queue, (PD *)cp);
         //tmp = dequeue(receiver->senders_queue);
         //printf("dequeue %p \n",(PD *)tmp);
         //printf("AFTER ADDDING P: %p\n",Process[receiver].senders_queue->head);
         //printf("length of receiver queue %p\n", receiver->senders_queue->head);
         //printf("length of receiver queue %p\n", receiver->senders_queue->head);
         //printf("length of receiver queue %p\n", receiver->senders_queue->head);
         Task_Next();
     }
     
}

void filter_unwantted_requests_for_msg_recv(struct Queue *queue, MASK m){
	struct ProcessDescriptor *curr = queue->head;
	while(curr !=NULL){
		struct ProcessDescriptor *tmp = curr;
		curr = curr->next;
		// remove requests that we don't want
		if(((unsigned int)tmp->rps.t & (unsigned int)m) == 0b00000000){
			RemoveQ(queue,tmp);
		}
	}
}

PID  Msg_Recv(MASK m, unsigned int *v ){
      //printf("RECEIVER ADDRESS: %p\n",cp);
      PD *first_sender;
      //filter_unwantted_requests_for_msg_recv(cp->senders_queue,m);
      //printf("length of queue %d\n", ItemsInQ(cp->senders_queue));
      first_sender = dequeue(cp->senders_queue);
      //printf("sender address: %p\n",first_sender);
      //no client thread has done sent
      if(first_sender == NULL){
         printf("no sender\n");
         cp->state = RECEIVEBLOCKED;
         Task_Next();
         return cp->rps.pid;
      }else{
         first_sender->state=REPLYBLOCKED;
         //add to reply queue 
         printf("message copy\n");
         enqueue(cp->reply_queue, first_sender);
         //check sender message type and MASK
         cp->rps.pid = first_sender->pid;
         cp->rps.v = first_sender->rps.v;
         cp->rps.t = first_sender->rps.t;
         printf("pidddd %d\n", first_sender->pid);
         return first_sender->pid;
      }
     
}

void Msg_Rply(PID id, unsigned int r ){
    PD *sender;
    //set current cp to sender
    sender = getProcess(id);
    if(sender != NULL && InQueue(cp->reply_queue,sender)){
        //current reply to sender
        RemoveQ(cp->reply_queue, sender);
        sender->rps.r=r;
        sender->state = READY;
        //TODO sechduler adds sender to Ready queue 
    }
}

//Asynchronous Communication
void Msg_ASend(PID id, MTYPE t, unsigned int v ){
     PD *receiver;
     receiver = getProcess(id);
     if(receiver==NULL){
         return;
     }
     //if p is waiting on receive
     if(receiver->state==RECEIVEBLOCKED){
         receiver->rps.pid = id;
         //message sent, receiver picks up message v
         receiver->rps.v = v;
         //message type
         receiver->rps.t = t;
         //dont have to wait for reply
     }else{
         Task_Next();
     }
     //else not op
}

/********************************************************************************
*			T E S T    C O D E
*********************************************************************************/

ISR(TIMER4_COMPA_vect){

  if (KernelActive) {
     cp ->kernel_request = NEXT;
     Enter_Kernel();
   }
    
}

void Blink()
{
  while(1){
    
     printf("task1");
	_delay_ms(2000);
    //Task_Next();
  }
}
void Blink2()
{
  while(1){
    
    printf("task2");
    _delay_ms(2000);
    //_delay_ms(2000);
    //Task_Next();
  }
}

//TEST IPC
void Blink_IPC()
{
  while(1){
     send_message();
  }
}
void Blink2_IPC()
{
  while(1){
    receive_message();
  }
}


void send_message(){
    unsigned int a=0;
    Msg_Send(2, 1, &a);
}

void receive_message(){
    unsigned int a=0;
    PID id = Msg_Recv(0xff,&a);
    
}

//TEST System Process
void test1(){
   Task_Create_System( Blink,1);
   Task_Create_System( Blink2,1 );
}

//TEST RR
void test2(){
   Task_Create_RR( Blink, 1);
   //Task_Create_System( Blink2,1 );
}

//TEST MAXProcess
void test3(){
    int i;
	for (i = 0; i < 20; i++) {
		Task_Create_RR(Blink, i);
	}
}

//test sendIPC
void testIPC(){
    
    Task_Create_System(Blink_IPC, 1);
    Task_Create_System(Blink2_IPC, 1);
}

int main() 
{
   uart_init();
   stdout = &uart_output;
   stdin = &uart_input;
   cli();
   DDRB=0x83;
   OS_Init();
   testIPC();
   sei();
   OS_Start();
   return -1;
}