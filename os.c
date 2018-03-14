#include <avr/io.h>
#include <avr/interrupt.h>

#include "os.h"


// A structure to represent a queue
struct Queue
{
    struct ProcessDescriptor *head;
    struct ProcessDescriptor *tail;
}

struct IPCRequest
{
  union{
    struct{
        unsigned PID pid;
        unsigned int v; //send message
        unsigned int r; //reply message
        MTYPE t; //messag type
        
    }send,receive,reply;
  }
}

typedef enum priority_level {
    SYSTEM = 3,
    PERIODIC = 2,
    RR = 1,
} PRIORITY_LEVEL;

typedef struct ProcessDescriptor 
{
   unsigned PID pid;    //process id
   unsigned char *sp;   /* stack pointer into the "workSpace" */
   unsigned char workSpace[WORKSPACE]; 
   ProcessState state;
   voidfuncptr  code;   /* function to be executed as a task */
   kernel_request_type request; //request type
   struct Queue *senders_queue; //sender queue
   struct Queue *reply_queue;  //reply queue
   struct ProcessDescriptor *recipient; //recipient of message
   struct IPCRequest rps; //IPC request
   struct ProcessDescriptor *next;
   int arg;
   PRIORITY_LEVEL priority; //The priority of the task
} PD;

typedef enum kernel_request_type 
{
   NONE = 0,
   CREATE,
   NEXT,
   TERMINATE
   SEND, 
   RECEIVE, 
   REPLY
} KERNEL_REQUEST_TYPE;

enum ProcessState {DEAD, READY, SLEEP, RUNNING, SUSPEND, SENDBLOCKED,
                   RECEIVEBLOCKED, REPLYBLOCKED};


/**
  * This table contains ALL process descriptors. It doesn't matter what
  * state a task is in.
  */
static PD Process[MAXTHREAD];

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


struct ProcessDescriptor *getProcess(Pid id);

struct Queue *system_queue;
struct Queue *periodic_queue;
struct Queue *rr_queue;

/* add process to add of the queue */
void enqueue(struct Queue *queue, struct ProcessDescriptor *p){
    if(queue->head==NULL){
        queue->head = p;
        queue->tail = p;
        queue->next = NULL;
    }else{
        queue->tail->next = p;
        queue->tail = p;
    }
}

/* delete the head from a queue */
static struct ProcessDescriptor *dequeue(struct Queue *queue)
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

  if(queue->head==NULL) return;     /* empty queue */
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
                prev->next = curr->next;
                p->next = NULL;
        }
  }
  if (queue->head == NULL) queue->tail = NULL;
}


/**Kernel Codd**/
void Kernel_Create_Task_At( PD *p, voidfuncptr f ) 
{   
   unsigned char *sp;

#ifdef DEBUG
   int counter = 0;
#endif

   //Changed -2 to -1 to fix off by one error.
   sp = (unsigned char *) &(p->workSpace[WORKSPACE-1]);

   /*----BEGIN of NEW CODE----*/
   //Initialize the workspace (i.e., stack) and PD here!

   //Clear the contents of the workspace
   memset(&(p->workSpace),0,WORKSPACE);

   //Notice that we are placing the address (16-bit) of the functions
   //onto the stack in reverse byte order (least significant first, followed
   //by most significant).  This is because the "return" assembly instructions 
   //(rtn and rti) pop addresses off in BIG ENDIAN (most sig. first, least sig. 
   //second), even though the AT90 is LITTLE ENDIAN machine.

   //Store terminate at the bottom of stack to protect against stack underrun.
   *(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
   *(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;
   *(unsigned char *)sp-- = 0x00;

   //Place return address of function at bottom of stack
   *(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
   *(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;
   *(unsigned char *)sp-- = 0x00;

#ifdef DEBUG
   //Fill stack with initial values for development debugging
   //Registers 0 -> 31 and the status register
   for (counter = 0; counter < 33; counter++)
   {
      *(unsigned char *)sp-- = counter;
   }
#else
   //Place stack pointer at top of stack
   sp = sp - 34;
#endif
      
   p->sp = sp;		/* stack pointer into the "workSpace" */
   p->code = f;		/* function to be executed as a task */
   p->request = NONE;

   /*----END of NEW CODE----*/

   p->state = READY;
   
}


/**
  *  Create a new task
  */
static void Kernel_Create_Task( voidfuncptr f ) 
{
   int x;

   if (Tasks >= MAXTHREAD) return;  /* Too many task! */

   /* find a DEAD PD that we can use  */
   for (x = 0; x < MAXTHREAD; x++) {
       if (Process[x].state == DEAD) break;
   }

   if(x<MAXTHREAD){
       ++Tasks;
       Kernel_Create_Task_At( &(Process[x]), f );
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
       cp->request = NONE; /* clear its request */

       /* activate this newly selected task */
       CurrentSp = cp->sp;
       Exit_Kernel();    /* or CSwitch() */

       /* if this task makes a system call, it will return to here! */

        /* save the Cp's stack pointer */
       cp->sp = CurrentSp;

       switch(cp->request){
       case CREATE:
           Kernel_Create_Task( cp->code );
           break;
       case NEXT:
	   case NONE:
           /* NONE could be caused by a timer interrupt */
          cp->state = READY;
          Dispatch();
          break;
       case TERMINATE:
          /* deallocate all resources used by this task */
          cp->state = DEAD;
          Dispatch();
          break;
       default:
          /* Houston! we have a problem here! */
          break;
       }
    } 
}



static void Dispatch(){
    /* find the next READY task
    * Note: if there is no READY task, then this will loop forever!.
    */
    if (cp->state != RUNNING)
	{
        if (system_queue.head != NULL)
		{
			cp = Dequeue(&system_queue);
		}else if(periodic_queue.head != NULL){
            cp = Dequeue(&periodic_queue);
        }else if(rr_queue.head != NULL){
            cp = Dequeue(&rr_queue);
        }else{
            //idle
        }
    }
    CurrentSp = cp->sp;
    cp->state = RUNNING;
}


/*================
  * RTOS  API  and Stubs
  *================
  */

/**
  * This function initializes the RTOS and must be called before any other
  * system calls.
  */
void OS_Init() 
{
   int x;

   Tasks = 0;
   KernelActive = 0;
   NextP = 0;
	//Reminder: Clear the memory for the task on creation.
   for (x = 0; x < MAXTHREAD; x++) {
      memset(&(Process[x]),0,sizeof(PD));
      Process[x].state = DEAD;
   }
    
   cp->state = READY;
}


/**
  * This function starts the RTOS after creating a few tasks.
  */
void OS_Start() 
{   
   if ( (! KernelActive) && (Tasks > 0)) {
       Disable_Interrupt();
      /* we may have to initialize the interrupt vector for Enter_Kernel() here. */

      /* here we go...  */
      KernelActive = 1;
      Kernel_main_loop();
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


// The calling task gets its initial "argument" when it was created.
int  Task_GetArg(void){
     return cp->arg;
}

// It returns the calling task's PID.
PID  Task_Pid(void){
     return cp->pid;
}

struct ProcessDescriptor *getProcess(Pid id){
    unsigned int i;
    if(id>0){
        i=(id-1)%MAXTHREAD;
        return &(Process[i]);
    }
    return NULL;
}


/****IPC*****/
//client thread
void Msg_Send(PID id, MTYPE t, unsigned int *v){
     struct PD *receiver;
     //assign PD by id to receiver
     receiver = getProcess(id);
     if(receiver==NULL){
         return;
     }
     //current process recipient to receiver
     cp->recipient = receiver；
     if(receiver->state==RECEIVEBLOCKED){
         receiver->rps.receive.pid = Task_Pid();
         //message sent, receiver picks up message v
         receiver->rps.receive.v = v;
         //message type
         receiver->rps.receive.t = t;
         //client thread becomes reply blocks
         cp->state = REPLYBLOCKED;
         enqueue(&(receiver->reply_queue), cp);
         receiver->state = READY;
         Dispatch();
     }else{ /*receiver thread is not ready */
        //the client thread becomes sendblock
         cp->state = SENDBLOCKED;
         //add receiver to the sender queue
         enqueue(&(receiver->senders_queue), cp);
         Dispatch();
     }
     
}

PID  Msg_Recv(MASK m, unsigned int *v ){
      struct PD *first_sender;
      first_sender = dequeue(&(cp->senders_queue));
      //no client thread has done sent
      if(first_sender == NULL || ((unsigned int)first_sender->rps.send.t) & (unsigned int)m ==0b00000000){
         cp->state = RECEIVEBLOCKED;
         Dispatch();
         return &(cp->rps.send.pid);
      }else{
         first_sender->state=REPLYBLOCKED;
         //add to reply queue 
         enqueue(&(cp->reply_queue), first_sender);
         //check sender message type and MASK
         cp->rps.receive.pid = first_sender->pid;
         cp->rps.receive.v = first_sender->rps.send.v;
         cp->rps.receive.t = first_sender->rps.send.t;
         return  &(cp->rps.receive.pid);
      }
     
}

void Msg_Rply(PID id, unsigned int r ){
    struct PD *sender;
    //set current cp to sender
    sender = getProcess(id);
    if(sender->state == REPLYBLOCKED){
        //current reply to sender
        RemoveQ(&(sender->reply_queue), sender);
        sender->rps.send.r=r;
        sender->recipient = NULL:
        sender->state = READY;
        //TODO sechduler adds sender to Ready queue 
        Dispatch();
    }
}

//Asynchronous Communication
void Msg_ASend(PID id, MTYPE t, unsigned int v ){
     struct PD *receiver;
     receiver = getProcess(id);
     if(receiver==NULL){
         return;
     }
     //if p is waiting on receive
     if(receiver-->state==RECEIVEBLOCKED){
         receiver->rps.receive.pid = id;
         //message sent, receiver picks up message v
         receiver->rps.receive.v = v;
         //message type
         receiver->rps.receive.t = t;
         //dont have to wait for reply
     }else{
         Dispatch();
     }
     //else not op
}