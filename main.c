/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "mplib3.h"
#include "matrix2d.h"

#define ARGS_NUM 9

typedef struct {
    int id;
    int iter;
    int start;
    int end;
    int linhas;
    int colunas;
} args_fatia_t;

void *myThread(void *a) {

    args_fatia_t *args   = (args_fatia_t*)a;
    int id = args->id;
    int linhas = args->linhas;
    int colunas = args->colunas;
    int start = args->start;
    int end = args->end;
    DoubleMatrix2D *fatia = dm2dNew(end - start + 2, colunas);
    DoubleMatrix2D *fatia_aux = dm2dNew(end - start + 2, colunas);
    DoubleMatrix2D *tmp;

    if (fatia == NULL) {
        fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para a fatia.\n\n");
    }

    for(int k = 0; k < end-start+2; k++) {
        receberMensagem(0, id, dm2dGetLine(fatia, k), colunas*sizeof(double));
    }

    double value;
    for(int iter = 0; iter < args->iter; iter++) {
        for(int i = start; i < end + 1; i++) {
            for(int j = 1; j < colunas-1; j++) {
                value = ( dm2dGetEntry(fatia, i-1, j) + dm2dGetEntry(fatia, i+1, j) +
                          dm2dGetEntry(fatia, i, j-1) + dm2dGetEntry(fatia, i, j+1) ) / 4.0;
                dm2dSetEntry(fatia_aux, i-start, j, value);
            }
        }
        if(id > 1) {
            enviarMensagem(id, id-1, dm2dGetLine(fatia_aux, 1), colunas*sizeof(double));
        }
        if(id < linhas) {
            enviarMensagem(id, id+1, dm2dGetLine(fatia_aux, end-start), colunas*sizeof(double));
        }
        if(id > 1) {
            receberMensagem(id-1, id, dm2dGetLine(fatia_aux, 0), colunas*sizeof(double));
        }
        if(id < linhas) {
            receberMensagem(id+1, id, dm2dGetLine(fatia_aux, end-start+1), colunas*sizeof(double));
        }

        tmp = fatia_aux;
        fatia_aux = fatia;
        fatia = tmp;
    }

    for(int l = 0; l < end-start; l++) {
        enviarMensagem(id, 0, dm2dGetLine(fatia, l+1), colunas*sizeof(double));
    }

    return 0;
}

/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/

DoubleMatrix2D *simul(DoubleMatrix2D *matrix, int linhas, int colunas, int numIteracoes, int numTrabs, int numMsgs) {

    args_fatia_t *slave_args;
    pthread_t *slaves;

    if(linhas < 2 || colunas < 2) {
        return NULL;
    }

    int k = linhas/numTrabs;

    slave_args = (args_fatia_t*)malloc(numTrabs*sizeof(args_fatia_t));
    slaves = (pthread_t*)malloc(numTrabs*sizeof(pthread_t));

    for (int t=0; t<numTrabs; t++) {
        slave_args[t].id = t+1;
        slave_args[t].iter = numIteracoes;
        slave_args[t].start = k*t+1;
        slave_args[t].end = k*(t+1);
        slave_args[t].linhas = linhas;
        slave_args[t].colunas = colunas;
        pthread_create(&slaves[t], NULL, myThread, &slave_args[t]);
        for(int j = 0; j < k+2; j++) {
            enviarMensagem(0, t+1, dm2dGetLine(matrix, k*t+j), colunas*sizeof(double));
        }
    }

    for(int l = 0; l < numTrabs; l++) {
        for(int m = 0; m < k; m++) {
            receberMensagem(l+1, 0, dm2dGetLine(matrix, k*l+m), colunas*sizeof(double));
        }
    }

    for (int i=0; i<numTrabs; i++) {
        pthread_join(slaves[i], NULL);
    }

    return matrix;
}

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name)
{
    int value;

    if(sscanf(str, "%d", &value) != 1) {
        fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
        exit(1);
    }
    return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name)
{
    double value;

    if(sscanf(str, "%lf", &value) != 1) {
        fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
        exit(1);
    }
    return value;
}


/*--------------------------------------------------------------------
| Function: main
---------------------------------------------------------------------*/

int main (int argc, char** argv) {

    if(argc != ARGS_NUM) {
        fprintf(stderr, "\nNumero invalido de argumentos.\n");
        fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes trab csz\n\n");
        return 1;
    }

    /* argv[0] = program name */
    int N = parse_integer_or_exit(argv[1], "N");
    double tEsq = parse_double_or_exit(argv[2], "tEsq");
    double tSup = parse_double_or_exit(argv[3], "tSup");
    double tDir = parse_double_or_exit(argv[4], "tDir");
    double tInf = parse_double_or_exit(argv[5], "tInf");
    int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");
    int trab = parse_integer_or_exit(argv[7], "trab");
    int csz = parse_integer_or_exit(argv[8], "csz");

    DoubleMatrix2D *matrix, *result;

    fprintf(stderr, "\nArgumentos:\n N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d trab=%d csz=%d\n",
            N, tEsq, tSup, tDir, tInf, iteracoes, trab, csz);

    if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || trab < 1 || csz < 0 || N%trab != 0) {
        fprintf(stderr, "\nErro: Argumentos invalidos.\n Lembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, número de trabalhadores >= 1, N mod trab = 0 e capacidade de mensagem >= 0\n\n");
        return 1;
    }

    if(inicializarMPlib(csz, trab+1)) {
        fprintf(stderr, "\nErro ao inicializar MPlib\n");
        return -1;
    }

    matrix = dm2dNew(N+2, N+2);

    if (matrix == NULL) {
        fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para a matrize.\n\n");
        libertarMPlib();
        return -1;
    }

    dm2dSetLineTo(matrix, 0, tSup);
    dm2dSetLineTo(matrix, N+1, tInf);
    dm2dSetColumnTo(matrix, 0, tEsq);
    dm2dSetColumnTo(matrix, N+1, tDir);

    result = simul(matrix, N+2, N+2, iteracoes, trab, csz);

    if (result == NULL) {
        printf("\nErro na simulacao.\n\n");
        libertarMPlib();
        return -1;
    }

    dm2dPrint(result);

    libertarMPlib();
    dm2dFree(matrix);

    return 0;
}
