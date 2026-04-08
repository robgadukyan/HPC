#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_N 100000000ULL

static double now_sec(void) {
    return omp_get_wtime();
}

static void fill_data(unsigned char *a, size_t n, unsigned int seed) {
    srand(seed);
    for (size_t i = 0; i < n; ++i) {
        a[i] = (unsigned char)(rand() % 256);
    }
}

static long long sum_hist(const long long hist[256]) {
    long long s = 0;
    for (int i = 0; i < 256; ++i) s += hist[i];
    return s;
}

static void hist_naive_race(const unsigned char *a, size_t n, long long hist[256]) {
    memset(hist, 0, 256 * sizeof(long long));

    #pragma omp parallel for
    for (size_t i = 0; i < n; ++i) {
        hist[a[i]]++;
    }
}

static void hist_critical(const unsigned char *a, size_t n, long long hist[256]) {
    memset(hist, 0, 256 * sizeof(long long));

    #pragma omp parallel for
    for (size_t i = 0; i < n; ++i) {
        int v = (int)a[i];
        #pragma omp critical
        {
            hist[v]++;
        }
    }
}

static void hist_reduction(const unsigned char *a, size_t n, long long hist[256]) {
    memset(hist, 0, 256 * sizeof(long long));

    #pragma omp parallel for reduction(+:hist[:256])
    for (size_t i = 0; i < n; ++i) {
        hist[a[i]]++;
    }
}

int main(int argc, char **argv) {
    size_t n = DEFAULT_N;
    if (argc > 1) {
        n = (size_t)strtoull(argv[1], NULL, 10);
    }

    unsigned char *a = (unsigned char *)malloc(n * sizeof(unsigned char));
    if (!a) {
        fprintf(stderr, "Failed to allocate array of size %zu\n", n);
        return 1;
    }

    fill_data(a, n, 12345u);

    long long h_naive[256], h_critical[256], h_reduction[256];
    double t0, t1;

    t0 = now_sec();
    hist_naive_race(a, n, h_naive);
    t1 = now_sec();
    double t_naive = t1 - t0;

    t0 = now_sec();
    hist_critical(a, n, h_critical);
    t1 = now_sec();
    double t_critical = t1 - t0;

    t0 = now_sec();
    hist_reduction(a, n, h_reduction);
    t1 = now_sec();
    double t_reduction = t1 - t0;

    long long s_naive = sum_hist(h_naive);
    long long s_critical = sum_hist(h_critical);
    long long s_reduction = sum_hist(h_reduction);

    printf("Task 1 - Parallel Histogram\n");
    printf("N = %zu, threads = %d\n\n", n, omp_get_max_threads());
    printf("Naive (race)     : %.6f sec, total = %lld\n", t_naive, s_naive);
    printf("With critical    : %.6f sec, total = %lld\n", t_critical, s_critical);
    printf("With reduction   : %.6f sec, total = %lld\n", t_reduction, s_reduction);
    printf("Expected total   : %zu\n", n);

    free(a);
    return 0;
}
