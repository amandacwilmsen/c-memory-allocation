#include <stdlib.h>
#include <stdio.h>
#include "mymemory.h"

/* Cria um pool de size bytes. Retorna NULL para tamanho zero ou falha de alocacao. */
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

/*
 * Verifica se size bytes cabem em [inicio, fim), com alinhamento de max_align_t.
 * Os limites sao deslocamentos no pool; grava o inicio alinhado em *posicao.
 */
static int cabe_na_lacuna(size_t inicio, size_t fim, size_t size,
    size_t *posicao)
{
    if (inicio > fim) {
        return 0;
    }

    /*
     * _Alignof(tipo) informa o alinhamento exigido pelo tipo, em bytes:
     * seu endereco deve ser multiplo desse valor (nao e o tamanho do tipo).
     * max_align_t tem alinhamento suficiente para todos os tipos basicos de C.
     * Usamos esse valor para ajustar o inicio de cada bloco no pool.
     */
    size_t alinhamento = _Alignof(max_align_t);
    size_t resto = inicio % alinhamento;
    size_t ajuste = (resto == 0) ? 0 : alinhamento - resto;
    size_t disponivel = fim - inicio;

    /* Valida o ajuste antes de subtrair para evitar underflow de size_t. */
    if (ajuste > disponivel || size > disponivel - ajuste) {
        return 0;
    }

    *posicao = inicio + ajuste;
    return 1;
}

/*
 * Aloca por First Fit e mantem a lista ordenada por endereco.
 * Retorna NULL para pedido invalido, falta de espaco ou falha de alocacao.
 */
void* mymemory_alloc(mymemory_t *memory, size_t size)
{
    if (memory == NULL || memory->pool == NULL || size == 0) {
        return NULL;
    }

    allocation_t *anterior = NULL;
    allocation_t *atual = memory->head;
    size_t fim_anterior = 0;
    size_t posicao = 0;
    int encontrou = 0;

    /* Busca a primeira lacuna adequada antes de cada bloco ocupado. */
    while (atual != NULL) {
        size_t inicio_atual = (size_t)((char *)atual->start
                                     - (char *)memory->pool);

        if (cabe_na_lacuna(fim_anterior, inicio_atual, size, &posicao)) {
            encontrou = 1;
            break;
        }

        fim_anterior = inicio_atual + atual->size;
        anterior = atual;
        atual = atual->next;
    }

    /* Verifica a lacuna final; com a lista vazia, considera todo o pool. */
    if (!encontrou &&
        cabe_na_lacuna(fim_anterior, memory->total_size, size, &posicao)) {
        encontrou = 1;
    }

    if (!encontrou) {
        return NULL;
    }

    /* O registro de controle e alocado fora do pool. */
    allocation_t *novo = malloc(sizeof(allocation_t));
    if (novo == NULL) {
        return NULL;
    }

    novo->start = (char *)memory->pool + posicao;
    novo->size = size;
    novo->next = atual;

    /* Insere o registro mantendo a ordem por endereco. */
    if (anterior == NULL) {
        memory->head = novo;
    } else {
        anterior->next = novo;
    }

    return novo->start;
}

/* Exibe o endereco e o tamanho de cada alocacao. */
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

/*
 * Calcula alocacoes e regioes livres, incluindo sobras de alinhamento.
 * O maior intervalo livre pode exigir alinhamento para uma nova alocacao.
 */
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

        /* Conta a lacuna entre blocos, incluindo sobras de alinhamento. */
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

    /* Inclui o espaco final; com a lista vazia, todo o pool e um fragmento. */
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

/*
 * Libera os registros, o pool e o gerenciador. Aceita NULL.
 * Os ponteiros para o gerenciador e seus blocos ficam invalidos.
 */
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

    /* Libera todos os dados de uma vez. */
    free(memory->pool);
    free(memory);
}

/*
 * Libera o bloco iniciado em ptr sem apagar seus dados.
 * Ignora NULL e enderecos que nao iniciam uma alocacao atual.
 */
void mymemory_free(mymemory_t *memory, void *ptr)
{
    if (memory == NULL || ptr == NULL) {
        return;
    }

    allocation_t *atual = memory->head;
    allocation_t *anterior = NULL;

    while (atual != NULL) {
        /* Apenas o endereco inicial identifica a alocacao. */
        if (atual->start == ptr) {
            if (anterior == NULL) {
                memory->head = atual->next;
            } else {
                anterior->next = atual->next;
            }

            /*
             * Libera o registro; os dados pertencem ao pool.
             * Lacunas contiguas sao reconhecidas como uma unica regiao.
             */
            free(atual);
            return;
        }

        anterior = atual;
        atual = atual->next;
    }
}
