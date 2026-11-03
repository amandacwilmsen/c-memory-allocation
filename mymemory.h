#ifndef MYMEMORY_H
#define MYMEMORY_H

#include <stddef.h>

/*
 * Registro de um bloco atualmente alocado: start aponta para seu primeiro
 * byte, size informa o tamanho solicitado e next liga o proximo registro.
 * Os registros ficam fora do pool e formam uma lista em ordem de endereco.
 */
typedef struct allocation {
    void *start;
    size_t size;
    struct allocation *next;
} allocation_t;

typedef struct {
    void *pool; /* Bloco unico que armazena os dados das alocacoes. */
    size_t total_size; /* Capacidade do pool em bytes, definida na criacao. */
    allocation_t *head; /* Alocacoes ordenadas por endereco inicial. */
} mymemory_t;

/*
 * Cria um gerenciador com um pool de size bytes inicialmente livres.
 * Retorna NULL para tamanho zero ou falha ao reservar gerenciador ou pool.
 */
mymemory_t* mymemory_init(size_t size);

/*
 * Reserva size bytes na primeira lacuna adequada, em ordem de endereco
 * (First Fit), com alinhamento de max_align_t para os tipos fundamentais.
 * Retorna NULL se o gerenciador for invalido, size for zero, nao houver
 * espaco adequado ou a reserva do registro de controle falhar.
 */
void* mymemory_alloc(mymemory_t *memory, size_t size);

/*
 * Devolve ao pool o bloco identificado por ptr, permitindo reutiliza-lo.
 * Aceita apenas o endereco inicial de uma alocacao atual deste gerenciador;
 * NULL e outros enderecos sao ignorados. O conteudo nao e apagado.
 */
void mymemory_free(mymemory_t *memory, void *ptr);

/* Exibe o endereco inicial e o tamanho de cada bloco alocado. */
void mymemory_display(mymemory_t *memory);

/*
 * Imprime a quantidade de alocacoes, os bytes alocados e livres, o tamanho
 * da maior regiao livre e a quantidade de regioes livres (fragmentos).
 * Inclui sobras de alinhamento; o maior intervalo informado pode precisar
 * de ajuste de alinhamento para receber uma nova alocacao.
 */
void mymemory_stats(mymemory_t *memory);

/*
 * Libera todos os registros, o pool e o gerenciador, incluindo blocos que
 * o usuario ainda nao liberou. Depois, seus ponteiros nao podem ser usados.
 * Se memory for NULL, nao realiza nenhuma operacao.
 */
void mymemory_cleanup(mymemory_t *memory);

#endif /* MYMEMORY_H */
