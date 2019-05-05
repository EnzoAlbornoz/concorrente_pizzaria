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
int util_ceil(int value,int divider);
void* recepcao_func(void* args);
ticket_t* entrar_fila_recepcao(int g_mesas);
int espera_recepcao(int g_mesas);
// Infos
    // Pizzaria
int pizzaria_is_open;
sem_t sem_pizzaria_done;
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
int _total_mesas_n;
pthread_mutex_t mtx_mesas;
    // Grupos
//int _grupos_n;
//int _pessoas_n;
//pthread_mutex_t mtx_grupos;
    //Recepção
sem_t sem_mesa_livre;
queue_t recepcao;
pthread_t recepcionista;

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
    printf("ENTREGANDO PIZZA! - ");
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
    do {
        printf("==Esperando Sem SmartDeck==\n");
        sem_wait(&sem_smart_deck);
        // Recebeu o pedido e vai montá-lo
        // Retira o ticket de pedido
        pedido_t* pedido = (pedido_t*) queue_wait(&smart_deck);
        pizza_t* pizza = pizzaiolo_montar_pizza(pedido);
        sem_init(&(pizza->pizza_pronta),0,0);
        // Olha incessantemente para o forno buscando espaco para enfiar a pizza
        sem_wait(&sem_espacos_forno);
        printf("COLOQUEI NO FORNO -> Existem %d pizzas lá\n",(2-sem_ret_getValue(&sem_espacos_forno)));
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
        // E a utiliza para tirar a pizza dos profundos fogos do inferno
        pizzaiolo_retirar_forno(pizza);
        sem_post(&sem_espacos_forno);
        printf("TIREI DO FORNO -> Existem %d pizzas lá\n",(2-sem_ret_getValue(&sem_espacos_forno)));
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
    } while(pizzaria_is_open || sem_ret_getValue(&sem_smart_deck) || !sem_ret_getValue(&sem_pizzaria_done) );
    printf("[PIZZAIOLO] TERMINEI TD!\n");
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
    _total_mesas_n = n_mesas;
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
    //-------------------------RECEPCAO-DE-CLIENTES----------------------------------------
    queue_init(&recepcao, (n_grupos - 1)); //Pq no minimo tera um grupo ocupando todas as mesas
    sem_init(&sem_mesa_livre,0,1);

    // Chama o recepcionista para a recepcao
    pthread_create(&recepcionista,NULL,recepcao_func,NULL);
    sem_init(&sem_pizzaria_done,0,0);
    //-------------------------RECEPCAO-DE-CLIENTES----------------------------------------
}

void pizzeria_close() {
    // Fecha as portas da pizzaria com portas de ferro e toras
    pizzaria_is_open = 0;
    printf("FECHANDO PIZZARIA !! &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n\n\n");
}

void pizzeria_destroy() {
    printf("ESPERANDO CLIENTES SAIREM!===================================================\n");
    sem_wait(&sem_pizzaria_done);
    printf("[LOG] [SMART_DESK] [LENGTH] %d\n",smart_deck.size);
    printf("[LOG] [RECEPCAO] [LENGTH] %d\n",recepcao.size);
    printf("[LOG] [MESAS] [VAGAS] %d\n",_mesas_vagas_n);
    printf("\n\n\nBOOOMMMMMM!!!!!!\n\n\nDESUTRUINDO PIZZARIA!============================\n");
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
    pthread_mutex_destroy(&mtx_mesas);
            // Grupos/Pessoas
    //pthread_mutex_destroy(&mtx_grupos);
        // Outros
        // Pizziolos
    printf("%d\n",_pizzaiolos_n);
    for (int i = 0; i < _pizzaiolos_n; ++i) {
        printf("[PIZZAIOLO] ESPERANDO PIZZAIOLO %d\n",i);
        pthread_join(pizzaiolos[i],NULL);
        printf("[PIZZAIOLO %d] Retornou \n",i);
    }
    free(pizzaiolos);
    //-------------------------COZINHA-E-GAROCONS(REL_COZINHA)-----------------------------
    //-------------------------RECEPCAO-DE-CLIENTES----------------------------------------
    sem_destroy(&sem_mesa_livre);

    queue_destroy(&recepcao);
    // Espera o recepcionista voltar para casa
    int ret_val = 0;
    void* ptr_ret_val = (void*)&ret_val;
    pthread_join(recepcionista,&ptr_ret_val);
    printf("[RECEPCIONISTA] Retornou com status %d\n",WEXITSTATUS(ret_val));
    //-------------------------RECEPCAO-DE-CLIENTES----------------------------------------
}

int pegar_mesas(int tam_grupo) {

    // Retorna erro se está fechado
    if (!pizzaria_is_open) {
        return -1;
    }

    // Caso nao esteja fechado

        // Calcula quantas mesas devem ser usadas
    int g_mesas = util_ceil(tam_grupo,4);

    // Se tiver mesas sobrandos pode entrar
    pthread_mutex_lock(&mtx_mesas);
    if ( _mesas_vagas_n && _mesas_vagas_n >= g_mesas ) {
        _mesas_vagas_n -= g_mesas;
        pthread_mutex_unlock(&mtx_mesas);
        return 0;
    } else {
        pthread_mutex_unlock(&mtx_mesas);
        int res = espera_recepcao(g_mesas);
        return res;
    }
}

int espera_recepcao(int g_mesas) {
    // Criar o ticket e espera dormindo pela resposta
    ticket_t* t = entrar_fila_recepcao(g_mesas);
    sem_wait(&(t->alert));
    int ref = t->refused;
    sem_destroy(&(t->alert));
    free(t);
    return ((ref) ? -1 : 0);
}

ticket_t* entrar_fila_recepcao(int g_mesas) {
    ticket_t* ticket = (ticket_t*) malloc(sizeof(ticket_t));
    sem_init(&(ticket->alert),0,0);
    ticket->g_mesas = g_mesas;
    ticket->refused = 1;
    queue_push_back(&recepcao,(void*)ticket);
    return ticket;
}

void* recepcao_func(void* args) {
    // Enquanto a pizaria estiver aberta ele está atendendo
    do {
        // Fica esperando por mesas vagas
        sem_wait(&sem_mesa_livre);
        // Verifica se tem vaga p/ alum deles
        for(int i = 0; i < recepcao.size;++i) {
            // Pega o ticket verifica tamanho e dá resposta
            ticket_t* t = (ticket_t*) queue_wait(&recepcao);
            if (!pizzaria_is_open) {
                // Pizzaria fechou com ele na fila
                // Diz q foi rejeitado
                t->refused = 1;
                // Manda o aviso
                sem_post(&(t->alert));
                break;
            }
            pthread_mutex_lock(&mtx_mesas);
            if(t->g_mesas <= _mesas_vagas_n) {
                // Pega as mesas
                _mesas_vagas_n -= (t->g_mesas);
                // Diz q n foi rejeitado
                t->refused = 0;
                // Manda o aviso
                sem_post(&(t->alert));
                pthread_mutex_unlock(&mtx_mesas);
                break;
            }
            pthread_mutex_unlock(&mtx_mesas);
            // Não entrou na pizzaria e continua na fila entao é reenserido
            queue_push_back(&recepcao,t);
        }
        // Vai esperar mais alguem sair para verificar a fila
        printf("[RECEPCAO] [STATUS] [EM FILA] %d Grupos.\n",recepcao.size);
    } while(pizzaria_is_open || recepcao.size);

    return NULL;
}

void garcom_tchau(int tam_grupo) {
    // Pega o numero de mesas utilizadas
    int g_mesas = util_ceil(tam_grupo,4);
    // As libera
    pthread_mutex_lock(&mtx_mesas);
    _mesas_vagas_n += g_mesas;
    // Avisa a recepcao se ele n estiver verificando ninguém
    // int l = sem_ret_getValue(&sem_mesa_livre);
    // if (!l) { sem_post(&sem_mesa_livre); }
    // Verifica se é o ultimo cliente
    printf("[CLIENTES] [TCHAU] VAGARAM %d MESAS!!\n",g_mesas);
    printf("[CLIENTES] [TCHAU] SOBRARAM %d MESAS OCUPADOS !! \n",(_total_mesas_n - _mesas_vagas_n));
    if ( _mesas_vagas_n == _total_mesas_n ) {
        sem_post(&sem_pizzaria_done);
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

// Utils zone

int sem_ret_getValue(sem_t* sem_ptr) {
    int k = 0;
    sem_getvalue(sem_ptr,&k);
    return k;
}

int util_ceil(int value,int divider) {
    int k = value;
    k  = value / divider;
    k += (value % 4) ? 1 : 0;
    return k;
}