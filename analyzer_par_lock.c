#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

#define TABLE_SIZE    131071
#define MAX_LINE      1024
#define MAX_URL       512
#define BATCH_SIZE    10000

// mesma função hash do hash_table.c
static size_t hash_djb2(const char* str, size_t size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % size;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Uso: %s <log_file> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int nthreads = atoi(argv[2]);
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

    // um lock por bucket
    omp_lock_t* bucket_locks = malloc(TABLE_SIZE * sizeof(omp_lock_t));
    for (int i = 0; i < TABLE_SIZE; i++)
        omp_init_lock(&bucket_locks[i]);

    FILE* f_log = fopen(argv[1], "r");
    if (!f_log) {
        perror("log");
        ht_destroy(ht);
        free(bucket_locks);
        return EXIT_FAILURE;
    }

    char** batch = malloc(BATCH_SIZE * sizeof(char*));
    for (int i = 0; i < BATCH_SIZE; i++)
        batch[i] = malloc(MAX_LINE);

    int count = 0;
    while (1) {
        count = 0;
        while (count < BATCH_SIZE && fgets(batch[count], MAX_LINE, f_log))
            count++;
        if (count == 0) break;

        #pragma omp parallel for schedule(dynamic, 256)
        for (int i = 0; i < count; i++) {
            char url[MAX_URL];
            char* inicio = strstr(batch[i], "GET ");
            if (!inicio) continue;
            inicio += 4;

            char* fim = strstr(inicio, " HTTP");
            if (!fim) continue;

            size_t len = (size_t)(fim - inicio);
            if (len >= MAX_URL) continue;

            strncpy(url, inicio, len);
            url[len] = '\0';

            CacheNode* node = ht_get(ht, url);
            if (node) {
                size_t bucket = hash_djb2(url, TABLE_SIZE); // bucket correto
                omp_set_lock(&bucket_locks[bucket]);
                node->hit_count++;
                omp_unset_lock(&bucket_locks[bucket]);
            }
        }
    }

    for (int i = 0; i < BATCH_SIZE; i++) free(batch[i]);
    free(batch);
    fclose(f_log);

    for (int i = 0; i < TABLE_SIZE; i++)
        omp_destroy_lock(&bucket_locks[i]);
    free(bucket_locks);

    ht_save_results(ht, "results.csv");
    ht_destroy(ht);
    return EXIT_SUCCESS;
}    if (!f_log) {
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
