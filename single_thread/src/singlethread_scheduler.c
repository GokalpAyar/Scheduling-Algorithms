#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int start_time;        
    int first_response;    
} Task;

// Atomic assignment of TIDs
int assign_tid() {
    static int tid_counter = 0;
    return __sync_fetch_and_add(&tid_counter, 1); 
}


void run(Task *task, int slice) {
    printf("Running task %s (tid: %d) for %d ms\n", task->name, task->tid, slice);
}

// First-Come, First-Served (FCFS)
void fcfs(Task *tasks, int n) {
    int time = 0;
    for (int i = 0; i < n; i++) {
        tasks[i].start_time = time;
        tasks[i].waiting_time = time;
        time += tasks[i].burst;
        tasks[i].turnaround_time = time;
        tasks[i].response_time = tasks[i].waiting_time;
        run(&tasks[i], tasks[i].burst);
    }
}

// Shortest Job First (SJF)
void sjf(Task *tasks, int n) {
    
    for (int i = 0; i < n-1; i++) {
        for (int j = i+1; j < n; j++) {
            if (tasks[i].burst > tasks[j].burst) {
                Task temp = tasks[i];
                tasks[i] = tasks[j];
                tasks[j] = temp;
            }
        }
    }
    fcfs(tasks, n);  
}

// Shortest Remaining Time First (SRTF)
void srtf(Task *tasks, int n) {
    int time = 0, complete = 0;
    while (complete != n) {
        int min_burst = 999, shortest = -1;
        for (int i = 0; i < n; i++) {
            if (tasks[i].remaining_time > 0 && tasks[i].remaining_time < min_burst) {
                min_burst = tasks[i].remaining_time;
                shortest = i;
            }
        }

        if (shortest == -1) {
            time++;
            continue;
        }

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
        run(&tasks[shortest], 1);
    }
}

// Round-Robin (RR)
void rr(Task *tasks, int n, int quantum) {
    int time = 0;
    int remaining_tasks = n;
    
    while (remaining_tasks > 0) {
        for (int i = 0; i < n; i++) {
            if (tasks[i].remaining_time > 0) {
                if (tasks[i].first_response == 0) {
                    tasks[i].response_time = time;
                    tasks[i].first_response = 1;
                }
                
                if (tasks[i].remaining_time > quantum) {
                    run(&tasks[i], quantum);
                    time += quantum;
                    tasks[i].remaining_time -= quantum;
                } else {
                    run(&tasks[i], tasks[i].remaining_time);
                    time += tasks[i].remaining_time;
                    tasks[i].remaining_time = 0;
                    tasks[i].turnaround_time = time;
                    tasks[i].waiting_time = time - tasks[i].burst;
                    remaining_tasks--;
                }
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
        printf("Task %s (tid: %d) - Waiting time: %d, Turnaround time: %d, Response time: %d\n", tasks[i].name, tasks[i].tid, tasks[i].waiting_time, tasks[i].turnaround_time, tasks[i].response_time);
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

    // Choose the scheduling algorithm
    if (strcmp(algorithm, "fcfs") == 0) {
        fcfs(tasks, n);
    } else if (strcmp(algorithm, "sjf") == 0) {
        sjf(tasks, n);
    } else if (strcmp(algorithm, "srtf") == 0) {
        srtf(tasks, n);
    } else if (strcmp(algorithm, "rr") == 0) {
        rr(tasks, n, QUANTUM);
    } else {
        printf("Unknown algorithm: %s\n", algorithm);
        return -1;
    }


    calculate_average_times(tasks, n);

    return 0;
}