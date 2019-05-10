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
int pizzaria_is_open = 1;
int pizzaria_is_done = 0;
sem_t sem_pizzeria_done;
    // Garçons
int _garcons_n;
pthread_t* garcons;
sem_t garcons_disponiveis;
sem_t sem_entregar_pizza;
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
int _total_mesas_n;
int _mesas_vagas_n;
pthread_mutex_t mtx_mesas;
    //Recepção
sem_t sem_recepcao;
int _grupos_recepcao;
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
    while(1){
        sem_wait(&sem_entregar_pizza);
        if(pizzaria_is_done){
            break;
        }
    //printf("ENTREGANDO PIZZA! - \n");
    //pthread_mutex_init(&espaco_mesa->mtx_pegador_pizza, NULL);
        garcom_entregar(espaco_mesa);
    //sem_destroy(&(espaco_mesa->pizza_pronta));
        sem_post(&sem_espaco_mesa);
        sem_post(&garcons_disponiveis);
    //printf("Pizza entregue, indo embora\n");
    }

    return NULL;
}
    // Pizzaiolos
//void pizzaiolo_colocar_mesa(pizza_t* pizza) {
//}

void* pizzaiolo_func(void* args) {
    // Prepara seus utencilios
    // Limpa o bigode
    // Espera os pedidos
    while(1) {
        //printf("==Esperando Sem SmartDeck==\n");
        sem_wait(&sem_smart_deck);
        // Verifica se é comunicado de que seu turno acabou
        if(pizzaria_is_done){
            //printf("Pizzaiolo indo embora\n");
            break;        
        }
        // Recebeu o pedido e vai montá-lo
        // Retira o ticket de pedido
        pedido_t* pedido = (pedido_t*) queue_wait(&smart_deck);
        pizza_t* pizza = pizzaiolo_montar_pizza(pedido);
        sem_init(&(pizza->pizza_pronta),0,0);
        // Olha incessantemente para o forno buscando espaco para enfiar a pizza
        //printf("Esperando espaco forno\n");
        sem_wait(&sem_espacos_forno);
        //printf("COLOQUEI NO FORNO -> Existem %d pizzas lá\n",(2-sem_ret_getValue(&sem_espacos_forno)));
        // Sai na porrada com os outros pizzaiolos para pegar a pá de pizza
        //printf("Esperando Pa 1\n");
        pthread_mutex_lock(&mtx_pa_pizza);
        // Apos sair com algumas costelas quebradas ele tem a pá de pizza e usa para colocar a pizza no forno
        pizzaiolo_colocar_forno(pizza);
        // Joga a pá de pizza na cabeça do outro pizzaiolo e deixa de usá-la
        pthread_mutex_unlock(&mtx_pa_pizza);
        // Fica cheirando a pizza ficar pronta
        //printf("Esperando Pizza pronta 1\n");
        sem_wait(&(pizza->pizza_pronta));
        // Com seu olhar ameacador cai na porrada novamente para pegar a pá de pizza
        //printf("Esperando Pa 2\n");
        pthread_mutex_lock(&mtx_pa_pizza);
        // Com ferimentos graves consegue a pá de pizza
        // E a utiliza para tirar a pizza dos profundos fogos do inferno
        pizzaiolo_retirar_forno(pizza);
        sem_post(&sem_espacos_forno);
        //printf("TIREI DO FORNO -> Existem %d pizzas lá\n",(2-sem_ret_getValue(&sem_espacos_forno)));
        //pizzaiolo_colocar_mesa(pizza);
        // Vai colocar a pizza na mesa
        //printf("Esperando Espaco Mesa\n");
        sem_wait(&sem_espaco_mesa);
        espaco_mesa = pizza;
        // Ele arremessa novamente sua pá de pizza para o seu local de descanso
        pthread_mutex_unlock(&mtx_pa_pizza);
        // Apressadamente ele soca o sino da pizzaria para indicar que há uma pizza pronta
        //printf("Chamando Garcom\n");
        garcom_chamar();
        sem_post(&sem_entregar_pizza);
        // Confere a cara do garçom e manda ele entregar a pizza
        //pthread_setname_np(garcom, "garcom"); //Robson
            //pthread_detach(garcom);
        // Ele pega uma semente dos deuses e a consome,recuperando suas energias e esperando o próximo pedido
    }
    return NULL;
}

void pizza_assada(pizza_t* pizza) {
    sem_post(&(pizza->pizza_pronta));
}


void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas,
                   int n_garcons, int tam_deck, int n_grupos) {
       // Outros
    pizzaria_is_open = 1;
    pizzaria_is_done = 0;
    // Inicializa a pizzaria
        // Smart Deck
    queue_init(&smart_deck,tam_deck);
    sem_init(&sem_smart_deck,0,0);
        // Garcons
    _garcons_n = n_garcons;
    sem_init(&garcons_disponiveis,0,n_garcons);
    sem_init(&sem_entregar_pizza,0,0);
    garcons = malloc(sizeof(pthread_t) * n_garcons);
    for (int i=0; i<n_garcons; i++){
        pthread_create(&garcons[i], NULL, garcom_func, NULL);
    }
        // Cozinha
            // Forno
    sem_init(&sem_espacos_forno,0,tam_forno);
            // Pa de Pizza
    pthread_mutex_init(&mtx_pa_pizza,NULL);
            // Mesa das Pizzas
    sem_init(&sem_espaco_mesa,0,1);
            // Pizziolos
    _pizzaiolos_n = n_pizzaiolos;
    pizzaiolos = malloc(sizeof(pthread_t) * _pizzaiolos_n); //Robson
    for (int i = 0; i < n_pizzaiolos; ++i) {
        pthread_create(&pizzaiolos[i],NULL,pizzaiolo_func,(void*)NULL);
    }
        // Mesas
    _total_mesas_n = n_mesas;
    _mesas_vagas_n = n_mesas;
    pthread_mutex_init(&mtx_mesas, NULL);
        //Recepção
    sem_init(&sem_recepcao,0,0);
    sem_init(&sem_pizzeria_done,0,0);
    _grupos_recepcao = 0;
    
}

void pizzeria_close() {
    // Fecha as portas da pizzaria com portas de ferro e toras
    pizzaria_is_open = 0;
    int aux;
    pthread_mutex_lock(&mtx_mesas);
    aux = _grupos_recepcao;
    pthread_mutex_unlock(&mtx_mesas);

    //Diz as pessoas da recepção para irem embora
    for (int i=0; i<aux; i++) {
        sem_post(&sem_recepcao);
    }
}

void pizzeria_destroy() {

    //Verifica se ha pessoas na pizzaria
    //printf("FECHANDOOOOOOOOOPIZZARIA================\n");
    sem_wait(&sem_pizzeria_done);
    //printf("NAO HA MAIS NINGUEM NA PIZZARIA===\n");

    // Destroi a pizzaria com uma arma termo-nuclear
    //-------------------------COZINHA-E-GAROCONS(REL_COZINHA)-----------------------------
        // Smart Deck
    queue_destroy(&smart_deck);
    sem_destroy(&sem_smart_deck);
        // Garcons
    sem_destroy(&garcons_disponiveis);
    sem_init(&sem_entregar_pizza, 0, 0);
    for (int i = 0; i < _garcons_n; ++i) {
        pthread_join(garcons[i],NULL);
    }
        // Cozinha
            // Forno
    sem_destroy(&sem_espacos_forno);
            // Pa de Pizza
    pthread_mutex_destroy(&mtx_pa_pizza);
            // Mesa das Pizzas
    sem_destroy(&sem_espaco_mesa);
            // Pizziolos
    printf("%d\n",_pizzaiolos_n);
    for (int i = 0; i < _pizzaiolos_n; ++i) {
        //printf("[PIZZAIOLO] ESPERANDO PIZZAIOLO %d\n",i);
        pthread_join(pizzaiolos[i],NULL);
        //printf("[PIZZAIOLO %d] Retornou \n",i);
    }
    free(pizzaiolos);
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
    //int quant_mesas = tam_grupo / 4;
    //quant_mesas += (tam_grupo % 4) ? 1 : 0;
    int quant_mesas = ceil(((double)tam_grupo) / 4);
    //printf("%d   %d\n",quant_mesas1, quant_mesas);
   
    //O grupo verifica se há uma quantidade de mesas vagas necessarias
    pthread_mutex_lock(&mtx_mesas);
    if (quant_mesas <= _mesas_vagas_n) {
        //Pegam as mesas necessarias
        //printf("==Pegar Mesas %d %d %d\n",_mesas_vagas_n, quant_mesas, tam_grupo);
        _mesas_vagas_n -= quant_mesas;
        pthread_mutex_unlock(&mtx_mesas);
        //return 0;
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
                //pthread_mutex_lock(&mtx_mesas);
                //_grupos_recepcao--;
                //pthread_mutex_unlock(&mtx_mesas);
                return -1;
            }
            //Tentam pegar a quantidade de mesas necessarias
            pthread_mutex_lock(&mtx_mesas);
            if (quant_mesas <= _mesas_vagas_n) {
                _mesas_vagas_n -= quant_mesas;
                _grupos_recepcao--;
                pthread_mutex_unlock(&mtx_mesas);
                break; //Deu diferenca na velocidade quanto a return
            }
            pthread_mutex_unlock(&mtx_mesas);
        }
    }
    return 0;
}

void garcom_tchau(int tam_grupo) {
    // Calculam quantas mesas serao liberadas
    //int quant_mesas = tam_grupo / 4;
    //quant_mesas += (tam_grupo % 4) ? 1 : 0;
    int quant_mesas = ceil(((double)tam_grupo) / 4);

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
    
    //Se pizzaria nao esta aberta
    if(!pizzaria_is_open){
        //Se todos ja foram embora
        pthread_mutex_lock(&mtx_mesas);
        if(_mesas_vagas_n == _total_mesas_n){
            //printf("Pizzeria Done!\n==========================\n\n");
            sem_post(&sem_pizzeria_done);
    
            pizzaria_is_done = 1;
            for(int i =0; i<_pizzaiolos_n; i++){
                sem_post(&sem_smart_deck);
            }
            for(int i =0; i<_garcons_n; i++){
                sem_post(&sem_entregar_pizza);
            }
            
        }
        pthread_mutex_unlock(&mtx_mesas);
    }
    //Libera garcon para nova atividade
    sem_post(&garcons_disponiveis);
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

