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
void getInput(char *input);
void printCommand(struct input *aCommand);
struct input *parseUserInput(char *input);


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

    // Starting program
    printf("$ smallsh\n");

    // Start loop that will make up the smallsh getting user input and executing
    //while (*smallLoop == 0) {

    // Get user input
    getInput(userInput);

    // Parse user input into input struct
    parseUserInput(userInput);

    // Understand command and redirect
    





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

void getInput(char *input) {

    // colon for each new line
    printf(": ");                        

    // grab user input and store in usrInput ptr
    fflush(stdout);
    fgets(input, MAX_COMMAND, stdin); 

    // remove new line from end of input
    // must do this or else the strtok_r does not notice the & at end
    input[strcspn(input, "\n")] = 0;

    // // Return back to loop if comment
    // if(input[0] == '#'){
    //     printf("Comment");
    //     return;
    // }
    // // Return back to loop if new line (just enter from user)
    // else if(strcmp(input, "\n")==0){
    //     printf("New Line");
    //     return;
    // }
    // // Parse
    // else {
    //     parseUserInput(input);
    // }
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

