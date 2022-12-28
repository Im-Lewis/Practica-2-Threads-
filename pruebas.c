#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

int n_habitantes = 1200;
int n_centros_vacunacion = 5;
int n_fabricas = 3;
int n_vacunados_tanda = 120;
int n_vacunas_ini = 15;
int n_vacunas_x_fabrica = 400;
int n_min_tanda = 25;
int n_max_tanda = 50;
int t_min_fabricacion_x_tanda = 20;
int t_max_fabricacion_x_tanda = 40;
int t_max_reparto = 3;
int t_habitante_ver_citacion = 4;
int t_max_desplazamiento_habitante = 2;


//-------------------------------------------------------------------------------------------------------------
//DECLARACION DE VARIABLES
//-------------------------------------------------------------------------------------------------------------
//contador de vacunas
int vacunas_total = 0;

//Contador de gente esperando en total
int gente_en_espera = 0;

pthread_mutex_t mutex;

//Condicion del mutex de los habitantes (una condicion por centro de vacunacion)
pthread_cond_t *cond;

//Centros de vacunacion
typedef struct {
    int n_vacunas;
    int n_gente_esperando;
    int n_vacunas_recibidas;
    int numero_centro;
} tCentro;

//Array de centros
tCentro *centros;

//Array de fabricas y array de argumentos de fabricas
pthread_t *fabricas;
int *args_fabricas;

//Array de habitantes y array de argumentos de habitantes
pthread_t *habitantes;
int *args_habitantes;

//cabeceras de funciones
void *fabricar_repartir_vacunas(void *arg);
void *vacunarse(void *arg);


//-------------------------------------------------------------------------------------------------------------
//COMIENZO DEL PROGRAMA
//-------------------------------------------------------------------------------------------------------------
int main(){
    //Iniciamos el generador de numeros aleatorios
    srand(time(NULL));

    //iniciamos el mutex
    pthread_mutex_init(&mutex, NULL);

    //Array de condiciones, una por cada centro de vacunacion
    cond = (pthread_cond_t *)calloc(n_centros_vacunacion, sizeof(pthread_cond_t));

    //iniciamos las condiciones del mutex
    for(int i = 0; i < n_centros_vacunacion; i++){
        pthread_cond_init(&cond[i], NULL);
    }

    //Array de centros de vacunacion
    centros = (tCentro *)calloc(n_centros_vacunacion, sizeof(tCentro));

    //Asignar el numero de centro
    for(int i = 1; i <= n_centros_vacunacion; i++){
        centros[i].numero_centro = i;
    }

    //Asignar numero de vacunas iniciales a cada centro
    for(int i = 0; i < n_centros_vacunacion; i++){
        centros[i].n_vacunas = n_vacunas_ini;
    }

    //Declaramos un array de hilos de fabricas de vacunas
    fabricas = (pthread_t *)calloc(n_fabricas, sizeof(pthread_t));

    //Declaramos un array con los argumentos que les pasaremos a cada hilo
    args_fabricas = (int *)calloc(n_fabricas, sizeof(int));

    //Creamos cada uno de los hilos de las fabricas
    for(int i = 1; i <= n_fabricas; i++){
        args_fabricas[i-1] = i;
        pthread_create(&fabricas[i], NULL, fabricar_repartir_vacunas, &args_fabricas[i-1]);
    }

    //Creamos cada uno de los hilos de los habitantes
    for(int i = 1; i <= (n_habitantes/n_vacunados_tanda); i++){
        //Array de hilos de habitantes
        habitantes = (pthread_t *)calloc(n_habitantes, sizeof(pthread_t));

        //Declaramos una array con los argumentos que les pasaremos a cada hilo
        args_habitantes = (int *)calloc(n_habitantes, sizeof(int));

        for(int j = ((120*(i-1))+1); j <= (120*i); j++){
            args_habitantes[j-1] = j;
            pthread_create(&habitantes[j], NULL, vacunarse, &args_habitantes[j-1]);
        }

        //Esperar a que terminen los hilos de los habitantes
        for (int k = 1; k <= n_habitantes; k++){
            pthread_join(habitantes[k], NULL);
        }
    }

    //Esperamos a que terminen todos los hilos de las fabricas de vacunas
    for(int i = 1; i <= n_fabricas; i++){
        pthread_join(fabricas[i], NULL);
    }

    //Liberamos el mutex
    pthread_mutex_destroy(&mutex);

    //Liberamos la condicion
    for(int i = 0; i < n_centros_vacunacion; i++){
        pthread_cond_destroy(&cond[i]);
    }

    printf("Vacunacion finalizada\n");
    return 0;
}


//-------------------------------------------------------------------------------------------------------------
//FUNCIONES AUXILIARES
//-------------------------------------------------------------------------------------------------------------
void *fabricar_repartir_vacunas(void *arg){
    int vac_fabricadas = 0;
    int vac_repartir;
    int num_fabrica = *((int*)arg);

    while(vac_fabricadas < n_vacunas_x_fabrica) {
        //tiempo aleatorio de fabricacion de las vacunas
        int t_fabricacion = t_min_fabricacion_x_tanda + rand() % (t_max_fabricacion_x_tanda - t_min_fabricacion_x_tanda + 1);
        sleep(t_fabricacion);

        //numero aleatorio de vacunas fabricadas en esta tanda
        int num_vacunas = n_min_tanda + rand() % (n_max_tanda - n_min_tanda + 1);

        //Si el random es mayor que las que quedan lo cambio por el numero que falta por fabricar
        if (num_vacunas > n_vacunas_x_fabrica - vac_fabricadas) {
            num_vacunas = n_vacunas_x_fabrica - vac_fabricadas;
        }

        vac_repartir = num_vacunas;
        vacunas_total += num_vacunas;
        printf("Fabrica %d prepara %d vacunas\n", num_fabrica, num_vacunas);

        //sumar al contador de fabricadas de esa fabrica
        vac_fabricadas += num_vacunas;

        //Tiempo en repartir esta tanda de vacunas
        int t_reparto = 1 + rand() % (t_max_reparto - 1 + 1);

        //Adquirimos el mutex para repartir las vacunas entre los distintos centros
        pthread_mutex_lock(&mutex);

        //repartir entre los 5 centros si hay gente en espera
        //Si hay gente en cola vamos terminando las colas en orden hasta que no haya gente en cola

        //Tiempo de reparto de la tanda de vacunas
        sleep (t_reparto);

        for(int i = 0; i < n_centros_vacunacion; i++){
            //Si hay gente en cola y tengo mas de las que necesitan
            if ((centros[i].n_gente_esperando > 0) && (vac_repartir > centros[i].n_gente_esperando)) {
                centros[i].n_vacunas += centros[i].n_gente_esperando;
                vac_repartir -= centros[i].n_gente_esperando;

                printf("Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, centros[i].n_gente_esperando, centros[i].numero_centro);
                pthread_cond_broadcast(&cond[i]);

            } else if ((centros[i].n_gente_esperando > 0) && (vac_repartir < centros[i].n_gente_esperando)){
                centros[i].n_vacunas += vac_repartir;
                vac_repartir = 0;

                printf("Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, centros[i].n_gente_esperando, centros[i].numero_centro);
                pthread_cond_broadcast(&cond[i]);

            }
        }
        //si no hay gente en cola las repartimos de una en una
        if((gente_en_espera == 0) && (vac_repartir > 0)){
            //Tiempo de reparto de la tanda de vacunas
            sleep (t_reparto);

            while(vac_repartir > 0){
                for (int i = 0; i < n_centros_vacunacion; i++) {
                    centros[i].n_vacunas++;
                    vac_repartir--;

                    printf("Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, 1, centros[i].numero_centro);
                    pthread_cond_broadcast(&cond[i]);
                }
            }
        }

        //Liberamos el mutex
        pthread_mutex_unlock(&mutex);
    }

    printf("Fabrica %d ha fabricado todas sus vacunas\n", num_fabrica);
    pthread_exit(NULL);
    return NULL;
}


//-------------------------------------------------------------------------------------------------------------
void *vacunarse(void *arg){
    int num_habitante = *((int*)arg);

    //Tiempo en darse cuenta de que se tiene que ir a vacunar
    int t_ver_citacion = 1 + rand() % (t_habitante_ver_citacion - 1 + 1);
    sleep(t_ver_citacion);

    //Seleccion del centro de vacunacion
    int centro_selec = 1 + rand() % (n_centros_vacunacion - 1 + 1);

    printf("Habitante %d elige el centro %d para vacunarse\n", num_habitante, centro_selec);

    //Tiempo en desplazarse al centro de vacunacion
    int t_desplazarse = 1 + rand() % (t_max_desplazamiento_habitante - 1 + 1);
    sleep(t_desplazarse);

    //Adquirimos el mutex para restar uno a las vacunas del centro
    pthread_mutex_lock(&mutex);

    if (centros[centro_selec-1].n_vacunas > 0){
        //El habitante se vacuna
        centros[centro_selec-1].n_vacunas--;

        printf("Habitante %d vacunado en el centro %d\n", num_habitante, centro_selec);
    }
    else{
        //Sumamos uno a la lista de espera del centro
        centros[centro_selec-1].n_gente_esperando++;
        gente_en_espera++;

        //bloquear ejecucion del hilo con mutex con condicion para esperar hasta que haya vacunas
        while(centros[centro_selec-1].n_vacunas == 0){
            pthread_cond_wait(&cond[centro_selec-1], &mutex);
        }

        //Restamos uno al numero de vacunas
        centros[centro_selec].n_vacunas--;

        printf("Habitante %d vacunado en el centro %d\n", num_habitante, centro_selec);

        //Sale de la cola de espera del centro
        centros[centro_selec-1].n_gente_esperando--;
        gente_en_espera--;
    }

    //Liberamos el mutex
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
    return NULL;
}
