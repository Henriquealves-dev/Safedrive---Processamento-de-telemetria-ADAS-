# Safedrive — Processamento de Telemetria ADAS

Motor de decisão para um sistema **ADAS** (*Advanced Driver Assistance
Systems*) implementado em **Linguagem C puro**, na forma de uma calculadora
de telemetria acionada por menu. O programa simula o processamento de um
fluxo de amostras de sensores veiculares (radar, lidar, câmera e sensores de
faixa), calcula distâncias seguras de frenagem e produz um relatório de
risco por amostra.

Projeto acadêmico da disciplina **Algoritmos e Programação II** — FCI,
Universidade Presbiteriana Mackenzie.

## Sumário

- [Restrições técnicas do enunciado](#restrições-técnicas-do-enunciado)
- [Estrutura do projeto](#estrutura-do-projeto)
- [Como compilar e executar](#como-compilar-e-executar)
- [Estruturas de dados (as 5 matrizes)](#estruturas-de-dados-as-5-matrizes)
- [Menu interativo](#menu-interativo)
- [Regras de negócio](#regras-de-negócio)
- [Referência das funções](#referência-das-funções)
- [Exemplo de execução](#exemplo-de-execução)
- [Decisões de projeto](#decisões-de-projeto)

## Restrições técnicas do enunciado

O enunciado impõe restrições propositais, para forçar o domínio de matrizes,
passagem de parâmetros e modularização. Todas foram seguidas à risca:

| Restrição | Como foi atendida |
|---|---|
| Proibido `struct` | Nenhuma struct no projeto; tudo é matriz/escalar. |
| Proibido `malloc`/`realloc` | Todas as matrizes são vetores **estáticos** de tamanho fixo (`MAX_AMOSTRAS = 100`), declarados em `main`. |
| Proibido ponteiro explícito | Nenhuma variável `tipo *nome` no código. Matrizes são passadas por parâmetro usando **notação de array** (`float m[][2]`), não notação de ponteiro. |
| Proibida variável global | Não existe nenhuma variável fora de função. Constantes usam `#define` (macros de pré-processador, não variáveis). |
| Cálculo sem E/S | Nenhuma função de cálculo (`calcular_*`, `avaliar_*`, `processar_*`) contém `scanf` ou `printf`. Todos os dados chegam por parâmetro e o resultado sai por `return` ou é escrito direto na matriz recebida. |
| Impressão restrita | Apenas `exibir_relatorio()`, em `telemetria.c`, imprime dados de telemetria. |
| Leitura só na `main` | Todo `scanf` do programa está em `main()`, em `src/main.c`. |

## Estrutura do projeto

```
.
├── Makefile              # build (make / make run / make clean)
├── README.md
└── src/
    ├── main.c             # main(): menu, leitura de dados (scanf), orquestração
    ├── telemetria.h       # constantes, índices de coluna e protótipos documentados
    └── telemetria.c       # implementação: geração de dados, regras A-E e relatório
```

## Como compilar e executar

Pré-requisito: `gcc` (ou outro compilador C compatível com C99).

```bash
make          # compila e gera o executável ./safedrive
make run      # compila (se necessário) e já executa
make clean    # remove o executável gerado
```

Ou, manualmente:

```bash
gcc -Wall -Wextra -std=c99 -o safedrive src/main.c src/telemetria.c
./safedrive
```

O projeto compila sem nenhum warning com `-Wall -Wextra` e foi validado com
`-fsanitize=address,undefined` (sem erros de memória ou comportamento
indefinido).

## Estruturas de dados (as 5 matrizes)

Todas declaradas em `main()` com `MAX_AMOSTRAS = 100` linhas (uma por
amostra de tempo) e consumidas pelas demais funções via parâmetro:

| Matriz | Dimensão | Coluna 0 | Coluna 1 | Coluna 2 |
|---|---|---|---|---|
| `velocidades` | `[100][2]` | velocidade atual (km/h) | velocidade do veículo à frente (km/h) | — |
| `sensores_frontais` | `[100][3]` | radar (m) | lidar (m) | câmera (m) |
| `sensores_laterais` | `[100][2]` | distância faixa esquerda (m) | distância faixa direita (m) | — |
| `processamento` | `[100][2]` | distância validada (mediana) (m) | distância segura calculada (m) | — |
| `status` (inteiros) | `[100][3]` | risco frontal (0/1/2) | risco faixa esquerda (0/1/2) | risco faixa direita (0/1/2) |

As constantes `COL_*` em `telemetria.h` nomeiam cada uma dessas colunas
(ex.: `COL_VEL_ATUAL`, `COL_RADAR`, `COL_DIST_SEGURA`), para que o código
nunca use um índice numérico "solto".

## Menu interativo

Ao iniciar, o programa pergunta o **atrito da via** (ex.: `0.70`) e a
**sensibilidade do ADAS** (`1` Esportivo, `2` Normal ou `3` Seguro) — uma
única vez, antes do laço do menu. Em seguida:

1. **Carregar dados iniciais** — gera 50 amostras aleatórias plausíveis nas
   três matrizes de entrada (velocidades, sensores frontais, sensores
   laterais).
2. **Inserir nova amostra** — lê do teclado as 2 velocidades e as 5 leituras
   de sensores do instante atual e grava na próxima linha livre.
3. **Processar e exibir relatório de riscos** — executa, em sequência, a
   fusão de sensores, o cálculo de distância segura, a análise de risco
   frontal, o assistente de faixa e, por fim, o relatório.
4. **Sair** — encerra o simulador.

## Regras de negócio

### A) Fusão de sensores (`calcular_mediana3` / `processar_fusao_sensores`)

Sensores reais têm ruído, então cada leitura frontal é combinada pela
**mediana** das três (radar, lidar, câmera) — o valor central "descarta" a
leitura mais discrepante das três. O resultado vira a *distância validada*.

### B) Distância segura de frenagem (`calcular_distancia_segura` / `processar_distancia_segura`)

```
v_ms      = velocidade_atual_kmh / 3.6
distancia = (v_ms × tempoReacao) + v_ms² / (2 × atrito × 9.81)
```

`tempoReacao` depende da sensibilidade escolhida: **1.0 s** (Esportivo),
**1.5 s** (Normal) ou **2.0 s** (Seguro). O primeiro termo é a distância
percorrida durante o tempo de reação; o segundo é a distância física de
frenagem (quanto maior o atrito, menor a distância de frenagem).

> A velocidade usada na fórmula é a **velocidade atual do próprio veículo**
> (coluna 0 de `velocidades`): é ela quem determina quanto o carro percorre
> reagindo e quanto ele demora para parar — ver [Decisões de projeto](#decisões-de-projeto).

### C) Análise de risco frontal / AEB (`avaliar_risco_frontal` / `processar_risco_frontal`)

```
velocidadeRelativa = velocidadeAtual − velocidadeFrente

se velocidadeRelativa <= 0            → SEGURO (carro da frente não está sendo alcançado)
senão, comparando validada x segura:
    validada >= segura                → SEGURO
    validada >= 50% da segura         → ATENÇÃO
    validada <  50% da segura         → RISCO DE COLISÃO (AEB acionado)
```

### D) Assistente de faixa dinâmico (`calcular_margem_dinamica` / `avaliar_faixa` / `processar_assistente_faixa`)

```
margemDinamica = 0.50 m + max(0, velocidadeAtual − 80.0) × 0.01 m
```

Acima de 80 km/h, a margem exigida cresce 1 cm para cada 1 km/h extra.
Define-se ainda uma zona de atenção de 0.20 m logo acima dessa margem.
Cada faixa (esquerda/direita) é avaliada de forma independente:

```
leitura <  margemDinamica              → PERIGO DE INVASÃO
leitura <  margemDinamica + 0.20       → ATENÇÃO
caso contrário                         → NORMAL
```

### E) Relatório (`exibir_relatorio`)

Única função do programa autorizada a usar `printf` para dados de
telemetria. Para cada amostra registrada, imprime um painel com os dados de
entrada (velocidades e as 5 leituras de sensores), os dados processados
(distância validada e distância segura), o status textual de cada uma das 3
colunas de `status`, e a decisão geral:

- Qualquer coluna com código `2` → **STATUS GERAL: INTERVENÇÃO CRÍTICA EXIGIDA**
- Nenhuma `2`, mas alguma coluna `1` → **STATUS GERAL: ATENÇÃO**
- Todas as colunas `0` → **STATUS GERAL: NORMAL**

## Referência das funções

Todas declaradas em `src/telemetria.h` (com comentários de documentação) e
implementadas em `src/telemetria.c`, exceto `main`, que fica em
`src/main.c`. "Pura" = não faz E/S, só calcula e devolve o resultado.

| Função | Tipo | O que faz |
|---|---|---|
| `gerar_float_aleatorio(min, max)` | pura | Sorteia um `float` no intervalo `[min, max]`. |
| `inicializar_matrizes(...)` | orquestração | Preenche `num_registros` linhas das matrizes de entrada com dados aleatórios plausíveis (opção 1 do menu). |
| `armazenar_amostra(...)` | orquestração | Grava uma amostra (já lida em `main`) na linha `num_amostras` e devolve `num_amostras + 1`. |
| `calcular_mediana3(a, b, c)` | pura | Mediana de três leituras (regra A). |
| `processar_fusao_sensores(...)` | orquestração | Aplica a mediana a todas as amostras e grava em `processamento[][0]`. |
| `tempo_reacao_por_sensibilidade(s)` | pura | Converte `1/2/3` em `1.0/1.5/2.0` segundos. |
| `calcular_distancia_segura(v, t, atrito)` | pura | Fórmula da distância segura de frenagem (regra B). |
| `processar_distancia_segura(...)` | orquestração | Aplica a fórmula a todas as amostras e grava em `processamento[][1]`. |
| `avaliar_risco_frontal(...)` | pura | Classifica o risco frontal 0/1/2 (regra C). |
| `processar_risco_frontal(...)` | orquestração | Aplica a classificação a todas as amostras e grava em `status[][0]`. |
| `calcular_margem_dinamica(v)` | pura | Margem lateral de segurança para a velocidade atual (regra D). |
| `avaliar_faixa(leitura, margem)` | pura | Classifica o risco de uma faixa 0/1/2 (regra D). |
| `processar_assistente_faixa(...)` | orquestração | Aplica a avaliação às duas faixas de todas as amostras e grava em `status[][1]` e `status[][2]`. |
| `exibir_relatorio(...)` | **impressão** | Única função com `printf` de telemetria; monta o painel final (regra E). |
| `main()` | E/S | Lê `atrito`/`sensibilidade`, roda o menu, é a única função com `scanf`. |

## Exemplo de execução

```
$ ./safedrive
============================================================
   SAFEDRIVE - Motor de decisao ADAS (Linguagem C)
============================================================

Informe o coeficiente de atrito da via (ex.: 0.70 para asfalto seco): 0.7
Informe a sensibilidade do ADAS (1-Esportivo, 2-Normal, 3-Seguro): 2

------------------------ MENU SAFEDRIVE ------------------------
1. Carregar dados iniciais (50 amostras aleatorias)
2. Inserir nova amostra
3. Processar e exibir relatorio de riscos
4. Sair
Amostras registradas: 0 / 100
Escolha uma opcao: 2

-- Inserir nova amostra (instante atual) --
Velocidade atual (km/h): 100
Velocidade do veiculo a frente (km/h): 80
Leitura do radar (m): 40
Leitura do lidar (m): 42
Leitura da camera (m): 38
Distancia ate a faixa esquerda (m): 0.85
Distancia ate a faixa direita (m): 0.3

Amostra registrada com sucesso (linha 1).
...
Escolha uma opcao: 3

============================================================
           RELATORIO SAFEDRIVE - RISCOS ADAS
============================================================

---------------- Amostra   1 de   1 ----------------
Dados de entrada:
  Velocidade atual ................  100.00 km/h
  Velocidade do veiculo a frente ...   80.00 km/h
  Radar ............................   40.00 m
  Lidar ............................   42.00 m
  Camera ...........................   38.00 m
  Distancia faixa esquerda .........    0.85 m
  Distancia faixa direita ..........    0.30 m
Dados processados:
  Distancia validada (mediana) .....   40.00 m
  Distancia segura exigida .........   97.85 m
Status:
  Frontal ......: RISCO DE COLISAO (AEB ACIONADO)
  Faixa esquerda: ATENCAO
  Faixa direita.: PERIGO DE INVASAO
>>> STATUS GERAL: INTERVENCAO CRITICA EXIGIDA <<<

============================================================
```

(Esse resultado foi conferido manualmente: `100 km/h = 27.78 m/s`;
distância segura `= 27.78×1.5 + 27.78²/(2×0.7×9.81) ≈ 97.85 m`; como a
distância validada de 40 m é menor que 50% de 97.85 m, o AEB é acionado.)

## Decisões de projeto

Pontos em que o enunciado deixava espaço para interpretação, e a escolha
feita aqui:

- **Textos do relatório sem acentos** (`ATENCAO`, `COLISAO`, `INVASAO`...).
  Opção deliberada para evitar problemas de codificação entre o editor, o
  `gcc` e o terminal em que o programa for executado (comum em
  Windows/`cmd.exe` ou terminais fora de UTF-8). O conteúdo e a lógica são
  exatamente os pedidos no enunciado; se o ambiente de avaliação garantir
  UTF-8 de ponta a ponta, basta reintroduzir os acentos nas strings dentro
  de `exibir_relatorio` (`src/telemetria.c`).
- **`printf`/`scanf` fora de `exibir_relatorio`/`main`**: a restrição do
  enunciado ("apenas uma função... poderá conter `printf`") foi entendida
  como valendo para as **funções de cálculo/lógica** — que de fato não têm
  nenhum `printf`/`scanf`. Os `printf` de menu e prompts, e os `scanf` de
  leitura, ficam em `main()`, que é a única função com `scanf` do projeto,
  como pedido explicitamente ("a leitura de dados deve ocorrer
  exclusivamente na função main").
- **Velocidade usada na fórmula de distância segura**: o enunciado usa
  apenas "v", sem dizer qual coluna. Foi adotada a **velocidade atual** do
  próprio veículo (`velocidades[][0]`), já que a distância de frenagem
  depende de quão rápido o próprio carro está indo — é também a mesma
  variável usada na regra D (assistente de faixa).
- **"Carregar dados iniciais" (opção 1)**: sempre (re)escreve as linhas
  `0..49` com 50 novas amostras aleatórias e ajusta o contador para 50 —
  pensada como o passo inicial de carga de uma massa de dados de
  demonstração, podendo ser usada de novo para reamostrar esse bloco
  inicial.
- **Matriz cheia**: se as 100 amostras já estiverem ocupadas, a opção 2
  apenas avisa o usuário e não sobrescreve nada, em vez de travar ou
  sobrescrever uma amostra existente.

---

Desenvolvido com apoio do [Claude Code](https://claude.com/claude-code).
