CC = gcc
MPICC = mpicc
CFLAGS = -O3 -Wall -Wextra -std=c11
LDLIBS = -lm

BIN_DIR = bin
SEQ_SRC = src/sequential/tsp_seq.c
MPI_SRC = src/parallel/tsp_mpi.c
SEQ_BIN = $(BIN_DIR)/tsp_seq
MPI_BIN = $(BIN_DIR)/tsp_mpi

.PHONY: all sequential parallel clean

all: sequential parallel

sequential: $(SEQ_BIN)

parallel: $(MPI_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(SEQ_BIN): $(SEQ_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

$(MPI_BIN): $(MPI_SRC) | $(BIN_DIR)
	$(MPICC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(SEQ_BIN) $(MPI_BIN)
