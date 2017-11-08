// ALP PISKIN -- PROJECT 3

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
 
#define MAXTHREADS 10

// Msg struct from the assignment
struct msg {
    int iFrom;
    int value;
    int cnt;
    int tot;
};

// Message request for NB part
struct msgreq {
		int ito;
		struct msg mes;
};
 
// Mailbox struct that is associated with each thread
struct mailbox {
    int t_index;
    struct msg current;
    sem_t rsem;
    sem_t ssem;
    struct msg total;
};
 
// Struct for I/O part
struct input_ll {
    int from;
    int to;
    struct input_ll* next;
};
 
// Function Prototypes
struct input_ll* getInputFromStdin();
void SendMsg(int iTo, struct msg *pmsg);
int NBSendMsg(int iTo, struct msg *pmsg);
void RecvMsg(int iFrom, struct msg *pmsg);
void InitMailbox(int index);
void *adder(void * arg);

// Global Array of Mailboxes
struct mailbox *mailbox_list;
 
int main(int argc, char** argv) {
		
		// Argument check
    if (argc < 1 || argc > 3) {
        perror("ERROR: Invalid Input\n");
        exit(1);
    }
		
		// Argument check for NB
		int status = 1;
		if (argc > 2) {
			char *nbinput = (char *)malloc(3 * sizeof(char));
			nbinput = argv[2];
			if (strcmp(nbinput, "nb") == 0) {
					status = 0;
			}
			else {
					perror("ERROR: Invalid Input\n");
					exit(1);
			}
		}
		if (status) {
			printf("Normal Mode:\n");
		}
		else {
			printf("NB Mode:\n");
		}
		
		// Initializes the iterator by reading input and filling the struct
    struct input_ll* iterator = getInputFromStdin();
    struct input_ll* root = iterator;
		
		// Just an alternative safety check
		/*
	 	int count = iterator->to;
    while(iterator->next != NULL){
			  iterator = iterator->next;
				count++;
    }
 		*/

		// Determine the thread number and check for overflow and invalid thread number input
    int thread_num = atoi(argv[1]);
    if (thread_num <= 0) {
        perror("ERROR: Invalid Thread Number\n");
        exit(1);
    }
 
    if (atoi(argv[1]) > MAXTHREADS) {
        thread_num = MAXTHREADS;
    }

		/*
		if (count > thread_num) {
			printf("Requested threads are less than required threads: \nRequested: %d\nRequired (Safe Option): %d\nWill proceed with %d threads\n\n", thread_num, count, count);
			thread_num = count;
		}
 		*/

    thread_num++;
 
    mailbox_list = (struct mailbox *) malloc((thread_num) * sizeof(struct mailbox));
 		
		// Array of ids of threads
    pthread_t thread_list[thread_num];
 		
		// Initialize Mailboxes with incrementing index
		// Pthread_create call with adder
    for(int i = 0; i < thread_num; i++) {
        InitMailbox(i);
        struct mailbox * currentmb = &mailbox_list[i];
        if(i != 0) if (pthread_create(&thread_list[i], NULL, adder, (void *) currentmb) != 0) {
            perror("Child Thread creation error!\n");
            exit(1);
        }
    }
 		
		// Return to default iterator
    iterator = root;
		
		// Send messages read from the iterator
    while(iterator->next != NULL){
        struct msg m;
        m.value = iterator->from;
        SendMsg(iterator->to, &m);
        iterator = iterator->next;
    }
 		
		// Send -1 messages and receive the summary messages from children then destroy threads and semaphores
    for (int i = 1; i < thread_num; i++) {
        struct msg m;
        m.value = -1;
        SendMsg(i, &m);
        RecvMsg(0, &m);
        printf("The result from thread %d is %d from %d operations during %d seconds\n", i, m.value, m.cnt, m.tot);
        (void)pthread_join(thread_list[i], NULL);
        sem_t *ptr = &mailbox_list[i].rsem;
        (void)sem_destroy(ptr);
    } 
    return 0;
}
 
void InitMailbox(int index) {
    struct mailbox *mb = &mailbox_list[index];
    mb->t_index = index;
    mb->total.iFrom = index;
    if (sem_init(&mb->ssem, 0, 1) < 0) {
        perror("Child Thread: Semaphore Sender Error!\n");
        exit(1);
    }
    if (sem_init(&mb->rsem, 0, 0) < 0) {
        perror("Child Thread: Semaphore Receiver Error!\n");
        exit(1);
    }
}

// SendMsg and RecvMsg inspired from Producer/Consumer
 
void SendMsg(int iTo, struct msg *pmsg) {
    struct mailbox* mb = &mailbox_list[iTo];
    sem_wait(&mb->ssem);
    mb->current = *pmsg; 
    sem_post(&mb->rsem);
}

int NBSendMsg(int iTo, struct msg *pmsg) {
		struct mailbox* mb = &mailbox_list[iTo];
		int result = sem_trywait(&mb->ssem);
		mb->current = *pmsg;
		sem_post(&mb->rsem);
		return result;
}

 
// Fills the struct inside
void RecvMsg(int iFrom, struct msg *pmsg) {
    struct mailbox* mb = &mailbox_list[iFrom];
    sem_wait(&mb->rsem);
		pmsg->cnt = mb->current.cnt;
    pmsg->iFrom = mb->current.iFrom;
    pmsg->tot = mb->current.tot;
    pmsg->value = mb->current.value;
    sem_post(&mb->ssem);
}
 
void *adder(void *arg) {
		// Argument initialization
		int start = (int)time(NULL);
    struct mailbox *mb;
    mb = (struct mailbox *)arg;
		
		// Receive messages and if value is -1 stop
		// If not increment on count and add the value of the new message to the accumulated values so far
    while(1){
        struct msg m;
        RecvMsg(mb->t_index, &m);
        if(m.value < 0){
            break;
        } else {
            mb->total.value += m.value;
            mb->total.cnt ++;
						sleep(1); // Sleep gracefully
        }
    }
		int end = (int)time(NULL);
		mb->total.tot = end - start;
    SendMsg(0, &mb->total); // Summary message (total struct = summary message container)
    return NULL;
}
 
// I/O..
struct input_ll* getInputFromStdin(){
    struct input_ll* root = (struct input_ll*) malloc(sizeof(struct input_ll));
    struct input_ll* temp = root;
 
    unsigned str_size = 200;
    char s[str_size];
 
    int current_line = 1;
 
    while(1){
        char* success = fgets(s, str_size, stdin);
        if(success){
            int from;
            int to;
            int count = sscanf(s, "%d %d", &from, &to);
            if(count == 2 && from > 0 ){
                struct input_ll* next = (struct input_ll*) malloc(sizeof(struct input_ll));
                temp->from = from;
                temp->to = to;
                temp->next = next;
                temp = next;    
            } else if (count != 2) {
                printf("ERROR: End of File has been reached earlier at line: %d\n", current_line);
                break;
            } else {
                printf("ERROR IN FILE, Line: %d\n", current_line);
            }   
            current_line++;
        } else {
            break;
        }
    }
 
    return root;
}
