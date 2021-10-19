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
 ************************************************************************************************/

struct input
{
    char* command;
    char arg[MAX_ARG];
    char* inputFile;
    char* outputFile;
    bool isBackground;
};

/************************************************************************************************
 * INITIALIZING FUNCTIONS USED IN MAIN
 ************************************************************************************************/
void getInput(char *input);
void parseInput();


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

    // Get user input and parse
    getInput(userInput);





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

}

/************************************************************************************************
 * Name: parseInput()
 * Description:
 * 
 ************************************************************************************************/

struct input *parseUserInput(char *input) {
    
}