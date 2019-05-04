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
sem_t sem_espacos_forno;
    // Pa de pizza
pthread_mutex_t mtx_pa_pizza;
    // Espaco na mesa
pizza_t* espaco_mesa;
sem_t sem_espaco_mesa; // Semaforo de 1 - > Tipo mutex
    // Mesas
int _mesas_vagas_n;
//int _total_mesas_n;
pthread_mutex_t mtx_mesas;
    // Grupos
//int _grupos_n;
//int _pessoas_n;
//pthread_mutex_t mtx_grupos;
    //Recepção
queue_t recepcao;

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

void* garcom_func(void* args) {
    // Garçons vêm se degladiando com pegadores de pizza para poder vir pegar uma pizza
    // Um dos garçons consegue vencer e pega uma pizza da mesa -> I.E Essa thread já é chamada dentro de uma exclusão
    //sem_init(&(espaco_mesa->mtx_pegador_pizza),0,espaco_mesa->fatias);
    pthread_mutex_init(&espaco_mesa->mtx_pegador_pizza, NULL);
    garcom_entregar(espaco_mesa);
    sem_destroy(&(espaco_mesa->pizza_pronta));
    sem_post(&sem_espaco_mesa);
    sem_post(&garcons_disponiveis);

    return NULL; //Robson
}
    // Pizzaiolos
void pizzaiolo_colocar_mesa(pizza_t* pizza) {
    sem_wait(&sem_espaco_mesa);
    espaco_mesa = pizza;
}

void* pizzaiolo_func(void* args) {
    // Prepara seus utencilios
    // Limpa o bigode
    // Espera os pedidos
    int smart_deck_length = 0;
    do{
        printf("==Esperando Sem SmartDeck\n");
        sem_wait(&sem_smart_deck);
        // Recebeu o pedido e vai montá-lo
        // Retira o ticket de pedido
        pedido_t* pedido = (pedido_t*) queue_wait(&smart_deck);
        pizza_t* pizza = pizzaiolo_montar_pizza(pedido);
        sem_init(&(pizza->pizza_pronta),0,0);
        // Olha incessantemente para o forno buscando espaco para enfiar a pizza
        sem_wait(&sem_espacos_forno);
        // Sai na porrada com os outros pizzaiolos para pegar a pá de pizza
        pthread_mutex_lock(&mtx_pa_pizza);
        // Apos sair com algumas costelas quebradas ele tem a pá de pizza e usa para colocar a pizza no forno
        pizzaiolo_colocar_forno(pizza);
        // Joga a pá de pizza na cabeça do outro pizzaiolo e deixa de usá-la
        pthread_mutex_unlock(&mtx_pa_pizza);
        // Fica cheirando a pizza ficar pronta
        sem_wait(&(pizza->pizza_pronta));
        // Com seu olhar ameacador cai na porrada novamente para pegar a pá de pizza
        pthread_mutex_lock(&mtx_pa_pizza);
        // Com ferimentos graves consegue a pá de pizza
        // E a utiliza para tirar a pizza dos profundos fogos do forno
        sem_post(&sem_espacos_forno);
        // Vai colocar a pizza na mesa
        pizzaiolo_colocar_mesa(pizza);
        // Ele arremessa novamente sua pá de pizza para o seu local de descanso
        pthread_mutex_unlock(&mtx_pa_pizza);
        // Apressadamente ele soca o sino da pizzaria para indicar que há uma pizza pronta
        garcom_chamar();
        // Confere a cara do garçom e manda ele entregar a pizza
        pthread_t garcom;
        pthread_create(&garcom,NULL,garcom_func,NULL);
        //pthread_setname_np(garcom, "garcom"); //Robson
        pthread_detach(garcom);
        // Ele pega uma semente dos deuses e a consome,recuperando suas energias e esperando o próximo pedido
    } while(pizzaria_is_open || sem_getvalue(&sem_smart_deck,&smart_deck_length));
    return NULL; //Robson Alteraçã
}

void pizza_assada(pizza_t* pizza) {
    sem_post(&(pizza->pizza_pronta));
}


void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas,
                   int n_garcons, int tam_deck, int n_grupos) {
    // Inicializa a pizzaria
        // Smart Deck
    queue_init(&smart_deck,tam_deck);
    sem_init(&sem_smart_deck,0,0);
        // Garcons
    sem_init(&garcons_disponiveis,0,n_garcons);
        // Cozinha
            // Forno
    sem_init(&sem_espacos_forno,0,tam_forno);
            // Pa de Pizza
    pthread_mutex_init(&mtx_pa_pizza,NULL);
            // Mesa das Pizzas
    sem_init(&sem_espaco_mesa,0,1);
        // Grupos
            // Mesas
    //_total_mesas_n = n_mesas;
    _mesas_vagas_n = n_mesas;
    //printf("%d\n",_mesas_vagas_n);
    pthread_mutex_init(&mtx_mesas,NULL);
            // Grupos/Pessoas
    //_grupos_n = 0;
    //_pessoas_n = 0;
    //pthread_mutex_init(&mtx_grupos,NULL);
        // Outros
    pizzaria_is_open = 1;
        // Pizziolos
    _pizzaiolos_n = n_pizzaiolos;
    pizzaiolos = malloc(sizeof(pthread_t) * _pizzaiolos_n); //Robson
    for (int i = 0; i < n_pizzaiolos; ++i) {
        pthread_create(&pizzaiolos[i],NULL,pizzaiolo_func,(void*)NULL);
    }
        //Recepção
    queue_init(&recepcao, (n_grupos - 1)); //Pq no minimo tera um grupo ocupando todas as mesas
}

void pizzeria_close() {
    // Fecha as portas da pizzaria com portas de ferro e toras
    pizzaria_is_open = 0;
}

void pizzeria_destroy() {
    // Destroi a pizzaria com uma arma termo-nuclear
    //-------------------------COZINHA-E-GAROCONS(REL_COZINHA)-----------------------------
        // Smart Deck
    queue_destroy(&smart_deck);
    sem_destroy(&sem_smart_deck);
        // Garcons
    sem_destroy(&garcons_disponiveis);
        // Cozinha
            // Forno
    sem_destroy(&sem_espacos_forno);
            // Pa de Pizza
    pthread_mutex_destroy(&mtx_pa_pizza);
            // Mesa das Pizzas
    sem_destroy(&sem_espaco_mesa);
        // Grupos
            // Mesas
    //pthread_mutex_destroy(&mtx_mesas);
            // Grupos/Pessoas
    //pthread_mutex_destroy(&mtx_grupos);
        // Outros
        // Pizziolos
    printf("%d\n",_pizzaiolos_n);
    for (int i = 0; i < _pizzaiolos_n; ++i) {
        //int ret_val = 0;
        //int* ptr_ret_val = &ret_val;
        pthread_join(pizzaiolos[i],NULL);//Robson
    }
    free(pizzaiolos);
    //-------------------------COZINHA-E-GAROCONS(REL_COZINHA)-----------------------------
    

    queue_destroy(&recepcao);
}

int pegar_mesas(int tam_grupo) {
    if (!pizzaria_is_open) {
        return -1;
    }

    int quant_mesas = tam_grupo / 4;
    quant_mesas += (tam_grupo % 4) ? 1 : 0;
   
    pthread_mutex_lock(&mtx_mesas);
    if (quant_mesas <= _mesas_vagas_n) { //Podem brigar por uma mesa
        printf("==Pegar Mesas %d %d %d\n",_mesas_vagas_n, quant_mesas, tam_grupo);
        _mesas_vagas_n -= quant_mesas;
    } else {
        printf("==Recepcao %d %d %d\n",_mesas_vagas_n, quant_mesas, tam_grupo);
        int* val = malloc(sizeof(int) * quant_mesas);
        *(val) = quant_mesas;        
        queue_push_back(&recepcao, val);
        pthread_mutex_unlock(&mtx_mesas);
        return -1;
    }
    pthread_mutex_unlock(&mtx_mesas);
    return 0;
}

void garcom_tchau(int tam_grupo) {
    int quant_mesas = tam_grupo / 4;
    quant_mesas += (tam_grupo % 4) ? 1 : 0;

    pthread_mutex_lock(&mtx_mesas);
    _mesas_vagas_n += quant_mesas;
    printf("==Tchau %d %d %d\n",_mesas_vagas_n, quant_mesas, tam_grupo);
    if (!queue_empty(&recepcao)) {
        int i = queue_size(&recepcao);
        printf("Vendo Recepcao %d\n", i);
        int quant_mesas_fila;
        while (i > 0) {
            void* aux = queue_wait(&recepcao);
            quant_mesas_fila = *((int*) aux);
            free(aux);
            if (quant_mesas_fila <= _mesas_vagas_n) {
                printf("Achou Grupo %d\n", queue_size(&recepcao));
                _mesas_vagas_n -=quant_mesas_fila;
                break;
            }
            int* val = malloc(sizeof(int) * quant_mesas_fila);
            *(val) = quant_mesas_fila;        
            queue_push_back(&recepcao, (void*)val); 
            i--;
        } 
        //Voltando Fila ao normal
        while(i > 1){
            void* aux = queue_wait(&recepcao);
            quant_mesas_fila = *((int*)aux);
            free(aux);
            int* val = malloc(sizeof(int) * quant_mesas_fila);
            *(val) = quant_mesas_fila;        
            queue_push_back(&recepcao, (void*)val); 
        }
    }
    pthread_mutex_unlock(&mtx_mesas);
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
