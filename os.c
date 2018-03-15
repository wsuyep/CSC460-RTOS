#include <avr/io.h>
#include <avr/interrupt.h>

#include "os.h"
#include "kernel.h"
#include "util.h"

/********************************************************************************
*			G L O B A L    V A R I A B L E S
*********************************************************************************/
static PD Process[MAXTHREAD];

static Queue SystemProcess;
static Queue PeriodicProcess;
static Queue RoundRobinProcess;
static Queue DeadPool;

volatile static unsigned int NumSysTasks;
volatile static unsigned int NumPeriodTasks;
volatile static unsigned int NumRRTasks;
volatile static unsigned int Tasks;

volatile static PD* cp;

volatile unsigned char *KernelSp;
volatile unsigned char *CurrentSp;

volatile static unsigned int NextP;

volatile static unsigned int KernelActive;

/********************************************************************************
*			OS (Kernel methods)
*********************************************************************************/

static void Dispatch()
{
     /* find the next READY task
       * Note: if there is no READY task, then this will loop forever!.
       */
    while(1){
       if(SystemProcess.head!=NULL){
           cp = dequeue(&SystemProcess);
           break;
       }else if(PeriodicProcess.head!=NULL){
           cp = dequeue(&PeriodicProcess);
           break;
       }else if(RoundRobinProcess.head!=NULL){
           cp = dequeue(&RoundRobinProcess);
           break;
       }else{
           //TODO IDLE
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
   p->state = READY;
   p->senders_queue.head = NULL;
   p->senders_queue.tail = NULL;
   p->reply_queue.head = NULL;
   p->reply_queue.tail = NULL;
   p->rps.pid = 0;
   p->rps.v = 0;
   p->rps.r = 0;
   p->rps.t = 0;
   p->rps.status=0;
   Tasks++;
   p->next =NULL; //TODO: put it into ready queue
}

static void Kernel_Create_Task( voidfuncptr f ) 
{
   int x;

   if (Tasks == MAXPROCESS) return;  /* Too many task! */

   /* find a DEAD PD that we can use  */
   for (x = 0; x < MAXPROCESS; x++) {
       if (Process[x].state == DEAD) break;
   }

   ++Tasks;
   Kernel_Create_Task_At( &(Process[x]), f );

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
       Cp->request = NONE; /* clear its request */

       /* activate this newly selected task */
       CurrentSp = Cp->sp;
       Exit_Kernel();    /* or CSwitch() */

       /* if this task makes a system call, it will return to here! */

        /* save the Cp's stack pointer */
       Cp->sp = CurrentSp;

       switch(Cp->request){
       case CREATE:
           Kernel_Create_Task( Cp->code );
           break;
       case NEXT:
	   case NONE:
           /* NONE could be caused by a timer interrupt */
          Cp->state = READY;
          Dispatch();
          break;
       case TERMINATE:
          /* deallocate all resources used by this task */
          Cp->state = DEAD;
          Dispatch();
          break;
       default:
          /* Houston! we have a problem here! */
          break;
       }
    } 
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
  * For this example, we only support cooperatively multitasking, i.e.,
  * each task gives up its share of the processor voluntarily by calling
  * Task_Next().
  */
void Task_Create( voidfuncptr f)
{
   if (KernelActive ) {
     Disable_Interrupt();
     Cp ->request = CREATE;
     Cp->code = f;
     Enter_Kernel();
   } else { 
      /* call the RTOS function directly */
      Kernel_Create_Task( f );
   }
}

/**
  * The calling task gives up its share of the processor voluntarily.
  */
void Task_Next() 
{
   if (KernelActive) {
     Disable_Interrupt();
     Cp ->request = NEXT;
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
      Cp -> request = TERMINATE;
      Enter_Kernel();
     /* never returns here! */
   }
}


//TODO
PID Task_Create_System(void (*f)(void), int arg){
    
}

PID Task_Create_RR(void (*f)(void), int arg){
    
}

PID Task_Create_Period(void (*f)(void), int arg, TICK period, TICK wcet, TICK offset){
    
}

void Task_Next(void){
    
}



/********************************************************************************
*			IPC SRR
*********************************************************************************/

/****IPC*****/
//client thread
void Msg_Send(PID id, MTYPE t, unsigned int *v){
     struct PD *receiver;
     //assign PD by id to receiver
     receiver = getProcess(id);
     if(receiver==NULL){
         return;
     }

     if(receiver->state==RECEIVEBLOCKED){
         receiver->rps.pid = Task_Pid();
         //message sent, receiver picks up message v
         receiver->rps.v = v;
         //message type
         receiver->rps.t = t;
         //client thread becomes reply blocks
         cp->state = REPLYBLOCKED;
         receiver->state=READY;
         enqueue(&(receiver->reply_queue), cp);
         // TODO: add receiver back to ready queue
         dispatch();
     }else{ /*receiver thread is not ready */
        //the client thread becomes sendblock
         cp->state = SENDBLOCKED;
         //add receiver to the sender queue
         enqueue(&(receiver->senders_queue), cp);
         Dispatch();
     }
     
}

void filter_unwantted_requests_for_msg_recv(struct ProcessQueue *queue, MASK m){
	struct ProcessDescriptor *curr = queue->head;
	while(curr !=NULL){
		struct ProcessDescriptor *tmp = curr;
		curr = curr->next;
		// remove requests that we don't want
		if(((unsigned int)tmp->rps.send.t & (unsigned int)m) == 0b00000000){
			RemoveQ(queue,tmp);
		}
	}
}

PID  Msg_Recv(MASK m, unsigned int *v ){
      struct PD *first_sender;
      filter_unwantted_requests_for_msg_recv(&(cp->senders_queue),m);
      first_sender = dequeue(&(cp->senders_queue));

      //no client thread has done sent
      if(first_sender == NULL){
         cp->state = RECEIVEBLOCKED;
         Dispatch();
         return &(cp->rps.pid);
      }else{
         first_sender->state=REPLYBLOCKED;
         //add to reply queue 
         enqueue(&(cp->reply_queue), first_sender);
         //check sender message type and MASK
         cp->rps.pid = first_sender->pid;
         cp->rps.v = first_sender->rps.v;
         cp->rps.t = first_sender->rps.t;
         return  &(first_sender->pid);
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
     if(receiver-->state==RECEIVEBLOCKED){
         receiver->rps.pid = id;
         //message sent, receiver picks up message v
         receiver->rps.v = v;
         //message type
         receiver->rps.t = t;
         //dont have to wait for reply
     }else{
         Dispatch();
     }
     //else not op
}