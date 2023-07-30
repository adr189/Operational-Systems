/*
* Alexandre Rodrigues 54472      
*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "main.h"
#include "process.h"


int main(int argc, char *argv[]) {
  //init data structures
  struct main_data* data = create_dynamic_memory(sizeof(struct main_data));
  struct communication_buffers* buffers = create_dynamic_memory(sizeof(struct communication_buffers));
  buffers->main_rest = create_dynamic_memory(sizeof(struct rnd_access_buffer));
  buffers->rest_driv = create_dynamic_memory(sizeof(struct circular_buffer));
  buffers->driv_cli = create_dynamic_memory(sizeof(struct rnd_access_buffer));
  //execute main code
  main_args(argc, argv, data);
  create_dynamic_memory_buffers(data);
  create_shared_memory_buffers(data, buffers);
  launch_processes(buffers, data);
  user_interaction(buffers, data);
  //release memory before terminating
  destroy_dynamic_memory(data);
  destroy_dynamic_memory(buffers->main_rest);
  destroy_dynamic_memory(buffers->rest_driv);
  destroy_dynamic_memory(buffers->driv_cli);
  destroy_dynamic_memory(buffers);
}

/* Função que lê os argumentos da aplicação, nomeadamente o número
* máximo de operações, o tamanho dos buffers de memória partilhada
* usados para comunicação, e o número de clientes, de motoristas e de
* restaurantes. Guarda esta informação nos campos apropriados da
* estrutura main_data.
*/
void main_args(int argc, char* argv[], struct main_data* data) {
	data->max_ops       = atoi(argv[1]);  
	data->buffers_size  = atoi(argv[2]);
	data->n_restaurants = atoi(argv[3]);
	data->n_drivers     = atoi(argv[4]);
	data->n_clients     = atoi(argv[5]);

}

/* Função que reserva a memória dinâmica necessária para a execução
* do MAGNAEATS, nomeadamente para os arrays *_pids e *_stats da estrutura 
* main_data. Para tal, pode ser usada a função create_dynamic_memory.
*/
void create_dynamic_memory_buffers(struct main_data* data) {
	data->restaurant_pids  = create_dynamic_memory(sizeof(int) * data->n_restaurants);
	data->driver_pids      = create_dynamic_memory(sizeof(int) * data->n_drivers);
	data->client_pids      = create_dynamic_memory(sizeof(int) * data->n_clients);
	data->restaurant_stats = create_dynamic_memory(sizeof(int) * sizeof(data->restaurant_stats));
	data->driver_stats     = create_dynamic_memory(sizeof(int) * sizeof(data->driver_stats));
	data->client_stats     = create_dynamic_memory(sizeof(int) * sizeof(data->client_stats));
}

/* Função que reserva a memória partilhada necessária para a execução do
* MAGNAEATS. É necessário reservar memória partilhada para todos os buffers da
* estrutura communication_buffers, incluindo os buffers em si e respetivos
* pointers, assim como para o array data->results e variável data->terminate.
* Para tal, pode ser usada a função create_shared_memory.
*/
void create_shared_memory_buffers(struct main_data* data, struct communication_buffers* buffers) {
	buffers->main_rest->buffer  = create_shared_memory(STR_SHM_MAIN_REST_BUFFER, data->buffers_size * sizeof(struct operation));
	buffers->main_rest->ptrs    = create_shared_memory(STR_SHM_MAIN_REST_PTR, sizeof(int) * data->buffers_size);
	buffers->rest_driv->buffer  = create_shared_memory(STR_SHM_REST_DRIVER_BUFFER, data->buffers_size * sizeof(struct operation));
	buffers->rest_driv->ptrs    = create_shared_memory(STR_SHM_REST_DRIVER_PTR, sizeof(struct pointers));
	buffers->driv_cli->buffer   = create_shared_memory(STR_SHM_DRIVER_CLIENT_BUFFER, data->buffers_size * sizeof(struct operation));
	buffers->driv_cli->ptrs     = create_shared_memory(STR_SHM_DRIVER_CLIENT_PTR, sizeof(int) * data->buffers_size);
	data->results               = create_shared_memory(STR_SHM_RESULTS, sizeof(struct operation) * data->max_ops);
	data->terminate             = create_shared_memory(STR_SHM_TERMINATE, sizeof(int) * sizeof(data->terminate));
}

/* Função que inicia os processos dos restaurantes, motoristas e
* clientes. Para tal, pode usar as funções launch_*,
* guardando os pids resultantes nos arrays respetivos
* da estrutura data.
*/
void launch_processes(struct communication_buffers* buffers, struct main_data* data) { //CHECK LATER
 for (int n = 0; n < data->n_restaurants; n++) {
    int restaurant_pid = launch_restaurant(data->restaurant_pids[n], buffers, data);
    data->restaurant_pids[n] = restaurant_pid;
  }
  for (int n = 0; n < data->n_drivers; n++) {
    int driver_pid = launch_driver(data->driver_pids[n], buffers, data);
    data->driver_pids[n] = driver_pid;
  }
  for (int n = 0; n < data->n_clients; n++) {
    int client_pid = launch_client(data->client_pids[n], buffers, data);
    data->client_pids[n] = client_pid;
  }
}

/* Função que faz interação do utilizador, podendo receber 4 comandos:
* request - cria uma nova operação, através da função create_request
* status - verifica o estado de uma operação através da função read_status
* stop - termina o execução do MAGNAEATS através da função stop_execution
* help - imprime informação sobre os comandos disponiveis
*/
void user_interaction(struct communication_buffers* buffers, struct main_data* data) {
	printf("\nAções disponíveis: \n");
  printf("\t request client restaurant dish - criar um novo pedido\n");
  printf("\t status id - consultar o estado de um pedido \n");
  printf("\t stop - termina a execução do magnaeats.\n");
  printf("\t help - imprime informação sobre as ações disponíveis.\n");

  int op_counter = 0;
  int max_chars = 50;
  char user_input[max_chars];
  
  while (true) {
    printf("Introduzir ação: \n");
    scanf("%s", user_input);

    if (strcmp(user_input, "request") == 0) {
      create_request(&op_counter, buffers, data);
      op_counter++;
    } 
    else if (strcmp(user_input, "status") == 0) {
      read_status(data);
    } 
    else if (strcmp(user_input, "help") == 0) {
      printf("Ações disponíveis: \n");
      printf("\t request client restaurant dish - criar um novo pedido\n");
      printf("\t status id - consultar o estado de um pedido \n");
      printf("\t stop - termina a execução do magnaeats.\n");
      printf("\t help - imprime informação sobre as ações disponíveis.\n");
    } 
     else if (strcmp(user_input, "stop") == 0) {
      stop_execution(data, buffers);
    }  
    else  {
      printf("Ação não reconhecida, insira 'help' para assistência.\n");
    }
  } 
  
  stop_execution(data, buffers);
}

/* Se o limite de operações ainda não tiver sido atingido, cria uma nova
* operação identificada pelo valor atual de op_counter e com os dados passados em
* argumento, escrevendo a mesma no buffer de memória partilhada entre main e restaurantes.
* Imprime o id da operação e incrementa o contador de operações op_counter.
*/
void create_request(int* op_counter, struct communication_buffers* buffers, struct main_data* data) {
	if (*op_counter < data->max_ops) {
		struct operation* new_operation = malloc(sizeof(struct operation));
    	int client, rest; 
    	int max_chars = 100;
    	char* dish = create_dynamic_memory(max_chars * sizeof(char));
    
    	scanf("%d %d %s", &client, &rest, dish);
    
    	new_operation->requested_rest    = rest;
    	new_operation->requesting_client = client;
    	new_operation->requested_dish    = dish;
		new_operation->id                = *op_counter;
    	new_operation->status            = 'I';
    	data->results[*op_counter]       = *new_operation;
      
		write_main_rest_buffer(buffers->main_rest, data->buffers_size, new_operation); 
		printf("O pedido: #%d foi criado!\n", new_operation->id);
    	op_counter++;
    	free(new_operation);
	}
}

/* Função que lê um id de operação do utilizador e verifica se a mesma
* é valida. Em caso afirmativo,
* imprime informação da mesma, nomeadamente o seu estado, o id do cliente
* que fez o pedido, o id do restaurante requisitado, o nome do prato pedido
* e os ids do restaurante, motorista, e cliente que a receberam e processaram.
*/
void read_status(struct main_data* data) {
	int user_id;
	scanf("%d", &user_id);
	struct operation* user_operation = malloc(sizeof(struct operation));
	*user_operation = data->results[user_id];
	if (data->max_ops >= user_id && user_id > 0) {
		printf("Pedido %d ", user_id);
		printf("com estado %d ", user_operation->status);
		printf("requisitado pelo cliente %d ", user_operation->requesting_client);
		printf("ao restaurante %d ", user_operation->requested_rest);
		printf("com o prato %s, ", user_operation->requested_dish);
		printf("foi tratado pelo restaurante %d, ", user_operation->receiving_rest);
		printf("encaminhado pelo motorista %d, ", user_operation->receiving_driver);
		printf("e enviado ao cliente %d!", user_operation->receiving_client);
	}
	free(user_operation);
}

/* Função que termina a execução do programa MAGNAEATS. Deve começar por 
* afetar a flag data->terminate com o valor 1. De seguida, e por esta
* ordem, deve esperar que os processos filho terminem, deve escrever as
* estatisticas finais do programa, e por fim libertar
* as zonas de memória partilhada e dinâmica previamente 
* reservadas. Para tal, pode usar as outras funções auxiliares do main.h.
*/
void stop_execution(struct main_data* data, struct communication_buffers* buffers) {
	*(data->terminate) = 1;
	wait_processes(data);
	write_statistics(data);
	destroy_memory_buffers(data, buffers);
}

/* Função que espera que todos os processos previamente iniciados terminem,
* incluindo restaurantes, motoristas e clientes. Para tal, pode usar a função 
* wait_process do process.h.
*/
void wait_processes(struct main_data* data) {
	for (int n = 0; n < data->n_restaurants; n++) {
		wait_process(data->restaurant_pids[n]);
	}
	for (int n = 0; n < data->n_drivers; n++) {
		wait_process(data->driver_pids[n]);
	}
	for (int n = 0; n < data->n_clients; n++) {
		wait_process(data->client_pids[n]);
	} 
}

/* Função que imprime as estatisticas finais do MAGNAEATS, nomeadamente quantas
* operações foram processadas por cada restaurante, motorista e cliente.
*/
void write_statistics(struct main_data* data) {
	printf("\nTerminando o MAGNAEATS! Imprimindo estatísticas:\n");
	for (int n = 0; n < data->n_restaurants; n++) {
		printf("Restaurante %d preparou : %d pedidos!\n", n, data->restaurant_stats[data->restaurant_pids[n]]);
	}
	for (int n = 0; n < data->n_drivers; n++) {
		printf("Motorista %d entregou : %d pedidos!\n", n, data->driver_stats[data->driver_pids[n]]);
	}
	for (int n = 0; n < data->n_clients; n++) {
		printf("Cliente %d recebeu : %d pedidos!\n", n, data->client_stats[data->client_pids[n]]);
	}
}

/* Função que liberta todos os buffers de memória dinâmica e partilhada previamente
* reservados na estrutura data.
*/
void destroy_memory_buffers(struct main_data* data, struct communication_buffers* buffers){
	destroy_dynamic_memory(buffers->main_rest->ptrs);
	destroy_dynamic_memory(buffers->rest_driv->ptrs);
	destroy_dynamic_memory(buffers->driv_cli->ptrs);
	destroy_dynamic_memory(data->restaurant_stats);
	destroy_dynamic_memory(data->driver_stats);
	destroy_dynamic_memory(data->client_stats);

	destroy_shared_memory(STR_SHM_MAIN_REST_BUFFER, buffers->main_rest, sizeof(struct operation) * sizeof(buffers->main_rest));
	destroy_shared_memory(STR_SHM_MAIN_REST_PTR, buffers->main_rest->ptrs, sizeof(int) * data->buffers_size);
	destroy_shared_memory(STR_SHM_REST_DRIVER_BUFFER,  buffers->rest_driv, sizeof(struct operation) * sizeof(buffers->rest_driv));
	destroy_shared_memory(STR_SHM_REST_DRIVER_PTR, buffers->rest_driv->ptrs, sizeof(struct pointers));
	destroy_shared_memory(STR_SHM_DRIVER_CLIENT_BUFFER, buffers->driv_cli, sizeof(struct operation) * sizeof(buffers->driv_cli));
	destroy_shared_memory(STR_SHM_DRIVER_CLIENT_PTR, buffers->driv_cli->ptrs, sizeof(int) * data->buffers_size);
	destroy_shared_memory(STR_SHM_RESULTS, data->results, sizeof(struct operation) * sizeof(data->max_ops));
	destroy_shared_memory(STR_SHM_TERMINATE, data->terminate, sizeof(int) * sizeof(data->terminate));		
}
