/*
// Projeto SO - exercicio 1
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#include "matrix2d.h"

DoubleMatrix2D *matrix, *matrix_aux;
int counter = 0;
bool turnstile1 = false;
bool turnstile2 = false;
pthread_mutex_t counterMutex;
pthread_cond_t counterCond;
pthread_cond_t maxdCond;
double threadsMaxD = 0;
bool end = false;
int enditer;

/*--------------------------------------------------------------------
| Type: thread_info
| Description: Estrutura com Informação para Trabalhadoras
---------------------------------------------------------------------*/

typedef struct {
    int id;
    int N;
    int iter;
    int trab;
    int tam_fatia;
    double maxD;
} thread_info;


/*--------------------------------------------------------------------
| Function: tarefa_trabalhadora
| Description: Função executada por cada tarefa trabalhadora.
|              Recebe como argumento uma estrutura do tipo thread_info.
---------------------------------------------------------------------*/

void *tarefa_trabalhadora(void* args) {
    thread_info *tinfo = (thread_info *) args;

    DoubleMatrix2D *m = matrix;
    DoubleMatrix2D *m_aux = matrix_aux;
    DoubleMatrix2D *m_tmp;
    int N = tinfo->N;
    int iter = tinfo->iter;
    int ntrab = tinfo->trab;
    int tam_fatia = tinfo->tam_fatia;
    int id = tinfo->id;
    double value;
    double maxD = tinfo->maxD;

    int k;
    for(k = 0; k < iter; k++) {

        for (int i = (id-1)*tam_fatia; i < id*tam_fatia; i++) {
            for (int j = 0; j < N; j++) {
                value = ( dm2dGetEntry(m, i, j+1) + dm2dGetEntry(m, i+2, j+1) +
                          dm2dGetEntry(m, i+1, j) + dm2dGetEntry(m, i+1, j+2) ) / 4.0;
                dm2dSetEntry(m_aux, i+1, j+1, value);
                
                double dif = value - dm2dGetEntry(m, i+1, j+1);
                
                pthread_mutex_lock(&counterMutex);
                if (dif > threadsMaxD) threadsMaxD=dif;
                pthread_mutex_unlock(&counterMutex);

            }
        }

        m_tmp = m_aux;
        m_aux = m;
        m = m_tmp;


        pthread_mutex_lock(&counterMutex);
        counter++;
        if(counter == ntrab) {
            turnstile2 = false;
            turnstile1 = true;
            pthread_cond_broadcast(&counterCond);
        }
        else {
            while(!turnstile1) pthread_cond_wait(&counterCond, &counterMutex);
        }

        counter--;
        if(counter == 0) {
            turnstile1 = false;
            turnstile2 = true;

            threadsMaxD<maxD ? (end = true) : (threadsMaxD = 0);

            pthread_cond_broadcast(&counterCond);
        }
        else {
            while(!turnstile2) pthread_cond_wait(&counterCond, &counterMutex);
        }
        pthread_mutex_unlock(&counterMutex);

        if(end) {
            enditer=k; 
            pthread_exit(NULL);
        }

    }

    enditer=k;
    pthread_exit(NULL);
}

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
| Description: Processa a string str, do parâmetro name, como um inteiro
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name) {
    int value;

    if(sscanf(str, "%d", &value) != 1) {
        fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
        exit(-1);
    }

    return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
| Description: Processa a string str, do parâmetro name, como um double
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name) {
    double value;

    if(sscanf(str, "%lf", &value) != 1) {
        fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
        exit(-1);
    }

    return value;
}

static void freeGlobal() {
    dm2dFree(matrix);
    dm2dFree(matrix_aux);

    if(pthread_mutex_destroy(&counterMutex) != 0) {
        fprintf(stderr, "\nErro ao destruir mutex\n");
        exit(1);
    }

    if(pthread_cond_destroy(&counterCond) != 0) {
        fprintf(stderr, "\nErro ao destruir variável de condição\n");
        exit(1);
    }
}

/*--------------------------------------------------------------------
| Function: main
| Description: Entrada do programa
---------------------------------------------------------------------*/

int main (int argc, char** argv) {
    int N;
    double tEsq, tSup, tDir, tInf;
    int iter;
    int trab;
    double maxD;
    int tam_fatia;
    int res;
    int i;
    thread_info *tinfo;
    pthread_t *trabalhadoras;

    if(argc != 9) {
        fprintf(stderr, "\nNúmero de Argumentos Inválido.\n");
        fprintf(stderr, "Utilização: heatSim_p1 N tEsq tSup tDir tInf iter trab maxD\n\n");
        return -1;
    }

    /* Ler Input */
    N = parse_integer_or_exit(argv[1], "n");
    tEsq = parse_double_or_exit(argv[2], "tEsq");
    tSup = parse_double_or_exit(argv[3], "tSup");
    tDir = parse_double_or_exit(argv[4], "tDir");
    tInf = parse_double_or_exit(argv[5], "tInf");
    iter = parse_integer_or_exit(argv[6], "iter");
    trab = parse_integer_or_exit(argv[7], "trab");
    maxD =  parse_double_or_exit(argv[8], "maxD");

    fprintf(stderr, "\nArgumentos:\n"
            " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iter=%d trab=%d maxD=%f",
            N, tEsq, tSup, tDir, tInf, iter, trab, maxD);


    /* Verificações de Input */
    if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 || trab < 1 || maxD < 0) {
        fprintf(stderr, "\nErro: Argumentos invalidos.\n"
                " Lembrar que N >= 1, temperaturas >= 0, iter >= 1, trab >=1 e maxD >= 0\n\n");
        return -1;
    }

    if (N % trab != 0) {
        fprintf(stderr, "\nErro: Argumento %s e %s invalidos\n"
                "%s deve ser multiplo de %s.", "N", "trab", "N", "trab");
        return -1;
    }

    /* Calcular Tamanho de cada Fatia */
    tam_fatia = N/trab;

    /* Criar Matriz Inicial */
    matrix = dm2dNew(N+2, N+2);
    matrix_aux = dm2dNew(N+2, N+2);

    if (matrix == NULL || matrix_aux == NULL) {
        fprintf(stderr, "\nErro ao criar Matrix2d.\n");
        return -1;
    }

    if(pthread_mutex_init(&counterMutex, NULL) != 0) {
        fprintf(stderr, "\nErro ao inicializar mutex\n");
        exit(1);
    }

    if(pthread_cond_init(&counterCond, NULL) != 0) {
        fprintf(stderr, "\nErro ao inicializar variável de condição\n");
        exit(1);
    }

    dm2dSetLineTo(matrix, 0, tSup);
    dm2dSetLineTo(matrix, N+1, tInf);
    dm2dSetColumnTo(matrix, 0, tEsq);
    dm2dSetColumnTo(matrix, N+1, tDir);

    dm2dCopy(matrix_aux, matrix);

    /* Reservar Memória para Trabalhadoras */
    tinfo = (thread_info *)malloc(trab * sizeof(thread_info));
    trabalhadoras = (pthread_t *)malloc(trab * sizeof(pthread_t));

    if (tinfo == NULL || trabalhadoras == NULL) {
        fprintf(stderr, "\nErro ao alocar memória para trabalhadoras.\n");
        freeGlobal();
        return -1;
    }

    /* Criar Trabalhadoras */
    for (i = 0; i < trab; i++) {
        tinfo[i].id = i+1;
        tinfo[i].N = N;
        tinfo[i].iter = iter;
        tinfo[i].trab = trab;
        tinfo[i].tam_fatia = tam_fatia;
        tinfo[i].maxD = maxD;
        res = pthread_create(&trabalhadoras[i], NULL, tarefa_trabalhadora, &tinfo[i]);

        if(res != 0) {
            fprintf(stderr, "\nErro ao criar uma tarefa trabalhadora.\n");
            freeGlobal();
            free(tinfo);
            free(trabalhadoras);
            return -1;
        }
    }

    /* Esperar que as Trabalhadoras Terminem */
    for (i = 0; i < trab; i++) {
        res = pthread_join(trabalhadoras[i], NULL);

        if (res != 0) {
            fprintf(stderr, "\nErro ao esperar por uma tarefa trabalhadora.\n");
            freeGlobal();
            free(tinfo);
            free(trabalhadoras);
            return -1;
        }
    }

    (enditer)%2==0 ? dm2dPrint(matrix_aux) : dm2dPrint(matrix);

    /* Libertar Memória */
    freeGlobal();
    free(tinfo);
    free(trabalhadoras);

    return 0;
}
