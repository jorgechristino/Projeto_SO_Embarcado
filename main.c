#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#define MAX_PROCESSES 100
#define BUFFER_SIZE 10
#define INT_MAX 9999

#define SUCCESS 0
#define FAIL 1

typedef struct
{
    int id;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int priority;
    int finishTime;
    int quantum;
} Process;

// output file
FILE *output;

// processes declaration
Process processes[MAX_PROCESSES];

// circular buffer
Process *buffer[BUFFER_SIZE];
int start = 0, end = 0;
typedef int (*scheduler)(int numProcesses, int currentTime);

// initialize kernel
void kernelInit(void)
{
    start = 0;
    end = 0;
}

// adds process in buffer
char kernelAddProc(Process *newProc)
{
    // checking for free space
    if (((end + 1) % BUFFER_SIZE) != start)
    {
        buffer[end] = newProc;
        end = (end + 1) % BUFFER_SIZE;
        return SUCCESS;
    }
    return FAIL;
}

// remove process from buffer
void kernelRemoveProc()
{
    if (start != end)
    {
        start = (start + 1) % BUFFER_SIZE;
    }
}

// reads the input file
int readFile()
{
    FILE *file;
    int numProcesses = 0;
    int arrivalTime, burstTime, priority;

    file = fopen("../input.txt", "r");

    if (file == NULL)
    {
        printf("Error opening the file.\n");
        return 0;
    }

    // read each line from the file and create the processes
    while (fscanf(file, "%d %d %d", &arrivalTime, &burstTime, &priority) == 3)
    {
        Process p;
        p.id = numProcesses + 1;
        p.arrivalTime = arrivalTime;
        p.burstTime = burstTime;
        p.remainingTime = burstTime;
        p.priority = priority;
        p.quantum = 0;
        processes[numProcesses++] = p;
    }

    fclose(file);

    return numProcesses;
}

// print processes in current time
void printProcesses(int numProcesses, int currentProcess, int currentTime)
{
    int i;

    fprintf(output, "\n[%d-%d]\t", currentTime, currentTime + 1);
    for (i = 0; i < numProcesses; i++)
    {
        if (i == currentProcess)
        {
            fprintf(output, "##\t");
        }
        else
        {
            if (processes[i].arrivalTime <= currentTime && processes[i].remainingTime > 0)
            {
                fprintf(output, "--\t");
            }
            else
            {
                fprintf(output, "  \t");
            }
        }
    }
}

// gets sum of the total time of all processes
int getTime(int numProcesses)
{
    int i;
    int total = 0;
    for (i = 0; i < numProcesses; i++)
    {
        total += processes[i].burstTime;
    }
    return total;
}

// gets sum of the quantum of all processes
int getQuantum(int numProcesses)
{
    int i;
    int total = 0;
    for (i = 0; i < numProcesses; i++)
    {
        total += processes[i].quantum;
    }
    return total;
}

// strn scheduler
int srtn(int numProcesses, int currentTime)
{
    int shortestTime = INT_MAX;
    int chosenProcess = -1;
    // choose the process with the shortest remaining time
    for (int i = 0; i < numProcesses; i++)
    {
        if (processes[i].arrivalTime <= currentTime && processes[i].remainingTime < shortestTime && processes[i].remainingTime > 0)
        {
            chosenProcess = i;
            shortestTime = processes[i].remainingTime;
        }
    }
    return chosenProcess;
}

// guaranteed scheduler
int guaranteed(int numProcesses, int currentTime)
{
    float shortestFactor = INT_MAX;
    int chosenProcess = -1;
    // In a system with n processes, ideally each process
    // must receive 1/n of the time
    // Let h is the total time spent by a process on the CPU
    // Let t be the elapsed time since its creation
    // So t/n is the time this process should have used
    // Thus, f=h/(t/n) is a factor that determines whether the process used
    // more or less than it should.
    // If a process has f=0.5, it has used half of fair time.
    // You can scale the processes by smaller value of f.

    for (int i = 0; i < numProcesses; i++)
    {
        int h = processes[i].burstTime;
        int t = currentTime - processes[i].arrivalTime;
        float f;
        if (t > 0)
        {
            f = (float)h / ((float)t / (float)numProcesses);
        }
        else
        {
            f = 0;
        }

        if (processes[i].arrivalTime <= currentTime && f < shortestFactor && processes[i].remainingTime > 0)
        {
            chosenProcess = i;
            shortestFactor = f;
        }
    }
    return chosenProcess;
}

// fair share scheduler
int fairShare(int numProcesses, int currentTime)
{
    int quantum = 0;
    int chosenProcess = -1;
    int highestPriority = 0;
    quantum = getTime(numProcesses) / numProcesses; // divides time by number of processes

    // if all quantum is 0, each process gets total quantum again
    if (getQuantum(numProcesses) == 0)
    {
        for (int i = 0; i < numProcesses; i++)
        {
            processes[i].quantum = quantum;
        }
    }

    for (int i = 0; i < numProcesses; i++)
    {
        if (processes[i].remainingTime == 0)
        { // define quantum 0 se o processo já tiver sido finalizado
            processes[i].quantum = 0;
        }
        if (processes[i].arrivalTime <= currentTime && processes[i].quantum != 0 && processes[i].priority > highestPriority)
        { // seleciona processo de maior prioridade
            highestPriority = processes[i].priority;
            chosenProcess = i;
        }
        if (processes[i].arrivalTime <= currentTime && processes[i].quantum != 0 && processes[i].quantum < quantum)
        { // seleciona processo que ainda está utilizando sua parcela de tempo
            chosenProcess = i;
            break;
        }
    }
    processes[chosenProcess].quantum--;
    return chosenProcess;
}

// lottery scheduler
int lottery(int numProcesses, int currentTime)
{
    // Os bilhetes da loteria vão ser distribuídos de acordo com a prioridade
    int ticketAmount = 0;
    int ticket;
    int chosenProcess = -1;

    int i;
    for (i = 0; i < numProcesses; i++)
    {
        if (processes[i].remainingTime > 0 && processes[i].arrivalTime <= currentTime)
            ticketAmount += processes[i].priority;
    }

    ticket = rand() % ticketAmount;

    while (ticket >= 0)
    {
        chosenProcess++;
        if (processes[chosenProcess].remainingTime > 0 && processes[chosenProcess].arrivalTime <= currentTime)
            ticket -= processes[chosenProcess].priority;
    }
    return chosenProcess;
}

void kernelLoop(Process processes[], int numProcesses, scheduler schedulerType)
{
    int currentTime = 0;
    int completed = 0;
    int chosenProcess = 0;
    int i, j;

    while (completed != numProcesses)
    {
        chosenProcess = -1;

        // adds the processes that arrived in buffer
        for (i = 0; i < numProcesses; i++)
        {
            if (processes[i].arrivalTime <= currentTime && processes[i].remainingTime > 0)
            {
                // check if the process is already in the buffer
                j = start;
                int bufferPosition = -1;
                while (j != end)
                {
                    if (buffer[j] == &processes[i])
                    {
                        bufferPosition = j;
                        break;
                    }
                    j = (j + 1) % BUFFER_SIZE;
                }
                if (bufferPosition == -1)
                {
                    kernelAddProc(&processes[i]);
                }
            }
        }

        //choose the process according to the type of scheduler
        chosenProcess = schedulerType(numProcesses, currentTime);

        // checks if any process has been scheduled
        if (chosenProcess == -1)
        {
            currentTime++;
        }
        else
        {
            // check if the process is already in the buffer
            j = start;
            int bufferPosition = -1;
            while (j != end)
            {
                if (buffer[j] == &processes[chosenProcess])
                {
                    bufferPosition = j;
                    break;
                }
                j = (j + 1) % BUFFER_SIZE;
            }

            // if is not already in the buffer, adds in buffer
            if (bufferPosition == -1)
            {
                if (kernelAddProc(&processes[chosenProcess]) == FAIL)
                {
                    // if the buffer is full,
                    // removes the process in start position
                    bufferPosition = end;
                    kernelRemoveProc();
                    kernelAddProc(&processes[chosenProcess]);
                }
            }

            // printBuffer(currentTime, shortestProcess+1);
            // process execution
            printProcesses(numProcesses, chosenProcess, currentTime);
            processes[chosenProcess].remainingTime--;
            currentTime++;

            // if process ended, remove from buffer
            if (processes[chosenProcess].remainingTime == 0)
            {
                processes[chosenProcess].finishTime = currentTime;
                // switch the process to start position
                Process *aux = buffer[start];
                buffer[start] = buffer[bufferPosition];
                buffer[bufferPosition] = aux;
                kernelRemoveProc();
                completed++;
            }
        }
    }
}

int main()
{
    srand(time(NULL));
    int selectScheduler = 0;
    int numProcesses = readFile();
    output = fopen("output.txt", "w");

    if (output == NULL)
    {
        printf("Error opening file.\n");
        return 1;
    }

    fprintf(output, "%s", "Time\t");
    for (int i = 0; i < numProcesses; i++)
    {
        fprintf(output, "P%d\t", processes[i].id);
    }

    kernelInit();
    printf("[0] - SRTN (Shortest Remaining Time Next)\n");
    printf("[1] - Guaranteed Scheduling\n");
    printf("[2] - Fair Share Scheduling\n");
    printf("[3] - Lottery Scheduling\n");
    printf("Enter scheduler type: ");
    scanf("%d", &selectScheduler);

    //select the type of scheduler
    switch(selectScheduler){
        case 0:
            kernelLoop(processes, numProcesses, srtn);
            break;
        case 1:
            kernelLoop(processes, numProcesses, guaranteed);
            break;
        case 2:
            kernelLoop(processes, numProcesses, fairShare);
            break;
        case 3:
            kernelLoop(processes, numProcesses, lottery);
            break;
    }
    

    // print statistics
    fprintf(output, "%s", "\n\nProcess\t\t Arrival Time\t Burst Time\t Finish Time\t Turnaround Time\t Waiting Time\n");
    for (int i = 0; i < numProcesses; i++)
    {
        int tat = processes[i].finishTime - processes[i].arrivalTime;
        int wt = tat - processes[i].burstTime;

        fprintf(output, "P%d\t\t\t%d\t\t%d\t\t%d\t\t%d\t\t\t%d\n",
                processes[i].id,
                processes[i].arrivalTime,
                processes[i].burstTime,
                processes[i].finishTime,
                tat,
                wt);
    }

    fclose(output);
    return 0;
}