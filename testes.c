/*
 * Testes do gerenciador First Fit. Compile sem main.c:
 * clang -std=c11 -Wall -Wextra -Wpedantic -g testes.c mymemory.c -o testes
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mymemory.h"

/* Encerra o teste se a condicao falhar. */
static void verificar(int condicao, const char *mensagem)
{
    if (!condicao) {
        fprintf(stderr, "FALHOU: %s\n", mensagem);
        exit(EXIT_FAILURE);
    }
}

/* Indicadores de mymemory_stats. */
typedef struct {
    size_t alocacoes;
    size_t alocada;
    size_t livre;
    size_t maior;
    size_t fragmentos;
} estatisticas_t;

/* Captura mymemory_stats em arquivo temporario e restaura stdout. */
static void capturar_estatisticas(mymemory_t *memory, char *texto,
    size_t capacidade)
{
    verificar(capacidade > 1, "buffer para capturar estatisticas");
    FILE *captura = tmpfile();
    verificar(captura != NULL, "criar arquivo temporario para estatisticas");
    verificar(fflush(stdout) == 0, "esvaziar stdout antes da captura");
    int saida_original = dup(STDOUT_FILENO);
    verificar(saida_original >= 0, "salvar stdout");
    verificar(dup2(fileno(captura), STDOUT_FILENO) >= 0,
              "redirecionar stdout");

    mymemory_stats(memory);

    verificar(fflush(stdout) == 0, "esvaziar estatisticas capturadas");
    verificar(dup2(saida_original, STDOUT_FILENO) >= 0, "restaurar stdout");
    verificar(close(saida_original) == 0, "fechar copia de stdout");
    verificar(fseek(captura, 0, SEEK_SET) == 0, "reler estatisticas");
    size_t lidos = fread(texto, 1, capacidade - 1, captura);
    texto[lidos] = '\0';
    verificar(!ferror(captura) && feof(captura), "ler toda a saida");
    verificar(fclose(captura) == 0, "fechar captura");
}

/* Converte a saida de mymemory_stats em valores numericos. */
static estatisticas_t ler_estatisticas(mymemory_t *memory)
{
    char texto[256];
    estatisticas_t e = {0};
    int fim = 0;
    capturar_estatisticas(memory, texto, sizeof(texto));
    int campos = sscanf(texto,
        "Alocacoes: %zu\nMemoria alocada: %zu\nMemoria livre: %zu\n"
        "Maior bloco livre: %zu\nFragmentos: %zu\n%n",
        &e.alocacoes, &e.alocada, &e.livre, &e.maior, &e.fragmentos, &fim);
    verificar(campos == 5 && fim > 0 && texto[fim] == '\0',
              "a saida de mymemory_stats possui os cinco campos esperados");
    return e;
}

/* Compara as estatisticas com os valores esperados. */
static void verificar_estatisticas(mymemory_t *memory, size_t alocacoes,
    size_t alocada, size_t livre, size_t maior, size_t fragmentos)
{
    estatisticas_t e = ler_estatisticas(memory);
    if (e.alocacoes != alocacoes || e.alocada != alocada ||
        e.livre != livre || e.maior != maior || e.fragmentos != fragmentos) {
        fprintf(stderr,
            "Esperado: %zu alocacoes, %zu alocados, %zu livres, "
            "%zu maior, %zu fragmentos\n"
            "Recebido: %zu alocacoes, %zu alocados, %zu livres, "
            "%zu maior, %zu fragmentos\n",
            alocacoes, alocada, livre, maior, fragmentos,
            e.alocacoes, e.alocada, e.livre, e.maior, e.fragmentos);
        verificar(0, "estatisticas devem corresponder aos valores esperados");
    }
}

/* Valida as medicoes com os totais mantidos pelo teste. */
static void verificar_medicao(estatisticas_t e, size_t qtd,
    size_t alocada_esperada, size_t tamanho_pool)
{
    verificar(e.alocacoes == qtd, "quantidade de alocacoes no comparativo");
    verificar(e.alocada == alocada_esperada,
              "total alocado no comparativo");
    verificar(e.livre == tamanho_pool - alocada_esperada,
              "total livre no comparativo");
    verificar(e.maior <= e.livre,
              "maior bloco livre nao ultrapassa os bytes livres");
    verificar(e.fragmentos <= qtd + 1,
              "numero de regioes livres dentro do limite da lista");
}

/* Verifica alinhamento, limites, ordem, ausencia de sobreposicao e contagem. */
static void verificar_lista(mymemory_t *memory, size_t quantidade_esperada)
{
    verificar(memory != NULL && memory->pool != NULL, "pool valido");

    uintptr_t base = (uintptr_t)memory->pool;
    size_t fim_anterior = 0;
    size_t quantidade = 0;

    for (allocation_t *atual = memory->head;
         atual != NULL;
         atual = atual->next) {
        uintptr_t endereco = (uintptr_t)atual->start;
        verificar(endereco >= base, "bloco nao pode comecar antes do pool");
        verificar(endereco % _Alignof(max_align_t) == 0,
                  "bloco alinhado para max_align_t");

        size_t inicio = (size_t)(endereco - base);
        verificar(inicio <= memory->total_size, "bloco comeca dentro do pool");
        verificar(atual->size > 0, "bloco deve ter tamanho positivo");
        verificar(atual->size <= memory->total_size - inicio,
                  "bloco nao pode ultrapassar o pool");
        verificar(inicio >= fim_anterior,
                  "lista ordenada e blocos sem sobreposicao");

        fim_anterior = inicio + atual->size;
        quantidade++;
        verificar(quantidade <= quantidade_esperada,
                  "lista nao pode ter mais nodos que o esperado");
    }
    verificar(quantidade == quantidade_esperada,
              "quantidade de alocacoes correta");
}

static void teste_basico_e_liberacoes(void)
{
    mymemory_t *memory = mymemory_init(256);
    verificar(memory != NULL, "inicializacao de 256 bytes");
    verificar_lista(memory, 0);
    verificar_estatisticas(memory, 0, 0, 256, 256, 1);

    unsigned char *a = mymemory_alloc(memory, 16);
    unsigned char *b = mymemory_alloc(memory, 32);
    unsigned char *c = mymemory_alloc(memory, 16);
    verificar(a != NULL && b != NULL && c != NULL,
              "tres alocacoes iniciais");
    verificar_lista(memory, 3);
    memset(a, 0x11, 16);
    memset(b, 0x22, 32);
    memset(c, 0x33, 16);

    /* Libera B e verifica se os dados de A e C foram preservados. */
    mymemory_free(memory, b);
    verificar_lista(memory, 2);
    verificar(a[0] == 0x11 && c[0] == 0x33,
              "liberar B nao modifica A ou C");
    puts("OK: liberar o nodo do meio");

    /* O First Fit deve reutilizar o inicio da lacuna de B. */
    unsigned char *d = mymemory_alloc(memory, 8);
    verificar(d == b, "First Fit reutiliza o inicio da lacuna de B");
    memset(d, 0x44, 8);
    verificar_lista(memory, 3);
    puts("OK: reutilizar a lacuna do meio");

    /* Ponteiros internos, externos ou NULL e gerenciador NULL devem ser ignorados. */
    int externo = 0;
    mymemory_free(memory, c + 1);
    mymemory_free(memory, &externo);
    mymemory_free(memory, NULL);
    mymemory_free(NULL, a);
    verificar_lista(memory, 3);
    verificar(c[0] == 0x33, "ponteiros invalidos nao afetam C");
    puts("OK: ignorar ponteiros invalidos");

    /* Libera o primeiro nodo e verifica se a segunda liberacao e ignorada. */
    mymemory_free(memory, a);
    verificar_lista(memory, 2);
    mymemory_free(memory, a);
    verificar_lista(memory, 2);
    puts("OK: liberar o primeiro nodo e ignorar dupla liberacao");

    /* Remove o ultimo nodo e depois o unico restante. */
    mymemory_free(memory, c);
    verificar_lista(memory, 1);
    mymemory_free(memory, d);
    verificar_lista(memory, 0);
    puts("OK: liberar o ultimo nodo e o unico nodo restante");

    verificar(mymemory_alloc(memory, 0) == NULL,
              "zero bytes deve retornar NULL");
    verificar(mymemory_alloc(memory, 257) == NULL,
              "solicitacao maior que o pool deve retornar NULL");
    verificar(mymemory_alloc(NULL, 10) == NULL,
              "gerenciador NULL deve retornar NULL");

    /* Um unico bloco deve poder ocupar todo o pool vazio. */
    void *completo = mymemory_alloc(memory, 256);
    verificar(completo != NULL, "alocacao que ocupa todo o pool");
    verificar_lista(memory, 1);
    verificar_estatisticas(memory, 1, 256, 0, 0, 0);
    verificar(mymemory_alloc(memory, 1) == NULL,
              "pool cheio nao pode aceitar mais um bloco");
    mymemory_free(memory, completo);
    verificar_lista(memory, 0);
    puts("OK: zero, excesso e pool completamente cheio");

    verificar_estatisticas(memory, 0, 0, 256, 256, 1);
    puts("OK: estatisticas de pool vazio e cheio");
    mymemory_cleanup(memory);
}

static void teste_first_fit_e_fragmentacao(void)
{
    /*
     * Tamanhos multiplos do alinhamento u evitam sobras.
     * O arranjo [4u][1u][2u][1u][6u] cria tres lacunas ao liberar 4u, 2u e 6u.
     */
    const size_t u = _Alignof(max_align_t);
    mymemory_t *memory = mymemory_init(14 * u);
    verificar(memory != NULL, "criar pool para diferenciar estrategias");
    void *primeira = mymemory_alloc(memory, 4 * u);
    void *separador_a = mymemory_alloc(memory, u);
    void *menor = mymemory_alloc(memory, 2 * u);
    void *separador_b = mymemory_alloc(memory, u);
    void *maior = mymemory_alloc(memory, 6 * u);
    verificar(primeira != NULL && separador_a != NULL && menor != NULL &&
              separador_b != NULL && maior != NULL, "preencher pool");
    verificar_lista(memory, 5);
    verificar_estatisticas(memory, 5, 14 * u, 0, 0, 0);

    mymemory_free(memory, primeira);
    mymemory_free(memory, menor);
    mymemory_free(memory, maior);
    verificar_lista(memory, 2);
    /* Ha 12u livres, mas nenhuma lacuna comporta 7u. */
    verificar_estatisticas(memory, 2, 2 * u, 12 * u, 6 * u, 3);
    verificar(mymemory_alloc(memory, 7 * u) == NULL,
              "fragmentacao impede pedido menor que o total livre");
    verificar_lista(memory, 2);
    verificar_estatisticas(memory, 2, 2 * u, 12 * u, 6 * u, 3);

    void *novo = mymemory_alloc(memory, 2 * u);
    verificar(novo == primeira, "First Fit escolhe a primeira lacuna de 4u");
    verificar(novo != menor, "Best Fit escolheria a lacuna de 2u");
    verificar(novo != maior, "Worst Fit escolheria a lacuna de 6u");
    verificar_lista(memory, 3);
    verificar_estatisticas(memory, 3, 4 * u, 10 * u, 6 * u, 3);

    /* Liberar os separadores deve unir as lacunas contiguas. */
    mymemory_free(memory, separador_a);
    verificar_estatisticas(memory, 2, 3 * u, 11 * u, 6 * u, 2);
    mymemory_free(memory, separador_b);
    verificar_estatisticas(memory, 1, 2 * u, 12 * u, 12 * u, 1);
    mymemory_free(memory, novo);
    verificar_lista(memory, 0);
    verificar_estatisticas(memory, 0, 0, 14 * u, 14 * u, 1);
    verificar(mymemory_alloc(memory, 14 * u) == memory->pool,
              "pool completo reutilizavel apos unir lacunas");
    /* Verifica cleanup com um bloco ainda alocado. */
    mymemory_cleanup(memory);
    puts("OK: First Fit distinto de Best/Worst Fit e estatisticas fragmentadas");
}

static void teste_estatisticas_com_alinhamento(void)
{
    const size_t u = _Alignof(max_align_t);
    mymemory_t *memory = mymemory_init(5 * u);
    verificar(memory != NULL, "criar pool para verificar alinhamento");
    void *a = mymemory_alloc(memory, u + 1);
    void *b = mymemory_alloc(memory, u);
    verificar(a == memory->pool && b == (char *)memory->pool + 2 * u,
              "enderecos respeitam o alinhamento de max_align_t");
    verificar_lista(memory, 2);
    /*
     * Entre A e B sobram u-1 bytes; ao final, 2u.
     * Se u for 1, apenas a regiao final e livre.
     */
    verificar_estatisticas(memory, 2, 2 * u + 1, 3 * u - 1, 2 * u,
                           u > 1 ? 2 : 1);
    mymemory_cleanup(memory);
    puts("OK: estatisticas incluem sobras de alinhamento");
}

static void teste_cinquenta_blocos(int ordem, estatisticas_t resultado[3])
{
    enum { N = 50, POOL_SIZE = 32768 };
    mymemory_t *memory = mymemory_init(POOL_SIZE);
    verificar(memory != NULL, "inicializacao do pool para 50 blocos");

    unsigned char *blocos[N] = {0};
    size_t tamanhos[N] = {0};
    int indices[N];
    size_t total_ativo = 0;

    /*
     * Usa os mesmos 50 tamanhos nas cinco ordens: sete exemplos
     * e outros 43 de 37 a 835 bytes, em passos de 19.
     */
    const size_t exemplos[] = {10, 25, 50, 100, 250, 500, 1000};
    const int qtd_exemplos = (int)(sizeof(exemplos) / sizeof(exemplos[0]));
    for (int i = 0; i < N; i++) {
        if (i < qtd_exemplos) {
            tamanhos[i] = exemplos[i];
        } else {
            tamanhos[i] = 37 + (size_t)(i - qtd_exemplos) * 19;
        }
        indices[i] = i;
    }
    if (ordem == 1) {
        for (int i = 0; i < N; i++) {
            indices[i] = N - 1 - i;
        }
    } else if (ordem == 2 || ordem == 3) {
        /* Ordena os indices por tamanho, preservando a identidade dos blocos. */
        for (int i = 1; i < N; i++) {
            int indice = indices[i];
            int j = i;
            while (j > 0 && (ordem == 2
                   ? tamanhos[indices[j - 1]] > tamanhos[indice]
                   : tamanhos[indices[j - 1]] < tamanhos[indice])) {
                indices[j] = indices[j - 1];
                j--;
            }
            indices[j] = indice;
        }
    } else if (ordem == 4) {
        /* Como 17 e 50 sao coprimos, (i * 17) % 50 percorre cada indice uma vez. */
        for (int i = 0; i < N; i++) {
            indices[i] = (i * 17) % N;
        }
    }

    int vistos[N] = {0};
    for (int posicao = 0; posicao < N; posicao++) {
        int i = indices[posicao];
        verificar(i >= 0 && i < N && !vistos[i],
                  "cada ordem deve conter os mesmos 50 IDs uma unica vez");
        vistos[i] = 1;
        blocos[i] = mymemory_alloc(memory, tamanhos[i]);
        verificar(blocos[i] != NULL, "alocar um dos 50 blocos");
        memset(blocos[i], (unsigned char)(i + 1), tamanhos[i]);
        total_ativo += tamanhos[i];
    }
    verificar_lista(memory, N);
    resultado[0] = ler_estatisticas(memory);
    verificar_medicao(resultado[0], N, total_ativo, POOL_SIZE);
    puts("OK: alocar 50 blocos variados");

    /* Libera os 25 indices impares em ordem decrescente e preserva os pares. */
    for (int i = N - 1; i >= 1; i -= 2) {
        mymemory_free(memory, blocos[i]);
        blocos[i] = NULL;
        total_ativo -= tamanhos[i];
    }
    verificar_lista(memory, N / 2);
    resultado[1] = ler_estatisticas(memory);
    verificar_medicao(resultado[1], N / 2, total_ativo, POOL_SIZE);

    /* Confere todos os bytes dos blocos restantes para detectar alteracoes. */
    for (int i = 0; i < N; i += 2) {
        for (size_t j = 0; j < tamanhos[i]; j++) {
            verificar(blocos[i][j] == (unsigned char)(i + 1),
                      "dados dos blocos restantes preservados");
        }
    }
    puts("OK: liberar 25 blocos fora da ordem e preservar os outros 25");

    unsigned char *novos[N / 2] = {0};
    for (int i = 0; i < N / 2; i++) {
        novos[i] = mymemory_alloc(memory, 8 + (size_t)(i % 5));
        verificar(novos[i] != NULL, "realocar em lacunas livres");
        memset(novos[i], 0xA5, 8 + (size_t)(i % 5));
        total_ativo += 8 + (size_t)(i % 5);
    }
    verificar_lista(memory, N);
    resultado[2] = ler_estatisticas(memory);
    verificar_medicao(resultado[2], N, total_ativo, POOL_SIZE);
    puts("OK: realizar 25 novas alocacoes apos as liberacoes");

    for (int i = 0; i < N; i += 2) {
        for (size_t j = 0; j < tamanhos[i]; j++) {
            verificar(blocos[i][j] == (unsigned char)(i + 1),
                      "dados preservados apos novas alocacoes");
        }
    }
    for (int i = 0; i < N / 2; i++) {
        for (size_t j = 0; j < 8 + (size_t)(i % 5); j++) {
            verificar(novos[i][j] == 0xA5,
                      "dados das novas alocacoes preservados");
        }
        mymemory_free(memory, novos[i]);
    }
    for (int i = N - 2; i >= 0; i -= 2) {
        mymemory_free(memory, blocos[i]);
    }
    verificar_lista(memory, 0);
    puts("OK: liberar todos os blocos sem perder nodos");

    verificar_estatisticas(memory, 0, 0, POOL_SIZE, POOL_SIZE, 1);
    mymemory_cleanup(memory);
}

/*
 * Compara as estatisticas; a ultima coluna indica a fracao livre fora
 * da maior regiao, sem medir eficiencia geral.
 */
static void imprimir_comparativo(const char *ordens[5],
    estatisticas_t resultados[5][3])
{
    const char *fases[] = {
        "Apos as 50 alocacoes iniciais",
        "Apos liberar 25 blocos",
        "Apos 25 novas alocacoes"
    };
    puts("\n=== COMPARACAO DE FRAGMENTACAO: FIRST FIT ===");
    puts("O pool e de 32768 bytes. Os mesmos 50 pedidos sao repetidos em 5 ordens.");

    for (int fase = 0; fase < 3; fase++) {
        printf("\n%s\n", fases[fase]);
        printf("%-24s %5s %9s %9s %9s %7s %12s\n",
               "Ordem", "Nodos", "Alocados", "Livres", "Maior", "Regioes",
               "Fora maior");
        for (int ordem = 0; ordem < 5; ordem++) {
            estatisticas_t e = resultados[ordem][fase];
            double percentual = e.livre == 0 ? 0.0 :
                100.0 * (double)(e.livre - e.maior) / (double)e.livre;
            printf("%-24s %5zu %9zu %9zu %9zu %7zu %11.1f%%\n",
                   ordens[ordem], e.alocacoes, e.alocada, e.livre,
                   e.maior, e.fragmentos, percentual);
        }
    }
    puts("\n'Fora maior' = porcentagem dos bytes livres que nao pertencem");
    puts("a maior regiao livre. Inclui pequenas sobras de alinhamento.");
    puts("Uma porcentagem menor indica memoria livre mais concentrada,");
    puts("mas nao comprova por si so maior eficiencia em todas as cargas.");
}

int main(void)
{
    verificar(mymemory_init(0) == NULL, "pool de tamanho zero deve falhar");
    /* cleanup deve aceitar NULL. */
    mymemory_cleanup(NULL);

    teste_basico_e_liberacoes();
    teste_first_fit_e_fragmentacao();
    teste_estatisticas_com_alinhamento();
    const char *ordens[5] = {
        "original", "inversa", "tamanho crescente", "tamanho decrescente",
        "permutacao deterministica"
    };
    estatisticas_t resultados[5][3] = {0};
    for (int ordem = 0; ordem < 5; ordem++) {
        printf("\n50 blocos na ordem: %s\n", ordens[ordem]);
        teste_cinquenta_blocos(ordem, resultados[ordem]);
    }

    imprimir_comparativo(ordens, resultados);
    puts("\nTODOS OS TESTES PASSARAM.");
    return 0;
}
