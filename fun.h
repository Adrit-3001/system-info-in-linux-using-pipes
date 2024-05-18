//
// Created by adrit on 2024-04-02.
//

#ifndef ASS_FUN_H
#define ASS_FUN_H
void print_system_info();

void print_users_info(char* result);

char* print_mem_info(int graphics, double preMemo, char* result);

void clearScreen();

void displayCores();

char* displayCPUUsage(long preTotal, long preIdle, int graphics, char* result);

void getInitialInfo(int samples, int delay);

#endif //ASS_FUN_H
