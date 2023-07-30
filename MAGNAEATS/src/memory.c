/*
* Alexandre Rodrigues 54472        
*/
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <string.h>
#include "memory.h"

/* Função que reserva uma zona de memória partilhada com tamanho indicado
* por size e nome name, preenche essa zona de memória com o valor 0, e 
* retorna um apontador para a mesma. Pode concatenar o resultado da função
* getuid() a name, para tornar o nome único para o processo.
*/
void* create_shared_memory(char* name, int size) {
	int fd; 
	int ret;
	int *ptr;
  
  char str[100];
  sprintf(str, "/%s%u", name, getuid());
	fd = shm_open(str, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR); 

	if (fd == -1) {
		perror(str);
    exit(1);
  }
  
	ret = ftruncate(fd, size);
  if (ret == -1) {
    perror(str);
    exit(2);
  }
	
	ptr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
    perror(str);
    exit(3);
  }
	
	return ptr;
}

/* Função que reserva uma zona de memória dinâmica com tamanho indicado
* por size, preenche essa zona de memória com o valor 0, e retorna um 
* apontador para a mesma.
*/
void* create_dynamic_memory(int size) {	
	int* ptr;
	ptr = malloc(size);
	memset(ptr, 0, size); 
	return ptr;
}

/* Função que liberta uma zona de memória dinâmica previamente reservada.
*/
void destroy_shared_memory(char* name, void* ptr, int size) {
	int temp;
  temp = munmap(ptr, size);
  if (temp == -1) {
      perror(name);
      exit(1);
  }

  char str[100];
  sprintf(str, "/%s%u", name, getuid());
  temp = shm_unlink(str);
  if (temp == -1) {
      perror(str);
      exit(1);
  }

	exit(0);
}

/* Função que liberta uma zona de memória partilhada previamente reservada.
*/
void destroy_dynamic_memory(void* ptr) {
	free(ptr);
}


/* Função que escreve uma operação no buffer de memória partilhada entre a Main
* e os restaurantes. A operação deve ser escrita numa posição livre do buffer, 
* tendo em conta o tipo de buffer e as regras de escrita em buffers desse tipo.
* Se não houver nenhuma posição livre, não escreve nada.
*/
void write_main_rest_buffer(struct rnd_access_buffer* buffer, int buffer_size, struct operation* op) {
  while (true) {
    for (int n = 0; n < buffer_size; n++) {
      if (buffer->ptrs[n] == 0) {
        buffer->buffer[n] = *op;	
        buffer->ptrs[n] = 1;
        break;
      }
    }
  }
}


/* Função que escreve uma operação no buffer de memória partilhada entre os restaurantes
* e os motoristas. A operação deve ser escrita numa posição livre do buffer, 
* tendo em conta o tipo de buffer e as regras de escrita em buffers desse tipo.
* Se não houver nenhuma posição livre, não escreve nada.
*/
void write_rest_driver_buffer(struct circular_buffer* buffer, int buffer_size, struct operation* op) {
	while(true) {
		while (((buffer->ptrs->in + 1) % buffer_size) == buffer->ptrs->out) { 
			; //do nothing
		}
		buffer->buffer[buffer->ptrs->in] = *op;	
		buffer->ptrs->in = (buffer->ptrs->in + 1) % buffer_size; 
	}
}


/* Função que escreve uma operação no buffer de memória partilhada entre os motoristas
* e os clientes. A operação deve ser escrita numa posição livre do buffer, 
* tendo em conta o tipo de buffer e as regras de escrita em buffers desse tipo.
* Se não houver nenhuma posição livre, não escreve nada.
*/
void write_driver_client_buffer(struct rnd_access_buffer* buffer, int buffer_size, struct operation* op) {
  while (true) {
		for (int n = 0; n < buffer_size; n++) {
			if (buffer->ptrs[n] == 0) {
				buffer->buffer[n] = *op;		
				buffer->ptrs[n] = 1;
				break;
			}
		}
  }
}


/* Função que lê uma operação do buffer de memória partilhada entre a Main
* e os restaurantes, se houver alguma disponível para ler que seja direcionada ao restaurante especificado.
* A leitura deve ser feita tendo em conta o tipo de buffer e as regras de leitura em buffers desse tipo.
* Se não houver nenhuma operação disponível, afeta op->id com o valor -1.
*/
void read_main_rest_buffer(struct rnd_access_buffer* buffer, int rest_id, int buffer_size, struct operation* op) {
  for (int n = 0; n < buffer_size; n++) {
    if (buffer->buffer[n].receiving_rest==rest_id && buffer->ptrs[n] == 1) {
      *op = buffer->buffer[n];
      buffer->ptrs[n] = 0;
      return;	
    }
    op->id = -1;
  }
}


/* Função que lê uma operação do buffer de memória partilhada entre os restaurantes e os motoristas,
* se houver alguma disponível para ler (qualquer motorista pode ler qualquer operação).
* A leitura deve ser feita tendo em conta o tipo de buffer e as regras de leitura em buffers desse tipo.
* Se não houver nenhuma operação disponível, afeta op->id com o valor -1.
*/
void read_rest_driver_buffer(struct circular_buffer* buffer, int buffer_size, struct operation* op) {
  if (buffer->ptrs->in == buffer->ptrs->out) { 
    op->id = -1;
  }
  else {
    *op = buffer->buffer[(buffer->ptrs->out)];
    buffer->ptrs->out = (buffer->ptrs->out + 1) % buffer_size; 
  }
}


/* Função que lê uma operação do buffer de memória partilhada entre os motoristas e os clientes,
* se houver alguma disponível para ler dirijida ao cliente especificado. A leitura deve
* ser feita tendo em conta o tipo de buffer e as regras de leitura em buffers desse tipo. Se não houver
* nenhuma operação disponível, afeta op->id com o valor -1.
*/
void read_driver_client_buffer(struct rnd_access_buffer* buffer, int client_id, int buffer_size, struct operation* op) {
  for (int n = 0; n < buffer_size; n++) {
    if (buffer->buffer[n].receiving_client == client_id && buffer->ptrs[n] == 1) {
      *op = buffer->buffer[n];
      buffer->ptrs[n] = 0;
      return; 
    }
    op->id = -1; 
  }
}