#include "clust.h"

void get_current_time(char* buffer, size_t size) {
    time_t time_now = time(NULL);
    struct tm* t = localtime(&time_now);
    strftime(buffer, size, "%H:%M:%S", t); // Formatul orei
}

int count_primes(int number) {
    int count = 0;
    for (int i = 2; i <= number; i++) {
        int number_prime = 1;
        for (int j = 2; j * j <= i; j++) {
            if (i % j == 0) {
                number_prime = 0;
                break;
            }
        }
        if (number_prime){
            count++;
        }
    }
    return count;
}

int count_prime_divisors(int number) {
    int count = 0;
    for (int i = 2; i <= number; i++) {
        if (number % i == 0) {
            int number_prime = 1;
            for (int j = 2; j * j <= i; j++) {
                if (i % j == 0) {
                    number_prime = 0;
                    break;
                }
            }
            if (number_prime){
                count++;
            }
        }
    }
    return count;
}

void swap(char* a, char* b) {
    char temp = *a;
    *a = *b;
    *b = temp;
}

void permute(char* input_string, int left, int right, char* anagram, char* final_anagrams) {
    if (left == right) {
        sprintf(anagram, "%s", input_string);
        strcat(final_anagrams, anagram);
        strcat(final_anagrams, " ");
    } else {
        for (int i = left; i <= right; i++) {
            swap((input_string + left), (input_string + i));
            permute(input_string, left + 1, right, anagram, final_anagrams);
            swap((input_string + left), (input_string + i)); // backtrack
        }
    }
}

void generate_anagrams(char* str, char* anagram, char* final_anagrams) {
    int length = strlen(str);
    permute(str, 0, length - 1, anagram, final_anagrams);
}

void process_serial(char* command_file, float* wait_time_serial_final) {
    FILE* input_command_file = fopen(command_file, "r");
    if (!input_command_file) {
        printf("Error: Could not open the command file.\n");
        return;
    }

    char line_command[MAX_LENGTH_LINE];
    char anagram[9]; //pentru cuvinte de maxim 7 caractere

    while (fgets(line_command, sizeof(line_command),input_command_file)) {

        if (strncmp(line_command, "WAIT", 4) == 0) {
            int wait_time = atoi(&line_command[5]);
            sleep(wait_time);
            (*wait_time_serial_final) += wait_time;

        } else if (strncmp(line_command, "CLI", 3) == 0) {

            char client_id[MAX_SIZE_ID], command_type[MAX_SIZE_COMMAND_TYPE], parameter[MAX_SIZE_PARAMETER], final_anagrams[MAX_SIZE_BUFFER_ANAGRAMS];
            final_anagrams[0]='\0';
            sscanf(line_command, "%s %s %s", client_id, command_type, parameter);  //stim ca linia e de forma %s %s %s si o despartim in id, comanda si parametru

            char client_file[MAX_FILE_LENGTH];
            sprintf(client_file, "%s_serial.txt", client_id);
            FILE* client_output = fopen(client_file, "a");
            if (!client_output) {
                printf("Error: Could not open anagram file for client %s.\n", client_id);
                continue;
            }

            if (strcmp(command_type, "PRIMES") == 0) {
                int n = atoi(parameter);
                int count = count_primes(n);
                fprintf(client_output, "PRIMES %d = %d\n", n, count);
            } else if (strcmp(command_type, "PRIMEDIVISORS") == 0) {
                int n = atoi(parameter);
                int count = count_prime_divisors(n);
                fprintf(client_output, "PRIMEDIVISORS %d = %d\n", n, count);
            } else if (strcmp(command_type, "ANAGRAMS") == 0) {
                fprintf(client_output, "ANAGRAMS %s: ", parameter);
                generate_anagrams(parameter, anagram, final_anagrams);
                fprintf(client_output,"%s\n",final_anagrams);
            }

            fclose(client_output);
        }

    }

    fclose(input_command_file);
}
