CC     = gcc
CFLAGS = -O2 -fopenmp -Wall -Wextra

TARGETS = analyzer_seq \
          analyzer_par_atomic \
          analyzer_par_critical \
          analyzer_par_lock \
          analyzer_par_atomic_padded

.PHONY: all clean validate

all: $(TARGETS)

analyzer_seq: analyzer_seq.c hash_table.c hash_table.h
	$(CC) $(CFLAGS) analyzer_seq.c hash_table.c -o analyzer_seq

analyzer_par_atomic: analyzer_par_atomic.c hash_table.c hash_table.h
	$(CC) $(CFLAGS) analyzer_par_atomic.c hash_table.c -o analyzer_par_atomic

analyzer_par_critical: analyzer_par_critical.c hash_table.c hash_table.h
	$(CC) $(CFLAGS) analyzer_par_critical.c hash_table.c -o analyzer_par_critical

analyzer_par_lock: analyzer_par_lock.c hash_table.c hash_table.h
	$(CC) $(CFLAGS) analyzer_par_lock.c hash_table.c -o analyzer_par_lock

analyzer_par_atomic_padded: analyzer_par_atomic_padded.c hash_table_padded.c hash_table_padded.h
	$(CC) $(CFLAGS) analyzer_par_atomic_padded.c hash_table_padded.c -o analyzer_par_atomic_padded

clean:
	rm -f $(TARGETS) results.csv sorted_res.csv sorted_gab.csv

validate:
	sort results.csv > sorted_res.csv
	sort gabarito_distribuido.csv > sorted_gab.csv
	diff -s sorted_res.csv sorted_gab.csv
