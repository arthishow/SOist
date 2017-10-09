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
    DoubleMatrix2D *m;
} args_fatia_t;

void *myThread(void *a) {

    args_fatia_t *args   = (args_fatia_t*)a;
    DoubleMatrix2D *fatia = dm2dNew(args->end - args->start + 2, args->colunas);
	int i;
    if (fatia == NULL) {
        fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para a fatia.\n\n");
    }

    double value;
    for(int iter = 0; iter < args->iter; iter++) {
        for(int i = args->start; i < args->end + 1; i++) {
            for(int j = 1; j < args->colunas-1; j++) {
                value = ( dm2dGetEntry(args->m, i-1, j) + dm2dGetEntry(args->m, i+1, j) +
                          dm2dGetEntry(args->m, i, j-1) + dm2dGetEntry(args->m, i, j+1) ) / 4.0;
                dm2dSetEntry(fatia, i-args->start, j, value);
            }
        }
        if(args->id > 0) {
            enviarMensagem(args->id, args->id-1, dm2dGetLine(fatia, 1), args->colunas*sizeof(double));
        }
        if(args->id < args->linhas-1) {
            enviarMensagem(args->id, args->id+1, dm2dGetLine(fatia, args->end-args->start), args->colunas*sizeof(double));
        }
        if(args->id > 0) {
            receberMensagem(args->id-1, args->id, dm2dGetLine(fatia, 0), args->colunas*sizeof(double));
        }
        if(args->id < args->linhas-1) {
            receberMensagem(args->id+1, args->id, dm2dGetLine(fatia, args->end-args->start+1), args->colunas*sizeof(double));
        }
    }

    for(int l = 0; l < args->end-args->start; l++) {
        dm2dSetLine(args->m, args->start + l, dm2dGetLine(fatia, l+1));
    }
    return 0;
}

/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/

DoubleMatrix2D *simul(DoubleMatrix2D *m, int linhas, int colunas, int numIteracoes, int numTrabs, int numMsgs) {

    int i, t;
    args_fatia_t *slave_args;
    pthread_t *slaves;

    if(linhas < 2 || colunas < 2) {
        return NULL;
    }

    int k = linhas/numTrabs;

    slave_args = (args_fatia_t*)malloc(numTrabs*sizeof(args_fatia_t));
    slaves = (pthread_t*)malloc(numTrabs*sizeof(pthread_t));

    for (t=0; t<numTrabs; t++) {
        slave_args[t].id = t;
        slave_args[t].iter = numIteracoes;
        slave_args[t].start = k*t+1;
        slave_args[t].end = k*(t+1);
        slave_args[t].colunas = colunas;
        slave_args[t].m = m;
        pthread_create(&slaves[t], NULL, myThread, &slave_args[t]);
    }

    for (i=0; i<numTrabs; i++) {
        pthread_join(slaves[i], NULL);
    }

    return m;
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

    if(inicializarMPlib(csz, trab)) {
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
