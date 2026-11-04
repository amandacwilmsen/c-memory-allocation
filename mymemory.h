#ifndef MYMEMORY_H
#define MYMEMORY_H

#include <stddef.h>

typedef struct allocation { /* Registro de um bloco alocado. */
    void *start;
    size_t size;
    struct allocation *next;
} allocation_t;

typedef struct { /* Gerenciador do pool e das alocacoes. */
    void *pool; /* Bloco unico de dados. */
    size_t total_size; /* Capacidade do pool em bytes. */
    allocation_t *head; /* Alocacoes ordenadas por endereco. */
} mymemory_t;

/* Cria um pool de size bytes. Retorna NULL para tamanho zero ou falha de alocacao. */
mymemory_t* mymemory_init(size_t size);

/*
 * Aloca size bytes por First Fit, com alinhamento de max_align_t.
 * Retorna NULL para pedido invalido, falta de espaco ou falha de alocacao.
 */
void* mymemory_alloc(mymemory_t *memory, size_t size);

/*
 * Libera o bloco iniciado em ptr sem apagar seus dados.
 * Ignora NULL e enderecos que nao iniciam uma alocacao atual.
 */
void mymemory_free(mymemory_t *memory, void *ptr);

/* Exibe o endereco e o tamanho de cada alocacao. */
void mymemory_display(mymemory_t *memory);

/*
 * Exibe alocacoes, bytes alocados e livres, maior regiao livre e fragmentos.
 * Inclui sobras de alinhamento; o maior intervalo pode exigir novo alinhamento.
 */
void mymemory_stats(mymemory_t *memory);

/* Libera registros, pool e gerenciador, invalidando seus ponteiros. Aceita NULL. */
void mymemory_cleanup(mymemory_t *memory);

#endif /* MYMEMORY_H */
