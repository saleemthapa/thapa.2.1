#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <stdbool.h>

#define SHM_KEY_CLOCK 987654321
#define SHM_KEY_PROCESS_TABLE 5678
#define SHM_SIZE_CLOCK sizeof(struct Clock)
#define SHM_SIZE_PROCESS_TABLE sizeof(struct PCB) * MAX_PROCESSES

#define MAX_PROCESSES 20

struct Clock {
    int seconds;
    int nanoseconds;
};

struct PCB {
    bool occupied;
    pid_t pid;
    int startSeconds;
    int startNano;
};

void initialize_clock(struct Clock *clock) {
    clock->seconds = 0;
    clock->nanoseconds = 0;
}

void increment_clock(struct Clock *clock, int incrementSeconds, int incrementNanoseconds) {
    clock->nanoseconds += incrementNanoseconds;
    if (clock->nanoseconds >= 1000000000) {
        clock->nanoseconds -= 1000000000;
        clock->seconds++;
    }
    clock->seconds += incrementSeconds;
}

int main(int argc, char *argv[]) {
    int process = 3;
    int simulation = 2;
    int time_limit = 7;
    int interval = 200;
    int opt;

    // Parse command line options using getopt
    while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) {
        switch (opt) {
            case 'h':
                // Help option, display usage information
                printf("Usage: %s [-n proc] [-s simul] [-t timelimitForChildren] [-i intervalInMsToLaunchChildren]\n", argv[0]);
                exit(EXIT_SUCCESS);
                break;

            case 'n':
                // Number of total children to launch
                process = atoi(optarg);
                break;

            case 's':
                // Number of simultaneous children to allow
                simulation = atoi(optarg);
                break;

            case 't':
                // Time limit for children
                time_limit = atoi(optarg);
                break;

            case 'i':
                // Interval in milliseconds to launch children
                interval = atoi(optarg);
                break;

            case '?':
                // Invalid option or missing argument
                if (optopt == 'n' || optopt == 's' || optopt == 't' || optopt == 'i') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                }
		else {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
                exit(EXIT_FAILURE);
                break;

            default:
                // Unexpected case
                abort();
        }
    }

    // Initialize shared memory for clock
	int shmid_clock = shmget(SHM_KEY_CLOCK, SHM_SIZE_CLOCK, IPC_CREAT | 0666);
    		if (shmid_clock == -1) {
        	perror("shmget (clock)");
        	exit(EXIT_FAILURE);
    	}
 // Attach to shared memory for clock
	struct Clock *clock = (struct Clock *)shmat(shmid_clock, NULL, 0);
    	if (clock == (void *) -1) {
        	perror("shmat (clock)");
        	exit(EXIT_FAILURE);
    	}

 // Initialize clock
    clock->seconds = 0;
    clock->nanoseconds = 0;

    // Print message indicating successful initialization
    printf("Successfully initialized shared memory for clock\n");
//Deatch shared clock
	shmdt(clock);
// Initialize shared memory for process table
    int shmid_process_table = shmget(SHM_KEY_PROCESS_TABLE, SHM_SIZE_PROCESS_TABLE, IPC_CREAT | 0666);
    if (shmid_process_table == -1) {
        perror("shmget (process table)");
        exit(EXIT_FAILURE);
    }

    // Attach to shared memory for process table
    struct PCB *process_table = (struct PCB *)shmat(shmid_process_table, NULL, 0);
    if (process_table == (void *) -1) {
        perror("shmat (process table)");
        exit(EXIT_FAILURE);
    }

    // Initialize process table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].occupied = false;
        process_table[i].pid = 0;
        process_table[i].startSeconds = 0;
        process_table[i].startNano = 0;
    }
// Print message indicating successful initialization of process table shared memory
    printf("Successfully initialized shared memory for process table\n");

    // Detach from shared memory for process table
    shmdt(process_table);
    // Main loop
    while (1) {
        // Increment system clock

        // Launch new worker process

        // Check if any child process has terminated

        // Update process table

        // Check if the maximum number of simultaneous worker processes has been reached

        // Sleep for a short period before the next iteration
        usleep(1000); // Sleep for 1 millisecond
    }


    // Remove shared memory

    return 0;
}

