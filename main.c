#include <stdio.h>
#include "mymemory.h"

int main(void)
{
    /*
     * Demonstra o uso do gerenciador com um pool de 100 bytes, isto e,
     * uma unica regiao de memoria onde os blocos A, B e C serao alocados.
     * As impressoes permitem acompanhar as alocacoes e os espacos livres;
     * os testes automaticos, incluindo os de 50 blocos, ficam em testes.c.
     */
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

    /*
     * A liberacao de A deixou seus 20 bytes disponiveis no inicio do pool.
     * Como o First Fit percorre os espacos livres em ordem de endereco e
     * escolhe o primeiro que comporta o pedido, C deve reutilizar o inicio
     * dessa regiao para armazenar seus 10 bytes, sem deslocar B.
     */
    
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

    /*
     * Verifica duas situacoes em que o gerenciador deve recusar o pedido:
     * 101 bytes ultrapassam a capacidade total do pool, que e de 100 bytes;
     * ja o pedido de 0 bytes e rejeitado por esta biblioteca por nao reservar
     * uma regiao de tamanho positivo. Nos dois casos, o retorno esperado e
     * NULL, indicando que nenhum novo bloco foi alocado.
     */
    void *grande = mymemory_alloc(memory, 101);

    printf("Alocacao de 101 bytes falhou? %s\n",
           grande == NULL ? "SIM" : "NAO");

    void *zero = mymemory_alloc(memory, 0);

    printf("Alocacao de 0 bytes falhou? %s\n",
           zero == NULL ? "SIM" : "NAO");

    mymemory_free(memory, b);
    mymemory_free(memory, c);

    /*
     * A ja foi liberado, e as chamadas acima liberam B e C. Assim, nao deve
     * restar nenhuma alocacao: os 100 bytes do pool ficam disponiveis como
     * uma unica regiao continua. As estatisticas devem mostrar 0 alocacoes,
     * 0 bytes alocados, 100 bytes livres, maior bloco livre de 100 bytes e
     * 1 fragmento livre.
     */
    printf("\nEstatisticas apos liberar tudo:\n");
    mymemory_stats(memory);

    mymemory_cleanup(memory);

    return 0;
}
