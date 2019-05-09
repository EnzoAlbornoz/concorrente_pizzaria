#include "pizzeria.h"
#include "queue.h"
#include "helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

// Declaracao de funcoes implementadas que não aparecem no 
void* garcom_func(void* args);
void pizzaiolo_colocar_mesa(pizza_t* pizza);
void* pizzaiolo_func(void* args);
int sem_ret_getValue(sem_t* sem_ptr);
int espera_recepcao(int quant_mesas);
// Infos
    // Pizzaria
int pizzaria_is_open;
    // Garçons
sem_t garcons_disponiveis;
    // Pizzaiolos
//int _pizzaiolos_n;
//pthread_t* pizzaiolos;
// Integracao
    // Smart Deck
//queue_t smart_deck;
//sem_t sem_smart_deck;
    // Forno 
//sem_t sem_espacos_forno;
    // Pa de pizza
//pthread_mutex_t mtx_pa_pizza;
    // Espaco na mesa
//pizza_t* espaco_mesa;
//sem_t sem_espaco_mesa; // Semaforo de 1 - > Tipo mutex
    // Mesas
int _total_mesas_n;
int _mesas_vagas_n;
pthread_mutex_t mtx_mesas;
    //Recepção
sem_t sem_recepcao;
sem_t sem_pizzeria_done;
int _grupos_recepcao;
// ----------------------------------------------------

// Funcoes
    // Garcons
void garcom_chamar() {
    sem_wait(&garcons_disponiveis);
}

void fazer_pedido(pedido_t* pedido) {
}

void* garcom_func(void* args) {
    return NULL;
}
    // Pizzaiolos
void pizzaiolo_colocar_mesa(pizza_t* pizza) {
}

void* pizzaiolo_func(void* args) {
    return NULL;
}

void pizza_assada(pizza_t* pizza) {
}


void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas,
                   int n_garcons, int tam_deck, int n_grupos) {
    // Inicializa a pizzaria
        // Garcons
    sem_init(&garcons_disponiveis,0,n_garcons);
        // Mesas
    _total_mesas_n = n_mesas;
    _mesas_vagas_n = n_mesas;
    pthread_mutex_init(&mtx_mesas, NULL);
        // Outros
    pizzaria_is_open = 1;
        //Recepção
    sem_init(&sem_recepcao,0,0);
    sem_init(&sem_pizzeria_done,0,0);
    _grupos_recepcao = 0;
}

void pizzeria_close() {
    // Fecha as portas da pizzaria com portas de ferro e toras
    pizzaria_is_open = 0;
}

void pizzeria_destroy() {

    //Verifica se ha pessoas na pizzaria
    printf("FECHANDOOOOOOOOOPIZZARIA================\n");
    sem_wait(&sem_pizzeria_done);
    printf("NAO HA MAIS NINGUEM NA PIZZARIA===\n");

    // Destroi a pizzaria com uma arma termo-nuclear
    //-------------------------COZINHA-E-GAROCONS(REL_COZINHA)-----------------------------
        // Garcons
    sem_destroy(&garcons_disponiveis);
        // Mesas
    pthread_mutex_destroy(&mtx_mesas);
        //Recepção
    sem_destroy(&sem_recepcao);
    sem_destroy(&sem_pizzeria_done);
}

int pegar_mesas(int tam_grupo) {
    // Se pizzaria esta fechada, o grupo não pode entrar
    if (!pizzaria_is_open) {
        return -1;
    }

    // Calcula a quantidade de mesas para o grupo
    int quant_mesas = tam_grupo / 4;
    quant_mesas += (tam_grupo % 4) ? 1 : 0;
   
    // O grupo verifica se há uma quantidade de mesas vagas necessarias
    pthread_mutex_lock(&mtx_mesas);
    if (quant_mesas <= _mesas_vagas_n) {
        //Pegam as mesas necessarias
        //printf("==Pegar Mesas %d %d %d\n",_mesas_vagas_n, quant_mesas, tam_grupo);
        _mesas_vagas_n -= quant_mesas;
        pthread_mutex_unlock(&mtx_mesas);
    } else {
        //Entram na recepção
        //printf("==Recepcao %d %d %d\n",_mesas_vagas_n, quant_mesas, tam_grupo);
        _grupos_recepcao++;
        pthread_mutex_unlock(&mtx_mesas);
        while (1) {
            //Esperam por mesas vagas
            sem_wait(&sem_recepcao);
            //Se pizzaria esta fechada eles vao embora
            if (!pizzaria_is_open) {
                _grupos_recepcao--;
                return -1;
            }
            //Tentam pegar a quantidade de mesas necessarias
            pthread_mutex_lock(&mtx_mesas);
            if (quant_mesas <= _mesas_vagas_n) {
                _mesas_vagas_n -= quant_mesas;
                _grupos_recepcao--;
                pthread_mutex_unlock(&mtx_mesas);
                return 0;
            }
            pthread_mutex_unlock(&mtx_mesas);
        }
    }
    return 0;
}

void garcom_tchau(int tam_grupo) {
    // Calculam quantas mesas serao liberadas
    int quant_mesas = tam_grupo / 4;
    quant_mesas += (tam_grupo % 4) ? 1 : 0;

    //printf("Esperando Mutex Mesas\n\n");
    pthread_mutex_lock(&mtx_mesas);
    //Vagam as mesas dizendo
    _mesas_vagas_n += quant_mesas;

    //Diz as pessoas da recepção que ha mesas vagas
    for (int i=0; i<_grupos_recepcao; i++) {
        sem_post(&sem_recepcao);
    }
    
    //pthread_mutex_lock(&mtx_recepcao);
    if(_mesas_vagas_n == _total_mesas_n && !pizzaria_is_open && !_grupos_recepcao){
        sem_post(&sem_pizzeria_done);
    }
    //pthread_mutex_unlock(&mtx_recepcao);
    //printf("==Tchau %d %d %d\n",_mesas_vagas_n, quant_mesas, tam_grupo);
    //printf("Indo realmente embora\n");
    //printf("==Mesas Vagas = %d\n",_mesas_vagas_n);
    pthread_mutex_unlock(&mtx_mesas);

    //Recepcao
}

int pizza_pegar_fatia(pizza_t* pizza) {
    pthread_mutex_lock(&pizza->mtx_pegador_pizza);
    if (pizza->fatias > 0) {
        pizza->fatias--;
        pthread_mutex_unlock(&pizza->mtx_pegador_pizza);
        //TODO Destroy mutex pegador
        return 0;
    }
    pthread_mutex_unlock(&pizza->mtx_pegador_pizza);
    return -1;
}

