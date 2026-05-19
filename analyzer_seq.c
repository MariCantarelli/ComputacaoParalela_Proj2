#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"

#define TABLE_SIZE 131071
#define MAX_LINE 1024
#define MAX_URL 512

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
        linha[strcspn(linha, "\n")] = '\0'; // tira o \n do final da linha
        ht_put(ht, linha); //coloca a URL na tabela hash
    }

    fclose(f_manifest);

    FILE* f_log = fopen(argv[1], "r");

    if (!f_log) {
        perror("log");
        ht_destroy(ht);
        return EXIT_FAILURE;
    }

    char url[MAX_URL];

    while (fgets(linha, sizeof(linha), f_log)) {
        char* inicio = strstr(linha, "GET ");

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
            node->hit_count++;
        }
    }

    fclose(f_log);
    ht_save_results(ht, "results.csv");
    ht_destroy(ht);
    return EXIT_SUCCESS;
}
