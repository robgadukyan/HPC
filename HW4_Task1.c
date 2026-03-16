#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

typedef struct {
    uint64_t a;
    uint64_t c;
    uint64_t g;
    uint64_t t;
} Counts;

typedef struct {
    const unsigned char *dna;
    size_t start;
    size_t end;
    Counts local;
} ThreadArg;

static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;
static Counts global_counts = {0, 0, 0, 0};

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void count_scalar_range(const unsigned char *dna, size_t start, size_t end, Counts *out) {
    out->a = out->c = out->g = out->t = 0;
    for (size_t i = start; i < end; ++i) {
        unsigned char ch = dna[i];
        if (ch == 'A') out->a++;
        else if (ch == 'C') out->c++;
        else if (ch == 'G') out->g++;
        else if (ch == 'T') out->t++;
    }
}

static void count_simd_range(const unsigned char *dna, size_t start, size_t end, Counts *out) {
    out->a = out->c = out->g = out->t = 0;
    size_t i = start;

#if defined(__AVX2__)
    const __m256i vA = _mm256_set1_epi8('A');
    const __m256i vC = _mm256_set1_epi8('C');
    const __m256i vG = _mm256_set1_epi8('G');
    const __m256i vT = _mm256_set1_epi8('T');

    for (; i + 32 <= end; i += 32) {
        __m256i v = _mm256_loadu_si256((const __m256i *)(dna + i));
        out->a += (uint64_t)__builtin_popcount((unsigned)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, vA)));
        out->c += (uint64_t)__builtin_popcount((unsigned)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, vC)));
        out->g += (uint64_t)__builtin_popcount((unsigned)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, vG)));
        out->t += (uint64_t)__builtin_popcount((unsigned)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, vT)));
    }
#endif

    for (; i < end; ++i) {
        unsigned char ch = dna[i];
        if (ch == 'A') out->a++;
        else if (ch == 'C') out->c++;
        else if (ch == 'G') out->g++;
        else if (ch == 'T') out->t++;
    }
}

static void *worker_mt(void *p) {
    ThreadArg *arg = (ThreadArg *)p;
    Counts local;
    count_scalar_range(arg->dna, arg->start, arg->end, &local);

    pthread_mutex_lock(&counts_mutex);
    global_counts.a += local.a;
    global_counts.c += local.c;
    global_counts.g += local.g;
    global_counts.t += local.t;
    pthread_mutex_unlock(&counts_mutex);
    return NULL;
}

static void *worker_simd_mt(void *p) {
    ThreadArg *arg = (ThreadArg *)p;
    count_simd_range(arg->dna, arg->start, arg->end, &arg->local);
    return NULL;
}

static Counts run_multithread(const unsigned char *dna, size_t n, int threads) {
    pthread_t *tid = (pthread_t *)malloc((size_t)threads * sizeof(pthread_t));
    ThreadArg *args = (ThreadArg *)malloc((size_t)threads * sizeof(ThreadArg));
    if (!tid || !args) {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }

    global_counts.a = global_counts.c = global_counts.g = global_counts.t = 0;

    size_t chunk = n / (size_t)threads;
    for (int i = 0; i < threads; ++i) {
        args[i].dna = dna;
        args[i].start = (size_t)i * chunk;
        args[i].end = (i == threads - 1) ? n : args[i].start + chunk;
        pthread_create(&tid[i], NULL, worker_mt, &args[i]);
    }
    for (int i = 0; i < threads; ++i) pthread_join(tid[i], NULL);

    free(tid);
    free(args);
    return global_counts;
}

static Counts run_simd_mt(const unsigned char *dna, size_t n, int threads) {
    pthread_t *tid = (pthread_t *)malloc((size_t)threads * sizeof(pthread_t));
    ThreadArg *args = (ThreadArg *)calloc((size_t)threads, sizeof(ThreadArg));
    if (!tid || !args) {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }

    size_t chunk = n / (size_t)threads;
    for (int i = 0; i < threads; ++i) {
        args[i].dna = dna;
        args[i].start = (size_t)i * chunk;
        args[i].end = (i == threads - 1) ? n : args[i].start + chunk;
        pthread_create(&tid[i], NULL, worker_simd_mt, &args[i]);
    }
    for (int i = 0; i < threads; ++i) pthread_join(tid[i], NULL);

    Counts total = {0, 0, 0, 0};
    for (int i = 0; i < threads; ++i) {
        total.a += args[i].local.a;
        total.c += args[i].local.c;
        total.g += args[i].local.g;
        total.t += args[i].local.t;
    }

    free(tid);
    free(args);
    return total;
}

static int same_counts(const Counts *x, const Counts *y) {
    return x->a == y->a && x->c == y->c && x->g == y->g && x->t == y->t;
}

int main(int argc, char **argv) {
    size_t mb = 256;
    int threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (threads < 1) threads = 4;

    if (argc > 1) mb = (size_t)strtoull(argv[1], NULL, 10);
    if (argc > 2) threads = atoi(argv[2]);
    if (threads < 1) threads = 1;

    size_t n = mb * 1024ull * 1024ull;
    unsigned char *dna = (unsigned char *)malloc(n);
    if (!dna) {
        fprintf(stderr, "Failed to allocate %zu bytes\n", n);
        return 1;
    }

    srand(12345);
    for (size_t i = 0; i < n; ++i) {
        unsigned r = (unsigned)rand() & 3u;
        dna[i] = (r == 0) ? 'A' : (r == 1) ? 'C' : (r == 2) ? 'G' : 'T';
    }

    Counts scalar, mt, simd, simd_mt;
    double t0, t1;

    t0 = now_sec();
    count_scalar_range(dna, 0, n, &scalar);
    t1 = now_sec();
    double t_scalar = t1 - t0;

    t0 = now_sec();
    mt = run_multithread(dna, n, threads);
    t1 = now_sec();
    double t_mt = t1 - t0;

    t0 = now_sec();
    count_simd_range(dna, 0, n, &simd);
    t1 = now_sec();
    double t_simd = t1 - t0;

    t0 = now_sec();
    simd_mt = run_simd_mt(dna, n, threads);
    t1 = now_sec();
    double t_simd_mt = t1 - t0;

    printf("DNA size: %zu MB\n", mb);
    printf("Threads used: %d\n\n", threads);
    printf("Counts (A C G T):\n");
    printf("%llu %llu %llu %llu\n\n",
           (unsigned long long)simd_mt.a,
           (unsigned long long)simd_mt.c,
           (unsigned long long)simd_mt.g,
           (unsigned long long)simd_mt.t);

    printf("Scalar time:                %.6f sec\n", t_scalar);
    printf("Multithreading time:        %.6f sec\n", t_mt);
    printf("SIMD time:                  %.6f sec\n", t_simd);
    printf("SIMD + Multithreading time: %.6f sec\n", t_simd_mt);

    int ok = same_counts(&scalar, &mt) && same_counts(&scalar, &simd) && same_counts(&scalar, &simd_mt);
    printf("\nVerification: %s\n", ok ? "PASSED" : "FAILED");

    free(dna);
    pthread_mutex_destroy(&counts_mutex);
    return ok ? 0 : 1;
}
