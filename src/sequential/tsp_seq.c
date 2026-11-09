#define _POSIX_C_SOURCE 199309L

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_POINTS 4096
#define EARTH_RADIUS_KM 6371.0088
#define PI 3.14159265358979323846

typedef struct {
    int id;
    char name[128];
    double lat;
    double lon;
} Point;

static double now_seconds(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static double deg2rad(double degrees)
{
    return degrees * PI / 180.0;
}

static double haversine_km(const Point *a, const Point *b)
{
    const double lat1 = deg2rad(a->lat);
    const double lat2 = deg2rad(b->lat);
    const double dlat = lat2 - lat1;
    const double dlon = deg2rad(b->lon - a->lon);
    const double sin_dlat = sin(dlat / 2.0);
    const double sin_dlon = sin(dlon / 2.0);
    double h = sin_dlat * sin_dlat +
               cos(lat1) * cos(lat2) * sin_dlon * sin_dlon;

    if (h > 1.0) {
        h = 1.0;
    }

    return 2.0 * EARTH_RADIUS_KM * asin(sqrt(h));
}

static void trim(char *s)
{
    size_t len = strlen(s);
    size_t start = 0;

    while (len > 0) {
        char c = s[len - 1];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            break;
        }
        s[--len] = '\0';
    }

    while (s[start] == ' ' || s[start] == '\t') {
        start++;
    }

    if (start > 0) {
        memmove(s, s + start, strlen(s + start) + 1);
    }
}

static int load_csv(const char *path, Point *points, int max_points)
{
    FILE *fp = fopen(path, "r");
    char line[512];
    int count = 0;

    if (fp == NULL) {
        fprintf(stderr, "ERROR: no se pudo abrir '%s': %s\n",
                path, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        int id;
        char name[128];
        double lat;
        double lon;

        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        if (sscanf(line, "%d,%127[^,],%lf,%lf", &id, name, &lat, &lon) != 4) {
            continue;
        }

        if (count >= max_points) {
            fprintf(stderr, "ERROR: se excedio el maximo de %d puntos\n",
                    max_points);
            fclose(fp);
            return -1;
        }

        trim(name);
        points[count].id = id;
        snprintf(points[count].name, sizeof(points[count].name), "%s", name);
        points[count].lat = lat;
        points[count].lon = lon;
        count++;
    }

    fclose(fp);
    return count;
}

static double *build_distance_matrix(const Point *points, int n)
{
    double *dist = malloc((size_t)n * (size_t)n * sizeof(double));

    if (dist == NULL) {
        fprintf(stderr, "ERROR: memoria insuficiente para la matriz de distancias\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        dist[(size_t)i * n + i] = 0.0;
        for (int j = i + 1; j < n; j++) {
            double d = haversine_km(&points[i], &points[j]);
            dist[(size_t)i * n + j] = d;
            dist[(size_t)j * n + i] = d;
        }
    }

    return dist;
}

static void nearest_neighbor(const double *dist, int n, int *tour)
{
    unsigned char *visited = calloc((size_t)n, sizeof(unsigned char));
    int current = 0;

    if (visited == NULL) {
        fprintf(stderr, "ERROR: memoria insuficiente para visited\n");
        exit(EXIT_FAILURE);
    }

    visited[0] = 1;
    tour[0] = 0;

    for (int step = 1; step < n; step++) {
        int best = -1;
        double best_dist = INFINITY;

        for (int j = 0; j < n; j++) {
            if (!visited[j]) {
                double d = dist[(size_t)current * n + j];
                if (d < best_dist) {
                    best_dist = d;
                    best = j;
                }
            }
        }

        visited[best] = 1;
        tour[step] = best;
        current = best;
    }

    free(visited);
}

static double tour_length(const double *dist, int n, const int *tour)
{
    double total = 0.0;

    for (int i = 0; i < n; i++) {
        int from = tour[i];
        int to = tour[(i + 1) % n];
        total += dist[(size_t)from * n + to];
    }

    return total;
}

static int two_opt(const double *dist, int n, int *tour)
{
    int improved = 1;
    int passes = 0;
    const int max_passes = 10000;

    while (improved && passes < max_passes) {
        improved = 0;
        passes++;

        for (int i = 0; i < n - 1; i++) {
            for (int k = i + 1; k < n; k++) {
                int a;
                int b;
                int c;
                int d;
                double before;
                double after;

                if (i == 0 && k == n - 1) {
                    continue;
                }

                a = tour[i];
                b = tour[(i + 1) % n];
                c = tour[k];
                d = tour[(k + 1) % n];

                before = dist[(size_t)a * n + b] + dist[(size_t)c * n + d];
                after = dist[(size_t)a * n + c] + dist[(size_t)b * n + d];

                if (after + 1e-12 < before) {
                    int left = i + 1;
                    int right = k;

                    while (left < right) {
                        int tmp = tour[left];
                        tour[left] = tour[right];
                        tour[right] = tmp;
                        left++;
                        right--;
                    }

                    improved = 1;
                }
            }
        }
    }

    return passes;
}

int main(int argc, char **argv)
{
    const char *csv_path = (argc > 1) ? argv[1] : "data/puntos_yacuiba.csv";
    Point *points;
    double *dist;
    int *tour;
    int n;
    int passes;
    double total_km;
    double t_start;
    double t_end;
    double t1;

    points = calloc(MAX_POINTS, sizeof(Point));
    if (points == NULL) {
        fprintf(stderr, "ERROR: memoria insuficiente para los puntos\n");
        return EXIT_FAILURE;
    }

    n = load_csv(csv_path, points, MAX_POINTS);
    if (n < 0) {
        free(points);
        return EXIT_FAILURE;
    }

    if (n < 2) {
        fprintf(stderr, "ERROR: se necesitan al menos 2 puntos\n");
        free(points);
        return EXIT_FAILURE;
    }

    t_start = now_seconds();

    dist = build_distance_matrix(points, n);

    tour = malloc((size_t)n * sizeof(int));
    if (tour == NULL) {
        fprintf(stderr, "ERROR: memoria insuficiente para la ruta\n");
        free(dist);
        free(points);
        return EXIT_FAILURE;
    }

    nearest_neighbor(dist, n, tour);
    passes = two_opt(dist, n, tour);
    total_km = tour_length(dist, n, tour);
    t_end = now_seconds();
    t1 = t_end - t_start;

    printf("============================================================\n");
    printf(" TSP secuencial - Recoleccion de residuos solidos (Yacuiba)\n");
    printf("============================================================\n");
    printf(" Archivo de datos  : %s\n", csv_path);
    printf(" Puntos cargados   : %d\n", n);
    printf(" Metodo            : vecino mas cercano + 2-opt\n");
    printf("------------------------------------------------------------\n");
    printf(" Ruta encontrada (deposito -> ... -> deposito):\n");

    for (int i = 0; i < n; i++) {
        int idx = tour[i];
        printf("   [%2d] id=%-3d %-32s (%.6f, %.6f)\n",
               i, points[idx].id, points[idx].name,
               points[idx].lat, points[idx].lon);
    }

    printf("   [%2d] regreso al deposito (id=%d)\n", n, points[tour[0]].id);
    printf("------------------------------------------------------------\n");
    printf(" Distancia total   : %.6f km\n", total_km);
    printf(" Pasadas 2-opt     : %d\n", passes);
    printf(" T1 (secuencial)   : %.9f s\n", t1);
    printf(" T1 (milisegundos) : %.6f ms\n", t1 * 1e3);
    printf("============================================================\n");

    free(tour);
    free(dist);
    free(points);

    return EXIT_SUCCESS;
}
