#include "pizzeria.h"
#include "queue.h"
#include "helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>

// Declaracao de funcoes implementadas que não aparecem no 
void* garcom_func(void* args);
void pizzaiolo_colocar_mesa(pizza_t* pizza);
void* pizzaiolo_func(void* args);
int sem_ret_getValue(sem_t* sem_ptr);
int espera_recepcao(int quant_mesas);
// Infos
    // Pizzaria
pthread_mutex_t mtx_pizzaria_open;
int pizzaria_is_open;
    // Garçons
sem_t garcons_disponiveis;
    // Mesas
int _total_mesas_n;
int _mesas_vagas_n;
pthread_mutex_t mtx_mesas;
    //Recepção
sem_t sem_recepcao;
sem_t sem_pizzeria_done;
int _grupos_recepcao;
// Cozinha
sem_t sem_forno;
pthread_mutex_t mtx_pa_pizza;
pthread_mutex_t mtx_pizza_entregar;
queue_t queue_smart_deck;
pthread_t* pizzaiolos;
int n_pizzaiolos_;
// ----------------------------------------------------

// Funcoes
    // Garcons
void garcom_chamar() {
    sem_wait(&garcons_disponiveis);
}

void fazer_pedido(pedido_t* pedido) {
    printf("PEDIDO FEITO!\n");
    queue_push_back(&queue_smart_deck,(void*)pedido);
    sem_post(&garcons_disponiveis);
}

void* garcom_func(void* args) {
    printf("ENTREGANDO PIZZA!\n");
    pizza_t* pizza = (pizza_t*) args;
    pthread_mutex_init(&pizza->mtx_pegador_pizza,NULL);
    pizza->mutex_exist = 1;
    garcom_entregar(pizza);
    sem_post(&garcons_disponiveis);
    return NULL;
}
    // Pizzaiolos
// void pizzaiolo_colocar_mesa(pizza_t* pizza) {
// }

void* pizzaiolo_func(void* args) {
    while(1) {
        // Espera por pedidos
        pedido_t* pedido = (pedido_t*) queue_wait(&queue_smart_deck);
        pthread_mutex_lock(&mtx_pizzaria_open);
        if (pedido == NULL && !pizzaria_is_open) {
            pthread_mutex_unlock(&mtx_pizzaria_open);
            break;
        }
        pthread_mutex_unlock(&mtx_pizzaria_open);
        // Pegou um pedido válido
        pizza_t* pizza = pizzaiolo_montar_pizza(pedido);
        sem_init(&(pizza->pizza_pronta),0,0);
        // Pega a pá e leva no forno
        sem_wait(&sem_forno);
        pthread_mutex_lock(&mtx_pa_pizza);
        pizzaiolo_colocar_forno(pizza);
        pthread_mutex_unlock(&mtx_pa_pizza);
        // Espera a pizza
        printf("PIZZAIOLO ESPERANDO PIZZA\n");
        sem_wait(&pizza->pizza_pronta);
        // Tira do forno
        printf("PIZZAIOLO ESPERANDO PA DE PIZZA PARA RETIRAR\n");
        pthread_mutex_lock(&mtx_pa_pizza);
        pizzaiolo_retirar_forno(pizza);
        printf("RETIRANDO DO FORNO PIZZA %p\n",pizza);
        pthread_mutex_unlock(&mtx_pa_pizza);
        sem_post(&sem_forno);
        // Coloca na mesa para o garcom
        pthread_mutex_lock(&mtx_pizza_entregar);
        sem_destroy(&(pizza->pizza_pronta));
        garcom_chamar();
        pthread_t garcom;
        pthread_create(&garcom,NULL,garcom_func,(void*)pizza);
        pthread_mutex_unlock(&mtx_pizza_entregar);
        pthread_detach(garcom);
        // Reinicia o processo
    }
    return NULL;
}

void pizza_assada(pizza_t* pizza) {
    // Notify semaphore 
    printf("PIZZA PRONTA DO FORNO!\n");
    sem_post(&pizza->pizza_pronta);
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
    pthread_mutex_init(&mtx_pizzaria_open,NULL);
    pizzaria_is_open = 1;
        //Recepção
    sem_init(&sem_recepcao,0,0);
    sem_init(&sem_pizzeria_done,0,0);
    _grupos_recepcao = 0;
    //-------------------------COZINHA-E-GAROCONS(REL_COZINHA)-----------------------------
    sem_init(&sem_forno,0,tam_forno);
    pthread_mutex_init(&mtx_pa_pizza,NULL);
    pthread_mutex_init(&mtx_pizza_entregar,NULL);
    queue_init(&queue_smart_deck,tam_deck);
    n_pizzaiolos_ = n_pizzaiolos;
    pizzaiolos = (pthread_t*) malloc(sizeof(pthread_t)*n_pizzaiolos);
    for(int i = 0;i < n_pizzaiolos;++i) {
        printf("[PIZZAIOLO %d] Criado!",i);
        pthread_create(&pizzaiolos[i],NULL,pizzaiolo_func,NULL);
    }
    //-------------------------COZINHA-E-GAROCONS(REL_COZINHA)-----------------------------
}

void pizzeria_close() {
    // Fecha as portas da pizzaria com portas de ferro e toras
    pthread_mutex_lock(&mtx_pizzaria_open); // Enzo
    pizzaria_is_open = 0;
    pthread_mutex_unlock(&mtx_pizzaria_open);// Enzo
    int aux;
    pthread_mutex_lock(&mtx_mesas);
    aux = _grupos_recepcao;
    pthread_mutex_unlock(&mtx_mesas);

    //Diz as pessoas da recepção que ha mesas vagas
    for (int i=0; i<aux; i++) {
        sem_post(&sem_recepcao);
    }
}

void pizzeria_destroy() {

    //Verifica se ha pessoas na pizzaria
    printf("FECHANDOOOOOOOOOPIZZARIA================\n");
    sem_wait(&sem_pizzeria_done);
    printf("NAO HA MAIS NINGUEM NA PIZZARIA===\n");

    // Destroi a pizzaria com uma arma termo-nuclear
    //-------------------------COZINHA-E-GAROCONS(REL_COZINHA)-----------------------------
    sem_destroy(&sem_forno);
    pthread_mutex_destroy(&mtx_pa_pizza);
    pthread_mutex_destroy(&mtx_pizza_entregar);
    pthread_mutex_destroy(&mtx_pizzaria_open);
    for(int i = 0;i < n_pizzaiolos_;++i) {
        printf("[PIZZAIOLO %d] Destruido!",i);
        pthread_join(pizzaiolos[i],NULL);
    }
    free(pizzaiolos);
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
    //int quant_mesas = ceil(((double)tam_grupo) / 4);
    //printf("%d   %d\n",quant_mesas1, quant_mesas);
   
    //O grupo verifica se há uma quantidade de mesas vagas necessarias
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
                pthread_mutex_lock(&mtx_mesas);
                _grupos_recepcao--;
                pthread_mutex_unlock(&mtx_mesas);
                return -1;
            }
            //Tentam pegar a quantidade de mesas necessarias
            pthread_mutex_lock(&mtx_mesas);
            if (quant_mesas <= _mesas_vagas_n) {
                _mesas_vagas_n -= quant_mesas;
                _grupos_recepcao--;
                pthread_mutex_unlock(&mtx_mesas);
                //break; //Deu diferenca na velocidade quanto a return
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
    //int quant_mesas = ceil(((double)tam_grupo) / 4);

    //printf("Esperando Mutex Mesas\n\n");
    int aux;
    pthread_mutex_lock(&mtx_mesas);
    //Vagam as mesas dizendo
    _mesas_vagas_n += quant_mesas;
    aux = _grupos_recepcao;
    pthread_mutex_unlock(&mtx_mesas);

    //Diz as pessoas da recepção que ha mesas vagas
    for (int i=0; i<aux; i++) {
        sem_post(&sem_recepcao);
    }
    
    //pthread_mutex_lock(&mtx_recepcao);
    pthread_mutex_lock(&mtx_pizzaria_open); // Enzo
    if(!pizzaria_is_open){
        pthread_mutex_lock(&mtx_mesas);
        if(_mesas_vagas_n == _total_mesas_n && !_grupos_recepcao){
            printf("Pizzeria Done!");
            for(int i = 0; i < n_pizzaiolos_;++i) {
                queue_push_back(&queue_smart_deck,NULL);
            }
            sem_post(&sem_pizzeria_done);
        }
        pthread_mutex_unlock(&mtx_mesas);
    }
    pthread_mutex_unlock(&mtx_pizzaria_open); // Enzo
    //pthread_mutex_unlock(&mtx_recepcao);
    //printf("==Tchau %d %d %d\n",_mesas_vagas_n, quant_mesas, tam_grupo);
    //printf("Indo realmente embora\n");
    //printf("==Mesas Vagas = %d\n",_mesas_vagas_n);

    //Recepcao
}

int pizza_pegar_fatia(pizza_t* pizza) {
    // if(!pizza->mutex_exist) { return -1; }
    pthread_mutex_lock(&pizza->mtx_pegador_pizza);
    if (pizza->fatias <= 0) {
        pthread_mutex_unlock(&pizza->mtx_pegador_pizza);
        return -1;
    }
    pizza->fatias--;
    if(!pizza->fatias) { 
        // pizza->mutex_exist = 0;
    }
    pthread_mutex_unlock(&pizza->mtx_pegador_pizza);
    // if(!pizza->mutex_exist) {pthread_mutex_destroy(&pizza->mtx_pegador_pizza);}
    return 0;
}

