# Gerenciador de Memória em C — First Fit

**Trabalho 2 — Programação de Software Básico**

Por Amanda Wilmsen e Betina Kusler

Biblioteca em **C11** que gerencia alocações dentro de um *pool* de memória pré-alocado. Utiliza uma **lista encadeada de blocos ocupados**, ordenada por endereço, e a estratégia **First Fit** (primeira lacuna adequada).

## Arquivos

| Arquivo | Função |
|---|---|
| `mymemory.h` | Estruturas e interface da biblioteca |
| `mymemory.c` | Implementação do gerenciador |
| `main.c` | Demonstração com pool de 100 bytes |
| `testes.c` | Testes automatizados e análise de fragmentação |

## Compilação e execução

**Exemplo:**

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror main.c mymemory.c -o exemplo
./exemplo
```

**Testes:**

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror testes.c mymemory.c -o testes
./testes
```

> `main.c` e `testes.c` possuem funções `main()` próprias: compile-os separadamente. Os testes utilizam funções POSIX e são destinados a macOS ou Linux.

## Funcionamento

- `mymemory_init()`: cria o gerenciador e reserva o pool.
- `mymemory_alloc()`: procura a primeira região livre suficiente, respeita o alinhamento e registra a alocação.
- `mymemory_free()`: remove o registro de um bloco válido e disponibiliza sua região para reutilização.
- `mymemory_display()`: mostra endereço e tamanho dos blocos ocupados.
- `mymemory_stats()`: exibe número de alocações, bytes alocados, bytes livres, maior região livre e quantidade de regiões livres.
- `mymemory_cleanup()`: libera os nodos de controle, o pool e o gerenciador.

`malloc()` e `free()` são usados apenas para o **pool** e as **estruturas de controle**. Os blocos solicitados são regiões internas do pool, não alocações independentes. O código implementa **somente First Fit**.

## Testes e fragmentação

Os testes verificam alocação e liberação, ponteiros inválidos, reutilização de lacunas, alinhamento, integridade dos dados e estatísticas. Também executam **50 tamanhos distintos (10 a 1.000 bytes)** em **cinco ordens**: original, inversa, crescente, decrescente e permutação determinística.

Em cada ordem, são liberados 25 blocos e realizadas 25 novas alocações. O programa compara a fragmentação em três momentos: após as 50 alocações, após as liberações e após as novas alocações.

Para salvar os resultados:

```sh
./testes > resultado_testes.txt
```

As estatísticas incluem sobras de alinhamento como regiões fisicamente livres. O número de fragmentos pode variar entre plataformas.
