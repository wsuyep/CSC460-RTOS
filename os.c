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
//#include "avr_console.h"
//#include "base_station.h"

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

static struct Queue SendersQueue;
static struct Queue ReplyQueue;

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
  printf("Error with error code: %d\n", error);
  while(1){
    
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

static unsigned int ItemsInQ(struct Queue * queue){
  unsigned int i = 0;
  PD *curr = queue->head;
  while(curr != NULL){
      i++;
      curr = curr->next;
  }
  return i;
}

void printAllItems(struct Queue *queue){
     PD* curr = queue->head;
     //printf("Stuff in Q: ");
     while(curr != NULL){
       //printf("  %d  ",curr->pid);
       curr = curr->next;
     }
     printf("\n");
   }

static void PutBackToReadyQueue(PD* p){
   if(p->pid >16 || p->pid <1){
     OS_Abort(5);
   }
     /*
     counter++;
     printf("%d.PUTTING BACK TO Q ID: %d  state: ",counter,p->pid);
     switch(p->state){
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
            }*/
   if(p->state == RUNNING) p->state = READY;
   
   /*
   printf("%d.AFTER BACK TO Q ID: %d  state: ",counter,p->pid);
     switch(p->state){
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
            }*/

   //counter++;
   //printf("%d.Putting back to readyQ: %d : %d\n",counter,p->pid,p->priority);
   switch(p->priority){
      case SYSTEM:
         //counter++;
         //printf("%d.before adding,items in System queue: %d\n",counter,ItemsInQ(&SystemProcess));
         enqueue(&SystemProcess,p);
         //counter++;
         //printf("%d.after adding item to System queue: %d\n",counter,ItemsInQ(&SystemProcess));
         break;
      case PERIODIC:
         //counter++;
         //printf("%d.before adding,items in Period queue: %d\n",counter,ItemsInQ(&PeriodicProcess));
         //enqueue(&PeriodicProcess,p);
         //counter++;
         //printf("%d.after adding items to Period queue: %d\n",counter,ItemsInQ(&PeriodicProcess));
         break;
      case RR:
         //counter++;
         //printf("%d.before adding,items in RR queue: %d\n",counter,ItemsInQ(&RoundRobinProcess));
         //printAllItems(&RoundRobinProcess);
         //printf("enqueue head: %d    tail: %d\n",RoundRobinProcess.head->pid,RoundRobinProcess.tail->pid);
         enqueue(&RoundRobinProcess,p);
         //printAllItems(&RoundRobinProcess);
         //printf("after enqueue head: %d    tail: %d\n",RoundRobinProcess.head->pid,RoundRobinProcess.tail->pid);
         //counter++;
         //printf("%d.after adding item to RR queue: %d\n",counter,ItemsInQ(&RoundRobinProcess));
         break;
      default:
         OS_Abort(2);
         break;
   }
}

static PD *GetFirstNonBlockProcess(struct Queue *queue){
    PD *curr = queue->head;
    while(curr != NULL){
        if(curr->state == READY){
           //printAllItems(&RoundRobinProcess);
           //printf("remove queue head: %d    tail: %d\n",RoundRobinProcess.head->pid,RoundRobinProcess.tail->pid);
           //printf("saljfie: %d\n",RoundRobinProcess.head->next->pid);
           RemoveQ(queue,curr);
           //printf("saljfie: %d\n",RoundRobinProcess.head->next->pid);
           //printAllItems(&RoundRobinProcess);
           //printf("remove queue head: %d    tail: %d\n",RoundRobinProcess.head->pid,RoundRobinProcess.tail->pid);
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
      counter++;
      printf("%d. pid: %d tick: %d offset: %d period: %d   (tick-offset) mod (period): %d\n",counter,curr->pid,ticks,curr->offset,curr->period,(ticks - curr->offset)%curr->period);
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
    counter++;
    //printf("%d.SystemQueue Length: %d\n",counter,ItemsInQ(&SystemProcess));
    //printf("%d.PeriodicQueue Length: %d\n",counter,ItemsInQ(&PeriodicProcess));
    //printf("%d.RRQueue Length: %d\n",counter,ItemsInQ(&RoundRobinProcess));
    while(1){
      //counter++;
      //printf("%d.Looking for new task ",counter);
      cp = GetFirstNonBlockProcess(&SystemProcess);
      if(cp){
        counter++;
        printf("%d.Selected new system task: %d\n",counter,cp->pid);
        break;
      }
      cp = GetFirstNonBlockPeriodicProcess();
      if(cp) {
        counter++;
        printf("%d.Selected new periodic task: %d\n",counter, cp->pid);
        break;
      }
      cp = GetFirstNonBlockProcess(&RoundRobinProcess);
      if(cp) {
        counter++;
        printf("%d.Selected new RR task: %d\n",counter,cp->pid);
        break;
      }
    }
    //counter++;
    //printf("%d.Changing stack pointer from : %p %p",counter,CurrentSp,CurrentSp+1);
    CurrentSp = cp ->sp;

    //printf("CP: %d   SP:%p\n",cp->pid,CurrentSp);
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
   //InitQueue(p->senders_queue);
   p->reply_queue = &ReplyQueue;
   p->senders_queue = &SendersQueue;
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

   cp->pid = 17;

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
       printf("-------STARTING OS--------\n\n\n");
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
      //counter++;
      //printf("%d.Entering timer interrupt, cp id: %d sp: %d\n",counter,cp->pid,cp->sp);

      /* if this task makes a system call, it will return to here! */

      /* save the cp's stack pointer */
      cp->sp = (unsigned char *)CurrentSp;

      switch(cp->kernel_request){
         case NEXT:
         case NONE:
            /* NONE could be caused by a timer interrupt */
            /*
            counter++;
            printf("%d.entered kernel, and cp id: %d  state is:",counter,cp->pid);
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
            }*/
            //printf("BEFORE: %d\n",cp->pid);
            PutBackToReadyQueue((PD *)cp);//TODO: put it back to ready queue
            //printf("AFTER: %d\n",cp->pid);
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
   if(KernelActive) Disable_Interrupt();
   PD *new_p = dequeue(&DeadPool);
   new_p->priority = SYSTEM;
   new_p->arg = arg;
   Kernel_Create_Task_At( new_p, f );
   if(KernelActive) Enable_Interrupt();
   return new_p->pid;
}

PID Task_Create_RR(void (*f)(void), int arg){
   if (Tasks == MAXTHREAD || DeadPool.head == NULL) return 0;
   if(KernelActive) Disable_Interrupt();
   PD *new_p = dequeue(&DeadPool);
   new_p->priority = RR;
   new_p->arg = arg;
   Kernel_Create_Task_At( new_p, f );
   if(KernelActive) Enable_Interrupt();
   return new_p->pid;
}

PID Task_Create_Period(void (*f)(void), int arg, TICK period, TICK wcet, TICK offset){
   if (Tasks == MAXTHREAD || DeadPool.head == NULL) return 0;
   if(KernelActive) Disable_Interrupt();
   PD *new_p = dequeue(&DeadPool);
   new_p->priority = PERIODIC;
   new_p->arg = arg;
   new_p->wcet = wcet;
   new_p->period = period;
   new_p->offset = offset;
   Kernel_Create_Task_At( new_p, f );
   enqueue(&PeriodicProcess,new_p);
   if(KernelActive) Enable_Interrupt();
   return new_p->pid;
}

/**
  * The calling task gives up its share of the processor voluntarily.
  */
void Task_Next()
{
   if (KernelActive) {
      Disable_Interrupt();
      cp -> kernel_request = NEXT;
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
      printf("TERMINATING TASK: %d\n",cp->pid);
      cp -> kernel_request = TERMINATE;
      cp -> state = DEAD;
      Enter_Kernel();
      /* never returns here! */
   }
}


/********************************************************************************
*			IPC SRR
*********************************************************************************/

PD* getFirstTypeMatchingProcess(struct Queue *queue, MASK m){
  PD *curr = queue->head;
  while(curr !=NULL){
    //printf(" AJIOFJ DPAIAWFJEWIO QUEUE id: %d   t  :%d\n",curr->pid,curr->rps.t);
    // remove requests that we don't want
    if(((unsigned int)curr->rps.t & (unsigned int)m) != 0b00000000){
      RemoveQ(queue,curr);
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

/****IPC*****/
//client thread
void Msg_Send(PID id, MTYPE t, unsigned int *v){
     //assign PD by id to receiver
     PD *receiver = &(Process[id-1]);
     if(receiver==NULL){
         OS_Abort(7);
     }
     //counter++;
     //printf("%d.SENDER ID: %d  state: ",counter,cp->pid);
     /*switch(cp->state){
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
     printf("%d.RECEIVER ID: %d  state: ",counter,receiver->pid);
     switch(receiver->state){
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
            }*/

     if(receiver->state==RECEIVEBLOCKED){
         //printf("receive block\n");
         receiver->rps.pid = Task_Pid();
         //message sent, receiver picks up message v
         receiver->rps.v = *v;
         //message type
         receiver->rps.t = t;
         //client thread becomes reply blocks
         cp->state = REPLYBLOCKED;
         receiver->state=READY;
         enqueue(receiver->reply_queue, (PD *)cp);
         printf("Sender: %d  sending message %d\n",cp->pid,*v);
         Task_Next();
         printf("Sender: %d  got reply message %d\n",cp->pid,cp->rps.r);
     }else{ /*receiver thread is not ready */
        //the client thread becomes sendblock
         //printf("sender block\n");
         cp->state = SENDBLOCKED;
         //add receiver to the sender queue
         enqueue(receiver->senders_queue, (PD *)cp);
         //printf("sender queue address %d\n", &(Process[1].senders_queue));
         cp->rps.pid = cp->pid;
         cp->rps.v = *v;
         cp->rps.t = t;
         cp->rps.status = 1;
         printf("Sender: %d  sending message %d\n",cp->pid,*v);
         Task_Next();
         printf("Sender: %d  got reply message %d\n",cp->pid,cp->rps.r);
         //we will be back here if receives reply
         //printf("IM BACK  reply    r: %d    status: %d\n",cp->rps.r,cp->rps.status);
     }
     
}


PID  Msg_Recv(MASK m, unsigned int *v ){

      //printf("RECEIVING MESSAGE: %d\n",cp->pid);
      PD *first_sender;
      //TODO FILTER
      //filter_unwantted_requests_for_msg_recv(cp->senders_queue,m);
      //printf("length of queue %d\n", ItemsInQ(cp->senders_queue));
      first_sender = getFirstTypeMatchingProcess(cp->senders_queue,m);
      //printf("sender queue pid at RECV %d\n\n", first_sender->pid);
      //no client thread has done sent
      if(first_sender == NULL){
         //printf("no sender\n");
         cp->state = RECEIVEBLOCKED;
         Task_Next();
         printf("Receiver: %d   received message: %d   from  sender %d\n",cp->pid,cp->rps.v,cp->rps.pid);
         return cp->rps.pid;
      }else{
         first_sender->state=REPLYBLOCKED;
         //add to reply queue 
         //printf("message copy\n");
         enqueue(cp->reply_queue, first_sender);
         //check sender message type and MASK
         cp->rps.pid = first_sender->pid;
         cp->rps.v = first_sender->rps.v;
         cp->rps.t = first_sender->rps.t;
         printf("Receiver: %d   received message: %d   from  sender %d\n",cp->pid,first_sender->rps.v,first_sender->pid);
         return first_sender->pid;
      }
     
}

void Msg_Rply(PID id, unsigned int r ){
    //set current cp to sender
    PD *sender = &(Process[id-1]);
    if(sender == NULL){
      OS_Abort(8);
    }
    if(InQueue(cp->reply_queue,sender)){
        //current reply to sender
        RemoveQ(cp->reply_queue, sender);
        sender->rps.r=r;
        sender->rps.status = 3;
        sender->state = READY;
        Task_Next();
        //TODO sechduler adds sender to Ready queue 
        //printf("REPLY DONE\n");
    }
}

//Asynchronous Communication
void Msg_ASend(PID id, MTYPE t, unsigned int v ){
     PD *receiver = &(Process[id-1]);
     if(receiver==NULL){
         OS_Abort(9);
     }
     //if p is waiting on receive
     if(receiver->state==RECEIVEBLOCKED){
         receiver->rps.pid = Task_Pid();
         //message sent, receiver picks up message v
         receiver->rps.v = v;
         //message type
         receiver->rps.t = t;

         receiver->state=READY;
     }
}

/********************************************************************************
*			T E S T    C O D E
*********************************************************************************/

ISR(TIMER4_COMPA_vect){
  ticks++;
  //counter++;
  //printf("INSIDE INTERRUPT SP:%p%p\n",cp->sp,cp->sp +1);
  //printf("%d.Entering timer interrupt, cp id: %d sp: %d\n",counter,cp->pid,cp->sp);
  //printf("number of: %d\n",ItemsInQ(&PeriodicProcess));

  if (KernelActive && cp->priority != 3) {
     cp ->kernel_request = NEXT;
     PutBackToReadyQueue((PD *)cp);
     Dispatch();
   }
    
}


void sys(){
  Task_Terminate();
}
void sys2(){
  //Disable_Interrupt();
  //printf("IN SYS2: %d\n",cp->pid);
  //Enable_Interrupt();
  Task_Terminate();
}
void Blink()
{
  while(1){

  }
}
void Blink2()
{
  while(1){
    _delay_ms(2000);
  }
}

void send_message(){
    unsigned int a=100;
    Msg_Send(2, 'a', &a);
}

void receive_message(){
    unsigned int a=0;
    PID id = Msg_Recv(0xff,&a);
    Msg_Rply(id,500);
    //printf("final return pid %d\n\n", id);
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

void Send(){
  unsigned int a = 49;
  Msg_Send(1,'a',&a);
}

void Receive(){
  unsigned int a = 1;
  PID sender = Msg_Recv('a',&a);
  Msg_Rply(sender, 94);
}

void Asend(){
  Msg_ASend(1,'a',50);
}

//TEST System Process
// test preemption and priority
void test1(){
   Task_Create_System( sys,1);
   Task_Create_System( sys,1);
   Task_Create_Period( Blink, 1, 4,1,0);
   Task_Create_Period( Blink2, 1, 4,1,1);
   Task_Create_RR(Blink, 1);
   Task_Create_RR(Blink, 1);
}

//TEST Send Receive Reply
void test2(){
   Task_Create_System(Send, 1);
   Task_Create_System(Receive, 1);
}

//TEST Send Receive Reply
void test3(){
   Task_Create_System(Receive, 1);
   Task_Create_System(Send, 1);
}

//TEST MAXProcess
void test4(){
    int i;
	for (i = 0; i < 20; i++) {
		Task_Create_RR(Blink, i);
	}
}

// Timing conflict
void test5(){
  Task_Create_Period( Blink ,1, 4,1,0);
  Task_Create_Period( Blink2,1, 4,1,0);
}

//Async send fail
void test6(){
  Task_Create_System(Asend,1);
  Task_Create_System(Receive,1);
}

//Async send success
void test7(){
  Task_Create_System(Receive,1);
  Task_Create_System(Asend,1);
}

//test sendIPC
void testIPC(){
    
    Task_Create_System(Blink_IPC, 1);
    Task_Create_System(Blink2_IPC, 1);
    
}

//int main() 
//{
//   uart_init();
//   stdout = &uart_output;
//   stdin = &uart_input;
//   
//   cli();
//   //DDRB=0x83;
//   OS_Init();
//   //test5();
//   //setupTimer();
//   config();
//   Task_Create_System(checkJoystick,1);
//   sei();
//   OS_Start();
//   return -1;
//}