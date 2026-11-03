#include <stdlib.h>
#include <stdio.h>
#include "mymemory.h"

mymemory_t* mymemory_init(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    mymemory_t *memory = malloc(sizeof(mymemory_t));

    if (memory == NULL) {
        return NULL;
    }

    memory->pool = malloc(size);

    if (memory->pool == NULL) {
        free(memory);
        return NULL;
    }

    memory->total_size = size;
    memory->head = NULL;

    return memory;
}

void mymemory_display(mymemory_t *memory)
{
    if (memory == NULL) {
        return;
    }

    allocation_t *atual = memory->head;

    if (atual == NULL) {
        printf("Nenhuma alocacao\n");
        return;
    }

    while (atual != NULL) {
        printf("Inicio: %p | Tamanho: %zu\n", atual->start, atual->size);
        atual = atual->next;
    }
}

void mymemory_stats(mymemory_t *memory)
{
    if (memory == NULL) {
        return;
    }

    size_t qtd = 0;
    size_t alocado = 0;
    size_t maior_livre = 0;
    size_t fragmentos = 0;
    size_t fim = 0;

    allocation_t *atual = memory->head;

    while (atual != NULL) {
        size_t inicio = (char *)atual->start - (char *)memory->pool;

        if (inicio > fim) {
            fragmentos++;
            if (inicio - fim > maior_livre) {
                maior_livre = inicio - fim;
            }
        }

        qtd++;
        alocado += atual->size;
        fim = inicio + atual->size;
        atual = atual->next;
    }

    if (memory->total_size > fim) {
        fragmentos++;
        if (memory->total_size - fim > maior_livre) {
            maior_livre = memory->total_size - fim;
        }
    }

    printf("Alocacoes: %zu\n", qtd);
    printf("Memoria alocada: %zu\n", alocado);
    printf("Memoria livre: %zu\n", memory->total_size - alocado);
    printf("Maior bloco livre: %zu\n", maior_livre);
    printf("Fragmentos: %zu\n", fragmentos);
}

void mymemory_cleanup(mymemory_t *memory)
{
    if (memory == NULL) {
        return;
    }

    allocation_t *atual = memory->head;

    while (atual != NULL) {
        allocation_t *prox = atual->next;
        free(atual);
        atual = prox;
    }

    free(memory->pool);
    free(memory);
}