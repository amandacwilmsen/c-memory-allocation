#include <stdio.h>
#include "mymemory.h"

int main(void)
{
    /* Demonstra alocacoes e liberacoes em um pool de 100 bytes. */
    mymemory_t *memory = mymemory_init(100);

    if (memory == NULL) {
        printf("Erro ao criar o pool.\n");
        return 1;
    }

    void *a = mymemory_alloc(memory, 20);

    void *b = mymemory_alloc(memory, 30);

    if (a == NULL || b == NULL) {
        printf("Erro ao alocar memoria.\n");
        mymemory_cleanup(memory);
        return 1;
    }

    mymemory_display(memory);
    mymemory_stats(memory);

    printf("\nLiberando A...\n");
    mymemory_free(memory, a);

    mymemory_display(memory);
    mymemory_stats(memory);

    /* C deve reutilizar o inicio da lacuna deixada por A (First Fit). */
    
printf("\nAlocando C com 10 bytes...\n");
void *c = mymemory_alloc(memory, 10);

if (c == NULL) {
    printf("Erro ao alocar C.\n");
    mymemory_cleanup(memory);
    return 1;
}

printf("C reutilizou o endereco de A? %s\n",
       c == a ? "SIM" : "NAO");


    mymemory_display(memory);
    mymemory_stats(memory);

    printf("\nTESTE DE ALOCACOES INVALIDAS\n");

    /* Pedidos de 101 e 0 bytes devem retornar NULL. */
    void *grande = mymemory_alloc(memory, 101);

    printf("Alocacao de 101 bytes falhou? %s\n",
           grande == NULL ? "SIM" : "NAO");

    void *zero = mymemory_alloc(memory, 0);

    printf("Alocacao de 0 bytes falhou? %s\n",
           zero == NULL ? "SIM" : "NAO");

    mymemory_free(memory, b);
    mymemory_free(memory, c);

    /* Sem alocacoes, os 100 bytes formam uma unica regiao livre. */
    printf("\nEstatisticas apos liberar tudo:\n");
    mymemory_stats(memory);

    mymemory_cleanup(memory);

    return 0;
}
