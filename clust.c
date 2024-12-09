//Afilipoae George-Marian, 3C.1.1
#include "clust.h"

//functia pentru a afla timpul pentru a afisa in fisierul de log
void get_current_time(char* buffer, size_t size) {
    time_t time_now = time(NULL);
    struct tm* time = localtime(&time_now);
    strftime(buffer, size, "%H:%M:%S", time); // formatul orei
}

//funtie in care primim un numar n si returnam numarul de numere prime pana la el
int count_primes(int number) {
    int count = 0;  // variabila pentru a tine evidenta numarului de numere prime
    for (int i = 2; i <= number; i++) {
        int number_prime = 1;// variabila care determina daca un numar este prim (1 = prim, 0 = nu)
        // verificam daca numarul curent `i` este divizibil cu oricare alt numar
        // mai mic sau egal cu radicalul sau
        for (int j = 2; j * j <= i; j++) {
            if (i % j == 0) { // daca i este divizibil cu j, atunci nu este numar prim
                number_prime = 0; // setam ca nu este prim
                break; //iesim din bucla
            }
        }
        if (number_prime){ //daca numarul e prim, crestem contorul
            count++;
        }
    }
    return count;//returnam numarul total de numere prime gasite
}

// functie care numara cati divizori primi are un numar dat
int count_prime_divisors(int number) {
    int count = 0;  // variabila pentru a tine evidenta numarului de divizori primi
    for (int i = 2; i <= number; i++) {
        if (number % i == 0) {    // verificam daca i este divizor al lui number
            int number_prime = 1;  // variabila care determina daca divizorul `i` este prim
            // verificam daca i este prim
            for (int j = 2; j * j <= i; j++) {
                if (i % j == 0) { // daca i este divizibil cu j, atunci nu este prim
                    number_prime = 0;// setam ca nu este prim
                    break;//iesim din bucla
                }
            }
            if (number_prime){//daca e prim incrementam contorul
                count++;
            }
        }
    }
    return count;//returnam numarul total de divizori primi
}

//functie de interschimbare
void swap(char* a, char* b) {
    char temp = *a;
    *a = *b;
    *b = temp;
}

// functie recursiva pentru generarea tuturor permutarilor unui sir de caractere
void permute(char* input_string, int left, int right, char* anagram, char* final_anagrams) {
    if (left == right) {// verificam daca am generat o permutare completa
        sprintf(anagram, "%s", input_string);
        // concatenam anagrama generata la sirul final al anagramelor
        strcat(final_anagrams, anagram);
        strcat(final_anagrams, " ");
    } else {
        for (int i = left; i <= right; i++) {// iteram prin toate pozitiile posibile pentru a genera permutari
            swap((input_string + left), (input_string + i));// schimbam caracterele intre pozitiile left si i
            permute(input_string, left + 1, right, anagram, final_anagrams);// apelam recursiv pentru a genera restul permutarii
            swap((input_string + left), (input_string + i));// revenim la starea initiala (backtracking)
        }
    }
}

// functie care initializeaza procesul de generare a anagramelor unui sir de caractere
void generate_anagrams(char* str, char* anagram, char* final_anagrams) {
    int length = strlen(str); // calculam lungimea sirului de intrare
    permute(str, 0, length - 1, anagram, final_anagrams);// apelam functia recursiva pentru a genera toate anagramele
}

// functie care proceseaza comenzile dintr-un fisier intr-un mod secvential
void process_serial(char* command_file, float* wait_time_serial_final) {
    FILE* input_command_file = fopen(command_file, "r");// deschidem fisierul cu comenzile
    if (!input_command_file) {
        printf("Eroare: nu s-a putut deschide fisierul de input.\n");
        return;
    }

    char line_command[MAX_LENGTH_LINE];
    char anagram[9]; //pentru cuvinte de maxim 7 caractere

    while (fgets(line_command, sizeof(line_command),input_command_file)) {//parcurgem linie cu linie fisierul

        if (strncmp(line_command, "WAIT", 4) == 0) {// verificam daca linia este o comanda WAIT
            int wait_time = atoi(&line_command[5]);// extragem timpul de asteptare
            sleep(wait_time);// asteptam timpul specificat
            (*wait_time_serial_final) += wait_time; // actualizam timpul total de asteptare

        } else if (strncmp(line_command, "CLI", 3) == 0) {
            // extragem informatiile din linia de comanda pentru client
            char client_id[MAX_SIZE_ID], command_type[MAX_SIZE_COMMAND_TYPE], parameter[MAX_SIZE_PARAMETER], final_anagrams[MAX_SIZE_BUFFER_ANAGRAMS];
            final_anagrams[0]='\0';// initializam sirul de anagrame ca fiind gol
            sscanf(line_command, "%s %s %s", client_id, command_type, parameter);  //stim ca linia e de forma %s %s %s si o despartim in id, comanda si parametru

            char client_file[MAX_FILE_LENGTH];
            sprintf(client_file, "%s_serial.txt", client_id);// generam numele fisierului pentru client
            FILE* client_output = fopen(client_file, "a");// deschidem fisierul clientului pentru scriere
            if (!client_output) {
                printf("Eroare: nu s-a  putut deschide anagram file pentru clientul %s.\n", client_id);
                fflush(stdout);
                continue;// trecem la urmatoarea comanda
            }
            // procesam tipul comenzii
            if (strcmp(command_type, "PRIMES") == 0) {
                int n = atoi(parameter); // convertim parametrul la int
                int count = count_primes(n); // numaram numerele prime
                fprintf(client_output, "PRIMES %d = %d\n", n, count); // scriem rezultatul in fisier
            } else if (strcmp(command_type, "PRIMEDIVISORS") == 0) {
                int n = atoi(parameter); // convertim parametrul la int
                int count = count_prime_divisors(n); // numaram divizorii primi
                fprintf(client_output, "PRIMEDIVISORS %d = %d\n", n, count); // scriem rezultatul in fisier
            } else if (strcmp(command_type, "ANAGRAMS") == 0) {
                fprintf(client_output, "ANAGRAMS %s: ", parameter); // scriem introducerea pentru anagrame
                generate_anagrams(parameter, anagram, final_anagrams); // generam anagramele
                fprintf(client_output, "%s\n", final_anagrams); // scriem anagramele in fisier
            }

            if(fclose(client_output)!=0){
                printf("Eroare: nu s-a putut inchide fisierul de output\n");
                exit(-1);
            }// inchidem fisierul clientului
        }

    }

    if(fclose(input_command_file)!=0){
        printf("Eroare: nu s-a putut inchide fisierul de input\n");
        exit(-1);
    }// inchidem fisierul de comenzi
}
