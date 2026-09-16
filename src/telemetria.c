/* ==========================================================================
 * Safedrive - Motor de decisao para o sistema ADAS
 * --------------------------------------------------------------------------
 * Implementacao das funcoes declaradas em telemetria.h. Nenhuma funcao
 * deste arquivo contem scanf, e apenas exibir_relatorio() contem printf:
 * todas as demais recebem seus dados por parametro e devolvem o resultado
 * com "return" (funcoes puras) ou escrevendo diretamente nas matrizes
 * recebidas por parametro (funcoes de orquestracao "processar_*"), como
 * pede a restricao de isolamento de E/S do enunciado.
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include "telemetria.h"

/* ==========================================================================
 * Geracao e carga de dados
 * ========================================================================== */

float gerar_float_aleatorio(float minimo, float maximo) {
    float escala = (float) rand() / (float) RAND_MAX;
    return minimo + escala * (maximo - minimo);
}

void inicializar_matrizes(float velocidades[][2],
                           float sensores_frontais[][3],
                           float sensores_laterais[][2],
                           int num_registros) {
    int i;
    float distancia_base;

    for (i = 0; i < num_registros; i++) {
        velocidades[i][COL_VEL_ATUAL]  = gerar_float_aleatorio(30.0f, 130.0f);
        velocidades[i][COL_VEL_FRENTE] = gerar_float_aleatorio(0.0f, 130.0f);

        /* As tres leituras frontais oscilam em torno de uma mesma distancia
         * "real", simulando o ruido natural de radar/lidar/camera. */
        distancia_base = gerar_float_aleatorio(3.0f, 80.0f);
        sensores_frontais[i][COL_RADAR]  = distancia_base + gerar_float_aleatorio(-1.5f, 1.5f);
        sensores_frontais[i][COL_LIDAR]  = distancia_base + gerar_float_aleatorio(-1.5f, 1.5f);
        sensores_frontais[i][COL_CAMERA] = distancia_base + gerar_float_aleatorio(-1.5f, 1.5f);

        sensores_laterais[i][COL_FAIXA_ESQ] = gerar_float_aleatorio(0.10f, 1.30f);
        sensores_laterais[i][COL_FAIXA_DIR] = gerar_float_aleatorio(0.10f, 1.30f);
    }
}

int armazenar_amostra(float velocidades[][2],
                       float sensores_frontais[][3],
                       float sensores_laterais[][2],
                       int num_amostras,
                       float v_atual, float v_frente,
                       float radar, float lidar, float camera,
                       float dist_esq, float dist_dir) {
    velocidades[num_amostras][COL_VEL_ATUAL]  = v_atual;
    velocidades[num_amostras][COL_VEL_FRENTE] = v_frente;

    sensores_frontais[num_amostras][COL_RADAR]  = radar;
    sensores_frontais[num_amostras][COL_LIDAR]  = lidar;
    sensores_frontais[num_amostras][COL_CAMERA] = camera;

    sensores_laterais[num_amostras][COL_FAIXA_ESQ] = dist_esq;
    sensores_laterais[num_amostras][COL_FAIXA_DIR] = dist_dir;

    return num_amostras + 1;
}


/* ==========================================================================
 * Regra A - Fusao de sensores frontais (mediana)
 * ========================================================================== */

float calcular_mediana3(float a, float b, float c) {
    /* Mediana de 3 valores por comparacao direta, sem precisar ordenar
     * o trio inteiro: o valor "do meio" e aquele que fica entre os
     * outros dois (maior ou igual a um e menor ou igual ao outro). */
    if ((a >= b && a <= c) || (a <= b && a >= c)) {
        return a;
    }
    if ((b >= a && b <= c) || (b <= a && b >= c)) {
        return b;
    }
    return c;
}

void processar_fusao_sensores(float sensores_frontais[][3],
                               float processamento[][2],
                               int num_amostras) {
    int i;
    for (i = 0; i < num_amostras; i++) {
        processamento[i][COL_DIST_VALIDADA] = calcular_mediana3(
            sensores_frontais[i][COL_RADAR],
            sensores_frontais[i][COL_LIDAR],
            sensores_frontais[i][COL_CAMERA]
        );
    }
}


/* ==========================================================================
 * Regra B - Distancia segura de frenagem
 * ========================================================================== */

float tempo_reacao_por_sensibilidade(int sensibilidade) {
    if (sensibilidade == SENSIBILIDADE_ESPORTIVO) {
        return TEMPO_REACAO_ESPORTIVO;
    }
    if (sensibilidade == SENSIBILIDADE_SEGURO) {
        return TEMPO_REACAO_SEGURO;
    }
    return TEMPO_REACAO_NORMAL;
}

float calcular_distancia_segura(float velocidade_kmh, float tempo_reacao, float atrito) {
    float velocidade_ms = velocidade_kmh / 3.6f;
    float distancia_reacao   = velocidade_ms * tempo_reacao;
    float distancia_frenagem = (velocidade_ms * velocidade_ms) / (2.0f * atrito * GRAVIDADE);
    return distancia_reacao + distancia_frenagem;
}

void processar_distancia_segura(float velocidades[][2],
                                 float processamento[][2],
                                 int num_amostras,
                                 int sensibilidade,
                                 float atrito) {
    int i;
    float tempo_reacao = tempo_reacao_por_sensibilidade(sensibilidade);

    for (i = 0; i < num_amostras; i++) {
        processamento[i][COL_DIST_SEGURA] =
            calcular_distancia_segura(velocidades[i][COL_VEL_ATUAL], tempo_reacao, atrito);
    }
}


/* ==========================================================================
 * Regra C - Analise de risco frontal (AEB)
 * ========================================================================== */

int avaliar_risco_frontal(float vel_atual, float vel_frente,
                           float distancia_validada, float distancia_segura) {
    float velocidade_relativa = vel_atual - vel_frente;

    if (velocidade_relativa <= 0.0f) {
        return STATUS_SEGURO;
    }
    if (distancia_validada >= distancia_segura) {
        return STATUS_SEGURO;
    }
    if (distancia_validada >= 0.5f * distancia_segura) {
        return STATUS_ATENCAO;
    }
    return STATUS_RISCO;
}

void processar_risco_frontal(float velocidades[][2],
                              float processamento[][2],
                              int status[][3],
                              int num_amostras) {
    int i;
    for (i = 0; i < num_amostras; i++) {
        status[i][COL_STATUS_FRONTAL] = avaliar_risco_frontal(
            velocidades[i][COL_VEL_ATUAL],
            velocidades[i][COL_VEL_FRENTE],
            processamento[i][COL_DIST_VALIDADA],
            processamento[i][COL_DIST_SEGURA]
        );
    }
}


/* ==========================================================================
 * Regra D - Assistente de faixa dinamico
 * ========================================================================== */

float calcular_margem_dinamica(float velocidade_atual) {
    float margem = MARGEM_BASE_FAIXA;

    if (velocidade_atual > LIMIAR_VELOCIDADE_FAIXA) {
        margem += (velocidade_atual - LIMIAR_VELOCIDADE_FAIXA) * INCREMENTO_MARGEM_POR_KMH;
    }
    return margem;
}

int avaliar_faixa(float distancia_leitura, float margem_dinamica) {
    if (distancia_leitura < margem_dinamica) {
        return STATUS_RISCO;
    }
    if (distancia_leitura < margem_dinamica + ZONA_ATENCAO_FAIXA) {
        return STATUS_ATENCAO;
    }
    return STATUS_SEGURO;
}

void processar_assistente_faixa(float velocidades[][2],
                                 float sensores_laterais[][2],
                                 int status[][3],
                                 int num_amostras) {
    int i;
    float margem_dinamica;

    for (i = 0; i < num_amostras; i++) {
        margem_dinamica = calcular_margem_dinamica(velocidades[i][COL_VEL_ATUAL]);

        status[i][COL_STATUS_FAIXA_ESQ] =
            avaliar_faixa(sensores_laterais[i][COL_FAIXA_ESQ], margem_dinamica);
        status[i][COL_STATUS_FAIXA_DIR] =
            avaliar_faixa(sensores_laterais[i][COL_FAIXA_DIR], margem_dinamica);
    }
}


/* ==========================================================================
 * Regra E - Relatorio final (unica funcao com printf)
 * ========================================================================== */

void exibir_relatorio(float velocidades[][2],
                       float sensores_frontais[][3],
                       float sensores_laterais[][2],
                       float processamento[][2],
                       int status[][3],
                       int num_amostras) {
    int i;
    int status_frontal, status_esq, status_dir;

    printf("\n============================================================\n");
    printf("           RELATORIO SAFEDRIVE - RISCOS ADAS\n");
    printf("============================================================\n");

    if (num_amostras == 0) {
        printf("Nenhuma amostra registrada ate o momento.\n");
        printf("============================================================\n");
        return;
    }

    for (i = 0; i < num_amostras; i++) {
        status_frontal = status[i][COL_STATUS_FRONTAL];
        status_esq     = status[i][COL_STATUS_FAIXA_ESQ];
        status_dir     = status[i][COL_STATUS_FAIXA_DIR];

        printf("\n---------------- Amostra %3d de %3d ----------------\n", i + 1, num_amostras);

        printf("Dados de entrada:\n");
        printf("  Velocidade atual ................ %7.2f km/h\n", velocidades[i][COL_VEL_ATUAL]);
        printf("  Velocidade do veiculo a frente ... %7.2f km/h\n", velocidades[i][COL_VEL_FRENTE]);
        printf("  Radar ............................ %7.2f m\n", sensores_frontais[i][COL_RADAR]);
        printf("  Lidar ............................ %7.2f m\n", sensores_frontais[i][COL_LIDAR]);
        printf("  Camera ........................... %7.2f m\n", sensores_frontais[i][COL_CAMERA]);
        printf("  Distancia faixa esquerda ......... %7.2f m\n", sensores_laterais[i][COL_FAIXA_ESQ]);
        printf("  Distancia faixa direita .......... %7.2f m\n", sensores_laterais[i][COL_FAIXA_DIR]);

        printf("Dados processados:\n");
        printf("  Distancia validada (mediana) ..... %7.2f m\n", processamento[i][COL_DIST_VALIDADA]);
        printf("  Distancia segura exigida ......... %7.2f m\n", processamento[i][COL_DIST_SEGURA]);

        printf("Status:\n");

        printf("  Frontal ......: ");
        if (status_frontal == STATUS_SEGURO) {
            printf("SEGURO\n");
        } else if (status_frontal == STATUS_ATENCAO) {
            printf("ATENCAO\n");
        } else {
            printf("RISCO DE COLISAO (AEB ACIONADO)\n");
        }

        printf("  Faixa esquerda: ");
        if (status_esq == STATUS_SEGURO) {
            printf("NORMAL\n");
        } else if (status_esq == STATUS_ATENCAO) {
            printf("ATENCAO\n");
        } else {
            printf("PERIGO DE INVASAO\n");
        }

        printf("  Faixa direita.: ");
        if (status_dir == STATUS_SEGURO) {
            printf("NORMAL\n");
        } else if (status_dir == STATUS_ATENCAO) {
            printf("ATENCAO\n");
        } else {
            printf("PERIGO DE INVASAO\n");
        }

        if (status_frontal == STATUS_RISCO || status_esq == STATUS_RISCO || status_dir == STATUS_RISCO) {
            printf(">>> STATUS GERAL: INTERVENCAO CRITICA EXIGIDA <<<\n");
        } else if (status_frontal == STATUS_ATENCAO || status_esq == STATUS_ATENCAO || status_dir == STATUS_ATENCAO) {
            printf(">>> STATUS GERAL: ATENCAO <<<\n");
        } else {
            printf(">>> STATUS GERAL: NORMAL <<<\n");
        }
    }

    printf("\n============================================================\n");
}
