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

#define MAX_COMMAND 2048
#define MAX_ARG 512
#define _GNU_SOURCE

/************************************************************************************************
 * INITIALIZING GLOBAL VARIABLES
 ************************************************************************************************/
int errStatus = 0;
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
    int isBackground;
    int numArgs;
};

/************************************************************************************************
 * INITIALIZING FUNCTIONS USED IN MAIN
 ************************************************************************************************/
void getInput(char* input, int* noParse);
void printCommand(struct input* aCommand);
struct input* parseUserInput(char* input);
void checkExpansion(struct input* aCommand);
void chooseCommand(struct input* command, int* exitLoop);

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
    int inLoop = 0;
    int* ptrInLoop = &inLoop;
    int noParse = 0;
    int* noParsePtr = &noParse;

    // Starting program
    printf("$ smallsh\n");

    // Start loop that will make up the smallsh getting user input and executing
    while (inLoop == 0) {

        // Get user input
        getInput(userInput, noParsePtr);

        if (*noParsePtr == 0)
        {
            // Parse user input into input struct
            struct input* command = parseUserInput(userInput);
            // Expand with pid if needed
            checkExpansion(command);
            // Understand command and redirect to relevant functions
            chooseCommand(command, ptrInLoop);

            printCommand(command);
        };

    }

    return 0;
}

/************************************************************************************************
 * DEFINING FUNCTIONS USED IN MAIN
 ************************************************************************************************/

 /************************************************************************************************
   * Name: getInput()
   * Description:
   * Print new line to enter user input
   * Read user input input usrInput variable
   ************************************************************************************************/

void getInput(char* input, int* noParse)
{

    *noParse = 0;

    // colon for each new line
    printf(": ");

    // grab user input and store in usrInput ptr
    fflush(stdout);
    fgets(input, MAX_COMMAND, stdin);

    // Return back to loop if comment
    if (input[0] == '#')
    {
        printf("Comment");
        *noParse = 1;
        return;
    }
    // Return back to loop if new line (just enter from user)
    else if (strcmp(input, "\n") == 0)
    {
        printf("New Line");
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
    fflush(stdout);
    printCommand(newCmd);
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
 * convertArgToArray()
 * Descripton:
 * ****************************************************************************/
//char convertArgToArray(struct input* aCommand, char *newArray) {
  //  char* saveptr;
    //int ctr = 0;
//
  //  char* token = strtok_r(aCommand->listArgs, ";", &saveptr);
    //strcpy(newArray[0], token);
    //ctr++;
//
//    while ((token = strtok_r(NULL, ";", &saveptr))) {
//        strcpy(newArray[ctr], token);
//        ctr++;
//    }
//    return newArray;
//}


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
 * understandCommand
 * Descripton:
 * ****************************************************************************/

void chooseCommand(struct input* aCommand, int* exitLoop)
{
    fflush(stdout);
    // Check if command equals exit, if so exits shell
    if (strcmp(aCommand->command, "exit") == 0)
    {
        // CITATION: To easily kill all processes can utilize getppid() and kill()
        // https://stackoverflow.com/questions/897321/how-can-i-kill-all-processes-of-a-program
        *exitLoop = 1;
        pid_t ppid = getppid();
        kill(ppid, 0);
        exit(0);
    }

    // Check if command equals status
    else if (strcmp(aCommand->command, "status") == 0)
    {
        printf("Exit Value %d\n", errStatus);
        fflush(stdout);
    }

    // Check if command equals cd, if so change directory
    else if (strcmp(aCommand->command, "cd") == 0)
    {
        // If there are no arguments, go to HOME directory
        // This will chdir to HOME by using getenv 'HOME'
        if (aCommand->numArgs == 0) {
            // For testing
            //char startCwd[MAX_ARG];
            //getcwd(startCwd, sizeof(startCwd));
            //printf("starting path: %s\n", startCwd);

            chdir(getenv("HOME"));

            //char cwd[MAX_ARG];
            //getcwd(cwd, sizeof(cwd));
           //printf("new path: %s\n", cwd);
        }
        // If there are arguments, it will be a path
        // Change directory to path specified
        else {
            // Get the first argument from listArg which is the path
            char *firstArg;
            char* saveptr;
            char* token = strtok_r(aCommand->listArgs, ";", &saveptr);
            firstArg = calloc(strlen(token)+1, sizeof(char));
            strcpy(firstArg, token);
            //printf("argument: %s\n", firstArg);

            char startCwd[MAX_ARG];
            getcwd(startCwd, sizeof(startCwd));
            //printf("starting path: %s\n", startCwd);

            // Create full path by appending current path and arg path
            // chdir only works with full paths I found out
            char* fullPath;
            fullPath = calloc(strlen(firstArg) + 1 + strlen(startCwd), sizeof(char));
            strcat(fullPath, startCwd);
            strcat(fullPath, "/");
            strcat(fullPath, firstArg);
            
            // Change directory to path
            chdir(fullPath);

            //char cwd[MAX_ARG];
            //getcwd(cwd, sizeof(cwd));
            //printf("new path: %s\n", cwd);

            free(firstArg);
            free(fullPath);

        }
    }
    // If other, redirect to otherCommands()
    else
    {
        printf("other commands");
    }
    return;
}
