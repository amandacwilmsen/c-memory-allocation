#include <stdlib.h>
#include <stdio.h>
#include "mymemory.h"

/*
 * Reserva a estrutura de controle e um pool de size bytes para os dados.
 * A lista comeca vazia, pois nenhuma parte do pool foi entregue ao usuario.
 * Retorna NULL se size for zero ou se alguma reserva de memoria falhar.
 */
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
 * Verifica se um pedido de size bytes cabe em uma regiao livre do pool.
 * O intervalo [inicio, fim) inclui inicio e exclui fim; ambos sao distancias
 * em bytes em relacao ao inicio do pool, e nao enderecos absolutos.
 * O inicio escolhido deve respeitar max_align_t, o alinhamento exigido pelos
 * tipos fundamentais de C. Como malloc ja alinha a base do pool, basta
 * ajustar o deslocamento, descontando do espaco livre os bytes desse ajuste.
 * Retorna 1 se o pedido couber e grava esse deslocamento em *posicao;
 * caso contrario, retorna 0.
 */
static int cabe_na_lacuna(size_t inicio, size_t fim, size_t size,
    size_t *posicao)
{
    if (inicio > fim) {
        return 0;
    }

    size_t alinhamento = _Alignof(max_align_t);
    size_t resto = inicio % alinhamento;
    size_t ajuste = (resto == 0) ? 0 : alinhamento - resto;
    size_t disponivel = fim - inicio;

    /*
     * size_t nao representa valores negativos. Primeiro confirmamos que o
     * ajuste cabe na lacuna; so entao o subtraimos para verificar o pedido,
     * evitando que a subtracao produza um valor positivo muito grande.
     */
    if (ajuste > disponivel || size > disponivel - ajuste) {
        return 0;
    }

    *posicao = inicio + ajuste;
    return 1;
}

/*
 * Procura espaco pelo First Fit: percorre o pool em ordem de endereco e
 * usa a primeira regiao livre que comporte size bytes apos o alinhamento.
 * A lista registra apenas blocos ocupados; as lacunas sao deduzidas dos
 * intervalos entre eles. O novo bloco e inserido mantendo essa ordem.
 * Retorna seu endereco ou NULL para pedido invalido, falta de espaco
 * adequado ou falha ao reservar a estrutura de controle do novo bloco.
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

    /*
     * Para cada bloco ocupado, examina o intervalo entre o fim do bloco
     * anterior e o inicio do atual. Na primeira iteracao, fim_anterior e
     * zero, permitindo aproveitar tambem o espaco no inicio do pool.
     */
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

    /*
     * Se nenhuma lacuna anterior serviu, verifica o espaco entre o ultimo
     * bloco e o fim do pool. Quando a lista esta vazia, fim_anterior vale
     * zero e essa verificacao considera toda a capacidade do pool.
     */
    if (!encontrou &&
        cabe_na_lacuna(fim_anterior, memory->total_size, size, &posicao)) {
        encontrou = 1;
    }

    if (!encontrou) {
        return NULL; // Nenhuma lacuna comporta o tamanho solicitado.
    }

    /*
     * Reserva separadamente um nodo para registrar endereco e tamanho.
     * Esse controle nao consome bytes do pool: apenas os dados solicitados
     * pelo usuario sao armazenados na regiao escolhida dentro dele.
     */
    allocation_t *novo = malloc(sizeof(allocation_t));
    if (novo == NULL) {
        return NULL;
    }

    novo->start = (char *)memory->pool + posicao;
    novo->size = size;
    novo->next = atual;

    /*
     * Insere o nodo entre anterior e atual, ou no inicio se anterior for
     * NULL. A ordem por endereco permite localizar lacunas nas proximas
     * alocacoes e calcular os espacos livres nas estatisticas.
     */
    if (anterior == NULL) {
        memory->head = novo;
    } else {
        anterior->next = novo;
    }

    return novo->start;
}

/* Exibe o endereco inicial e o tamanho de cada bloco alocado. */
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
 * Percorre a lista para somar os bytes solicitados e contar as alocacoes.
 * Cada intervalo livre e contado como um fragmento, inclusive as sobras
 * de alinhamento. O maior bloco livre e o maior desses intervalos fisicos;
 * um novo pedido ainda pode precisar de ajuste de alinhamento para usa-lo.
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

        /*
         * Se o bloco atual comeca depois do fim do anterior, ha uma regiao
         * livre entre eles. Ela entra na contagem mesmo quando surgiu
         * apenas do ajuste de alinhamento, e nao de uma liberacao.
         */
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

    /*
     * Conta tambem o espaco apos o ultimo bloco, que o laco nao examina.
     * Sem alocacoes, fim continua zero: todo o pool forma um fragmento.
     */
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
 * Encerra o uso do gerenciador, mesmo que ainda existam blocos alocados.
 * Libera os nodos de controle e depois o pool e a estrutura do gerenciador.
 * A partir dessa chamada, nem o gerenciador nem os ponteiros para seus
 * blocos podem ser usados. Receber NULL nao realiza nenhuma operacao.
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

    /* Os blocos de dados pertencem ao mesmo pool e sao liberados de uma vez. */
    free(memory->pool);
    free(memory);
}

/*
 * Busca uma alocacao cujo endereco inicial seja exatamente ptr e remove
 * seu nodo da lista. A regiao passa a ser considerada livre nas buscas
 * seguintes, mas seus bytes nao sao apagados e o pool permanece reservado.
 * NULL e enderecos que nao correspondem ao inicio de uma alocacao atual
 * sao ignorados, sem modificar a lista.
 */
void mymemory_free(mymemory_t *memory, void *ptr)
{
    if (memory == NULL || ptr == NULL) {
        return;
    }

    allocation_t *atual = memory->head;
    allocation_t *anterior = NULL;

    while (atual != NULL) {
        /* Ponteiros para o interior de um bloco nao identificam alocacoes. */
        if (atual->start == ptr) {
            if (anterior == NULL) {
                memory->head = atual->next;
            } else {
                anterior->next = atual->next;
            }

            /*
             * Libera somente o registro de controle, nunca ptr diretamente,
             * pois os dados pertencem ao pool unico. Como nao ha uma lista
             * de blocos livres, lacunas vizinhas passam a ser reconhecidas
             * como uma unica regiao nas proximas buscas e estatisticas.
             */
            free(atual);
            return;
        }

        anterior = atual;
        atual = atual->next;
    }
}
