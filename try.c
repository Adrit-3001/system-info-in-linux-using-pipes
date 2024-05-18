#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "fun.h"
#define MAXSIZE 8192

// ***************** LOOKOUT FOR PATHOLOGICAL STATES **********************
// DO THE TIME THING I.R 1% SOMETHING. SEE PIAZZA

void sigint_handler() {
    // Handle the Ctrl-C signal. Ask the user if they want to quit
    char choice;
    printf("\nPlease confirm if you want to quit (y/n) ");
    scanf(" %c", &choice);
    if (choice == 'y' || choice == 'Y' || strcmp(&choice,"yes") == 0 || strcmp(&choice, "Yes") == 0) {
        exit(0);
    }
}

void sigtstp_handler() {
    // Ignore Ctrl-Z as per assignment description
    printf("\nCtrl + Z is ignored\n");
    return;
}

// Main function
int main(int argc, char *argv[]) {
    /* main function to initialize everything as well as check for
     * various command line arguments. Pipes are also implemented inside
     * the main function
     */

    // Signal handler for Ctrl-C
    signal(SIGINT, sigint_handler);

    // Signal handler for Ctrl-Z
    signal(SIGTSTP, sigtstp_handler);

    int samples = 10;
    int tdelay = 1;
    int last_info = 1; // for last information #system#
    int sys = 0;// for system
    int user = 0; // for user
    int def = 0; // default mode
    int seq = 0;
    int graphics = 0;
    int flag = 0; // flag used for tdelay and samples when only given as int
    clearScreen();
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            char *arg = argv[i];
            if (strncmp(arg, "--samples=", 10) == 0) {
                samples = atoi(arg + 10);
            } else if (strncmp(arg, "--tdelay=", 9) == 0) {
                tdelay = atoi(arg + 9);
            } else if (strcmp(arg, "--sequential") == 0) {
                seq = 1;
            } else if (strcmp(arg, "--system") == 0) {
                sys = 1;
            } else if (strcmp(arg, "--user") == 0) {
                user = 1;
            } else if ((strcmp(arg, "--graphics") == 0) || (strcmp(arg,"-g") == 0)) {
                graphics = 1;
            }
            else if(!flag){
                if ((strtol(argv[i], NULL, 10)) || !(strncmp(argv[argc-2], "--samples=", 10))) {
                    if (i + 1 < argc){
                        if (strtof(argv[i + 1], NULL) || !(strncmp(argv[argc-1], "--tdelay=", 9))) {
                            float temp = strtol(argv[i], NULL, 10);
                            if (temp) samples = (int)temp;
                            else samples = atoi(&argv[i + 1][10]);

                            temp = strtof(argv[i + 1], NULL);
                            if (temp) tdelay = temp;
                            else tdelay = atof(&argv[i + 1][9]);
                            flag = 1;
                        } else {
                            printf("need one more number :-(\n");
                            return 1;
                        }
                    } else {
                        printf("need one more number :-(\n");
                        return 1;
                    }
                }
            }
        }
    }
    if (sys || user) last_info = 0;
    else def = 1;
    if(!seq) {
        for (int i = 0; i < samples; i++) {
            printf("\n");
        }
        char mem_info[samples][MAXSIZE];
        for (int i = 0; i < samples; i++) {
            strcpy(mem_info[i], "\n");
        }
        // my design choice that I will leave space
        // for cpu graphics
        char cpu_info[samples][MAXSIZE];
        for (int i = 0; i < samples; i++) {
            strcpy(cpu_info[i], "\n");
        }

        printf("\033[%d;%dH", 0, 0); // cursor goes to top-left
        getInitialInfo(samples, tdelay);
        printf("---------------------------------------\n");

        for (int i = 0; i < samples; i++) {
            double preMemo;
            struct sysinfo si;
            if (sysinfo(&si) != 0) {
                printf("error fetching sysinfo :-(\n");
                return 1;
            }
            preMemo = (si.totalram - si.freeram) / (1024.0 * 1024.0 * 1024.0);
            FILE *statFile = fopen("/proc/stat", "r");
            long preUser, preNice, preSys, preIdle, preIowait, preIrq, preSoftirq;
            fscanf(statFile, "cpu %ld %ld %ld %ld %ld %ld %ld", &preUser, &preNice, &preSys, &preIdle, &preIowait,
                   &preIrq, &preSoftirq);
            long preTotal = preUser + preNice + preSys + preIdle + preIowait + preIrq + preSoftirq;
            fclose(statFile);

            // using sleep here so that I can compare easily instead
            // of doing it at last (I hope my assumption works)
            sleep(tdelay);

            // using some pcrs examples as reference to do pipes
            int pipe_mem[2];
            int pipe_users[2];
            int pipe_cpu[2];
            char tmp[MAXSIZE];
            char tmp2[MAXSIZE];

            if (pipe(pipe_mem) == -1 || pipe(pipe_users) == -1 || pipe(pipe_cpu) == -1) {
                perror("pipe");
                return 1;
            }

            pid_t pid_cpu;
            pid_t pid_mem;
            pid_t pid_user;

            if (sys || def) {
                // memory fork
                pid_mem = fork();
                if (pid_mem < 0) {
                    close(pipe_mem[0]);
                    close(pipe_mem[1]);
                    perror("mem fork");
                    exit(1);
                }
                if (pid_mem == 0) { // only do the following specific
                    // process in the child
                    close(pipe_mem[1]); // close write coz we don't need it
                    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
                    char buf[MAXSIZE];
                    // using ssize_t n for error handling
                    ssize_t n = read(pipe_mem[0], buf,sizeof(buf)); // read from the pipe
                    strcpy(mem_info[i], buf);
                    for (int k = 0; k < samples; k++) {
                        printf("%s", mem_info[k]);
                    }
                    if (n == -1) {
                        perror("read");
                        exit(1);
                    }
                    close(pipe_mem[0]); // close the read end of the pipe
                    exit(0);
                } else {
                    close(pipe_mem[0]);
                    strcpy(tmp, print_mem_info(graphics, preMemo, mem_info[i]));
                    strcat(tmp,"---------------------------------------\n");
                    ssize_t n = write(pipe_mem[1], tmp, sizeof(tmp));
                    if (n == -1) {
                        perror("write");
                        exit(1);
                    }
                    close(pipe_mem[1]);
                }
            }

            if (user || def) {
                // user fork
                pid_user = fork();
                if (pid_user < 0) {
                    perror("user fork");
                    exit(1);
                } else if (pid_user == 0) {
                    close(pipe_users[1]);
                    char buf[MAXSIZE];
                    ssize_t n = read(pipe_users[0], buf, sizeof(buf));
                    if (n == -1) {
                        perror("read");
                    }
                    printf("%s\n", buf);
                    close(pipe_users[0]);
                    exit(0);
                } else {
                    if(sys || def)waitpid(pid_mem, NULL, 0); // wait for mem
                    close(pipe_users[0]);
                    char buf[MAXSIZE];
                    print_users_info(buf);
//                    strcpy(buf, print_users_info());
                    ssize_t n = write(pipe_users[1], buf, sizeof(buf));
                    if (n == -1) {
                        perror("write");
                        exit(1);
                    }
                    close(pipe_users[1]);
                }
                if (user || def) waitpid(pid_user, NULL, 0);
            }

            if (sys || def) {
                // CPU usage fork
                pid_cpu = fork();
                if (pid_cpu < 0) {
                    perror("CPU fork");
                    exit(1);
                } else if (pid_cpu == 0) {
                    close(pipe_cpu[1]);
                    char buff[MAXSIZE]; // typo that I totally ignored T_T
                    ssize_t n = read(pipe_cpu[0], buff, sizeof(buff));
                    if (n == -1) {
                        perror("error");
                    }
                    char temp[MAXSIZE];
                    displayCPUUsage(preTotal, preIdle, 0, temp);
                    printf("%s", temp);
                    if (!graphics)
                        printf("---------------------------------------\n");
                    if (graphics) {
                        strcpy(cpu_info[i], buff);
                        for (int j = 0; j < samples; ++j) {
                            printf("%s", cpu_info[j]);
                        }
                    }
                    close(pipe_cpu[0]);
                    exit(0);
                } else {
                    close(pipe_cpu[0]);
                    if (user || def) waitpid(pid_user, NULL, 0);
                    displayCores();
                    strcpy(tmp2,displayCPUUsage(preTotal, preIdle, graphics,cpu_info[i]));
                    strcat(tmp2,"---------------------------------------\n");
                    ssize_t n = write(pipe_cpu[1], tmp2, sizeof(tmp2));
                    if (n == -1) {
                        perror("write");
                        exit(1);
                    }
                    close(pipe_cpu[1]);
                }
                // Wait for child processes to finish
                if (user || def) waitpid(pid_cpu, NULL, 0);
            }
        }
        if (last_info) print_system_info();
    }
    else{
        // assuming that sequential can be used with other cla like --user
        for (int i = 0; i < samples; ++i) {
            // design choice and assumption that samples are only 1 for sequential and delay can be adjusted by user
            // instead I will be using samples for iteration number :-)
            printf(">>> iteration %d\n", i);
            getInitialInfo(1, tdelay);
            printf("---------------------------------------\n");

            double preMemo;
            struct sysinfo si;
            if (sysinfo(&si) != 0) {
                printf("error fetching sysinfo :-(\n");
                return 1;
            }
            preMemo = (si.totalram - si.freeram) / (1024.0 * 1024.0 * 1024.0);
            FILE *statFile = fopen("/proc/stat", "r");
            long preUser, preNice, preSys, preIdle, preIowait, preIrq, preSoftirq;
            fscanf(statFile, "cpu %ld %ld %ld %ld %ld %ld %ld", &preUser, &preNice, &preSys, &preIdle, &preIowait,
                   &preIrq, &preSoftirq);
            long preTotal = preUser + preNice + preSys + preIdle + preIowait + preIrq + preSoftirq;

            // using sleep here so that I can compare easily instead
            // of doing it at last (I hope my assumption works)
            sleep(tdelay);
            char result[MAXSIZE];
            char result1[MAXSIZE];
            char result2[MAXSIZE];
            if (sys || def) {
                // design choice that i am not printing blank spaces :-)
                printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
                printf("%s", print_mem_info(graphics, preMemo, result));
                printf("---------------------------------------\n");
            }
            print_users_info(result2);
            if (user || def)printf("%s\n", result2);
            if (sys || def){
                displayCores();
                if(graphics) printf("%s", displayCPUUsage(preTotal, preIdle, 0, result1));
                printf("%s", displayCPUUsage(preTotal, preIdle, graphics, result1));
                printf("---------------------------------------\n");
            }
            if (last_info) print_system_info();
            fclose(statFile);
        }
    }

    return 0;
}