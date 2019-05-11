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
void* pizzaiolo_func(void* args);

// Pizzaria
int _pizzaria_aberta;
int _pizzaria_encerrada;
pthread_mutex_t mtx_pizzaria_aberta;

// Garçons
int _garcons_n;
//pthread_t* garcons;
sem_t sem_garcons_disponiveis;

// Pizzaiolos
int _pizzaiolos_n;
pthread_t* pizzaiolos;

// Smart Deck
queue_t queue_smart_deck;

// Cozinha
pizza_t* _pizza_balcao;
pthread_mutex_t mtx_pa_pizza;
sem_t sem_forno;
sem_t sem_balcao;
//sem_t sem_entregar_pizza;

// Mesas
int _total_mesas_n;
int _mesas_vagas_n;
pthread_mutex_t mtx_mesas;

// Recepção
int _grupos_recepcao_n;
sem_t sem_recepcao;

// Cozinha
//pthread_mutex_t mtx_pizza_entregar;

// ----------------------------------------------------

// Funcoes

void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas,
                   int n_garcons, int tam_deck, int n_grupos) {
    // Inicializa a pizzaria
    // Pizzaria
    _pizzaria_aberta = 1;
    _pizzaria_encerrada = 0;
    pthread_mutex_init(&mtx_pizzaria_aberta, NULL);

    // Garçons
    _garcons_n = n_garcons;
    sem_init(&sem_garcons_disponiveis,0,_garcons_n);

    // Smart Deck
    queue_init(&queue_smart_deck, tam_deck);    

    // Cozinha
    _pizza_balcao = NULL;
    pthread_mutex_init(&mtx_pa_pizza,NULL);
    sem_init(&sem_forno,0,tam_forno);
    sem_init(&sem_balcao,0,1);
    //sem_init(&sem_entregar_pizza,0,0);

    // Mesas
    _total_mesas_n = n_mesas;
    _mesas_vagas_n = n_mesas;
    pthread_mutex_init(&mtx_mesas, NULL);

    // Recepção
    _grupos_recepcao_n = 0;
    sem_init(&sem_recepcao,0,0);
    //pthread_mutex_init(&mtx_pizza_entregar,NULL);

    // Pizzaiolos
    _pizzaiolos_n = n_pizzaiolos;
    pizzaiolos = malloc(sizeof(pthread_t) * _pizzaiolos_n);
    for(int i = 0;i < _pizzaiolos_n; i++) {
        pthread_create(&pizzaiolos[i],NULL,pizzaiolo_func,NULL);
    }
}

void pizzeria_close() {
    // Fecha as portas da pizzaria com portas de ferro e toras
    pthread_mutex_lock(&mtx_pizzaria_aberta);
    _pizzaria_aberta = 0;
    pthread_mutex_unlock(&mtx_pizzaria_aberta);

    //Dispensando as pessoas da recepcao
    int grupos_recepcao_n_aux;
    pthread_mutex_lock(&mtx_mesas);
    grupos_recepcao_n_aux = _grupos_recepcao_n;
    pthread_mutex_unlock(&mtx_mesas);

    //Diz as pessoas da recepção para irem embora
    for (int i = 0; i < grupos_recepcao_n_aux; i++) {
        sem_post(&sem_recepcao);
    }
}

void pizzeria_destroy() {

    //Verifica se ha pessoas na pizzaria
    printf("FECHANDOOOOOOOOOPIZZARIA================\n");
    while (1) {
        pthread_mutex_lock(&mtx_mesas);
        if (_mesas_vagas_n == _total_mesas_n) {
            pthread_mutex_unlock(&mtx_mesas);
            break; 
        }
        pthread_mutex_unlock(&mtx_mesas);
    }
    printf("NAO HA MAIS NINGUEM NA PIZZARIA===\n");

    // Destroi a pizzaria com uma arma termo-nuclear
    // Pizzaria
    _pizzaria_encerrada = 1;
    pthread_mutex_destroy(&mtx_pizzaria_aberta);

    // Garcons
    sem_destroy(&sem_garcons_disponiveis);

    // Pizzaiolos
    //Informe que seu dia de trabalho acabou    
    for (int i = 0; i < _pizzaiolos_n; i++) {
        queue_push_back(&queue_smart_deck, NULL);
    }

    for (int i = 0; i < _pizzaiolos_n; i++) {
        pthread_join(pizzaiolos[i],NULL);
    }
    free(pizzaiolos);
    
    // Smart Deck
    queue_destroy(&queue_smart_deck);

    // Cozinha
    pthread_mutex_destroy(&mtx_pa_pizza);
    sem_destroy(&sem_forno);
    sem_destroy(&sem_balcao);
    
    // Mesas
    pthread_mutex_destroy(&mtx_mesas);

    //Recepção
    sem_destroy(&sem_recepcao);
}

    // Garcons
void garcom_chamar() {
    sem_wait(&sem_garcons_disponiveis);
}

void fazer_pedido(pedido_t* pedido) {
    queue_push_back(&queue_smart_deck,(void*)pedido);
    sem_post(&sem_garcons_disponiveis);
}

void* garcom_func(void* args) {
    //printf("ENTREGANDO PIZZA!\n");
    // Pega a pizza do balcao para entregar
    pizza_t* pizza_pronta = _pizza_balcao;
    // Libera espaco no balcao
    sem_post(&sem_balcao);
    pthread_mutex_init(&pizza_pronta->mtx_pegador_pizza,NULL);
    pizza_pronta->mutex_exist = 1;
    garcom_entregar(pizza_pronta);
    sem_post(&sem_garcons_disponiveis);
    return NULL;
}

void* pizzaiolo_func(void* args) {
    while (1) {
        // Espera por pedidos
        pedido_t* pedido = (pedido_t*) queue_wait(&queue_smart_deck);
        //Se for anuncio de fim de trabalho TODO
        if (pedido == NULL) {
            break;
        }

        // Pegou um pedido válido
        pizza_t* pizza = pizzaiolo_montar_pizza(pedido);
        //free(pedido); 
        sem_init(&(pizza->pizza_pronta),0,0);
        // Pega a pá e leva no forno
        sem_wait(&sem_forno);
        pthread_mutex_lock(&mtx_pa_pizza);
        pizzaiolo_colocar_forno(pizza);
        pthread_mutex_unlock(&mtx_pa_pizza);
        // Espera a pizza
        //printf("PIZZAIOLO ESPERANDO PIZZA\n");
        sem_wait(&pizza->pizza_pronta);
        // Tira do forno e coloca no balcao
        pthread_mutex_lock(&mtx_pa_pizza);
        pizzaiolo_retirar_forno(pizza);
        sem_post(&sem_forno); // Libera o Forno
        sem_wait(&sem_balcao); //Possivel DeadLock
        _pizza_balcao = pizza; // Coloca no Balcao
        pthread_mutex_unlock(&mtx_pa_pizza); // Libera a Pa

        sem_destroy(&(pizza->pizza_pronta));
        garcom_chamar();
        pthread_t garcom;
        pthread_create(&garcom,NULL,garcom_func,NULL);
        // Reinicia o processo
    }
    return NULL;
}

void pizza_assada(pizza_t* pizza) {
    sem_post(&pizza->pizza_pronta);
}

int pegar_mesas(int tam_grupo) {
    // Se pizzaria esta fechada, o grupo não pode entrar
    pthread_mutex_lock(&mtx_pizzaria_aberta);
    if (!_pizzaria_aberta) {
        pthread_mutex_unlock(&mtx_pizzaria_aberta);
        return -1;
    }
    pthread_mutex_unlock(&mtx_pizzaria_aberta);

    // Calcula a quantidade de mesas para o grupo
    int quant_mesas = tam_grupo / 4;
    quant_mesas += (tam_grupo % 4) ? 1 : 0;
    //int quant_mesas = ceil(((double)tam_grupo) / 4);
   
    //O grupo verifica se há uma quantidade de mesas vagas necessarias
    pthread_mutex_lock(&mtx_mesas);
    if (quant_mesas <= _mesas_vagas_n) {
        //Pegam as mesas necessarias
        _mesas_vagas_n -= quant_mesas;
        pthread_mutex_unlock(&mtx_mesas);
    } else {
        //Entram na recepção
        _grupos_recepcao_n++;
        pthread_mutex_unlock(&mtx_mesas);

        //Espera na recpcao
        while (1) {
            //Esperam por mesas vagas
            sem_wait(&sem_recepcao);
            //Se pizzaria esta fechada eles vao embora
            if (!_pizzaria_aberta) {
                //pthread_mutex_lock(&mtx_mesas);
                //_grupos_recepcao_n--;
                //pthread_mutex_unlock(&mtx_mesas);
                return -1;
            }
            //Tentam pegar a quantidade de mesas necessarias
            pthread_mutex_lock(&mtx_mesas);
            if (quant_mesas <= _mesas_vagas_n) {
                _mesas_vagas_n -= quant_mesas;
                _grupos_recepcao_n--;
                pthread_mutex_unlock(&mtx_mesas);
                break; //Deu diferenca na velocidade quanto a return
                //return 0;
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

    int grupos_recepcao_n_aux;
    pthread_mutex_lock(&mtx_mesas);
    //Vagam as mesas dizendo
    _mesas_vagas_n += quant_mesas;
    grupos_recepcao_n_aux = _grupos_recepcao_n;
    pthread_mutex_unlock(&mtx_mesas);

    //Diz as pessoas da recepção que ha mesas vagas
    for (int i = 0; i < grupos_recepcao_n_aux; i++) {
        sem_post(&sem_recepcao);
    }

    // Libera o garcom
    sem_post(&sem_garcons_disponiveis);
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
