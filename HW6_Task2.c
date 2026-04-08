#include <float.h>
#include <math.h>
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

static double min_distance_reduction(const double *a, size_t n) {
    double min_diff = DBL_MAX;

    #pragma omp parallel for reduction(min:min_diff)
    for (size_t i = 1; i < n; ++i) {
        double diff = fabs(a[i] - a[i - 1]);
        if (diff < min_diff) min_diff = diff;
    }

    return min_diff;
}

int main(int argc, char **argv) {
    size_t n = DEFAULT_N;
    if (argc > 1) {
        n = (size_t)strtoull(argv[1], NULL, 10);
    }
    if (n < 2) {
        fprintf(stderr, "N must be at least 2\n");
        return 1;
    }

    double *a = (double *)malloc(n * sizeof(double));
    if (!a) {
        fprintf(stderr, "Failed to allocate array of size %zu\n", n);
        return 1;
    }

    fill_data(a, n, 2026u);

    double t0 = now_sec();
    double min_diff = min_distance_reduction(a, n);
    double t1 = now_sec();

    printf("Task 2 - Global Minimum Distance\n");
    printf("N = %zu, threads = %d\n", n, omp_get_max_threads());
    printf("min |A[i]-A[i-1]| = %.17g\n", min_diff);
    printf("Time: %.6f sec\n", t1 - t0);

    free(a);
    return 0;
}
