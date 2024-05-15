# Variáveis
CC = gcc
CFLAGS = -Wall -Wextra

# Programas
PROGRAMS = cliente servidor

# Regra padrão para compilar todos os programas
all: $(PROGRAMS)

servidor: servidor.c
	$(CC) $(CFLAGS) -o servidor servidor.c

cliente: cliente.c
	$(CC) $(CFLAGS) -o cliente cliente.c

clean:
	rm -f cliente servidor
