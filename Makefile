CC     = gcc
CFLAGS = -O2 -fopenmp -Wall -Wextra

TARGETS = analyzer_seq \
          analyzer_par_atomic \
          analyzer_par_critical \
          analyzer_par_lock \
          analyzer_par_atomic_padded

LOG_DIST = log_distribuido.txt
LOG_CONC = log_concorrente.txt

.PHONY: all clean validate \
        perf-ipc-dist perf-ipc-conc \
        perf-cache-atomic perf-cache-padded \
        valgrind-atomic valgrind-padded \
        time-critical time-atomic time-lock

# -------------------------------------------------------
# Compilação
# -------------------------------------------------------

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

# -------------------------------------------------------
# Validação (Seção 7 do enunciado)
# -------------------------------------------------------

validate: analyzer_seq
	./analyzer_seq $(LOG_DIST)
	sort results.csv > sorted_res.csv
	sort gabarito_distribuido.csv > sorted_gab.csv
	diff -s sorted_res.csv sorted_gab.csv

# -------------------------------------------------------
# Experimento A — Escalabilidade com perf (Seção 8.3)
# Mede IPC (instructions + cycles) no log distribuído
# -------------------------------------------------------

perf-ipc-dist: analyzer_par_atomic
	@echo "=== 1 thread ==="
	OMP_NUM_THREADS=1 perf stat -e instructions,cycles \
		./analyzer_par_atomic $(LOG_DIST) 1
	@echo "=== 2 threads ==="
	OMP_NUM_THREADS=2 perf stat -e instructions,cycles \
		./analyzer_par_atomic $(LOG_DIST) 2
	@echo "=== 4 threads ==="
	OMP_NUM_THREADS=4 perf stat -e instructions,cycles \
		./analyzer_par_atomic $(LOG_DIST) 4
	@echo "=== 8 threads ==="
	OMP_NUM_THREADS=8 perf stat -e instructions,cycles \
		./analyzer_par_atomic $(LOG_DIST) 8

# -------------------------------------------------------
# Experimento B — Contenção com perf (Seção 8.5)
# Mede IPC no log concorrente (hotspots)
# -------------------------------------------------------

perf-ipc-conc: analyzer_par_atomic analyzer_par_critical analyzer_par_lock
	@echo "=== atomic ==="
	perf stat -e instructions,cycles \
		./analyzer_par_atomic $(LOG_CONC) 8
	@echo "=== critical ==="
	perf stat -e instructions,cycles \
		./analyzer_par_critical $(LOG_CONC) 8
	@echo "=== lock ==="
	perf stat -e instructions,cycles \
		./analyzer_par_lock $(LOG_CONC) 8

# -------------------------------------------------------
# Experimento B — System time (Seção 8.5)
# -------------------------------------------------------

time-critical: analyzer_par_critical
	/usr/bin/time -v ./analyzer_par_critical $(LOG_CONC) 8

time-atomic: analyzer_par_atomic
	/usr/bin/time -v ./analyzer_par_atomic $(LOG_CONC) 8

time-lock: analyzer_par_lock
	/usr/bin/time -v ./analyzer_par_lock $(LOG_CONC) 8

# -------------------------------------------------------
# Experimento C — False Sharing com perf (Seção 8.6)
# Compara cache-references e cache-misses: atomic vs padded
# -------------------------------------------------------

perf-cache-atomic: analyzer_par_atomic
	perf stat -e cache-references,cache-misses \
		./analyzer_par_atomic $(LOG_CONC) 8

perf-cache-padded: analyzer_par_atomic_padded
	perf stat -e cache-references,cache-misses \
		./analyzer_par_atomic_padded $(LOG_CONC) 8

# -------------------------------------------------------
# Experimento C — False Sharing com Valgrind/Cachegrind
# -------------------------------------------------------

valgrind-atomic: analyzer_par_atomic
	valgrind --tool=cachegrind \
		./analyzer_par_atomic $(LOG_CONC) 8

valgrind-padded: analyzer_par_atomic_padded
	valgrind --tool=cachegrind \
		./analyzer_par_atomic_padded $(LOG_CONC) 8

# -------------------------------------------------------
# Limpeza
# -------------------------------------------------------

clean:
	rm -f $(TARGETS) results.csv sorted_res.csv sorted_gab.csv cachegrind.out.*
