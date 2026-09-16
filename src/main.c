/* ==========================================================================
 * Safedrive - Motor de decisao para o sistema ADAS
 * --------------------------------------------------------------------------
 * Ponto de entrada do programa. E a UNICA funcao do projeto que contem
 * scanf (leitura de dados do usuario) e a unica que declara as cinco
 * matrizes bidimensionais exigidas pelo enunciado. Todo o processamento
 * (calculos e regras de negocio) fica em telemetria.c; main() apenas lê
 * dados, guarda-os nas matrizes e aciona as funcoes na ordem correta.
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "telemetria.h"

int main(void) {
    /* As cinco matrizes bidimensionais exigidas pelo enunciado (secao 3),
     * todas declaradas e "consolidadas" aqui na main. */
    float velocidades[MAX_AMOSTRAS][2];
    float sensores_frontais[MAX_AMOSTRAS][3];
    float sensores_laterais[MAX_AMOSTRAS][2];
    float processamento[MAX_AMOSTRAS][2];
    int   status[MAX_AMOSTRAS][3];

    int   num_amostras = 0;   /* quantas linhas das matrizes estao em uso */
    int   sensibilidade = 0;  /* 1 = Esportivo, 2 = Normal, 3 = Seguro    */
    float atrito = 0.0f;      /* coeficiente de atrito da via             */
    int   opcao;

    float v_atual, v_frente, radar, lidar, camera, dist_esq, dist_dir;

    srand((unsigned int) time(NULL));

    printf("============================================================\n");
    printf("   SAFEDRIVE - Motor de decisao ADAS (Linguagem C)\n");
    printf("============================================================\n\n");

    /* O atrito precisa ser estritamente positivo: ele entra no denominador
     * da formula de distancia de frenagem (regra B) e atrito <= 0 geraria
     * divisao por zero ou uma distancia negativa sem sentido fisico. */
    do {
        printf("Informe o coeficiente de atrito da via (ex.: 0.70 para asfalto seco): ");
        scanf("%f", &atrito);
    } while (atrito <= 0.0f);

    do {
        printf("Informe a sensibilidade do ADAS (1-Esportivo, 2-Normal, 3-Seguro): ");
        scanf("%d", &sensibilidade);
    } while (sensibilidade < SENSIBILIDADE_ESPORTIVO || sensibilidade > SENSIBILIDADE_SEGURO);

    do {
        printf("\n------------------------ MENU SAFEDRIVE ------------------------\n");
        printf("1. Carregar dados iniciais (%d amostras aleatorias)\n", AMOSTRAS_CARGA_INICIAL);
        printf("2. Inserir nova amostra\n");
        printf("3. Processar e exibir relatorio de riscos\n");
        printf("4. Sair\n");
        printf("Amostras registradas: %d / %d\n", num_amostras, MAX_AMOSTRAS);
        printf("Escolha uma opcao: ");
        scanf("%d", &opcao);

        switch (opcao) {

            case 1:
                inicializar_matrizes(velocidades, sensores_frontais, sensores_laterais,
                                      AMOSTRAS_CARGA_INICIAL);
                num_amostras = AMOSTRAS_CARGA_INICIAL;
                printf("\n%d amostras iniciais foram carregadas com sucesso.\n", AMOSTRAS_CARGA_INICIAL);
                break;

            case 2:
                if (num_amostras >= MAX_AMOSTRAS) {
                    printf("\nAviso: limite maximo de %d amostras atingido.\n", MAX_AMOSTRAS);
                    break;
                }

                printf("\n-- Inserir nova amostra (instante atual) --\n");
                printf("Velocidade atual (km/h): ");
                scanf("%f", &v_atual);
                printf("Velocidade do veiculo a frente (km/h): ");
                scanf("%f", &v_frente);
                printf("Leitura do radar (m): ");
                scanf("%f", &radar);
                printf("Leitura do lidar (m): ");
                scanf("%f", &lidar);
                printf("Leitura da camera (m): ");
                scanf("%f", &camera);
                printf("Distancia ate a faixa esquerda (m): ");
                scanf("%f", &dist_esq);
                printf("Distancia ate a faixa direita (m): ");
                scanf("%f", &dist_dir);

                num_amostras = armazenar_amostra(velocidades, sensores_frontais, sensores_laterais,
                                                  num_amostras, v_atual, v_frente,
                                                  radar, lidar, camera, dist_esq, dist_dir);

                printf("\nAmostra registrada com sucesso (linha %d).\n", num_amostras);
                break;

            case 3:
                /* Aciona sequencialmente todas as funcoes de calculo e,
                 * por fim, a funcao de relatorio -- exatamente como pede
                 * o item 3 do menu no enunciado. */
                processar_fusao_sensores(sensores_frontais, processamento, num_amostras);
                processar_distancia_segura(velocidades, processamento, num_amostras,
                                            sensibilidade, atrito);
                processar_risco_frontal(velocidades, processamento, status, num_amostras);
                processar_assistente_faixa(velocidades, sensores_laterais, status, num_amostras);

                exibir_relatorio(velocidades, sensores_frontais, sensores_laterais,
                                  processamento, status, num_amostras);
                break;

            case 4:
                printf("\nEncerrando o simulador Safedrive. Ate logo!\n");
                break;

            default:
                printf("\nOpcao invalida. Tente novamente.\n");
                break;
        }
    } while (opcao != 4);

    return 0;
}
