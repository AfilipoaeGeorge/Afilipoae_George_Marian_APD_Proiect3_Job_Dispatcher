//Afilipoae George-Marian, 3C.1.1
#include "clust.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);//initializam mediul mpi

    int rank, size;
    float start,end,serial = 0,parallel = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);//obtinem rankul procesului curent
    MPI_Comm_size(MPI_COMM_WORLD, &size);//obtine numarul total de procese

    if (size < 2) {// verificam daca avem cel putin un master si un worker
        printf("Eroare: trebuie cel putin un master si un worker.\n");
        MPI_Finalize();//inchidem mpi
        exit(-1);//iesim din program
    }

    if (rank == 0) {//cod pentru dispatcher(rank=0)
        FILE* log_file_parallel = fopen(LOG_FILE_PARALLEL, "w");//deschidem fisierul de log pentru sciere
        if (!log_file_parallel) {//verificam daca s a deschis
            perror("Eroare: nu se deschide fisierul de log.");
            MPI_Abort(MPI_COMM_WORLD, -1);//daca nu, terminam procesele cu eroare
        }

        float wait_time_serial_final = 0.0;

        start = MPI_Wtime();//incepem masurarea timpului pentru executia seriala
        process_serial(COMMAND_FILE, &wait_time_serial_final);//apelam executia seriala
        end = MPI_Wtime();//terminam masurarea timpulu pentru executia seriala
        serial = end - start - wait_time_serial_final;  //scadem wait-ul pentru a calcula speedup-ul fara el

        printf("serial time = %f\t\twait_time(%f)\n", serial,wait_time_serial_final);
        fflush(stdout);

        start = MPI_Wtime(); // start cu MPI pentru a calcula timpul de executie cu MPI(paralel)

        FILE* file_input_command = fopen(COMMAND_FILE, "r");//deschidem fisierul de comenzi
        if (!file_input_command) {//verificam daca s a deschis
            perror("Eroare: nu se deschide fisierul de input in paralel.");
            MPI_Abort(MPI_COMM_WORLD, -1);//daca nu, terminam procesele cu eroare
        }

        char line_command[MAX_LENGTH_LINE];
        int current_worker = 1; // dam start cu primul worker
        int command_id = 0; //si comanda 0
        MPI_Status status;

        // citim fiecare linie din fisierul de comenzi
        while (fgets(line_command, sizeof(line_command), file_input_command)) {//citim linie cu linie pana la final
            if (strncmp(line_command, "WAIT", 4) == 0) {// verificam daca linia este o comanda WAIT
                int wait_time = atoi(&line_command[5]); // extragem timpul de asteptare
                sleep(wait_time);// asteptam timpul specificat
                start += wait_time; //aici am adunat wait_time pentru a calcula speedup-ul fara el(e ca si cum as fi scazut la final din end wait_time-ul)

            } else if (strncmp(line_command, "CLI", 3) == 0) {// verificam daca linia este o comanda CLI
                char command_with_id[MAX_LENGTH_LINE*2];
                snprintf(command_with_id, sizeof(command_with_id), "%d %s", command_id, line_command);// adaugam id-ul comenzii la linia de comanda

                // inregistreaza trimiterea comenzii in fisierul de lg\og
                char timestamp[MAX_BUFFER_SIZE_TIMESTAMP];
                get_current_time(timestamp, sizeof(timestamp));//obitnem timpul curent
                fprintf(log_file_parallel, "[%s] Trimite comanda la worker-ul %d: %s\n", timestamp, current_worker, command_with_id);
                fflush(log_file_parallel);

                MPI_Send(command_with_id, strlen(command_with_id) + 1, MPI_CHAR, current_worker, COMMAND_TAG, MPI_COMM_WORLD);//trimitem comanda catre worker
                command_id++;//incrementam id-ul comenzii
                current_worker++;//trecem la urmatorul worker
                if (current_worker > size - 1) {//revenim la primul worker daca s-a dat la toti
                    current_worker = 1;
                }
            }
        }

        for (int i = 1; i <= size - 1; i++) {//trimitem mesajele de finalizare spre toti workerii
            MPI_Send(FINAL_MESSAGE, strlen(FINAL_MESSAGE) + 1, MPI_CHAR, i, COMMAND_TAG, MPI_COMM_WORLD);
        }

        while (command_id > 0) {//asteptam rezultatele de la workeri
            char result[MAX_SIZE_RESULT];
            char client_id[MAX_SIZE_ID], remain_result[MAX_SIZE_RESULT];
            MPI_Recv(result, sizeof(result), MPI_CHAR, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &status);//primim rezultatul de la un worker

            char timestamp[MAX_BUFFER_SIZE_TIMESTAMP];
            get_current_time(timestamp, sizeof(timestamp)); //dupa ce primim rezultatul printam si in log timpul
            fprintf(log_file_parallel, "[%s] Primeste comanda de la worker-ul %d: %s\n\n", timestamp, status.MPI_SOURCE, result);
            fflush(log_file_parallel);

            command_id--;//decremenntam numarul de comenzi ramase de procesat

            sscanf(result, "%s %[^\n]", client_id, remain_result);//despartim rezultatul in id si restul rezultatului
            char client_file[MAX_FILE_LENGTH];
            sprintf(client_file, "%s_parallel.txt", client_id);//generam numele fisierului de iesire pentru client
            FILE* client_output = fopen(client_file, "a");//il deschidem
            if (client_output) {//verificam daca s a deschis
                fprintf(client_output, "%s\n", remain_result);//scriem rezultatul in fisier
                if(fclose(client_output)!=0){
                    printf("Eroare: nu s-a inchis fisierul de iesire\n");
                    exit(-1);
                }//inchidem fisierul
            } else {
                printf("Eroare: nu se poate deschide fisierul clientului: %s\n", client_file);
            }
        }

        if(fclose(file_input_command)!=0){
            printf("Eroare: nu s-a inchis fisierul de intrare\n");
            exit(-1);
        }//inchidem fisierul de comenzi
        if(fclose(log_file_parallel)!=0){
            printf("Eroare: nu s-a inchis fisierul de log\n");
            exit(-1);
        }//inchidem fisierul de log
    } else {//cod pentru workeri(rank>0)
        char command[MAX_LENGTH_LINE];
        MPI_Status status;

        while (1) {//bucla de procesare a comenzilor
            MPI_Recv(command, sizeof(command), MPI_CHAR, 0, COMMAND_TAG, MPI_COMM_WORLD, &status);//primeste o comanda de la dispatcher

            if (strcmp(command, FINAL_MESSAGE) == 0) {//verificam daca e mesajul de finalizare
                break;//daca da, intrerupem
            }

            char client_id[MAX_SIZE_ID], command_type[MAX_SIZE_COMMAND_TYPE], parameter[MAX_SIZE_PARAMETER], anagram[MAX_CHARACTER_IN_ANAGRAM_BUFFER], anagrams[MAX_SIZE_BUFFER_ANAGRAMS];
            int cmd_id;
            //initializam sirurile
            anagram[0] = '\0';
            anagrams[0] = '\0';

            sscanf(command, "%d %s %s %s", &cmd_id, client_id, command_type, parameter);//despartim comanda

            // printf("Worker %d proceseaza comanda: %s %s %s\n", rank, command_type, parameter, client_id);
            // fflush(stdout);

            char result_message[MAX_SIZE_BUFFER_ANAGRAMS+MAX_SIZE_ID+MAX_SIZE_PARAMETER+10];
            if (strcmp(command_type, "PRIMES") == 0) {//procesam primes
                int n = atoi(parameter);
                int count = count_primes(n);
                snprintf(result_message, sizeof(result_message), "%s PRIMES %d = %d", client_id, n, count);

            } else if (strcmp(command_type, "PRIMEDIVISORS") == 0) {//procesam comanda primesdivisors
                int n = atoi(parameter);
                int count = count_prime_divisors(n);
                snprintf(result_message, sizeof(result_message), "%s PRIMEDIVISORS %d = %d", client_id, n, count);

            } else if (strcmp(command_type, "ANAGRAMS") == 0) {//procesam comanda anagrams
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
        end = MPI_Wtime();//terminarea masurarii timpului paralel
        parallel = end-start;//calculam timpul paralel
        printf("parallel time = %f\n",parallel);//afisam timpul paralel
        fflush(stdout);

        FILE* speedup = fopen("speedup.txt","a");//deschidem fisierul de speedup
        if(speedup == NULL){//verificam daca s a deschis
            printf("Error: open speedup file\n");
            exit(-1);//daca nu, iesim cu exit
        }
        printf("speedup = %f\n",serial/parallel);//calculam si afisam speedupul
        fflush(stdout);
        fprintf(speedup,"speedup=%f proccess=%d serial_time=%f parallel_time=%f\n",serial/parallel,size,serial,parallel);
        if(fclose(speedup)!=0){
            printf("Eroare: nu s-a inchis fisierul de speedup\n");
            exit(-1);

        }//incidem fisierul de speedup
    }

    MPI_Finalize();//inchidem mediul mpi
    return 0;
}
