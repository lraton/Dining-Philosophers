#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>

static int * stopFlag;

void SignalHandler(int iSignalCode) {
    //printf("\nCtrl+c rilevato\n");
    * stopFlag = 1;
}

int main(int argc, char * argv[]) {

    //Variabili per gesitre i signal
    struct sigaction sa;
    memset( & sa, '\0', sizeof(struct sigaction));
    sa.sa_handler = SignalHandler;
    sigaction(SIGINT, & sa, NULL);

    //shared memory per poter usare la variabile in ogni child
    stopFlag = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    * stopFlag = 0;

    //flags selezionate
    char * p;
    long conv = strtol(argv[1], & p, 10);
    int checkdeadlock = strtol(argv[2], & p, 2);
    int NoDeadlock = strtol(argv[3], & p, 2);
    int checkStarvation = strtol(argv[4], & p, 2);

    //Struct per salvare i semafori per comodità
    struct {
        char name[128];
        sem_t * sem;
    }
    sem[conv];
    pid_t pids[conv]; //Vettore per i vari child

    //matrice per rilevamento deadlock shared memory, potrei utilizzare una semplice varibile ma in quel caso dovrei usare un'altro semaforo'
    int * w[conv][conv];
    for (int i = 0; i < conv; i++) {
        for (int j = 0; j < conv; j++) {
            w[i][j] = mmap(NULL, sizeof * w, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
            * w[i][j] = 0;
        }
    }

    int value = 0; // variabile per valore semafori

    //Genero i semafori in base a quanti filosofi
    for (int i = 0; i < conv; i++) {
        //genero per ogni semaforo un nome univoco
        sprintf(sem[i].name, "/semaphore%zu", i);
        //stampo il nome del semaforo per test
        for (int j = 0; j < strlen(sem[i].name); j++) {
            printf("%c", sem[i].name[j]);
        }

        //nel dubbio chiudo e unlinko i semafori prima di crearli, per evitare che erano rimasti salvati nel sistema ed evitare problemi
        sem_close(sem[i].sem);
        sem_unlink(sem[i].name);
        printf("\n");
        //creo i semafori
        if ((sem[i].sem = sem_open(sem[i].name, O_CREAT, S_IRWXU, 1)) == SEM_FAILED) {
            printf("Errore in sem_open, errno = %d\n", errno);
            exit(EXIT_FAILURE);
        }
        //mi stampo il numero del semaforo e il suo valore per test
        /*
        sem_getvalue(sem[i].sem, & value);
        printf("%d %d\n", i, value);
        */
    }

    //Variabile sem_timedwait
    int s = 0;

    /* Start children. */
    for (int i = 0; i <= conv; i++) { //Genero un processo in più che uso per controllare lo stallo
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork ");
            exit(-1);
        }
        if (pids[i] == 0) {
            //child;
            if (i != conv) { //controllo che non sto usando l'ultimo child'
                if (NoDeadlock == 1 || checkStarvation == 1) { // controllo se l'utente ha richiesto di controllare lo starvation o semplicemente l'algoritmo senza deadlock
                    while ( * stopFlag != 1) { //il ciclo si stppa solo se viene attivata la flag per stoppare tutto

                        //Timeout starvation
                        struct timespec timeout;
                        if (clock_gettime(CLOCK_REALTIME, & timeout) == -1) {
                            return -1;
                        }
                        timeout.tv_sec += 8; // Set the timeout to 8 seconds

                        //filosofo prende la forchetta sinistra e destra
                        if (i != conv - 1) {
                            printf("Filosofo %d aspetta la forchetta sinistra %d\n", i, i);
                            if (checkStarvation == 1) { //se il controllo dello starvation è attivo faccio il timedwait sennò il wait normale
                                s = sem_timedwait(sem[i].sem, & timeout);
                            } else {
                                sem_wait(sem[i].sem);
                            }
                            if ( * stopFlag == 1 || s == -1) { //Dopo ogni wait effettuo il controllo se il timedwait è scaduto o se è attiva la flag per stopapre tutto
                                if (s == -1) {
                                    printf("\nStarvation detected \n");
                                }
                                * stopFlag = 1;
                                break;
                            }

                            printf("Filosofo %d ha preso la forchetta sinistra %d\n", i, i);
                            printf("Filosofo %d aspetta la forchetta destra %d\n", i, i + 1);
                            if (checkStarvation == 1) {
                                s = sem_timedwait(sem[i + 1].sem, & timeout);
                            } else {
                                sem_wait(sem[i + 1].sem);
                            }
                            if ( * stopFlag == 1 || s == -1) {
                                if (s == -1) {
                                    printf("\nStarvation detected \n");
                                }
                                * stopFlag = 1;
                                break;
                            }
                            printf("Filosofo %d ha preso la forchetta destra %d\n", i, i + 1);
                        } else { //se è l'ultimo filosofo prendo prima la destra poi la sinistra
                            printf("Filosofo %d aspetta la forchetta destra 0 \n", i);
                            if (checkStarvation == 1) {
                                s = sem_timedwait(sem[0].sem, & timeout);
                            } else {
                                sem_wait(sem[0].sem);
                            }
                            if ( * stopFlag == 1 || s == -1) {
                                if (s == -1) {
                                    printf("\nStarvation detected \n");
                                }
                                * stopFlag = 1;
                                break;
                            }
                            printf("Filosofo %d ha preso la forchetta destra 0 \n", i);
                            //sleep(1);
                            printf("Filosofo %d aspetta la forchetta sinistra %d \n", i, i);
                            if (checkStarvation == 1) {
                                s = sem_timedwait(sem[i].sem, & timeout);
                            } else {
                                sem_wait(sem[i].sem);
                            }
                            if ( * stopFlag == 1 || s == -1) {
                                if (s == -1) {
                                    printf("\nStarvation detected \n");
                                }
                                * stopFlag = 1;
                                break;
                            }
                            printf("Filosofo %d ha preso la forchetta sinistra %d \n", i, i);
                        }

                        //filosofo mangia
                        printf("\n");
                        printf("Filosofo %d mangia\n", i);
                        printf("\n");
                        //sleep(1);
                        if ( * stopFlag == 1 || s == -1) {
                            if (s == -1) {
                                printf("\nStarvation detected \n");
                            }
                            * stopFlag = 1;
                            break;
                        }

                        //filosofo libera le forchette
                        if (i != conv - 1) {
                            printf("Filosofo %d libera la forchetta destra %d\n", i, i + 1);
                            sem_post(sem[i + 1].sem);
                            printf("Filosofo %d libera la forchetta sinistra %d\n", i, i);
                            sem_post(sem[i].sem);
                        } else {
                            printf("Filosofo %d libera la forchetta sinistra %d\n", i, i);
                            sem_post(sem[i].sem);
                            printf("Filosofo %d libera la forchetta destra 0\n", i);
                            sem_post(sem[0].sem);
                        }
                    }
                } else { //algoritmo che va in deadlock
                    while ( * stopFlag != 1) {
                        //filosofo prense la forchetta sinistra
                        if (i != conv - 1) {
                            printf("Filosofo %d aspetta la forchetta sinistra %d\n", i, i);
                            sem_wait(sem[i].sem);
                            * w[i][i] = 1; //aggiorno la matrice di deadlock
                            if ( * stopFlag == 1) { //il controllo lo effettuo dopo ogni wait così da poter uscire in caso vada in deadlock
                                break;
                            }
                            printf("Filosofo %d ha preso la forchetta sinistra %d\n", i, i);
                            //sleep(1);
                            printf("Filosofo %d aspetta la forchetta destra %d\n", i, i + 1);
                            sem_wait(sem[i + 1].sem);
                            * w[i][i + 1] = 1;
                            if ( * stopFlag == 1) {
                                break;
                            }
                            printf("Filosofo %d ha preso la forchetta destra %d\n", i, i + 1);
                        } else {
                            printf("Filosofo %d aspetta la forchetta sinistra %d \n", i, i);
                            sem_wait(sem[i].sem);
                            * w[i][i] = 1;
                            if ( * stopFlag == 1) {
                                break;
                            }
                            printf("Filosofo %d ha preso la forchetta sinistra %d \n", i, i);
                            //sleep(1);
                            printf("Filosofo %d aspetta la forchetta destra 0 \n", i);
                            sem_wait(sem[0].sem);
                            * w[i][0] = 1;
                            if ( * stopFlag == 1) {
                                break;
                            }
                            printf("Filosofo %d ha preso la forchetta destra 0 \n", i);
                        }

                        //filosofo mangia
                        printf("\n");
                        printf("Filosofo %d mangia\n", i);
                        printf("\n");
                        //sleep(1);

                        //filosofo libera le forchette
                        if (i != conv - 1) {
                            printf("Filosofo %d libera la forchetta sinistra %d\n", i, i);
                            sem_post(sem[i].sem);
                            * w[i][i] = 0;
                            //sleep(1);
                            printf("Filosofo %d libera la forchetta destra %d\n", i, i + 1);
                            sem_post(sem[i + 1].sem);
                            * w[i][i + 1] = 0;
                        } else {
                            printf("Filosofo %d libera la forchetta sinistra %d\n", i, i);
                            sem_post(sem[i].sem);
                            * w[i][i] = 0;
                            //sleep(1);
                            printf("Filosofo %d libera la forchetta destra 0\n", i);
                            sem_post(sem[0].sem);
                            * w[i][0] = 0;
                        }
                    }

                }
                fflush(stdout);
                return (3);
            } else { //processo per controlalre stallo
                if (checkdeadlock == 1 && checkStarvation == 0) { //viene attivato soltanto se è attivo solamente il checkDeadlock
                    while ( * stopFlag != 1) { // controlla la matrice finchè non trova un deadlock
                        int count = 0;
                        int stallo = 0;
                        for (int k = 0; k < conv; k++) {
                            for (int j = 0; j < conv; j++) {
                                if (k == j) {
                                    if ( * w[k][j] == 1) {
                                        count++;
                                    }
                                }
                            }
                        }
                        if (count == conv) { //se trova il deadlock lo scrive a video
                            printf("\nDeadlock detected, ctrl+c to exit \n");
                            * stopFlag = 1;
                        }
                        sleep(1);
                    }
                }
                fflush(stdout);
                return (4);
            }

        } else {
            // parent
        }
    }

    //termino tutto
    for (int l = 0; l <= conv; l++) {

        int wstatus;

        pids[l] = waitpid(pids[l], & wstatus, 0);

        if (l != conv) { //Ho un child in più rispetto al numero dei semafori, quindi evito di controllarli quando chiudo l'ultimo semaforo
            sem_post(sem[l].sem);

            if (sem_close(sem[l].sem) < 0)
                perror(sem[l].name);

            if (sem_unlink(sem[l].name) < 0)
                perror(sem[l].name);
        }

        if (WIFEXITED(wstatus))
            printf("Il child è terminato normalmente %d. Exit status del child = %d\n", pids[l], WEXITSTATUS(wstatus));
        else
            printf("Il child %d NON è terminato normalmente!!!\n", pids[l]);
        fflush(stdout);
    }
}