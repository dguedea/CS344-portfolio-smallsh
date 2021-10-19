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

/************************************************************************************************
 * INITIALIZING GLOBAL VARIABLES
 ************************************************************************************************/

/************************************************************************************************
 * STRUCT FOR USER INPUT
 * Will capture command, a linked list of arguments, the inputFile (if any),
 * the outputFile (if any) and if it should run in the background
 ************************************************************************************************/

// Struct to store user input to create the full command
struct input
{
    char* command;
    // struct arguments *listArgs;
    char listArgs[MAX_ARG];
    char* inputFile;
    char* outputFile;
    int isBackground;
};

// To create a linked list of arguments since there can be multiple
// struct arguments 
// {
//     char *arg;
//     struct arguments *next;
// };

/************************************************************************************************
 * INITIALIZING FUNCTIONS USED IN MAIN
 ************************************************************************************************/
void getInput(char *input, int *noParse);
void printCommand(struct input *aCommand);
struct input *parseUserInput(char *input);
void checkExpansion(struct input *aCommand);
// void understandCommand(struct input *command);


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

int main() {

    // Initialize Variables
    char *userInput = calloc(MAX_COMMAND, sizeof(char));
    int* smallLoop = 0;
    int noParse = 0;
    int *noParsePtr = &noParse;

    // Starting program
    printf("$ smallsh\n");

    // Start loop that will make up the smallsh getting user input and executing
    //while (*smallLoop == 0) {

    // Get user input
    getInput(userInput, noParsePtr);

    if (*noParsePtr == 0) {
        // Parse user input into input struct
        struct input *command = parseUserInput(userInput);
        checkExpansion(command);
        printCommand(command);

        // Understand command and redirect to relevant functions
        // understandCommand(command);
    };



    //}



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

void getInput(char *input, int *noParse) {

    *noParse = 0;

    // colon for each new line
    printf(": ");                        

    // grab user input and store in usrInput ptr
    fflush(stdout);
    fgets(input, MAX_COMMAND, stdin); 

    // Return back to loop if comment
    if(input[0] == '#'){
        printf("Comment");
        *noParse = 1;
        return;
    }
    // Return back to loop if new line (just enter from user)
    else if(strcmp(input, "\n")==0){
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

struct input *parseUserInput(char *input) {

    // Initialize the struct with isBackground initialized to false
    struct input *newCmd = malloc(sizeof(struct input));
    newCmd->command = NULL;
    newCmd->inputFile = NULL;
    newCmd->outputFile = NULL;
    newCmd->isBackground = 0;

    // Set up linked list for argument
    // struct arguments *newArg = malloc(sizeof(struct arguments));
    // struct arguments *head = NULL;
    // struct arguments *tail = NULL;

    char *saveptr;

    // remove new line from end of input
    // must do this or else the strtok_r does not notice the & at end
    input[strcspn(input, "\n")] = 0;

    // Store command since that is always first
    char *token = strtok_r(input, " ", &saveptr);
    newCmd->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(newCmd->command, token);

    // Loop through rest to assign to struct
    while((token = strtok_r(NULL, " ", &saveptr))) {
        // check if input file
        if(strcmp(token, "<") == 0) {
            token = strtok_r(NULL, " ", &saveptr);
            newCmd->inputFile = calloc(strlen(token)+1, sizeof(char));
            strcpy(newCmd->inputFile, token);
        }
        // check if output file
        else if(strcmp(token, ">") == 0) {
            token = strtok_r(NULL, " ", &saveptr);
            newCmd->outputFile = calloc(strlen(token)+1, sizeof(char));
            strcpy(newCmd->outputFile, token);
        }
        // check if background process
        else if(strcmp(token, "&") == 0) {
            newCmd->isBackground = 1;
        }
        // else add to argument char array with ; in between
        else {
            strcat(newCmd->listArgs, token);
            strcat(newCmd->listArgs, ";");
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

void printCommand(struct input *aCommand)
{
    printf("Command: %s\n Input: %s\n Output: %s\n Args: %s\n Background: %d\n", aCommand->command,
           aCommand->inputFile,
           aCommand->outputFile,
           aCommand->listArgs,
           aCommand->isBackground);
}

/******************************************************************************
 * checkExpansion()
 * Descripton: 
 * ****************************************************************************/

void checkExpansion(struct input *aCommand)
{
    // Get process ID 
    int processId = getpid();
    // Convert process ID int to string
    int length = snprintf(NULL, 0, "%d", processId);
    char *strPid = malloc(length + 1);
    asprintf(&strPid, "%i", processId);

    int endCmd = strlen(aCommand->command);

    // Check command for variable expansion & update with pid
    for(int i = 0; i <= endCmd; ++ i){
        char *newStr = malloc(length+1+strlen(aCommand->command)-2);
        if (aCommand->command[i] == '$' &&  aCommand->command[i] == aCommand->command[i+1]) {
            i = i+1;
            
            // Copy up to i to newStr
            strncpy(newStr, aCommand->command, (i-1));
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

    for(int i = 0; i <= endArg; ++ i){
        char *newStr = malloc(length+1+strlen(aCommand->listArgs)-2);
        if (aCommand->listArgs[i] == '$' &&  aCommand->listArgs[i] == aCommand->listArgs[i+1]) {
            i = i+1;
            // Copy up to i to newStr
            strncpy(newStr, aCommand->listArgs, (i-1));
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

    // Check output for variable expansion & update with pid

    free(strPid);
}


/******************************************************************************
 * understandCommand
 * Descripton: 
 * ****************************************************************************/

void understandCommand(struct input *aCommand)
{
    // Check for variable expansion
    // If $$ in command or an arg
    fflush(stdout);
    printf("To do");
}
