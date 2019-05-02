#include "pizzeria.h"
#include "queue.h"
#include "helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

// Infos
    // Pizzaria
int pizzaria_is_open;
    // Garçons
sem_t garcons_disponiveis;

    // Pizzaiolos
int _pizzaiolos_n;
pthread_t* pizzaiolos;
// Integracao
    // Smart Deck
queue_t smart_deck;
sem_t sem_smart_deck;
    // Forno 
sem_t espacos_forno;
    // Pa de pizza
pthread_mutex_t pa_pizza;
    // Espaco na mesa
pizza_t* espaco_mesa;
sem_t sem_espaco_mesa;
    // Pizzas
int _pizzas_entregar;
pthread_mutex_t mtx_pizzas_entregar;
// ----------------------------------------------------

// Funcoes
    // Garcons
void garcom_chamar() {
    sem_wait(&garcons_disponiveis);
}

void fazer_pedido(pedido_t* pedido) {
    queue_push_back(&smart_deck,(void*)pedido);
    sem_post(&sem_smart_deck);
}

void garcom_func(void* args) {
    // Garçons vêm se degladiando com pegadores de pizza para poder vir pegar uma pizza
    // Um dos garçons consegue vencer e pega uma pizza da mesa -> I.E Essa thread já é chamada dentro de uma exclusão
    garcom_entregar(espaco_mesa);
    sem_post(&sem_espaco_mesa);
}
    // Pizzaiolos

void pizzaiolo_func(void* args) {
    // Prepara seus utencilios
    // Limpa o bigode
    // Espera os pedidos
    int smart_deck_length = 0;
    while(pizzaria_is_open || sem_getvalue(&sem_smart_deck,&smart_deck_length)) {
        sem_wait(&sem_smart_deck);
        // Recebeu o pedido e vai montá-lo
        // Retira o ticket de pedido
        pedido_t* pedido = (pedido_t*) queue_wait(&smart_deck);
        pizza_t* pizza = pizzaiolo_montar_pizza(pedido);
        sem_init(&(pizza->pizza_pronta),0,0);
        // Sai na porrada com os outros pizzaiolos para pegar a pá de pizza
        pthread_mutex_lock(&pa_pizza);
        // Apos sair com algumas costelas quebradas ele tem a pá de pizza
        pizzaiolo_colocar_forno(pizza);
        // Joga a pá de pizza na cabeça do outro pizzaiolo e deixa de usá-la
        pthread_mutex_unlock(&pa_pizza);
        // Fica cheirando a pizza ficar pronta
        sem_wait(&(pizza->pizza_pronta));
        // Com seu olhar ameacador cai na porrada novamente para pegar a pá de pizza
        pthread_mutex_lock(&pa_pizza);
        // Com ferimentos graves consegue a pá de pizza
        // Vai colocar a pizza na mesa
        pizzaiolo_colocar_mesa(pizza);
        // Ele arremessa novamente sua pá de pizza para o seu local de descanso
        pthread_mutex_unlock(&pa_pizza);
        // Apressadamente ele soca o sino da pizzaria para indicar que há uma pizza pronta
        garcom_chamar();
        // Confere a cara do garçom e manda ele entregar a pizza
        pthread_t garcom;
        pthread_create(&garcom,NULL,garcom_func,NULL);
        pthread_setname_np(garcom, "garcom");
        pthread_detach(garcom);
        // Ele pega uma semente dos deuses e a consome,recuperando suas energias e esperando o próximo pedido
    }
}

void pizza_assada(pizza_t* pizza) {
    sem_post(&(pizza->pizza_pronta));
}

void coloca_pizza_mesa(pizza_t* pizza) {
    sem_wait(&sem_espaco_mesa);
    // TODO !!!!!!!!!!!!!!!!!!!!!!!!!!
    espaco_mesa = pizza;
}

void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas,
                   int n_garcons, int tam_deck, int n_grupos) {

}

void pizzeria_close() {
}

void pizzeria_destroy() {
}

void pizza_assada(pizza_t* pizza) {

}

int pegar_mesas(int tam_grupo) {
    return -1; //erro: não fui implementado (ainda)!
}

void garcom_tchau(int tam_grupo) {
}

int pizza_pegar_fatia(pizza_t* pizza) {
    return -1; // erro: não fui implementado (ainda)!
}
