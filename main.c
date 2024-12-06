#include "clust.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    float start,end,serial = 0,parallel = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("Eroare: trebuie cel putin un master si un worker.\n");
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        FILE* log_file_parallel = fopen(LOG_FILE_PARALLEL, "w");
        if (!log_file_parallel) {
            perror("Eroare: nu se deschide fisierul de log.");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        float wait_time_serial_final = 0.0;

        start = MPI_Wtime();
        process_serial(COMMAND_FILE, &wait_time_serial_final);
        end = MPI_Wtime();
        serial = end - start - wait_time_serial_final;  //scadem wait-ul pentru a calcula speedup-ul fara el

        printf("serial time = %f\t\twait_time(%f)\n", serial,wait_time_serial_final);
        fflush(stdout);

        start = MPI_Wtime(); // start cu MPI pentru a calcula timpul de executie cu MPI(paralel)

        FILE* file_input_command = fopen(COMMAND_FILE, "r");
        if (!file_input_command) {
            perror("Eroare: nu se deschide fisierul de input in paralel.");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        char line_command[MAX_LENGTH_LINE];
        int current_worker = 1; // dam start cu primul worker
        int command_id = 0; //si comanda 0
        MPI_Status status;

        while (fgets(line_command, sizeof(line_command), file_input_command)) {//citim linie cu linie pana la final
            if (strncmp(line_command, "WAIT", 4) == 0) {
                int wait_time = atoi(&line_command[5]); 
                sleep(wait_time);
                start += wait_time; //aici am adunat wait_time pentru a calcula speedup-ul fara el(e ca si cum as fi scazut la final din end wait_time-ul)

            } else if (strncmp(line_command, "CLI", 3) == 0) {
                char command_with_id[MAX_LENGTH_LINE*2];
                snprintf(command_with_id, sizeof(command_with_id), "%d %s", command_id, line_command);

                // inregistreaza trimiterea comenzii
                char timestamp[MAX_BUFFER_SIZE_TIMESTAMP];
                get_current_time(timestamp, sizeof(timestamp));
                fprintf(log_file_parallel, "[%s] Trimite comanda la worker-ul %d: %s\n", timestamp, current_worker, command_with_id);
                fflush(log_file_parallel);

                MPI_Send(command_with_id, strlen(command_with_id) + 1, MPI_CHAR, current_worker, COMMAND_TAG, MPI_COMM_WORLD);
                command_id++;
                current_worker++;
                if (current_worker > size - 1) {
                    current_worker = 1;
                }
            }
        }

        for (int i = 1; i <= size - 1; i++) {
            MPI_Send(FINAL_MESSAGE, strlen(FINAL_MESSAGE) + 1, MPI_CHAR, i, COMMAND_TAG, MPI_COMM_WORLD);
        }

        while (command_id > 0) {
            char result[MAX_SIZE_RESULT];
            char client_id[MAX_SIZE_ID], remain_result[MAX_SIZE_RESULT];
            MPI_Recv(result, sizeof(result), MPI_CHAR, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &status);

            char timestamp[MAX_BUFFER_SIZE_TIMESTAMP];
            get_current_time(timestamp, sizeof(timestamp)); //dupa ce primim rezultatul printam si in log timpul
            fprintf(log_file_parallel, "[%s] Primeste comanda de la worker-ul %d: %s\n\n", timestamp, status.MPI_SOURCE, result);
            fflush(log_file_parallel);

            command_id--;

            sscanf(result, "%s %[^\n]", client_id, remain_result);
            char client_file[MAX_FILE_LENGTH];
            sprintf(client_file, "%s_parallel.txt", client_id);
            FILE* client_output = fopen(client_file, "a");
            if (client_output) {
                fprintf(client_output, "%s\n", remain_result);
                fclose(client_output);
            } else {
                printf("Eroare: nu se poate deschide fisierul clientului: %s\n", client_file);
            }
        }

        fclose(file_input_command);
        fclose(log_file_parallel);
    } else {
        char command[MAX_LENGTH_LINE];
        MPI_Status status;

        while (1) {
            MPI_Recv(command, sizeof(command), MPI_CHAR, 0, COMMAND_TAG, MPI_COMM_WORLD, &status);

            if (strcmp(command, FINAL_MESSAGE) == 0) {
                break;
            }

            char client_id[MAX_SIZE_ID], command_type[MAX_SIZE_COMMAND_TYPE], parameter[MAX_SIZE_PARAMETER], anagram[MAX_CHARACTER_IN_ANAGRAM_BUFFER], anagrams[MAX_SIZE_BUFFER_ANAGRAMS];
            int cmd_id;

            anagram[0] = '\0';
            anagrams[0] = '\0';

            sscanf(command, "%d %s %s %s", &cmd_id, client_id, command_type, parameter);

            // printf("Worker %d proceseaza comanda: %s %s %s\n", rank, command_type, parameter, client_id);
            // fflush(stdout);

            char result_message[MAX_SIZE_BUFFER_ANAGRAMS+MAX_SIZE_ID+MAX_SIZE_PARAMETER+10];
            if (strcmp(command_type, "PRIMES") == 0) {
                int n = atoi(parameter);
                int count = count_primes(n);
                snprintf(result_message, sizeof(result_message), "%s PRIMES %d = %d", client_id, n, count);

            } else if (strcmp(command_type, "PRIMEDIVISORS") == 0) {
                int n = atoi(parameter);
                int count = count_prime_divisors(n);
                snprintf(result_message, sizeof(result_message), "%s PRIMEDIVISORS %d = %d", client_id, n, count);

            } else if (strcmp(command_type, "ANAGRAMS") == 0) {
                generate_anagrams(parameter, anagram, anagrams);
                snprintf(result_message, sizeof(result_message), "%s ANAGRAMS %s: %s", client_id, parameter, anagrams);
            }

            // trimiterea rezultatului inapoi la dispatcher
            MPI_Send(result_message, strlen(result_message) + 1, MPI_CHAR, 0, RESULT_TAG, MPI_COMM_WORLD);
            // printf("Worker %d trimite rezultatul: %s\n", rank, result_message);
            // fflush(stdout);
        }
    }

    if(rank == 0){
        end = MPI_Wtime();
        parallel = end-start;
        printf("parallel time = %f\n",parallel);
        fflush(stdout);

        printf("speedup = %f\n",serial/parallel);
        fflush(stdout);
    }

    MPI_Finalize();
    return 0;
}