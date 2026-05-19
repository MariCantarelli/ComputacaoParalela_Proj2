#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "hash_table.h"

#define TABLE_SIZE    131071
#define MAX_LINE      1024
#define MAX_URL       512
#define MAX_LOG_LINES 10000000

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Uso: %s <log_file> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int nthreads = atoi(argv[2]);
    if (nthreads < 1) nthreads = 1;
    omp_set_num_threads(nthreads);

    HashTable* ht = ht_create(TABLE_SIZE);

    FILE* f_manifest = fopen("manifest.txt", "r");
    if (!f_manifest) {
        perror("manifest.txt");
        ht_destroy(ht);
        return EXIT_FAILURE;
    }
    char linha[MAX_LINE];
    while (fgets(linha, sizeof(linha), f_manifest)) {
        linha[strcspn(linha, "\n")] = '\0';
        ht_put(ht, linha);
    }
    fclose(f_manifest);

    FILE* f_log = fopen(argv[1], "r");
    if (!f_log) {
        perror("log");
        ht_destroy(ht);
        return EXIT_FAILURE;
    }

    char** linhas = malloc(MAX_LOG_LINES * sizeof(char*));
    if (!linhas) {
        ht_destroy(ht);
        return EXIT_FAILURE;
    }
    long total_linhas = 0;
    while (fgets(linha, sizeof(linha), f_log) && total_linhas < MAX_LOG_LINES) {
        linhas[total_linhas++] = strdup(linha);
    }
    fclose(f_log);

    #pragma omp parallel for schedule(dynamic, 1024)
    for (long i = 0; i < total_linhas; i++) {
        char url[MAX_URL];
        char* inicio = strstr(linhas[i], "GET ");
        if (!inicio) continue;
        inicio += 4;

        char* fim = strstr(inicio, " HTTP");
        if (!fim) continue;

        size_t len = fim - inicio;
        if (len >= MAX_URL) continue;

        strncpy(url, inicio, len);
        url[len] = '\0';

        CacheNode* node = ht_get(ht, url);
        if (node) {
            #pragma omp critical
            {
                node->hit_count++;
            }
        }
    }

    for (long i = 0; i < total_linhas; i++) {
        free(linhas[i]);
    }
    free(linhas);

    ht_save_results(ht, "results.csv");
    printf("Resultados salvos em results.csv\n");
    ht_destroy(ht);
    return EXIT_SUCCESS;
}
