#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>

int GetArgs(char* myargv[], int* amount_of_pipes);

const int MAX_AMOUNT_OF_PIPES = 10;

int main()
{
    while (true)   
    { 
        char* myargv[1024] = {};
        printf("\nmyshell$ ");
        
        int amount_of_pipes = 0;
        int argv_counter = GetArgs(myargv, &amount_of_pipes);
        if (strncmp(myargv[0], "myexit", strlen("myexit")) == 0)
            break;

        int amount_of_cycles = (amount_of_pipes / MAX_AMOUNT_OF_PIPES);
        int last_cycle = amount_of_pipes % MAX_AMOUNT_OF_PIPES;

        if (last_cycle != 0 || amount_of_pipes == 0) 
            amount_of_cycles++;
        else 
            last_cycle = MAX_AMOUNT_OF_PIPES; 

        int prev_fd_to_read = 0; 
        int fds[2] = {}; 
            
        for (int i0 = 0; i0 < amount_of_cycles; i0++)
        {
            int amount_of_pipes_in_current_cycle = last_cycle;
            if (i0 != amount_of_cycles - 1)
                amount_of_pipes_in_current_cycle = MAX_AMOUNT_OF_PIPES;

        for (int i = 0; i < amount_of_pipes_in_current_cycle + 1; i++)
        {
            if (i < amount_of_pipes_in_current_cycle)
            {
                if (pipe(fds) < 0)
                    perror("pipe");
            }

            int p = fork();

            if (p == 0)
            {
                if (i > 0) 
                {
                    dup2(prev_fd_to_read, 0); 
                    close(prev_fd_to_read);
                }

                if (i < amount_of_pipes_in_current_cycle || (i0 < amount_of_cycles - 1 && i == amount_of_pipes_in_current_cycle))
                {
                    close(fds[0]);
                    dup2(fds[1], 1); 
                    close(fds[1]);
                }

                if (i0 == 0)
                    execvp(myargv[0], myargv + (i * (i0 + 1)));
                else
                    execvp(myargv[i * (i0 + 1)], myargv + (i * (i0 + 1)));
                perror("execvp failed");

                return 0;
            }

            if (i > 0)
            {
                close(prev_fd_to_read);
            }

            if (i < amount_of_pipes_in_current_cycle || (i0 < amount_of_cycles - 1 && i == amount_of_pipes_in_current_cycle))
            {
                close(fds[1]);
                prev_fd_to_read = fds[0];
            }
        }

        for (int i = 0; i < amount_of_pipes_in_current_cycle + 1; i++)
            wait(0);
    }
    }
}

int GetArgs(char* myargv[], int* amount_of_pipes)
{
    char command[1024] = {};
    char c = 0;
    int j = 0;
    int argv_counter = 0;

    while ((c = getchar()) != '\n') 
    {
        if (c == '|')
        {
            myargv[argv_counter] = NULL;
            argv_counter++;
            (*amount_of_pipes)++;
        }
        if (c == '|' || c == ' ' || c == '\n')
        {
            if (j == 0)
                continue;
                 
            char* new_mem = (char*)calloc(j + 1, sizeof(char));
            myargv[argv_counter] = memcpy(new_mem, command, j);
            myargv[argv_counter][j] = '\0';
            argv_counter++;
            j = 0;
            continue;
        }

        command[j] = c;
        j++;
    }
    
    char* new_mem = (char*)calloc(j + 1, sizeof(char));
    myargv[argv_counter] = memcpy(new_mem, command, j);
    argv_counter++;
    myargv[argv_counter] = NULL;

    return argv_counter;
}

