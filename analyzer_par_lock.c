#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"

#define TABLE_SIZE 131071
#define MAX_LINE 1024
#define MAX_URL 512
#define MAX_LOG_LINES 10000000

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: %s <log_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    HashTable* ht = ht_create(TABLE_SIZE);

    FILE* f_manifest = fopen("manifest.txt", "r");
    if (!f_manifest) {
        perror("manifest.txt");
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

    char** log_entradas = malloc(MAX_LOG_LINES * sizeof(char*));
    if (!log_entradas) return EXIT_FAILURE;

    int total_linhas = 0;

    while (fgets(linha, sizeof(linha), f_log) &&
           total_linhas < MAX_LOG_LINES) {
        log_entradas[total_linhas++] = strdup(linha);
    }

    fclose(f_log);

    omp_lock_t* bucket_locks = malloc(TABLE_SIZE * sizeof(omp_lock_t));

    for (int i = 0; i < TABLE_SIZE; i++) {
        omp_init_lock(&bucket_locks[i]);
    }

    #pragma omp parallel for schedule(dynamic, 1024)
    for (int i = 0; i < total_linhas; i++) {

        char url[MAX_URL];

        char* linha_atual = log_entradas[i];
        char* inicio = strstr(linha_atual, "GET ");
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
            size_t bucket = (unsigned long)node % TABLE_SIZE; 

            omp_set_lock(&bucket_locks[bucket]);
            node->hit_count++;
            omp_unset_lock(&bucket_locks[bucket]);
        }

        free(log_entradas[i]);
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        omp_destroy_lock(&bucket_locks[i]);
    }

    free(bucket_locks);
    free(log_entradas);

    ht_save_results(ht, "results.csv");
    ht_destroy(ht);

    return EXIT_SUCCESS;
}
