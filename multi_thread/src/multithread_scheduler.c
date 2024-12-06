#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_TASKS 10
#define QUANTUM 10

typedef struct task {
    int tid;
    char name[4];
    int priority;
    int burst;
    int waiting_time;
    int turnaround_time;
    int response_time;
    int remaining_time;
    int first_response;
} Task;

pthread_mutex_t mutex;
int global_time = 0;  

// Atomic assignment of TIDs
int assign_tid() {
    static int tid_counter = 0;
    return __sync_fetch_and_add(&tid_counter, 1);
}

// Simulate task execution with multithreading
void *run_task_fcfs(void *arg) {
    Task *task = (Task *)arg;

    pthread_mutex_lock(&mutex);

    
    task->waiting_time = global_time;
    task->response_time = task->waiting_time;

    printf("Running task %s (tid: %d) for %d ms\n", task->name, task->tid, task->burst);
    
    usleep(task->burst * 1000);  

    global_time += task->burst;
    task->turnaround_time = global_time;

    pthread_mutex_unlock(&mutex);

    pthread_exit(0);
}

// First-Come, First-Served (FCFS) multithreaded
void fcfs_multithread(Task *tasks, int n) {
    pthread_t threads[MAX_TASKS];

    for (int i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, run_task_fcfs, (void *)&tasks[i]);
        pthread_join(threads[i], NULL);  
    }
}

// Shortest Job First (SJF) multithreaded
void sjf_multithread(Task *tasks, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (tasks[i].burst > tasks[j].burst) {
                Task temp = tasks[i];
                tasks[i] = tasks[j];
                tasks[j] = temp;
            }
        }
    }
    fcfs_multithread(tasks, n);  
}
// Shortest Remaining Time First (SRTF) multithreaded
void srtf_multithread(Task *tasks, int n) {
    int complete = 0, time = 0;
    int min_burst, shortest;
    int check = 0;

    while (complete != n) {
        min_burst = 999;
        shortest = -1;

        
        for (int i = 0; i < n; i++) {
            if (tasks[i].remaining_time > 0 && tasks[i].remaining_time < min_burst) {
                min_burst = tasks[i].remaining_time;
                shortest = i;
                check = 1;
            }
        }

        if (shortest == -1) {
            time++;
            continue;
        }

        
        printf("Running task %s (tid: %d) for 1 ms\n", tasks[shortest].name, tasks[shortest].tid);

        
        if (tasks[shortest].first_response == 0) {
            tasks[shortest].response_time = time;
            tasks[shortest].first_response = 1;
        }

        tasks[shortest].remaining_time -= 1;
        time++;

        if (tasks[shortest].remaining_time == 0) {
            complete++;
            tasks[shortest].turnaround_time = time;
            tasks[shortest].waiting_time = time - tasks[shortest].burst;
        }
    }
}


// Round-Robin (RR) multithreaded
void rr_multithread(Task *tasks, int n, int quantum) {
    int time = 0;
    int remaining_tasks = n;
    
    while (remaining_tasks > 0) {
        for (int i = 0; i < n; i++) {
            if (tasks[i].remaining_time > 0) {
                pthread_mutex_lock(&mutex);

                if (tasks[i].first_response == 0) {
                    tasks[i].response_time = time;
                    tasks[i].first_response = 1;
                }

                pthread_mutex_unlock(&mutex);

                int slice = (tasks[i].remaining_time > quantum) ? quantum : tasks[i].remaining_time;

                printf("Running task %s (tid: %d) for %d ms\n", tasks[i].name, tasks[i].tid, slice);
                usleep(slice * 1000);  
                pthread_mutex_lock(&mutex);
                time += slice;  
                tasks[i].remaining_time -= slice;

                
                if (tasks[i].remaining_time == 0) {
                    tasks[i].turnaround_time = time;
                    tasks[i].waiting_time = time - tasks[i].burst;
                    remaining_tasks--;
                }

                pthread_mutex_unlock(&mutex);
            }
        }
    }
}

// Calculate average times
void calculate_average_times(Task *tasks, int n) {
    int total_waiting_time = 0, total_turnaround_time = 0, total_response_time = 0;
    for (int i = 0; i < n; i++) {
        total_waiting_time += tasks[i].waiting_time;
        total_turnaround_time += tasks[i].turnaround_time;
        total_response_time += tasks[i].response_time;
        printf("Task %s (tid: %d) - Waiting time: %d, Turnaround time: %d, Response time: %d\n",
               tasks[i].name, tasks[i].tid, tasks[i].waiting_time, tasks[i].turnaround_time, tasks[i].response_time);
    }

    printf("Average Waiting Time: %.2f\n", (float)total_waiting_time / n);
    printf("Average Turnaround Time: %.2f\n", (float)total_turnaround_time / n);
    printf("Average Response Time: %.2f\n", (float)total_response_time / n);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <schedule file> <algorithm>\n", argv[0]);
        return -1;
    }

    char *filename = argv[1];
    char *algorithm = argv[2];
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Could not open file %s\n", filename);
        return -1;
    }

    Task tasks[MAX_TASKS];
    int n = 0;

    // Read tasks from file and assign TIDs atomically
    while (fscanf(file, "%s %d %d", tasks[n].name, &tasks[n].priority, &tasks[n].burst) != EOF) {
        tasks[n].tid = assign_tid();
        tasks[n].remaining_time = tasks[n].burst;
        tasks[n].first_response = 0;
        n++;
    }
    fclose(file);

    pthread_mutex_init(&mutex, NULL);  

    // Choose the scheduling algorithm
    if (strcmp(algorithm, "fcfs") == 0) {
        fcfs_multithread(tasks, n);  
    } else if (strcmp(algorithm, "sjf") == 0) {
        sjf_multithread(tasks, n);  
    } else if (strcmp(algorithm, "srtf") == 0) {
        srtf_multithread(tasks, n);  // 
    } else if (strcmp(algorithm, "rr") == 0) {
        rr_multithread(tasks, n, QUANTUM);  
    } else {
        printf("Unknown algorithm: %s\n", algorithm);
        return -1;
    }

    
    calculate_average_times(tasks, n);

    pthread_mutex_destroy(&mutex);  

    return 0;
}