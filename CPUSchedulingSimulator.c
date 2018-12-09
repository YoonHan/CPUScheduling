#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESS_NUM 15

#define FCFS 1
#define SJF  2
#define SJF_PRE  3
#define PRIORITY 4
#define PRIORITY_PRE 5
#define RR 6

#define TRUE 1
#define FALSE 0
#define GANTT_UNIT_NUM 200
#define GANTT_BLOCK_NUM 50

// Process
typedef struct Process* processPointer;
typedef struct Process{
    int pid;
    int priority;
    int arrivalTime;
    int CPUburst;
    int IOburst;
    int CPUremainingTime;
    int IOremainingTime;
    int waitingTime;
    int turnaroundTime;
}Process;

// Gantt chart variables and functions
typedef struct GanttBlock* ganttBlockPointer;
typedef struct GanttBlock{
    int pid;
    int burstTime;
}GanttBlock;
int ganttUnits[GANTT_UNIT_NUM];              // For storing gantt units
int GU_num = 0;
ganttBlockPointer ganttBlocks[GANTT_BLOCK_NUM];  // each element is for one gantt block
int GB_num = 0;

/* Global variables */
processPointer runningProcess = NULL;   // currently running process
int timeConsumed = 0;                   // processing time of a process
int computationIdle = 0;
int computationStart = 0;
int computationEnd = 0;
int mode = 0;
int totalProcessNum = 0;
int ioProcessNum = 0;
int timeQuantum = 0;
int IOprobability = 1;

void initializeGanttUnits(){
    int i;
    GU_num = 0;
    for(i = 0; i < GU_num; i++) ganttUnits[i] = NULL;
}

void initializeGanttBlocks(){
    int i;
    GB_num = 0;
    for(i = 0; i < GANTT_BLOCK_NUM; i++) ganttBlocks[i] = NULL;
}

void ganttUnitToBlock(int* gantt_units){
    int i;
    int pid = gantt_units[0];
    int count = 0;
    // initialize ganttBlocks
    initializeGanttBlocks();

    for(i = 0; i < GU_num - 1; i++){
        if(pid == gantt_units[i]) count++;
        if(i != GU_num - 2 && pid != gantt_units[i+1]){
            ganttBlockPointer gbp = (ganttBlockPointer)malloc(sizeof(struct GanttBlock));
            gbp->pid = pid;
            gbp->burstTime = count;
            ganttBlocks[GB_num++] = gbp;
            pid = ganttUnits[i+1];
            count = 0;
        }
        if(i == GU_num - 2){
            if(pid == gantt_units[i+1]){
                ganttBlockPointer gbp = (ganttBlockPointer)malloc(sizeof(struct GanttBlock));
                gbp->pid = pid;
                gbp->burstTime = count + 1;
                ganttBlocks[GB_num++] = gbp;
            } else {
                ganttBlockPointer gbp1 = (ganttBlockPointer)malloc(sizeof(struct GanttBlock));
                ganttBlockPointer gbp2 = (ganttBlockPointer)malloc(sizeof(struct GanttBlock));
                gbp1->pid = pid;
                gbp1->burstTime = count;
                gbp2->pid = gantt_units[i+1];
                gbp2->burstTime = 1;
                ganttBlocks[GB_num++] = gbp1;
                ganttBlocks[GB_num++] = gbp2;
            }
        }
    }
}

void drawGanttChart(ganttBlockPointer* gantt_blocks){
    int i, j;
    int accumulatedTime = 0;
    // top
    printf("\n");
    printf("\t ****** GANTT CHART ******\n\n");
    printf(" ");
    for(i = 0; i < GB_num; i++){
        for(j = 0; j < gantt_blocks[i]->burstTime; j++) printf("--");
        printf(" ");
    }
    printf("\n|");
    // middle
    for(i = 0; i < GB_num; i++){
        if(gantt_blocks[i]->pid != 0){
            for(j = 0; j < gantt_blocks[i]->burstTime - 1; j++) printf(" ");
            printf("P%d", gantt_blocks[i]->pid);
            for(j = 0; j < gantt_blocks[i]->burstTime - 1; j++) printf(" ");
            printf("|");
        } else {
            for(j = 0; j < gantt_blocks[i]->burstTime; j++) printf("XX");
            printf("|");
        }
    }
    printf("\n ");
    // bottom
    for(i = 0; i < GB_num; i++){
        for(j = 0; j < gantt_blocks[i]->burstTime; j++) printf("--");
        printf(" ");
    }
    printf("\n");
    // timeline
    printf("0");
    for(i = 0; i < GB_num; i++){
        for(j = 0; j < gantt_blocks[i]->burstTime; j++) printf("  ");
        if(gantt_blocks[i]->burstTime > 9) printf("\b");
        if(accumulatedTime > 9) printf("\b");
        accumulatedTime += gantt_blocks[i]->burstTime;      // for RR algorithm
        printf("%d", accumulatedTime);
    }
    printf("\n");
}

void print_GU(int* test){               // need to be deleted later
    int i;
    for(i = 0; i < GU_num; i++){
        printf("%d\n", test[i]);
    }
}

void print_GB(ganttBlockPointer* test){ // need to be deleted later
    int i;
    for(i = 0; i < GB_num; i++){
        printf("pid:%d, burst:%d\n", test[i]->pid, test[i]->burstTime);
    }
}

/* Process Queue, Ready Queue, Waiting Queue, Terminated Queue */
// Creating Queues needed
processPointer processQueue[MAX_PROCESS_NUM];
processPointer readyQueue[MAX_PROCESS_NUM];
processPointer waitingQueue[MAX_PROCESS_NUM];
processPointer terminatedQueue[MAX_PROCESS_NUM];
int PQ_process_num = 0;
int RQ_process_num = 0;
int WQ_process_num = 0;
int TQ_process_num = 0;

/* functions related to PQ */
void insertInto_PQ(processPointer process){
    processQueue[PQ_process_num++] = process;
}
// sort processes in processQueue by their arrivalTime(ascend order)
void sort_PQ(){
    int i, j;
    processPointer temp;
    for(i = 1; i < PQ_process_num; i++){
        temp = processQueue[(j=i)];
        while(--j >= 0 && temp->arrivalTime < processQueue[j]->arrivalTime) processQueue[j+1] = processQueue[j];
        processQueue[j+1] = temp;
    }
}

/* functions related to RQ */
void insertInto_RQ(processPointer process){
    readyQueue[RQ_process_num++] = process;
}

int getProcByPid_RQ(int pid){
    int i;
    for(i = 0; i < RQ_process_num; i++){
        if(readyQueue[i]->pid == pid) return i;
    }
}

processPointer removeFrom_RQ(processPointer proc){
    if(RQ_process_num > 0){
        int temp = getProcByPid_RQ(proc->pid);

        processPointer removed = readyQueue[temp];
        int i;
        for(i = temp; i < RQ_process_num - 1; i++) readyQueue[i] = readyQueue[i+1];
        readyQueue[RQ_process_num - 1] = NULL;
        RQ_process_num--;
        return removed;
    } else {
        printf("Ready queue is empty");
        return NULL;
    }
}

void clear_RQ(){
    int i;
    for(i = 0; i < MAX_PROCESS_NUM; i++){
        readyQueue[i] = NULL;
    }
    RQ_process_num = 0;
}

/* functions related to WQ */
void insertInto_WQ(processPointer process){
    waitingQueue[WQ_process_num++] = process;
}

int getProcByPid_WQ(int pid){
    int i;
    for(i = 0; i < WQ_process_num; i++){
        if(waitingQueue[i]->pid == pid) return i;
    }
}

processPointer removeFrom_WQ(processPointer proc){
    if(WQ_process_num > 0){
        int temp = getProcByPid_WQ(proc->pid);

        processPointer removed = waitingQueue[temp];
        int i;
        for(i = temp; i < WQ_process_num - 1; i++) waitingQueue[i] = waitingQueue[i+1];
        waitingQueue[WQ_process_num - 1] = NULL;
        WQ_process_num--;
        return removed;
    } else {
        printf("Waiting queue is empty");
        return NULL;
    }
}

void clear_WQ(){
    int i;
    for(i = 0; i < MAX_PROCESS_NUM; i++){
        waitingQueue[i] = NULL;
    }
    WQ_process_num = 0;
}

/* functions related to TQ */
void insertInto_TQ(processPointer process){
    terminatedQueue[TQ_process_num++] = process;
}

void clear_TQ(){
    int i;
    for(i = 0; i < MAX_PROCESS_NUM; i++){
        terminatedQueue[i] = NULL;
    }
    TQ_process_num = 0;
}
// Create Processes
processPointer createProcess(int pid, int priority, int arrivalTime, int CPUburst, int IOBurst){
    processPointer newProcess = (processPointer)malloc(sizeof(struct Process));
    newProcess->pid = pid;
    newProcess->priority = priority;
    newProcess->arrivalTime = arrivalTime;
    newProcess->CPUburst = CPUburst;
    newProcess->IOburst = IOBurst;
    newProcess->CPUremainingTime = CPUburst;
    newProcess->IOremainingTime = IOBurst;
    newProcess->waitingTime = 0;
    newProcess->turnaroundTime = 0;

    insertInto_PQ(newProcess);
    return newProcess;
}

void createProcesses(int tot_proc_num, int io_proc_num){
    srand(time(NULL));

    int i;
    for(i = 0; i < tot_proc_num; i++){
        // CPU burst range is from 3 to 10
        // IO burst range is from 1 to 5
        createProcess(i+1, rand() % tot_proc_num + 1, rand() % (tot_proc_num + 5), rand() % 8 + 3, 0);
    }
    for(i = 0; i < io_proc_num; i++){
        int randomIndex = rand() % tot_proc_num;
        if(processQueue[randomIndex]->IOburst == 0){
            int randomIOburst = rand() % 5 + 1;     // IO burst [1:5]
            processQueue[randomIndex]->IOburst = randomIOburst;
            processQueue[randomIndex]->IOremainingTime = randomIOburst;
        } else i--;
    }
    sort_PQ();
}

// Initialize process info for iteration
void initializeProcessInfo(){
    int i;
    for(i = 0; i < PQ_process_num; i++){
        processQueue[i]->waitingTime = 0;
        processQueue[i]->turnaroundTime = 0;
        processQueue[i]->CPUremainingTime = processQueue[i]->CPUburst;
        processQueue[i]->IOremainingTime = processQueue[i]->IOburst;
    }
}
/* Scheduling algorithm and simulation */
void printProcessTable(processPointer* PQ){
    int i;
    printf("\n\n");
    printf("\t ****** Process Table ******\n\n");
    puts("+-----+------------+---------+--------------+-----------------+--------------+----------+");
    puts("| PID | Burst Time | IO Time | Waiting Time | Turnaround Time | Arrival Time | Priority |");
    puts("+-----+------------+---------+--------------+-----------------+--------------+----------+");
    for(i = 0; i < PQ_process_num; i++){
        printf("| %2d  |     %2d     |   %2d    |      %2d      |        %2d       |      %2d      |    %2d    |\n"
               , PQ[i]->pid, PQ[i]->CPUburst, PQ[i]->IOburst, PQ[i]->waitingTime
               , PQ[i]->turnaroundTime, PQ[i]->arrivalTime,PQ[i]->priority);
        puts("+-----+------------+---------+--------------+-----------------+--------------+----------+");
    }
}

void evaluateAlgorithm(){
    int waitingTotal = 0;
    int turnaroundTotal = 0;
    int i;
    processPointer p = NULL;
    for(i = 0; i < TQ_process_num; i++){
        p = terminatedQueue[i];
        waitingTotal += p->waitingTime;
        turnaroundTotal += p->turnaroundTime;
    }
    printf("\n\n");
    printf("\t ****** Evaluate Algorithm ******\n\n");
    puts("===========================================================================\n");
    printf("\t START TIME : %d || END TIME : %d || CPU UTILIZATION : %.2f%% \n\n", computationStart, computationEnd
           ,(double)(computationEnd - computationIdle) / (computationEnd - computationStart) * 100);

    printf("\t Average Waiting Time : %.2f \n\n", (double)(waitingTotal / TQ_process_num));
    printf("\t Average Turnaround Time : %.2f \n\n", (double)(turnaroundTotal / TQ_process_num));
    puts("===========================================================================");
}

processPointer FCFS_alg(){
    processPointer earliestProc = readyQueue[0];
    if(earliestProc != NULL){
        if(runningProcess != NULL) return runningProcess;
        else removeFrom_RQ(earliestProc);
    } else return runningProcess;           // there is no process in readyQueue
}

processPointer SJF_alg(int preemptive){
    processPointer shortestJobProc = readyQueue[0];

    if(shortestJobProc != NULL){
        int i;
        // pick shortestJobProc
        for(i = 0; i < RQ_process_num; i++){
            if(readyQueue[i]->CPUremainingTime <= shortestJobProc->CPUremainingTime){
                if(readyQueue[i]->CPUremainingTime == shortestJobProc->CPUremainingTime){
                    if(readyQueue[i]->arrivalTime < shortestJobProc->arrivalTime) shortestJobProc = readyQueue[i];
                } else {
                    shortestJobProc = readyQueue[i];
                }
            }
        }

        if(runningProcess != NULL){
            if(preemptive){
                if(runningProcess->CPUremainingTime >= shortestJobProc->CPUremainingTime){
                    if(runningProcess->CPUremainingTime == shortestJobProc->CPUremainingTime){
                        if(runningProcess->arrivalTime < shortestJobProc->arrivalTime) return runningProcess;
                        else if(runningProcess->arrivalTime == shortestJobProc->arrivalTime) return runningProcess;
                    }
                    puts("Preemption is occured!");
                    insertInto_RQ(runningProcess);
                    return removeFrom_RQ(shortestJobProc);
                }
                return runningProcess;
            }
            return runningProcess;          // non-preemptive
        } else {
            return removeFrom_RQ(shortestJobProc);
        }

    } else {
        return runningProcess;
    }
}

processPointer PRIORITY_alg(int preemptive){
    processPointer priorityProc = readyQueue[0];

    if(priorityProc != NULL){
        int i;
        // pick lower priority process
        for(i = 0; i < RQ_process_num; i++){
            if(readyQueue[i]->priority <= priorityProc->priority){
                if(readyQueue[i]->priority == priorityProc->priority){
                    if(readyQueue[i]->arrivalTime == priorityProc->arrivalTime) priorityProc = readyQueue[i];
                } else priorityProc = readyQueue[i];
            }
        }

        if(runningProcess != NULL){
            if(preemptive){
                if(runningProcess->priority >= priorityProc->priority){
                    if(runningProcess->priority == priorityProc->priority){
                        if(runningProcess->arrivalTime < priorityProc->arrivalTime) return runningProcess;
                        else if(runningProcess->arrivalTime == priorityProc->arrivalTime) return runningProcess;
                    }
                    puts("Preemption is occured");
                    insertInto_RQ(runningProcess);
                    return removeFrom_RQ(priorityProc);
                }
                return runningProcess;
            }
            return runningProcess;          // non-preemptive
        } else {
            return removeFrom_RQ(priorityProc);
        }

    } else {
        return runningProcess;
    }
}

processPointer RR_alg(int time_quantum){
    processPointer earliestProc = readyQueue[0];
    if(earliestProc != NULL){
        if(runningProcess != NULL){
            if(timeConsumed >= time_quantum){   // time limited
                insertInto_RQ(runningProcess);
                return removeFrom_RQ(earliestProc);
            } else return runningProcess;
        } else {
            return removeFrom_RQ(earliestProc);
        }
    } else {
        return runningProcess;
    }
}

processPointer scheduling(int alg, int preemptive, int time_quantum){
    processPointer selected = NULL;

    switch(alg){
    case FCFS:
        selected = FCFS_alg();
        break;
    case SJF:
        selected = SJF_alg(preemptive);
        break;
    case SJF_PRE:
        selected = SJF_alg(preemptive);
        break;
    case PRIORITY:
        selected = PRIORITY_alg(preemptive);
        break;
    case PRIORITY_PRE:
        selected = PRIORITY_alg(preemptive);
        break;
    case RR:
        selected = RR_alg(time_quantum);
        break;
    default:
        return NULL;
    }
    return selected;
}

void simulate(int currentTime, int alg, int preemptive, int time_quantum){
    int i;
    for(i = 0; i < PQ_process_num; i++){
        if(processQueue[i]->arrivalTime == currentTime) insertInto_RQ(processQueue[i]);
    }
    processPointer prevProcess = runningProcess;
    runningProcess = scheduling(alg, preemptive, time_quantum);

    printf("%d: ", currentTime);
    if(prevProcess != runningProcess){
        timeConsumed = 0;
    }
    // increasing waitingTime and turnaroundTime in RQ
    for(i = 0; i < RQ_process_num; i++){
        if(readyQueue[i]){
            readyQueue[i]->waitingTime++;
            readyQueue[i]->turnaroundTime++;
        }
    }
    // increasing turnaroundTime and decreasing IOremainingTime in WQ
    for(i = 0; i < WQ_process_num; i++){
        if(waitingQueue[i]){
            waitingQueue[i]->turnaroundTime++;
            waitingQueue[i]->IOremainingTime--;

            if(waitingQueue[i]->IOremainingTime <= 0){
                printf("(pid: %d) -> IO completed, ", waitingQueue[i]->pid);
                insertInto_RQ(removeFrom_WQ(waitingQueue[i--]));
            }
        }
    }
    // increasing turnaroundTime and decreasing CPUremainingTime of runningProcess
    if(runningProcess != NULL){
        runningProcess->CPUremainingTime--;
        runningProcess->turnaroundTime++;
        timeConsumed++;

        ganttUnits[GU_num++] = runningProcess->pid;     // put pid into ganttUnits

        printf("(pid: %d) -> running", runningProcess->pid);
        if(runningProcess->CPUremainingTime <= 0){
            insertInto_TQ(runningProcess);
            runningProcess = NULL;
            printf("-> terminated");
        } else {
            if(runningProcess->IOremainingTime > 0 && IOprobability == rand() % 4){     // randomize IO event
                insertInto_WQ(runningProcess);
                runningProcess = NULL;
                printf("-> IO request");
            }
        }
        printf("\n");
    } else {
        ganttUnits[GU_num++] = 0;                       // put idle into ganttUnits
        printf("idle\n");
        computationIdle++;
    }
}

void startSimulation(int alg, int preemptive, int time_quantum){
    int initial_process_num = PQ_process_num;           // save the initial number of processes for algorithm analyzing
    computationStart = processQueue[0]->arrivalTime;
    computationIdle = 0;
    int timeLapsed = 0;
    int isDone = FALSE;
    initializeGanttUnits();
    while(!isDone){
        simulate(timeLapsed, alg, preemptive, time_quantum);
        if(TQ_process_num == initial_process_num) isDone = TRUE;
        else timeLapsed++;
    }
    timeLapsed++;
    computationEnd = timeLapsed;
    /* Analyzing */
    printProcessTable(processQueue);        // Print All Process
    evaluateAlgorithm();                    // Print Evaluation result
    ganttUnitToBlock(ganttUnits);           // Transform Gantt Units to Gantt Blocks
    drawGanttChart(ganttBlocks);            // Draw Gantt Chart

    clear_RQ();
    clear_WQ();
    clear_TQ();
    initializeProcessInfo();
    runningProcess = NULL;
    timeConsumed = 0;
    computationIdle = 0;
    computationStart = 0;
    computationEnd = 0;
}

int main()
{
    printf("PLEASE ENTER THE NUMBER OF PROCESSES IN TOTAL : ");
    scanf("%d", &totalProcessNum);
    printf("PLEASE ENTER THE NUMBER OF IO PROCESSES IN TOTAL : ");
    scanf("%d", &ioProcessNum);
    printf("PLEASE ENTER TIME QUANTUM AMOUNT (at least greater than or equal to 3): ");
    scanf("%d", &timeQuantum);
    createProcesses(totalProcessNum, ioProcessNum);
    while(mode != 7){
        printf("\n\n");
        printf("================ MODE ===============\n");
        printf("\t1. FCFS\n");
        printf("\t2. SJF(non-preemptive)\n");
        printf("\t3. SJF(preemptive)\n");
        printf("\t4. PRIORITY(non-preemptive)\n");
        printf("\t5. PRIORITY(preemptive)\n");
        printf("\t6. ROUND ROBIN\n");
        printf("\t7. EXIT\n");
        printf("=====================================\n");
        printf("SELECT MODE : ");
        scanf("%d", &mode);

        switch(mode){
        case FCFS:
            printf("\n<FCFS Algorithm>\n");
            startSimulation(FCFS, FALSE, timeQuantum);
            break;
        case SJF:
            printf("\n<SJF Algorithm>\n");
            startSimulation(SJF, FALSE, timeQuantum);
            break;
        case SJF_PRE:
            printf("\n<Preemptive-SJF Algorithm>\n");
            startSimulation(SJF_PRE, TRUE, timeQuantum);
            break;
        case PRIORITY:
            printf("\n<Priority Algorithm>\n");
            startSimulation(PRIORITY, FALSE, timeQuantum);
            break;
        case PRIORITY_PRE:
            printf("\n<Preemptive-Priority Algorithm>\n");
            startSimulation(PRIORITY_PRE, TRUE, timeQuantum);
            break;
        case RR:
            printf("\n<Round Robin Algorithm>\n");
            startSimulation(RR, TRUE, timeQuantum);
            break;
        default:
            break;
        }
    }
    return 0;
}
