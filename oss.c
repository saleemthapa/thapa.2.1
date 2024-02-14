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

void initialize_clock(struct Clock *sys_clock) {
    sys_clock->seconds = 0;
    sys_clock->nanoseconds = 0;
}

void increment_clock(struct Clock *sys_clock, int incrementSeconds, int incrementNanoseconds) {
    sys_clock->nanoseconds += incrementNanoseconds;
    if (sys_clock->nanoseconds >= 1000000000) {
        sys_clock->nanoseconds -= 1000000000;
        sys_clock->seconds++;
    }
    sys_clock->seconds += incrementSeconds;
}

// Function to launch a new worker process
void launchWorkerProcess(struct PCB *process_table, struct Clock *sys_clock) {
    // Find an empty slot in the process table
    int i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (!process_table[i].occupied) {
            break;
        }
    }

    if (i == MAX_PROCESSES) {
        fprintf(stderr, "Error: Process table is full.\n");
        return; // No available slot in the process table
    }

    // Fork a new process
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        // Execute the worker program
        execl("worker", "worker", NULL); // Assuming the worker program is named "worker"

        // If execl returns, an error occurred
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        // Parent process

        // Update process table entry for the new child process
        process_table[i].occupied = true;
        process_table[i].pid = pid;
        process_table[i].startSeconds = sys_clock->seconds;
        process_table[i].startNano = sys_clock->nanoseconds;

        printf("Launched new worker process with PID %d.\n", pid);
	printf("Process Table Entry %d: Occupied: 1, PID: %d, StartS: %d, StartN: %d\n",
               i, process_table[i].pid, process_table[i].startSeconds, process_table[i].startNano);
    
    }
}

int main(int argc, char *argv[]) {
    int process = 3; // Default value, replace with your desired value
    int interval = 200; // Default value, replace with your desired value

    // Parse command line options using getopt
    int opt;
    while ((opt = getopt(argc, argv, "hn:i:")) != -1) {
        switch (opt) {
            case 'h':
                // Help option, display usage information
                printf("Usage: %s [-n proc] [-i intervalInMsToLaunchChildren]\n", argv[0]);
                exit(EXIT_SUCCESS);
                break;

            case 'n':
                // Number of total children to launch
                process = atoi(optarg);
                break;

            case 'i':
                // Interval in milliseconds to launch children
                interval = atoi(optarg);
                break;

            case '?':
                // Invalid option or missing argument
                if (optopt == 'n' || optopt == 'i') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                }
                break;

            default:
                // Unexpected case
                abort();
        }
    }

    // Initialize shared memory for clock
    int shmid_sys_clock = shmget(SHM_KEY_CLOCK, SHM_SIZE_CLOCK, IPC_CREAT | 0666);
    if (shmid_sys_clock == -1) {
        perror("shmget (sys_clock)");
        exit(EXIT_FAILURE);
    }

    // Attach to shared memory for clock
    struct Clock *sys_clock = (struct Clock *)shmat(shmid_sys_clock, NULL, 0);
    if (sys_clock == (void *) -1) {
        perror("shmat (sys_clock)");
        exit(EXIT_FAILURE);
    }

    // Initialize clock
    initialize_clock(sys_clock);

    // Print message indicating successful initialization
    printf("Successfully initialized shared memory for sys_clock\n");

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


    // Main loop
    bool stillChildrenToLaunch = true;
    while (stillChildrenToLaunch) {
        // Increment system clock
        increment_clock(sys_clock, 0, interval * 1000000); // Increment by 'interval' milliseconds

        // Launch new worker process if the number of simultaneous children is less than the limit
        int activeProcesses = 0;
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (process_table[i].occupied) {
                activeProcesses++;
            }
        }

        // Check if the maximum number of simultaneous worker processes has been reached
        if (activeProcesses < process) {
            launchWorkerProcess(process_table, sys_clock);
        }

        // Sleep for a short period before the next iteration
        usleep(1000); // Sleep for 1 millisecond
    }

    // Cleanup: Detach from shared memory
    shmdt(sys_clock);
    shmdt(process_table);

    // Remove shared memory segments
    shmctl(shmid_sys_clock, IPC_RMID, NULL);
    shmctl(shmid_process_table, IPC_RMID, NULL);

    return 0;
}
