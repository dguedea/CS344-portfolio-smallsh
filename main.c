/*
* Name: Danielle Guedea
* OSUID: guedead
* Description: Assignment 3: smallsh
* Used to create a small shell in C. It implements a subset of well-known features
* And execute 3 commands - exit, cd and status as well as others through exec()
* It supports input and output redirection and running commands in foreground or background
* Date: 10/19/2021
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define MAX_COMMAND 2048
#define MAX_ARG 512
#define _GNU_SOURCE

/************************************************************************************************
 * INITIALIZING GLOBAL VARIABLES
 ************************************************************************************************/
int errStatus = 0;
int backgroundAllowed = 0;   // 0 is allowed, 1 is not allowed
/************************************************************************************************
 * STRUCT FOR USER INPUT
 * Will capture command, a linked list of arguments, the inputFile (if any),
 * the outputFile (if any) and if it should run in the background
 ************************************************************************************************/

 // Struct to store user input to create the full command
struct input
{
    char* command;
    char listArgs[MAX_ARG]; // List of args delimited with ;
    char* inputFile;
    char* outputFile;
    int isBackground;       // 0 foreground 1 background
    int numArgs;            // Counts # of args to create array
};

// Struct to store pids for exiting and tracking termination
struct processes
{
    int pidArray[100];
    int numPids;
};

/************************************************************************************************
 * INITIALIZING FUNCTIONS USED IN MAIN
 ************************************************************************************************/
void getInput(char* input, int* noParse, struct processes* aProcess);
void printCommand(struct input* aCommand);
struct input* parseUserInput(char* input);
void checkExpansion(struct input* aCommand);
void chooseCommand(struct input* command, int* exitLoop, struct processes* aProcess, struct sigaction SIGINT_action);
void otherCommands(struct input* command, struct processes* aProcess, struct sigaction SIGINT_action);
void setExitStatus(int status);
void checkTermination(struct processes* aProcess);

void handle_SIGTSTP(int signo);

/*************************************************************************************************
 * MAIN FUNCTION
************************************************************************************************/
/*   Compile the program as follows:
*
*   gcc --std=gnu99 -o smallsh main.c
*
*  ./smallsh
*
*/

int main()
{

    // Initialize Variables
    char* userInput = calloc(MAX_COMMAND, sizeof(char));
    // Tracks if smallsh should keep executing
    int inLoop = 0;
    int* ptrInLoop = &inLoop;
    // Moves on to parse user input if not # or \n if 0
    int noParse = 0;
    int* noParsePtr = &noParse;

    // Create struct to store PIDs
    struct processes* pids = malloc(sizeof(struct processes));
    pids->numPids = 0;

    // Set up handling SIGINT parent to ignore
    // Adpated from Module 5: Custom Handlers for SIGINT, SIGUSR2 and SIGTERM
    struct sigaction SIGINT_action = { 0 };
    SIGINT_action.sa_handler = SIG_IGN;  //Since we want the parent to ignore Cntrl C
    sigfillset(&SIGINT_action.sa_mask);  //Will not interrupt
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    // Set up handing SIGTSTP parent and children ignore
    // Toggles to allow or disallow background processes
    // Adpated from Module 5: Custom Handlers for SIGINT, SIGUSR2, and SIGTERM
    struct sigaction SIGTSTP_action = { 0 };
    SIGTSTP_action.sa_handler = handle_SIGTSTP;  // Calls function that will toggle the background override
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;        // Using restart so it does not re-execute previous call (MOD 5)
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    // Starting program
    printf("$ smallsh\n");

    // Start loop that will make up the smallsh getting user input and executing
    while (inLoop == 0) {

        // Get user input
        getInput(userInput, noParsePtr, pids);

        if (*noParsePtr == 0)
        {
            // Parse user input into input struct
            struct input* command = parseUserInput(userInput);
            // Expand with pid if needed
            checkExpansion(command);
            // Understand command and redirect to relevant functions
            chooseCommand(command, ptrInLoop, pids, SIGINT_action);

            //printCommand(command);
        };

    }

    return 0;
}

/************************************************************************************************
 * DEFINING FUNCTIONS USED IN MAIN
 ************************************************************************************************/

 /************************************************************************************************
   * Name: handle_SIGTSTP()
   * Description:
   * Print new line to enter user input
   * Read user input input usrInput variable
   ************************************************************************************************/

void handle_SIGTSTP(int signo) {
    if (backgroundAllowed == 0) {
        backgroundAllowed = 1;
        char* message = "\nEntering foreground only mode (& is now ignored)\n: ";
        write(1, message, 52);
        fflush(stdout);
    }
    else if (backgroundAllowed == 1) {
        backgroundAllowed = 0;
        char* message = "\nExiting foreground only mode\n: ";
        write(1, message, 32);
        fflush(stdout);

    }

}

 /************************************************************************************************
   * Name: checkTermination()
   * Description:
   * Print new line to enter user input
   * Read user input input usrInput variable
   ************************************************************************************************/

void checkTermination(struct processes* aProcess) {

    int childStatus;
    pid_t childPid;

    for (int i = 0; i < aProcess->numPids; i++) {
        childPid = waitpid(aProcess->pidArray[i], &childStatus, WNOHANG);
        if (aProcess->pidArray[i] != NULL) {
            if (childPid != 0) {
                if (WIFEXITED(childStatus)) {
                    printf("background pid %d is done: terminated by signal %d\n", aProcess->pidArray[i], WEXITSTATUS(childStatus));
                    fflush(stdout);
                    aProcess->pidArray[i] = NULL;
                }
                else {
                    printf("background pid %d is done: terminated by signal %d\n", aProcess->pidArray[i], WTERMSIG(childStatus));
                    fflush(stdout);
                    aProcess->pidArray[i] = NULL;
                }
            }
        }

    }
}

/************************************************************************************************
  * Name: getInput()
  * Description:
  * Print new line to enter user input
  * Read user input input usrInput variable
  ************************************************************************************************/

void getInput(char* input, int* noParse, struct processes* aProcess)
{

    *noParse = 0;

    // Check for terminated commands
    checkTermination(aProcess);

    // Colon for each new line
    printf(": ");

    // grab user input and store in usrInput ptr
    fflush(stdout);
    fgets(input, MAX_COMMAND, stdin);

    // Return back to loop if comment
    if (input[0] == '#')
    {
        *noParse = 1;
        return;
    }
    // Return back to loop if new line (just enter from user)
    else if (strcmp(input, "\n") == 0)
    {
        *noParse = 1;
        return;
    }

    return;
}

/************************************************************************************************
 * Name: parseInput()
 * Description:
 *
 ************************************************************************************************/

struct input* parseUserInput(char* input)
{

    // Initialize the struct with isBackground initialized to false
    struct input* newCmd = malloc(sizeof(struct input));
    newCmd->command = NULL;
    newCmd->inputFile = NULL;
    newCmd->outputFile = NULL;
    newCmd->isBackground = 0;
    newCmd->numArgs = 0;

    // Set up linked list for argument
    // struct arguments *newArg = malloc(sizeof(struct arguments));
    // struct arguments *head = NULL;
    // struct arguments *tail = NULL;

    char* saveptr;

    // remove new line from end of input
    // must do this or else the strtok_r does not notice the & at end
    input[strcspn(input, "\n")] = 0;

    // Store command since that is always first
    char* token = strtok_r(input, " ", &saveptr);
    newCmd->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(newCmd->command, token);

    // Loop through rest to assign to struct
    while ((token = strtok_r(NULL, " ", &saveptr)))
    {
        // check if input file
        if (strcmp(token, "<") == 0)
        {
            token = strtok_r(NULL, " ", &saveptr);
            newCmd->inputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(newCmd->inputFile, token);
        }
        // check if output file
        else if (strcmp(token, ">") == 0)
        {
            token = strtok_r(NULL, " ", &saveptr);
            newCmd->outputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(newCmd->outputFile, token);
        }
        // check if background process
        else if (strcmp(token, "&") == 0)
        {
            newCmd->isBackground = 1;
        }
        // else add to argument char array with ; in between
        else
        {
            strcat(newCmd->listArgs, token);
            strcat(newCmd->listArgs, ";");
            newCmd->numArgs += 1;
        }
    }
    //fflush(stdout);
    //printCommand(newCmd);

    return newCmd;
}

/******************************************************************************
 * printCommand
 * Descripton: Print data from command struct (used in testing file parsing)
 *
 * Adapted from students.c from Assignment 1 344
 * CS 344 (Sept. 13, 2020) Citing source code https://replit.com/@cs344/studentsc#student_info1.txt
 * ****************************************************************************/

void printCommand(struct input* aCommand)
{
    printf("Command: %s\n Input: %s\n Output: %s\n Args: %s\n Background: %d\n Num Args: %d\n", aCommand->command,
        aCommand->inputFile,
        aCommand->outputFile,
        aCommand->listArgs,
        aCommand->isBackground,
        aCommand->numArgs);
}


/******************************************************************************
 * checkExpansion()
 * Descripton:
 * ****************************************************************************/

void checkExpansion(struct input* aCommand)
{
    // Get process ID
    int processId = getpid();
    // Convert process ID int to string
    int length = snprintf(NULL, 0, "%d", processId);
    char* strPid = malloc(length + 1);
    asprintf(&strPid, "%i", processId);

    int endCmd = strlen(aCommand->command);

    // Check command for variable expansion & update with pid
    for (int i = 0; i <= endCmd; ++i)
    {
        char* newStr = malloc(length + 1 + strlen(aCommand->command) - 2);
        if (aCommand->command[i] == '$' && aCommand->command[i] == aCommand->command[i + 1])
        {
            i = i + 1;

            // Copy up to i to newStr
            strncpy(newStr, aCommand->command, (i - 1));
            // Add PID string
            strcat(newStr, strPid);
            // Copy i to end to newStr
            strcat(newStr, aCommand->command + i + 1);
            // Set command to new str
            strcpy(aCommand->command, newStr);
            endCmd = strlen(newStr);
            free(newStr);
        }
    }

    // Check args for variable expansion & update with pid
    int endArg = strlen(aCommand->listArgs);

    for (int i = 0; i <= endArg; ++i)
    {
        char* newStr = malloc(length + 1 + strlen(aCommand->listArgs) - 2);
        if (aCommand->listArgs[i] == '$' && aCommand->listArgs[i] == aCommand->listArgs[i + 1])
        {
            i = i + 1;
            // Copy up to i to newStr
            strncpy(newStr, aCommand->listArgs, (i - 1));
            // Add PID string
            strcat(newStr, strPid);
            // Copy i to end to newStr
            strcat(newStr, aCommand->listArgs + i + 1);
            // Set command to new str
            strcpy(aCommand->listArgs, newStr);
            endCmd = strlen(newStr);
            free(newStr);
        }
    }

    // Check input for variable expansion & update with pid
    if (aCommand->inputFile != NULL)
    {
        int endI = strlen(aCommand->inputFile);

        for (int i = 0; i <= endI; ++i)
        {
            char* newStr = malloc(length + 1 + strlen(aCommand->inputFile) - 2);
            if (aCommand->inputFile[i] == '$' && aCommand->inputFile[i] == aCommand->inputFile[i + 1])
            {
                i = i + 1;
                // Copy up to i to newStr
                strncpy(newStr, aCommand->inputFile, (i - 1));
                // Add PID string
                strcat(newStr, strPid);
                // Copy i to end to newStr
                strcat(newStr, aCommand->inputFile + i + 1);
                // Set command to new str
                strcpy(aCommand->inputFile, newStr);
                endI = strlen(newStr);
                free(newStr);
            }
        }
    }

    // Check output for variable expansion & update with pid
    if (aCommand->outputFile != NULL)
    {
        int endO = strlen(aCommand->outputFile);

        for (int i = 0; i <= endO; ++i)
        {
            char* newStr = malloc(length + 1 + strlen(aCommand->outputFile) - 2);
            if (aCommand->outputFile[i] == '$' && aCommand->outputFile[i] == aCommand->outputFile[i + 1])
            {
                i = i + 1;
                // Copy up to i to newStr
                strncpy(newStr, aCommand->outputFile, (i - 1));
                // Add PID string
                strcat(newStr, strPid);
                // Copy i to end to newStr
                strcat(newStr, aCommand->outputFile + i + 1);
                // Set command to new str
                strcpy(aCommand->outputFile, newStr);
                endO = strlen(newStr);
                free(newStr);
            }
        }
    }

    free(strPid);
}

/******************************************************************************
 * chooseCommand
 * Descripton:
 * ****************************************************************************/

void chooseCommand(struct input* aCommand, int* exitLoop, struct processes* aProcess, struct sigaction SIGINT_action)
{
    // Check if command equals exit, if so exits shell
    if (strcmp(aCommand->command, "exit") == 0)
    {
        // Break the shell loop
        *exitLoop = 1;

        // Loop through pids to kill any that have not terminated
        int childStatus;
        pid_t childPid;

        for (int i = 0; i < aProcess->numPids; i++) {
            childPid = waitpid(aProcess->pidArray[i], &childStatus, WNOHANG);
            if (childPid == 0) {
                kill(aProcess->pidArray[i], SIGKILL);
            }
        }
        exit(0);
    }


    // Check if command equals status
    else if (strcmp(aCommand->command, "status") == 0)
    {
        printf("exit value %d\n", errStatus);
        fflush(stdout);
    }

    // Check if command equals cd, if so change directory
    else if (strcmp(aCommand->command, "cd") == 0)
    {
        // If there are no arguments, go to HOME directory
        // This will chdir to HOME by using getenv 'HOME'
        if (aCommand->numArgs == 0) {
            chdir(getenv("HOME"));
        }

        // If there are arguments, it will be a path
        // Change directory to path specified
        else {

            // Get the first argument from listArg which is the path
            char* firstArg;
            char* saveptr;
            char* token = strtok_r(aCommand->listArgs, ";", &saveptr);
            firstArg = calloc(strlen(token) + 1, sizeof(char));
            strcpy(firstArg, token);

            char startCwd[MAX_ARG];
            getcwd(startCwd, sizeof(startCwd));

            // Create full path by appending current path and arg path
            // chdir only works with full paths I found out
            char* fullPath;
            fullPath = calloc(strlen(firstArg) + 1 + strlen(startCwd), sizeof(char));
            strcat(fullPath, startCwd);
            strcat(fullPath, "/");
            strcat(fullPath, firstArg);

            // Change directory to path
            chdir(fullPath);

            free(firstArg);
            free(fullPath);

        }
    }
    // If other, redirect to otherCommands()
    else
    {
        otherCommands(aCommand, aProcess, SIGINT_action);
    }
    return;
}

/******************************************************************************
 * otherCommands
 * Descripton:
 * ****************************************************************************/

void otherCommands(struct input* aCommand, struct processes* aProcess, struct sigaction SIGINT_action) {
    // Create array of arguments to use in execv
    char* array[aCommand->numArgs + 1];

    // Add command
    array[0] = calloc(strlen(aCommand->command) + 1, sizeof(char));
    strcpy(array[0], aCommand->command);

    // Add any arguments
    if (aCommand->numArgs != 0) {
        char* saveP;
        char* t = strtok_r(aCommand->listArgs, ";", &saveP);
        array[1] = calloc(strlen(t) + 1, sizeof(char));
        strcpy(array[1], t);

        for (int i = 2; i <= aCommand->numArgs; i++) {
            t = strtok_r(NULL, ";", &saveP);
            array[i] = calloc(strlen(t) + 1, sizeof(char));
            strcpy(array[i], t);
        }
    }

    // Add null to end of array
    array[aCommand->numArgs + 1] = NULL;

    // If backgroundAllowed set to 1, remove isBackground so runs in foreground
    if (backgroundAllowed == 1) {
        aCommand->isBackground = 0;
    }

    // Fork and perform execvp()
    // Using execvp since I created an array & it is using PATH
    // Adapted from Module 4: using exec() with fork() example
    int childStatus;
    int result;

    // Fork new process
    pid_t spawnPid = fork();

    switch (spawnPid) {
    case -1: // If error spawning
        perror("fork()");
        errStatus = 1;
        break;
    case 0: // If success forking
        // Handle input 
        // Adapted from Module 5: Redirecting input/output example

        // Child in foreground must terminate with SIGINT (Cntrl C)
        if (aCommand->isBackground == 0) {
            SIGINT_action.sa_handler = SIG_DFL; // Will set to default behavior
            sigaction(SIGINT, &SIGINT_action, NULL);
        }

        if (aCommand->inputFile != NULL) {
            // Open file in read only mode
            int inputFD = open(aCommand->inputFile, O_RDONLY);
            // If error, print message and set status to 1
            if (inputFD == -1) {
                perror("");
                printf("cannot open %s for input\n", aCommand->inputFile);
                fflush(stdout);
                exit(1);
            }

            // Redirect stdin
            result = dup2(inputFD, 0);
            if (result == -1) {
                perror("error");
                //printf("cannot open %s for input\n", aCommand->inputFile);
                //fflush(stdout);
                exit(1);
            }
        }
        // If background command and no input file, set to /dev/null
        else if (aCommand->inputFile == NULL && aCommand->isBackground == 1) {
            // Set background stdin to /dev/null
            int backFD = open("/dev/null", O_RDONLY);
            if (backFD == -1) {
                perror("error opening /dev/null");
                exit(1);
            }
            result = dup2(backFD, 0);
            if (result == -1) {
                perror("error");
                exit(1);
            }
        }

        // Handle output
        // Adapted from Module 5: Redirecting input/output example

        if (aCommand->outputFile != NULL) {
            // Open file in write only and truncate if exits, create if not
            int outputFD = open(aCommand->outputFile, O_CREAT | O_WRONLY | O_TRUNC, 0777);
            // If error, print message and set status to 1
            if (outputFD == -1) {
                perror("error with output file");
                exit(1);
            }

            // Redirect stdout
            result = dup2(outputFD, 1);
            if (result == -1) {
                perror("output dup2()");
                exit(1);
            }
        }

        // For background process with no stdout set to /dev/null
        if (aCommand->outputFile == NULL && aCommand->isBackground == 1) {
            int outFD = open("/dev/null", O_CREAT | O_WRONLY | O_TRUNC);
            if (outFD == -1) {
                perror("error opening /dev/null");
                exit(1);
            }
            result = dup2(outFD, 1);
            if (result == -1) {
                perror("output dup2()");
                exit(1);
            }
        }
        execvp(array[0], array);
        perror("Error occurred: ");
        errStatus = 1;
        exit(1); // Exit if there is an error to kill the child 
        break;

    default: // For parent process
        // Background process, utilize WNOHANG to free shell
        if (aCommand->isBackground == 1) {
            printf("background pid is %d\n", spawnPid);
            fflush(stdout);
            // Add pid to array to track termination & kill upon exit
            aProcess->pidArray[aProcess->numPids] = spawnPid;
            aProcess->numPids++;
            spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
        }
        else {
            // Foreground process, utilize waitpid to wait on the child
            spawnPid = waitpid(spawnPid, &childStatus, 0);

            // Set error status
            setExitStatus(childStatus);

            // Since SIGINT exit is always 2, only print on Cntrl C exit
            if (errStatus == 2) {
                printf("terminated with signal %d\n", errStatus);
            }
            
        }
        //printf("Parent %d: child %d terminated, exiting\n", getpid(), spawnPid);
        //fflush(stdout);
        break;
    }

}

/******************************************************************************
 * setExitStatus
 * Descripton:
 * ****************************************************************************/

void setExitStatus(int status) {

    // Adapted from Module 4 Process API Interpreting Signal
    // Exited normally
    if (WIFEXITED(status)) {
        errStatus = WEXITSTATUS(status);
    }
    // Exited due to signal
    else {
        errStatus = WTERMSIG(status);
    }

}