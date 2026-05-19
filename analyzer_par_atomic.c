#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

#define TABLE_SIZE 131071
#define MAX_LINE   1024
#define MAX_URL    512

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

    FILE* f_size = fopen(argv[1], "r");
    if (!f_size) {
        perror("log");
        ht_destroy(ht);
        return EXIT_FAILURE;
    }
    fseek(f_size, 0, SEEK_END);
    long file_size = ftell(f_size);
    fclose(f_size);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int nthreads_actual = omp_get_num_threads();

        long chunk = file_size / nthreads_actual;
        long start = chunk * tid;
        long end = (tid == nthreads_actual - 1) ? file_size : start + chunk;

        FILE* f_log = fopen(argv[1], "r");  // cada thread tem seu próprio FILE*
        if (f_log) {
            fseek(f_log, start, SEEK_SET);

            if (tid != 0) {
                char aux[MAX_LINE];
                fgets(aux, sizeof(aux), f_log); // descarta linha cortada no meio
            }

            char local_linha[MAX_LINE];
            char url[MAX_URL];

            while (ftell(f_log) < end && fgets(local_linha, sizeof(local_linha), f_log)) {
                char* inicio = strstr(local_linha, "GET ");
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
                    #pragma omp atomic update
                    node->hit_count++;
                }
            }
            fclose(f_log);
        }
    }

    ht_save_results(ht, "results.csv");
    ht_destroy(ht);
    return EXIT_SUCCESS;
}
