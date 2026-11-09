#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int rank = 0;
    int size = 1;
    int name_len = 0;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    char *all_hosts = NULL;
    int rank_sum = 0;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    memset(hostname, 0, sizeof(hostname));
    MPI_Get_processor_name(hostname, &name_len);
    hostname[MPI_MAX_PROCESSOR_NAME - 1] = '\0';

    printf("[MPI] rank %d/%d | host=%s | pid=%ld\n",
           rank, size, hostname, (long)getpid());
    fflush(stdout);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        all_hosts = calloc((size_t)size, MPI_MAX_PROCESSOR_NAME);
        if (all_hosts == NULL) {
            fprintf(stderr, "ERROR: sin memoria para recolectar hostnames\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    MPI_Gather(hostname, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
               all_hosts, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
               0, MPI_COMM_WORLD);

    MPI_Allreduce(&rank, &rank_sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (rank == 0) {
        int unique_hosts = 0;

        for (int i = 0; i < size; i++) {
            const char *h = all_hosts + (size_t)i * MPI_MAX_PROCESSOR_NAME;
            int seen = 0;

            for (int j = 0; j < i; j++) {
                const char *prev = all_hosts + (size_t)j * MPI_MAX_PROCESSOR_NAME;
                if (strcmp(h, prev) == 0) {
                    seen = 1;
                    break;
                }
            }

            if (!seen) {
                unique_hosts++;
            }
        }

        printf("\n[MPI] Nodos participantes (%d):\n", unique_hosts);

        for (int i = 0; i < size; i++) {
            const char *h = all_hosts + (size_t)i * MPI_MAX_PROCESSOR_NAME;
            int seen = 0;

            for (int j = 0; j < i; j++) {
                const char *prev = all_hosts + (size_t)j * MPI_MAX_PROCESSOR_NAME;
                if (strcmp(h, prev) == 0) {
                    seen = 1;
                    break;
                }
            }

            if (!seen) {
                printf("  - %s\n", h);
            }
        }

        int expected = size * (size - 1) / 2;
        printf("[MPI] Allreduce(rank) = %d (esperado %d) -> %s\n",
               rank_sum, expected, rank_sum == expected ? "OK" : "ERROR");
        printf("[MPI] Smoke test completado: %d procesos en %d nodo(s)\n",
               size, unique_hosts);
        fflush(stdout);
    }

    free(all_hosts);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
