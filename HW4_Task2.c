#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

typedef struct {
    unsigned char *buf;
    size_t start;
    size_t end;
} ThreadArg;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void fill_buffer(unsigned char *buf, size_t n) {
    const char *chars =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        " !?.:,;-_@#$%&*()[]{}";
    size_t m = strlen(chars);

    srand(777);
    for (size_t i = 0; i < n; ++i) {
        buf[i] = (unsigned char)chars[(unsigned)rand() % m];
    }
}

static void upper_scalar(unsigned char *buf, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        if (buf[i] >= 'a' && buf[i] <= 'z') buf[i] = (unsigned char)(buf[i] - 32);
    }
}

static void upper_simd(unsigned char *buf, size_t start, size_t end) {
    size_t i = start;

#if defined(__AVX2__)
    const __m256i a1 = _mm256_set1_epi8('a' - 1);
    const __m256i z1 = _mm256_set1_epi8('z' + 1);
    const __m256i d = _mm256_set1_epi8(32);

    for (; i + 32 <= end; i += 32) {
        __m256i v = _mm256_loadu_si256((const __m256i *)(buf + i));
        __m256i ge_a = _mm256_cmpgt_epi8(v, a1);
        __m256i le_z = _mm256_cmpgt_epi8(z1, v);
        __m256i mask = _mm256_and_si256(ge_a, le_z);
        __m256i delta = _mm256_and_si256(mask, d);
        __m256i out = _mm256_sub_epi8(v, delta);
        _mm256_storeu_si256((__m256i *)(buf + i), out);
    }
#endif

    for (; i < end; ++i) {
        if (buf[i] >= 'a' && buf[i] <= 'z') buf[i] = (unsigned char)(buf[i] - 32);
    }
}

static void *worker_mt(void *p) {
    ThreadArg *a = (ThreadArg *)p;
    upper_scalar(a->buf, a->start, a->end);
    return NULL;
}

static void *worker_simd_mt(void *p) {
    ThreadArg *a = (ThreadArg *)p;
    upper_simd(a->buf, a->start, a->end);
    return NULL;
}

static void run_threads(unsigned char *buf, size_t n, int threads, int use_simd) {
    pthread_t *tid = (pthread_t *)malloc((size_t)threads * sizeof(pthread_t));
    ThreadArg *args = (ThreadArg *)malloc((size_t)threads * sizeof(ThreadArg));
    if (!tid || !args) {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }

    size_t chunk = n / (size_t)threads;
    for (int i = 0; i < threads; ++i) {
        args[i].buf = buf;
        args[i].start = (size_t)i * chunk;
        args[i].end = (i == threads - 1) ? n : args[i].start + chunk;
        if (use_simd) pthread_create(&tid[i], NULL, worker_simd_mt, &args[i]);
        else pthread_create(&tid[i], NULL, worker_mt, &args[i]);
    }
    for (int i = 0; i < threads; ++i) pthread_join(tid[i], NULL);

    free(tid);
    free(args);
}

int main(int argc, char **argv) {
    size_t mb = 256;
    int threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (threads < 1) threads = 4;

    if (argc > 1) mb = (size_t)strtoull(argv[1], NULL, 10);
    if (argc > 2) threads = atoi(argv[2]);
    if (threads < 1) threads = 1;

    size_t n = mb * 1024ull * 1024ull;

    unsigned char *src = (unsigned char *)malloc(n);
    unsigned char *ref = (unsigned char *)malloc(n);
    unsigned char *mt = (unsigned char *)malloc(n);
    unsigned char *simd = (unsigned char *)malloc(n);
    unsigned char *simd_mt = (unsigned char *)malloc(n);
    if (!src || !ref || !mt || !simd || !simd_mt) {
        fprintf(stderr, "Failed to allocate buffers\n");
        return 1;
    }

    fill_buffer(src, n);
    memcpy(ref, src, n);
    memcpy(mt, src, n);
    memcpy(simd, src, n);
    memcpy(simd_mt, src, n);

    upper_scalar(ref, 0, n);

    double t0, t1;

    t0 = now_sec();
    run_threads(mt, n, threads, 0);
    t1 = now_sec();
    double t_mt = t1 - t0;

    t0 = now_sec();
    upper_simd(simd, 0, n);
    t1 = now_sec();
    double t_simd = t1 - t0;

    t0 = now_sec();
    run_threads(simd_mt, n, threads, 1);
    t1 = now_sec();
    double t_simd_mt = t1 - t0;

    int ok = (memcmp(ref, mt, n) == 0) && (memcmp(ref, simd, n) == 0) && (memcmp(ref, simd_mt, n) == 0);

    printf("Buffer size: %zu MB\n", mb);
    printf("Threads used: %d\n\n", threads);
    printf("Multithreading time:      %.6f sec\n", t_mt);
    printf("SIMD time:                %.6f sec\n", t_simd);
    printf("SIMD + Multithreading:    %.6f sec\n", t_simd_mt);
    printf("\nVerification: %s\n", ok ? "PASSED" : "FAILED");

    free(src);
    free(ref);
    free(mt);
    free(simd);
    free(simd_mt);
    return ok ? 0 : 1;
}
