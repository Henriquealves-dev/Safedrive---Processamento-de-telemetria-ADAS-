#ifndef TELEMETRIA_H
#define TELEMETRIA_H

/* ==========================================================================
 * Safedrive - Motor de decisao para o sistema ADAS
 * --------------------------------------------------------------------------
 * Este cabecalho define as constantes do dominio (limites, sensibilidade,
 * codigos de status), os indices de coluna de cada matriz e o prototipo de
 * toda funcao que manipula telemetria.
 *
 * Convencao de camadas usada em todo o projeto:
 *   - Funcoes "calcular_*" / "avaliar_*" / "gerar_*" / "tempo_*": funcoes
 *     PURAS. Recebem valores escalares por parametro, nao leem (scanf) nem
 *     imprimem (printf) nada, e devolvem o resultado com "return".
 *   - Funcoes "processar_*" / "inicializar_matrizes" / "armazenar_amostra":
 *     funcoes de ORQUESTRACAO. Recebem as matrizes inteiras (por parametro,
 *     usando notacao de array) e um contador de linhas, percorrem as
 *     amostras em laco e chamam as funcoes puras para preencher as colunas
 *     de saida. Tambem nao fazem I/O.
 *   - exibir_relatorio: unica funcao do projeto autorizada a conter printf
 *     de dados processados (ver README.md, secao "Restricoes tecnicas").
 *   - main() (em main.c): unica funcao autorizada a conter scanf, e onde as
 *     cinco matrizes bidimensionais vivem de fato.
 * ========================================================================== */

/* Capacidade maxima de amostras (linhas) armazenadas nas matrizes. */
#define MAX_AMOSTRAS 100

/* Quantidade de amostras aleatorias geradas pela carga inicial (opcao 1). */
#define AMOSTRAS_CARGA_INICIAL 50

/* Niveis de sensibilidade do ADAS, informados pelo usuario no inicio do
 * programa (ver main()). */
#define SENSIBILIDADE_ESPORTIVO 1
#define SENSIBILIDADE_NORMAL    2
#define SENSIBILIDADE_SEGURO    3

/* Tempo de reacao do condutor/sistema (em segundos) associado a cada
 * sensibilidade. Usado por tempo_reacao_por_sensibilidade(). */
#define TEMPO_REACAO_ESPORTIVO 1.0f
#define TEMPO_REACAO_NORMAL    1.5f
#define TEMPO_REACAO_SEGURO    2.0f

/* Codigos de risco (0/1/2) usados em TODAS as colunas da matriz status.
 * O texto exibido no relatorio depende da coluna: a coluna frontal usa
 * "SEGURO/ATENCAO/RISCO DE COLISAO" e as colunas de faixa usam
 * "NORMAL/ATENCAO/PERIGO DE INVASAO" -- mas o numero guardado e o mesmo. */
#define STATUS_SEGURO  0
#define STATUS_ATENCAO 1
#define STATUS_RISCO   2

/* Parametros fisicos do assistente de faixa dinamico (regra D). */
#define MARGEM_BASE_FAIXA         0.50f  /* margem minima, em metros       */
#define LIMIAR_VELOCIDADE_FAIXA   80.0f  /* km/h a partir do qual a margem
                                             passa a crescer               */
#define INCREMENTO_MARGEM_POR_KMH 0.01f  /* metros somados por km/h acima
                                             do limiar                     */
#define ZONA_ATENCAO_FAIXA        0.20f  /* faixa de atencao, em metros,
                                             logo acima da margem dinamica */

/* Aceleracao da gravidade usada na formula de distancia de frenagem
 * (regra B), em m/s^2. */
#define GRAVIDADE 9.81f

/* --------------------------------------------------------------------------
 * Indices de coluna de cada matriz. Usar essas constantes em vez de numeros
 * soltos (ex.: velocidades[i][COL_VEL_ATUAL] em vez de velocidades[i][0])
 * deixa o codigo autoexplicativo e evita erros de digitacao de indice.
 * -------------------------------------------------------------------------- */

/* velocidades[MAX_AMOSTRAS][2] */
#define COL_VEL_ATUAL  0   /* velocidade atual do veiculo (km/h)        */
#define COL_VEL_FRENTE 1   /* velocidade do veiculo a frente (km/h)     */

/* sensores_frontais[MAX_AMOSTRAS][3] */
#define COL_RADAR  0       /* leitura do radar (m)                      */
#define COL_LIDAR  1       /* leitura do lidar (m)                      */
#define COL_CAMERA 2       /* leitura da camera (m)                     */

/* sensores_laterais[MAX_AMOSTRAS][2] */
#define COL_FAIXA_ESQ 0    /* distancia ate a faixa esquerda (m)        */
#define COL_FAIXA_DIR 1    /* distancia ate a faixa direita (m)         */

/* processamento[MAX_AMOSTRAS][2] */
#define COL_DIST_VALIDADA 0  /* distancia frontal validada (mediana) (m) */
#define COL_DIST_SEGURA   1  /* distancia segura de frenagem exigida (m) */

/* status[MAX_AMOSTRAS][3] (matriz de inteiros) */
#define COL_STATUS_FRONTAL   0  /* risco frontal / AEB                  */
#define COL_STATUS_FAIXA_ESQ 1  /* risco de invasao da faixa esquerda   */
#define COL_STATUS_FAIXA_DIR 2  /* risco de invasao da faixa direita    */


/* ==========================================================================
 * Geracao e carga de dados
 * ========================================================================== */

/**
 * Sorteia um numero em ponto flutuante no intervalo fechado [minimo, maximo].
 * Depende de rand(), portanto srand() deve ter sido chamado antes (main faz
 * isso uma unica vez, no inicio do programa).
 */
float gerar_float_aleatorio(float minimo, float maximo);

/**
 * Preenche as primeiras `num_registros` linhas das matrizes de entrada
 * (velocidades, sensores_frontais, sensores_laterais) com valores
 * aleatorios plausiveis, simulando um historico inicial de leituras.
 * Nao mexe em `processamento` nem em `status`: essas so sao calculadas
 * quando o usuario escolhe a opcao 3 do menu.
 */
void inicializar_matrizes(float velocidades[][2],
                           float sensores_frontais[][3],
                           float sensores_laterais[][2],
                           int num_registros);

/**
 * Grava uma nova amostra (instante atual) na proxima linha livre das
 * matrizes de entrada, isto e, na linha de indice `num_amostras`.
 * Retorna o novo total de amostras (`num_amostras + 1`), que o chamador
 * deve guardar de volta na sua variavel de contagem -- e assim, sem usar
 * ponteiros, que o "contador global" e atualizado.
 */
int armazenar_amostra(float velocidades[][2],
                       float sensores_frontais[][3],
                       float sensores_laterais[][2],
                       int num_amostras,
                       float v_atual, float v_frente,
                       float radar, float lidar, float camera,
                       float dist_esq, float dist_dir);


/* ==========================================================================
 * Regra A - Fusao de sensores frontais (mediana)
 * ========================================================================== */

/** Devolve a mediana (valor central) entre tres leituras de sensor. */
float calcular_mediana3(float a, float b, float c);

/**
 * Para cada amostra registrada, calcula a mediana das tres leituras de
 * sensores_frontais (radar/lidar/camera) e grava o resultado em
 * processamento[i][COL_DIST_VALIDADA].
 */
void processar_fusao_sensores(float sensores_frontais[][3],
                               float processamento[][2],
                               int num_amostras);


/* ==========================================================================
 * Regra B - Distancia segura de frenagem
 * ========================================================================== */

/** Converte o codigo de sensibilidade (1/2/3) no tempo de reacao (s). */
float tempo_reacao_por_sensibilidade(int sensibilidade);

/**
 * Calcula a distancia segura de frenagem para uma velocidade (km/h), um
 * tempo de reacao (s) e um coeficiente de atrito da via.
 * Formula: distancia = (v_ms * tempoReacao) + v_ms^2 / (2 * atrito * 9.81)
 * onde v_ms e a velocidade convertida de km/h para m/s (dividida por 3.6).
 */
float calcular_distancia_segura(float velocidade_kmh, float tempo_reacao, float atrito);

/**
 * Para cada amostra registrada, calcula a distancia segura de frenagem
 * (a partir da velocidade atual do veiculo, da sensibilidade escolhida e
 * do atrito da via) e grava o resultado em
 * processamento[i][COL_DIST_SEGURA].
 */
void processar_distancia_segura(float velocidades[][2],
                                 float processamento[][2],
                                 int num_amostras,
                                 int sensibilidade,
                                 float atrito);


/* ==========================================================================
 * Regra C - Analise de risco frontal (AEB)
 * ========================================================================== */

/**
 * Classifica o risco frontal (0 = seguro, 1 = atencao, 2 = risco de
 * colisao / AEB acionado) a partir da velocidade relativa entre os dois
 * veiculos e da comparacao entre distancia validada e distancia segura.
 */
int avaliar_risco_frontal(float vel_atual, float vel_frente,
                           float distancia_validada, float distancia_segura);

/**
 * Aplica avaliar_risco_frontal() a cada amostra registrada e grava o
 * codigo resultante em status[i][COL_STATUS_FRONTAL].
 */
void processar_risco_frontal(float velocidades[][2],
                              float processamento[][2],
                              int status[][3],
                              int num_amostras);


/* ==========================================================================
 * Regra D - Assistente de faixa dinamico
 * ========================================================================== */

/**
 * Calcula a margem lateral de seguranca dinamica (m) para uma velocidade
 * atual (km/h): parte da margem base e cresce conforme a velocidade
 * ultrapassa o limiar definido em LIMIAR_VELOCIDADE_FAIXA.
 */
float calcular_margem_dinamica(float velocidade_atual);

/**
 * Classifica o risco de invasao de uma faixa (0 = normal, 1 = atencao,
 * 2 = perigo de invasao) comparando a leitura do sensor lateral com a
 * margem dinamica calculada para o instante.
 */
int avaliar_faixa(float distancia_leitura, float margem_dinamica);

/**
 * Para cada amostra registrada, calcula a margem dinamica a partir da
 * velocidade atual e classifica separadamente a faixa esquerda e a
 * direita, gravando os codigos em status[i][COL_STATUS_FAIXA_ESQ] e
 * status[i][COL_STATUS_FAIXA_DIR].
 */
void processar_assistente_faixa(float velocidades[][2],
                                 float sensores_laterais[][2],
                                 int status[][3],
                                 int num_amostras);


/* ==========================================================================
 * Regra E - Relatorio final
 * ========================================================================== */

/**
 * Unica funcao do projeto com permissao para usar printf de dados de
 * telemetria. Percorre todas as amostras registradas e imprime, para cada
 * uma, um painel com os dados de entrada, os dados processados, o status
 * textual de cada coluna e a decisao geral do sistema (NORMAL / ATENCAO /
 * INTERVENCAO CRITICA EXIGIDA).
 */
void exibir_relatorio(float velocidades[][2],
                       float sensores_frontais[][3],
                       float sensores_laterais[][2],
                       float processamento[][2],
                       int status[][3],
                       int num_amostras);

#endif /* TELEMETRIA_H */
