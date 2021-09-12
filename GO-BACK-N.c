#include <stdio.h>
#include <stdlib.h> /* for malloc, free, srand, rand */
#include <string.h>
#include "debugging_gbn.h"
#include "packet_buffer.h"

/*******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */


/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Here I define some function prototypes to avoid warnings */
/* in my compiler. Also I declared these functions void and */
/* changed the implementation to match */
void init();
void generate_next_arrival();

int base;
int nextseqnum;
int expectedseqnum;

float estimatedRTT = 500;
float initialTime;

// const int windowSize = 8;
#define windowSize 8
//struct pkt packets[windowSize];

struct pkt sndpkt;
struct sender_debug sender_debug;
struct receiver_debug receiver_debug;
struct packet_buffer sender_buffer;

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = (float)0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/



void init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();


   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(9999);              /* init random number generator */
   sum = (float)0.0;         /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/(float)1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" );
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=(float)0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
  double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */
  x = (float)(rand()/mmm);            /* x should be uniform in [0,1] */
  return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void insertevent(struct event *p) {
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime);
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q;
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
   /* char *malloc(); // malloc redefinition removed */
   /* float ttime; // Unreferenced local variable removed */
   /* int tempint; // Unreferenced local variable removed */

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  (float)(time + x);
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
}

void printevlist()
{
  struct event *q;
  /* int i; // Unreferenced local variable removed */
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB)  /* A or B is trying to stop timer */ 
{
 struct event *q;/* ,*qold; // Unreferenced local variable removed */

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next)
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB, float increment) {
 struct event *q;
 struct event *evptr;
 /* char *malloc(); // malloc redefinition removed */

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }

/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  (float)(time + increment);
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
}


/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet) {
 struct pkt *mypktptr;
 struct event *evptr,*q;
 /* char *malloc(); // malloc redefinition removed */
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)
	printf("          TOLAYER3: packet being lost\n");
      return;
    }

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next)
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) )
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();



 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)
	printf("          TOLAYER3: packet being corrupted\n");
    }

  if (TRACE>2)
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
}

void tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)
        printf("%c",datasent[i]);
     printf("\n");
   }

}

//checksum takes seqnum, acknum, and payload into account to find corruption
int checksum(struct pkt packet){
	int checkSumValue = 0;

	checkSumValue += (packet.seqnum + packet.acknum);

   for(int i = 0; i < 20; i++) {
      checkSumValue += packet.payload[i];
   }

	return checkSumValue;
}

void A_send_packet(struct pkt packet)
{
  fprintf(sender_debug.sent_seqnums, "%d\n", packet.seqnum);
  tolayer3(0,packet);
}

void dump_packet_payload(struct pkt packet, FILE *fp)
{
  static char message_string[sizeof(packet.payload)+1];

  memset(message_string, 0, sizeof(message_string));
  memcpy(message_string, packet.payload, sizeof(packet.payload));
  fprintf(fp, "%s\n", message_string);
  fflush(fp);  
}

/* called from layer 5, passed the data to be sent to other side */
int A_output(struct msg message) {
  //checks to make sure that server is only wainting on at most 10 unacknowledged packets
  struct pkt packet;

  for (int i = 0; i < 20; i++)
    packet.payload[i] = message.data[i];

  /* packet.seqnum = nextseqnum; */
  /* packet.acknum = 0; */
  /* packet.checksum = checksum(packet); */
  if (!packet_buffer_push(&sender_buffer, message))
    return 0;
  struct pkt *next;
  if ((next = packet_buffer_get_next_for_window(&sender_buffer)) != NULL)
  {
    next->checksum = checksum(*next);
    A_send_packet(*next);
    
    /* tolayer3(0,packet); */
    if(sender_buffer.base_seq == sender_buffer.next_seq-1) {
      initialTime = time;
      starttimer(0,estimatedRTT);
    }
    return 1;
  }
  else
    return 0;
/* if(nextseqnum < base + windowSize) { */
/*         //make a packet that will be sent to B */

/*         packets[nextseqnum % windowSize] = packet; */

/*         //Sends the packet to layer 3 of B */
/* 	A_send_packet(packet); */
/*         /\* tolayer3(0,packet); *\/ */

/*         if(base == nextseqnum) { */
/*             initialTime = time; */
/*             starttimer(0,estimatedRTT); */
/*         } */

/*         nextseqnum++; */

/*         return (1); /\* Return a 0 to refuse the message *\/ */
/*     } */
/*     else { */
/*       dump_packet_payload(packet, sender_debug.dropped); */
/*       return (0); */
/*     } */
}

void A_debug_input(struct pkt packet)
{
  fprintf(sender_debug.received_acks, "%d\n", packet.acknum);
  fprintf(sender_debug.checksum_valid, "%d %d\n",
	  packet.checksum,
	  checksum(packet));
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
   //std::cout << "Layer 4 on side A has recieved a packet sent over the network from side B:" << packet << std::endl;
   //std::cout << "base = " << base << ", nextseqnum = " << nextseqnum << ", ack = " << packet.acknum << std::endl;
   printf("Side A has recieved a packet sent over the network from side B\n");

    //check if packet is corrupted
    //printf("%d, %d\n", packet.checksum, checksum(packet));
   A_debug_input(packet);
    if(packet.checksum == checksum(packet)) {
       printf("Checksum worked and acknowledgement number = %d\n", packet.acknum);
       // base = packet.acknum + 1;
       packet_buffer_recv_ack(&sender_buffer, packet.acknum);
        //RTT is complete for this packet, so we can use it in estimatedRTT calculation
        if(sender_buffer.base_ptr == sender_buffer.next_seq) {
            stoptimer(0);
            float sampleRTT = time - initialTime;
            estimatedRTT = (0.875 * estimatedRTT) + (0.125 * sampleRTT);
        }
        else {
            //restarts the timer
            initialTime = time;
            starttimer(0, estimatedRTT);
        }
    }
}

/* called when A's timer goes off */
void A_timerinterrupt() {
   //std::cout << "Side A's timer has gone off." << std::endl;
    
    //restart our time so estimatedRTT is calculated correctly later
   initialTime = time;
   starttimer(0,estimatedRTT);
   printf("A: TIMERINTERRUPT!\n");
    //Loop through packets that need to be resent based on most recent packet acknowledged (base)
   /* for(int t = base; t <= nextseqnum - 1; t++){ */
   /*      //Uses saved packets in array to resend */
   /* 		struct pkt startPacket = packets[t % windowSize];  */
   /*      //Sends packet back to B */
   /* 		//tolayer3(0,startPacket); */
   /* 		A_send_packet(startPacket); */
   /* 	} */
   for (struct pkt *p_i = packet_buffer_unacked_begin(&sender_buffer);
	//p_i != packet_buffer_unacked_end(&sender_buffer);
	!packet_buffer_iter_is_past_window(&sender_buffer, p_i);
	p_i = packet_buffer_next(&sender_buffer, p_i))
   {
     A_send_packet(*p_i);
   }
   
}

#define QSIZE 50 // from lab.

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
   /* base = 1; */
   /* nextseqnum = 1; */
   init_sender_debug(&sender_debug);
   sender_buffer.base_seq = 1;   
   sender_buffer.base_ptr = 0;

   sender_buffer.next_seq = 1;
   sender_buffer.next_seq_ptr = 1;

   sender_buffer.qsize = QSIZE;
   sender_buffer.window_size = windowSize;

   packet_buffer_allocate(&sender_buffer, QSIZE);
}


void B_debug_packet(struct pkt packet)
{
  static char expected = 0;  
  dump_packet_payload(packet, receiver_debug.received_packets);
  if (packet.payload[0] != (expected + 'a'))
    abort();
  else
    expected = (expected + 1) % 26;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
   //std::cout << "Layer 4 on side B has recieved a packet from layer 3 sent over the network from side A:" << packet << std::endl;

    //if packet is the correct seqnum and not corrupted
    printf("B: got: %s, seqnum: %d expected: %d\n", packet.payload, packet.seqnum, expectedseqnum);
    if(packet.checksum == checksum(packet) && packet.seqnum == expectedseqnum){
        //std::cout << "Packet is good, sending ack for packet " << packet.seqnum << std::endl;
        //extract the message
        struct msg message;
        for (int i = 0; i < 20; i++)
            message.data[i] = packet.payload[i];

        //delivers the message
	B_debug_packet(packet);
        tolayer5(1,message.data);


        //set the most recent correct packet
        sndpkt.seqnum = expectedseqnum;
        sndpkt.acknum = packet.seqnum;

        printf("Setting new packet: %d, %d\n", expectedseqnum, packet.seqnum);

        for (int i = 0; i < 20; i++)
            sndpkt.payload[i] = message.data[i];

        sndpkt.checksum = checksum(sndpkt);

        //send the acknowledgment packet back to A
        tolayer3(1, sndpkt);
        printf("Sending back to Packet A\n");

        //we are ready for the next packet in order to be received
        expectedseqnum++;
    }
    else {
        //sends the default if packet has wrong seqnum or is corrupted
        tolayer3(1, sndpkt);
    }
}

int B_output(struct msg message)  /* need be completed only for extra credit */ {
   //std::cout << "Layer 4 on side B has recieved a message from the application that should be sent to side A: " << message << std::endl;

    return (1); /* Return a 0 to refuse the message */
}

/* called when B's timer goes off */
void B_timerinterrupt() {
  printf("**BINT!\n");
   //std::cout << "Side B's timer has gone off." << std::endl;
}
   

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
   expectedseqnum = 1;
   sndpkt.seqnum = 0;
   sndpkt.acknum = 0;	
   sndpkt.checksum = 0;
   init_receiver_debug(&receiver_debug);
}

int main() {
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;

   int i,j;
   /* char c; // Unreferenced local variable removed */

   init();
   A_init();
   B_init();

   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */
            j = nsim % 26;
            for (i=0; i<20; i++)
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++)
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A)
               A_output(msg2give);
             else
               B_output(msg2give);
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A)
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
}
