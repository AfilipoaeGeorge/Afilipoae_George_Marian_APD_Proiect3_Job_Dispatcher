#ifndef __CLUST_H_
#define __CLUST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>
#include <time.h>

#define LOG_FILE_PARALLEL "log.txt"
#define COMMAND_FILE "commands.txt"
#define COMMAND_TAG 1
#define RESULT_TAG 2
#define FINAL_MESSAGE "FINAL"
#define MAX_SIZE_BUFFER_ANAGRAMS 45000
#define MAX_SIZE_ID 10
#define MAX_SIZE_COMMAND_TYPE 20
#define MAX_SIZE_PARAMETER 100
#define MAX_FILE_LENGTH 50
#define MAX_LENGTH_LINE 256
#define MAX_SIZE_RESULT 50000
#define MAX_CHARACTER_IN_ANAGRAM_BUFFER 9
#define MAX_BUFFER_SIZE_TIMESTAMP 20

int count_primes(int n);
int count_prime_divisors(int n);
void generate_anagrams(char* str, char* output, char* output_complete);
void get_current_time(char* buffer, size_t size);
void process_serial(char* command_file ,float* wait_time_final);

#endif