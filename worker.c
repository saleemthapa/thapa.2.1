
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdbool.h>

#define SHM_KEY_CLOCK 987654321
#define SHM_SIZE_CLOCK sizeof(struct Clock)

struct Clock {
    int seconds;
    int nanoseconds;
};

void print_initial_info(pid_t pid, pid_t ppid, struct Clock *clock, int term_seconds, int term_nanoseconds) {
    printf("WORKER PID:%d PPID:%d SysClockS: %d SysclockNano: %d TermTimeS: %d TermTimeNano: %d\n",
           pid, ppid, clock->seconds, clock->nanoseconds, term_seconds, term_nanoseconds);
    printf("--Just Starting\n");
}

void print_time_passed(pid_t pid, pid_t ppid, struct Clock *clock, int seconds_passed) {
    printf("WORKER PID:%d PPID:%d SysClockS: %d SysclockNano: %d TermTimeS: %d TermTimeNano: %d\n",
           pid, ppid, clock->seconds, clock->nanoseconds, clock->seconds + seconds_passed, clock->nanoseconds);
    printf("--%d seconds have passed since starting\n", seconds_passed);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s seconds nanoseconds\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int seconds = atoi(argv[1]);
    int nanoseconds = atoi(argv[2]);

    // Attach to shared memory for clock
    int shmid_clock = shmget(SHM_KEY_CLOCK, SHM_SIZE_CLOCK, 0666);
    if (shmid_clock == -1) {
        perror("shmget (clock)");
        exit(EXIT_FAILURE);
    }
    struct Clock *clock = (struct Clock *)shmat(shmid_clock, NULL, 0);
    if (clock == (void *) -1) {
        perror("shmat (clock)");
        exit(EXIT_FAILURE);
    }
    else{
	printf("Successfully attached to shared memory for clock\n");
	}

    // Get process IDs
    pid_t pid = getpid();
    pid_t ppid = getppid();

    // Calculate termination time
    int term_seconds = clock->seconds + seconds;
    int term_nanoseconds = clock->nanoseconds + nanoseconds;
    if (term_nanoseconds >= 1000000000) {
        term_seconds += 1;
        term_nanoseconds -= 1000000000;
    }

    // Print initial information
    print_initial_info(pid, ppid, clock, term_seconds, term_nanoseconds);

    // Loop until termination time is reached
    int seconds_passed = 0;
    while (clock->seconds < term_seconds || (clock->seconds == term_seconds && clock->nanoseconds < term_nanoseconds)) {
        if (clock->seconds > (term_seconds - seconds_passed)) {
            // Print final message and terminate if time has elapsed
            printf("WORKER PID:%d PPID:%d SysClockS: %d SysclockNano: %d TermTimeS: %d TermTimeNano: %d\n",
                   pid, ppid, clock->seconds, clock->nanoseconds, term_seconds, term_nanoseconds);
            printf("--Terminating\n");
            break;
        }

        // Print time passed if seconds have changed
        if (clock->seconds > seconds_passed) {
            print_time_passed(pid, ppid, clock, ++seconds_passed);
        }
    }

    // Detach from shared memory
    shmdt(clock);

    return 0;
}
