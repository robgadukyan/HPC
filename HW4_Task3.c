#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if defined(__SSE2__)
#include <immintrin.h>
#endif

typedef struct {
    int w;
    int h;
    int maxv;
    unsigned char *rgb;
} Image;

typedef struct {
    const unsigned char *r;
    const unsigned char *g;
    const unsigned char *b;
    unsigned char *gray;
    size_t start;
    size_t end;
    int use_simd;
} ThreadArg;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int next_token(FILE *fp, char *buf, size_t cap) {
    int ch;

    do {
        ch = fgetc(fp);
        if (ch == '#') {
            while (ch != '\n' && ch != EOF) ch = fgetc(fp);
        }
    } while (ch != EOF && isspace(ch));

    if (ch == EOF) return 0;

    size_t i = 0;
    while (ch != EOF && !isspace(ch) && ch != '#') {
        if (i + 1 < cap) buf[i++] = (char)ch;
        ch = fgetc(fp);
    }
    buf[i] = '\0';

    if (ch == '#') {
        while (ch != '\n' && ch != EOF) ch = fgetc(fp);
    }

    return 1;
}

static int read_ppm(const char *path, Image *img) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;

    char tok[64];
    if (!next_token(fp, tok, sizeof(tok))) {
        fclose(fp);
        return 0;
    }

    int is_p6 = 0;
    if (strcmp(tok, "P6") == 0) is_p6 = 1;
    else if (strcmp(tok, "P3") == 0) is_p6 = 0;
    else {
        fclose(fp);
        return 0;
    }

    if (!next_token(fp, tok, sizeof(tok))) { fclose(fp); return 0; }
    img->w = atoi(tok);
    if (!next_token(fp, tok, sizeof(tok))) { fclose(fp); return 0; }
    img->h = atoi(tok);
    if (!next_token(fp, tok, sizeof(tok))) { fclose(fp); return 0; }
    img->maxv = atoi(tok);

    if (img->w <= 0 || img->h <= 0 || img->maxv <= 0 || img->maxv > 255) {
        fclose(fp);
        return 0;
    }

    size_t bytes = (size_t)img->w * (size_t)img->h * 3u;
    img->rgb = (unsigned char *)malloc(bytes);
    if (!img->rgb) {
        fclose(fp);
        return 0;
    }

    if (is_p6) {
        if (fread(img->rgb, 1, bytes, fp) != bytes) {
            free(img->rgb);
            fclose(fp);
            return 0;
        }
    } else {
        for (size_t i = 0; i < bytes; ++i) {
            if (!next_token(fp, tok, sizeof(tok))) {
                free(img->rgb);
                fclose(fp);
                return 0;
            }
            int v = atoi(tok);
            if (v < 0) v = 0;
            if (v > img->maxv) v = img->maxv;
            img->rgb[i] = (unsigned char)v;
        }
    }

    fclose(fp);
    return 1;
}

static int write_p6(const char *path, int w, int h, int maxv, const unsigned char *rgb) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;

    fprintf(fp, "P6\n%d %d\n%d\n", w, h, maxv);
    size_t bytes = (size_t)w * (size_t)h * 3u;
    int ok = (fwrite(rgb, 1, bytes, fp) == bytes);
    fclose(fp);
    return ok;
}

static inline unsigned char gray_pixel(unsigned char r, unsigned char g, unsigned char b) {
    unsigned int y = 77u * (unsigned int)r + 150u * (unsigned int)g + 29u * (unsigned int)b + 128u;
    return (unsigned char)(y >> 8);
}

static void split_rgb(const unsigned char *rgb, unsigned char *r, unsigned char *g, unsigned char *b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        r[i] = rgb[3 * i + 0];
        g[i] = rgb[3 * i + 1];
        b[i] = rgb[3 * i + 2];
    }
}

static void gray_to_rgb(const unsigned char *gray, unsigned char *rgb, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        unsigned char v = gray[i];
        rgb[3 * i + 0] = v;
        rgb[3 * i + 1] = v;
        rgb[3 * i + 2] = v;
    }
}

static void grayscale_scalar(const unsigned char *r, const unsigned char *g, const unsigned char *b,
                             unsigned char *gray, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        gray[i] = gray_pixel(r[i], g[i], b[i]);
    }
}

static void grayscale_simd(const unsigned char *r, const unsigned char *g, const unsigned char *b,
                           unsigned char *gray, size_t start, size_t end) {
    size_t i = start;

#if defined(__SSE2__)
    const __m128i z = _mm_setzero_si128();
    const __m128i kr = _mm_set1_epi16(77);
    const __m128i kg = _mm_set1_epi16(150);
    const __m128i kb = _mm_set1_epi16(29);
    const __m128i rnd = _mm_set1_epi16(128);

    for (; i + 16 <= end; i += 16) {
        __m128i vr = _mm_loadu_si128((const __m128i *)(r + i));
        __m128i vg = _mm_loadu_si128((const __m128i *)(g + i));
        __m128i vb = _mm_loadu_si128((const __m128i *)(b + i));

        __m128i r0 = _mm_unpacklo_epi8(vr, z);
        __m128i r1 = _mm_unpackhi_epi8(vr, z);
        __m128i g0 = _mm_unpacklo_epi8(vg, z);
        __m128i g1 = _mm_unpackhi_epi8(vg, z);
        __m128i b0 = _mm_unpacklo_epi8(vb, z);
        __m128i b1 = _mm_unpackhi_epi8(vb, z);

        __m128i y0 = _mm_add_epi16(_mm_mullo_epi16(r0, kr), _mm_mullo_epi16(g0, kg));
        y0 = _mm_add_epi16(y0, _mm_mullo_epi16(b0, kb));
        y0 = _mm_add_epi16(y0, rnd);
        y0 = _mm_srli_epi16(y0, 8);

        __m128i y1 = _mm_add_epi16(_mm_mullo_epi16(r1, kr), _mm_mullo_epi16(g1, kg));
        y1 = _mm_add_epi16(y1, _mm_mullo_epi16(b1, kb));
        y1 = _mm_add_epi16(y1, rnd);
        y1 = _mm_srli_epi16(y1, 8);

        __m128i y = _mm_packus_epi16(y0, y1);
        _mm_storeu_si128((__m128i *)(gray + i), y);
    }
#endif

    for (; i < end; ++i) {
        gray[i] = gray_pixel(r[i], g[i], b[i]);
    }
}

static void *worker(void *p) {
    ThreadArg *a = (ThreadArg *)p;
    if (a->use_simd) grayscale_simd(a->r, a->g, a->b, a->gray, a->start, a->end);
    else grayscale_scalar(a->r, a->g, a->b, a->gray, a->start, a->end);
    return NULL;
}

static void run_threads(const unsigned char *r, const unsigned char *g, const unsigned char *b,
                        unsigned char *gray, size_t n, int threads, int use_simd) {
    pthread_t *tid = (pthread_t *)malloc((size_t)threads * sizeof(pthread_t));
    ThreadArg *args = (ThreadArg *)malloc((size_t)threads * sizeof(ThreadArg));
    if (!tid || !args) {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }

    size_t chunk = n / (size_t)threads;
    for (int i = 0; i < threads; ++i) {
        args[i].r = r;
        args[i].g = g;
        args[i].b = b;
        args[i].gray = gray;
        args[i].start = (size_t)i * chunk;
        args[i].end = (i == threads - 1) ? n : args[i].start + chunk;
        args[i].use_simd = use_simd;
        pthread_create(&tid[i], NULL, worker, &args[i]);
    }
    for (int i = 0; i < threads; ++i) pthread_join(tid[i], NULL);

    free(tid);
    free(args);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s input.ppm [output.ppm] [threads]\n", argv[0]);
        return 1;
    }

    const char *in = argv[1];
    const char *out = (argc > 2) ? argv[2] : "gray_output.ppm";
    int threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (threads < 1) threads = 4;
    if (argc > 3) threads = atoi(argv[3]);
    if (threads < 1) threads = 1;

    Image img = {0, 0, 0, NULL};
    if (!read_ppm(in, &img)) {
        fprintf(stderr, "Failed to read PPM: %s\n", in);
        return 1;
    }

    size_t n = (size_t)img.w * (size_t)img.h;
    size_t rgb_bytes = n * 3u;

    unsigned char *r = (unsigned char *)malloc(n);
    unsigned char *g = (unsigned char *)malloc(n);
    unsigned char *b = (unsigned char *)malloc(n);
    unsigned char *gray_scalar_buf = (unsigned char *)malloc(n);
    unsigned char *gray_simd_buf = (unsigned char *)malloc(n);
    unsigned char *gray_mt_buf = (unsigned char *)malloc(n);
    unsigned char *gray_simd_mt_buf = (unsigned char *)malloc(n);
    unsigned char *out_rgb = (unsigned char *)malloc(rgb_bytes);

    if (!r || !g || !b || !gray_scalar_buf || !gray_simd_buf || !gray_mt_buf || !gray_simd_mt_buf || !out_rgb) {
        fprintf(stderr, "Allocation failed\n");
        free(img.rgb);
        free(r); free(g); free(b);
        free(gray_scalar_buf); free(gray_simd_buf); free(gray_mt_buf); free(gray_simd_mt_buf);
        free(out_rgb);
        return 1;
    }

    split_rgb(img.rgb, r, g, b, n);

    double t0, t1;

    t0 = now_sec();
    grayscale_scalar(r, g, b, gray_scalar_buf, 0, n);
    t1 = now_sec();
    double t_scalar = t1 - t0;

    t0 = now_sec();
    grayscale_simd(r, g, b, gray_simd_buf, 0, n);
    t1 = now_sec();
    double t_simd = t1 - t0;

    t0 = now_sec();
    run_threads(r, g, b, gray_mt_buf, n, threads, 0);
    t1 = now_sec();
    double t_mt = t1 - t0;

    t0 = now_sec();
    run_threads(r, g, b, gray_simd_mt_buf, n, threads, 1);
    t1 = now_sec();
    double t_simd_mt = t1 - t0;

    int ok = (memcmp(gray_scalar_buf, gray_simd_buf, n) == 0) &&
             (memcmp(gray_scalar_buf, gray_mt_buf, n) == 0) &&
             (memcmp(gray_scalar_buf, gray_simd_mt_buf, n) == 0);

    gray_to_rgb(gray_simd_mt_buf, out_rgb, n);
    if (!write_p6(out, img.w, img.h, img.maxv, out_rgb)) {
        fprintf(stderr, "Failed to write output: %s\n", out);
        free(img.rgb);
        free(r); free(g); free(b);
        free(gray_scalar_buf); free(gray_simd_buf); free(gray_mt_buf); free(gray_simd_mt_buf);
        free(out_rgb);
        return 1;
    }

    printf("Image size: %d x %d\n", img.w, img.h);
    printf("Threads used: %d\n\n", threads);
    printf("Scalar time:                %.6f sec\n", t_scalar);
    printf("SIMD time:                  %.6f sec\n", t_simd);
    printf("Multithreading time:        %.6f sec\n", t_mt);
    printf("Multithreading + SIMD time: %.6f sec\n", t_simd_mt);
    printf("\nVerification: %s\n", ok ? "PASSED" : "FAILED");
    printf("Output image: %s\n", out);

    free(img.rgb);
    free(r); free(g); free(b);
    free(gray_scalar_buf); free(gray_simd_buf); free(gray_mt_buf); free(gray_simd_mt_buf);
    free(out_rgb);

    return ok ? 0 : 1;
}
