#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_N 50000000ULL

static double now_sec(void) {
    return omp_get_wtime();
}

static void fill_data(double *a, size_t n, unsigned int seed) {
    srand(seed);
    for (size_t i = 0; i < n; ++i) {
        a[i] = (double)rand() / (double)RAND_MAX;
    }
}

static double find_max_parallel(const double *a, size_t n) {
    double maxv = a[0];

    #pragma omp parallel for reduction(max:maxv)
    for (size_t i = 0; i < n; ++i) {
        if (a[i] > maxv) maxv = a[i];
    }

    return maxv;
}

static double filtered_sum_parallel(const double *a, size_t n, double threshold, size_t *count_out) {
    double sum = 0.0;
    size_t count = 0;

    #pragma omp parallel for reduction(+:sum,count)
    for (size_t i = 0; i < n; ++i) {
        if (a[i] > threshold) {
            sum += a[i];
            count++;
        }
    }

    *count_out = count;
    return sum;
}

int main(int argc, char **argv) {
    size_t n = DEFAULT_N;
    if (argc > 1) {
        n = (size_t)strtoull(argv[1], NULL, 10);
    }
    if (n < 1) {
        fprintf(stderr, "N must be >= 1\n");
        return 1;
    }

    double *a = (double *)malloc(n * sizeof(double));
    if (!a) {
        fprintf(stderr, "Failed to allocate array of size %zu\n", n);
        return 1;
    }

    fill_data(a, n, 404u);

    double t0 = now_sec();
    double maxv = find_max_parallel(a, n);
    double threshold = 0.8 * maxv;
    size_t count = 0;
    double sum = filtered_sum_parallel(a, n, threshold, &count);
    double t1 = now_sec();

    printf("Task 3 - Parallel Filtered Sum\n");
    printf("N = %zu, threads = %d\n", n, omp_get_max_threads());
    printf("max(A) = %.17g\n", maxv);
    printf("T = 0.8 * max(A) = %.17g\n", threshold);
    printf("count(A[i] > T) = %zu\n", count);
    printf("sum(A[i] > T) = %.17g\n", sum);
    printf("Time (max + sum): %.6f sec\n", t1 - t0);

    free(a);
    return 0;
}
