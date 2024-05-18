//
// Created by adrit on 2024-03-29.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <utmp.h>
#include <sys/resource.h>
#include "fun.h"

// MACRO I WILL USE
#define min_double(x, y) (((x) < (y)) ? (x) : (y))

// Function to print essential system information
void print_system_info() {
    /* this function uses 2 libraries(sysinfo, utsname) to get the required system information.
        first the function initializes them. After that it simply prints out the values using the
        right function call.
    */
    struct sysinfo si;
    struct utsname un;

    if (sysinfo(&si) != 0) {// just to see if it fails or not. if i dont put this, nothing is printed out.
        printf("error fetching sysinfo :-(\n");
        return;
    }
    if (uname(&un) != 0) {
        printf("error fetching utsname :-(\n");
        return;
    }

    printf("### System Information ###\n");
    printf("System Name = %s\n", un.sysname);
    printf("Machine Name = %s\n", un.nodename);
    printf("Version = %s\n", un.version);
    printf("Release = %s\n", un.release);
    printf("Architecture = %s\n", un.machine);
    printf("System running since last reboot: %ld days %ld:%ld:%ld (%ld:%ld:%ld)\n", si.uptime / (24 * 3600),
           (si.uptime % (24 * 3600)) / 3600, (si.uptime % 3600) / 60, si.uptime % 60,
           si.uptime / (3600), (si.uptime % 3600) / 60, si.uptime % 60);
    printf("---------------------------------------\n");
}

// Function to print information about user sessions
void print_users_info(char* result){
    /* this function uses 1 libraries(utmp) to get the required users information.
        first it uses setutent and getutent to populate data structure with information. After that it simply prints out the values using the
        the right function call. it also prints out the number of cores of a computer by counting the number of times
        "processor" was repeated in the proc/cpuinfo file. lastly if prints the current cpu usage. more information on how
        to do this can be found throughout the code and in the README file.
    */
    int user_count = 0;
    struct utmp *user;
    char info[UT_NAMESIZE +1];// this const. is given in man info for library and is 32

    setutent();//thanx to my TA ik about this
    user = getutent(); //getutent() populates the data structure with information
    //source: https://man7.org/linux/man-pages/man3/getutent.3.html

    strcpy(result, ""); // Initialize result string
    strcat(result, "### Sessions/users ###\n");
    while(user != NULL){
        if(user->ut_type == USER_PROCESS){// another const. with value 7
            strncpy(info, user->ut_user, UT_NAMESIZE);
            info[UT_NAMESIZE] = '\0';
            const char *ip_addr = user->ut_host;
            char *line = malloc(256); // Allocate memory for the line
            if (line == NULL) {
                printf("error allocating memory :-(\n");
            }
            sprintf(line, "%s\t%s (%s)\n", info, user->ut_line, ip_addr);
            strcat(result, line);
            free(line); // Free the allocated memory for the line
            user_count++;
        }
        user = getutent();
    }
    strcat(result, "---------------------------------------");
    endutent(); // close utmp file
}

// Function to print memory usage information
char* print_mem_info(int graphics, double preMemo, char* result){
    /* this function uses 2 libraries(sysinfo, sys/resource) to get the required memory information.
        first it prints the total memory usage by our program in kb. then it prints various memory info using the sysinfo
        library. more information can be found throughout the code and in the README file.
    */
    struct rusage memUsage;
    struct rusage* addr = &memUsage;

    if (getrusage(RUSAGE_SELF, addr) != 0) {// using info about sys/resource and getrusage (using man command)
        printf("error fetching usageInfo :-(\n");
        return NULL;
    }

    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        printf("error fetching sysinfo :-(\n");
        return NULL;
    }

    double physTotal = si.totalram / (1024.0 * 1024.0 * 1024.0);
    double physUsed = (si.totalram - si.freeram) / (1024.0 * 1024.0 * 1024.0);
    double virTotal = (si.totalram + si.totalswap) / (1024.0 * 1024.0 * 1024.0);
    double virUsed = (si.totalram - si.freeram + si.totalswap - si.freeswap) / (1024.0 * 1024.0 * 1024.0); // Wrong calculation
    if(!graphics){
        sprintf(result, "%.2f GB / %.2f GB -- %.2f GB / %.2f GB\n",
                physUsed, physTotal, virUsed, virTotal);
    } else{
        sprintf(result,"%.2f GB / %.2f GB -- %.2f GB / %.2f GB |", physUsed, physTotal, virUsed, virTotal);
        double change = (physUsed - preMemo)*100;
        if (change > 0) {
            for (int j = 0; j < min_double(change, 20); j++) {
                strcat(result, "#");
            }
            strcat(result, "*");
        } else {
            for (int j = 0; j < min_double(change * -1, 20); j++) {
                strcat(result, ":");
            }
            strcat(result, "@");
        }
        sprintf(result + strlen(result), " %.2f (%.2f)\n", change, physUsed);
    }
    return result;
}

// Function to clear the console screen
void clearScreen() {
    // using the info provided on the site: https://stackoverflow.com/questions/37774983/clearing-the-screen-by-printing-a-character
    printf("\033[2J"); // clear
    printf("\033[H");  // left
}

/* NEW THINGS */
void displayCores(){
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");//with the help of https://www.cyberciti.biz/faq/check-how-many-cpus-are-there-in-linux-system/
    int num = 0;
    char line[255];
    while (fgets(line, sizeof(line), cpuinfo) != NULL) {
        if (strncmp(line, "processor", 9) == 0) {// just sibling wont work, coz it counts multithreading as well, so dont risk it!
            num++;
        }
    }
    fclose(cpuinfo);
    printf("Number of cores: %d\n", num);
}

char* displayCPUUsage(long preTotal, long preIdle, int graphics, char* result){
    // to do the cpu usage, i will use info provided on the website
    // https://www.linuxhowtos.org/System/procstat.html
    // my design decision for cpu usage is to match the tdelay.
    // for more info on how to read this file see: https://www.baeldung.com/linux/total-process-cpu-usage#:~:text=2.2.-,The%20CPU%20Usage,been%20running%20in%20user%20mode.
    FILE *statFile = fopen("/proc/stat", "r");
    char line[256];
    strcpy(result, ""); // Initialize result string

    if (statFile != NULL) {
        unsigned long long user, nice, system, idle, iowait, irq, softirq;
        if (fgets(line, sizeof(line), statFile) != NULL) {
            sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu",
                   &user, &nice, &system, &idle, &iowait, &irq,
                   &softirq);
            unsigned long long totalCpuTime = user + nice + system +
                                              idle + iowait + irq + softirq;
            long total = totalCpuTime - preTotal;
            long idleFinal = idle - preIdle;
            float cpuUsage;
            // handle the division by 0 case :-) My design choice that i
            // will display the original cpu use
            if (total != 0) cpuUsage = ((float) (total - idleFinal) /total) * 100.0;
            else cpuUsage = ((float)(preTotal - preIdle) / preTotal) *100.0;
            if (!graphics) sprintf(result, "Total CPU usage: %.2f%%\n", cpuUsage);
            if (graphics) {
//                sprintf(result, "%.2f%%", cpuUsage);
                strcat(result, "\t");
                strcat(result, "||");
//                else strcat(result, "||");
                for (int i = 0; i < cpuUsage; ++i) {
                    strcat(result, "|");
                }
                sprintf(result + strlen(result), " %.2f\n", cpuUsage);
            }
        } else {
            fprintf(stderr,
                    "Error reading CPU usage information from /proc/stat.\n");
        }
        fclose(statFile);
    } else {
        fprintf(stderr, "Error opening /proc/stat\n");
    }
    return result;
}

void getInitialInfo(int samples, int delay){
    printf("Nbr of samples: %d -- every %d secs\n", samples, delay);
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf(" Memory Usage: %ld Kilobytes\n", usage.ru_maxrss);
}