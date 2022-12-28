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
int t_hab_ver_citacion = 4;
int t_max_desplazamiento_hab = 2;


//-------------------------------------------------------------------------------------------------------------
//DECLARACION DE VARIABLES
//-------------------------------------------------------------------------------------------------------------
int vacunas_totales = 0;

pthread_mutex_t mutex;

//Condicion del mutex de los habitantes (una condicion por centro de vacunacion)
pthread_cond_t hay_vacunas;

//Contador de gente esperando en total
int gente_en_espera = 0;

//Centros de vacunacion
typedef struct {
    int n_vacunas;
    int n_gente_esperando;
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
    //Configuaracion inial del problema
    printf("VACUNACION EN PANDEMIA: CONFIGURACION INICIAL\n");
    printf("Habitantes: %d\n", n_habitantes);
    printf("Centros de vacunacion: %d\n", n_centros_vacunacion);
    printf("Fabricas: %d\n", n_fabricas);
    printf("Vacunados por tanda: %d\n", n_vacunados_tanda);
    printf("Vacunas inicales en cada centro: %d\n", n_vacunas_ini);
    printf("Vacunas totales por fabrica: %d\n", n_vacunas_x_fabrica);
    printf("Minimo numero de vacunas fabricadas en cada tanda: %d\n", n_min_tanda);
    printf("Maximo numero de vacunas fabricadas en cada tanda: %d\n", n_max_tanda);
    printf("Tiempo minimo de fabricacion de una tanda de vacunas: %d\n", t_min_fabricacion_x_tanda);
    printf("Tiempo masximo de fabricacion de una tanda de vacunas: %d\n", t_max_fabricacion_x_tanda);
    printf("Tiempo maximo de reparto de vacunas a los centros: %d\n", t_max_reparto);
    printf("Tiempo maximo que un habitante tarda en ver que esta citado para vacunarse: %d\n", t_hab_ver_citacion);
    printf("Tiempo maximo de desplazamineto del habitante al centro de vacunacion: %d\n", t_max_desplazamiento_hab);
    printf("\nPROCESO DE VACUNACION\n\n");

    //Iniciamos el generador de numeros aleatorios
    srand(time(NULL));

    //iniciamos el mutex
    pthread_mutex_init(&mutex, NULL);

    //iniciamos la condicion del mutex
    pthread_cond_init(&hay_vacunas, NULL);

    centros = (tCentro *)calloc(n_centros_vacunacion, sizeof(tCentro));

    fabricas = (pthread_t *)calloc(n_fabricas, sizeof(pthread_t));
    args_fabricas = (int *)calloc(n_fabricas, sizeof(int));

    habitantes = (pthread_t *)calloc(n_habitantes, sizeof(pthread_t));
    args_habitantes = (int *)calloc(n_habitantes, sizeof(int));

    //Asignar el numero de centro
    for(int i = 0; i < n_centros_vacunacion; i++){
        centros[i].numero_centro = i;
    }

    //Asignar numero de vacunas iniciales a cada centro
    for(int i = 0; i < n_centros_vacunacion; i++){
        centros[i].n_vacunas = n_vacunas_ini;
    }

    //Creamos cada uno de los hilos de las fabricas
    for(int i = 0; i < n_fabricas; i++){
        args_fabricas[i] = i+1;
        pthread_create(&fabricas[i], NULL, fabricar_repartir_vacunas, &args_fabricas[i]);
    }

    //Creamos cada uno de los hilos de los habitantes
    for(int i = 0; i < (n_habitantes/n_vacunados_tanda); i++){

        for(int j = ((120*(i))+1); j < 120+(120*i); j++){
            args_habitantes[j-1] = j;
            pthread_create(&habitantes[j-1], NULL, vacunarse, &args_habitantes[j-1]);
        }

        //Esperar a que terminen los hilos de los habitantes
        for (int k = ((120*(i))+1); k < 120+(120*i); k++){
            pthread_join(habitantes[k], NULL);
        }
    }

    //Esperamos a que terminen todos los hilos de las fabricas de vacunas
    for(int i = 0; i < n_fabricas; i++){
        pthread_join(fabricas[i], NULL);
    }

    //Liberamos el mutex
    pthread_mutex_destroy(&mutex);

    //Liberamos la condicion
    pthread_cond_destroy(&hay_vacunas);

    printf("Vacunacion finalizada\n");
    return 0;
}


//-------------------------------------------------------------------------------------------------------------
//FUNCIONES AUXILIARES
//-------------------------------------------------------------------------------------------------------------
void *fabricar_repartir_vacunas(void *arg){
    int num_fabrica = *((int*)arg);
    int vac_fabricadas = 0;

    while(vac_fabricadas != n_vacunas_x_fabrica) {
        //tiempo aleatorio de fabricacion de las vacunas
        int t_fabricacion = t_min_fabricacion_x_tanda + rand() % (t_max_fabricacion_x_tanda - t_min_fabricacion_x_tanda + 1);
        sleep(t_fabricacion);

        //numero aleatorio de vacunas fabricadas en esta tanda
        int num_vacunas = n_min_tanda + rand() % (n_max_tanda - n_min_tanda + 1);

        //Si el random es mayor que las que quedan lo cambio por el numero que falta por fabricar
        if (num_vacunas > (n_vacunas_x_fabrica - vac_fabricadas)){
            num_vacunas = n_vacunas_x_fabrica - vac_fabricadas;
        }

        //sumar al contador de fabricadas de esa fabrica
        vac_fabricadas += num_vacunas;

        //sumar a las vacunas totales
        vacunas_totales += num_vacunas;

        printf("Fabrica %d prepara %d vacunas\n", num_fabrica, num_vacunas);

        //Tiempo en repartir esta tanda de vacunas
        int t_reparto = 1 + rand() % (t_max_reparto - 1 + 1);

        //Adquirimos el mutex para repartir las vacunas entre los distintos centros
        pthread_mutex_lock(&mutex);

        //repartir entre los 5 centros si hay gente en espera

        //Tiempo de reparto de la tanda de vacunas
        sleep (t_reparto);

        for(int i = 0; i < n_centros_vacunacion; i++){
            //Si hay gente en cola y tengo mas de las que necesitan
            if ((centros[i].n_gente_esperando > 0) && (num_vacunas > centros[i].n_gente_esperando)) {
                centros[i].n_vacunas += centros[i].n_gente_esperando;
                num_vacunas -= centros[i].n_gente_esperando;

                printf("Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, centros[i].n_gente_esperando, centros[i].numero_centro);

            } else if ((centros[i].n_gente_esperando > 0) && (num_vacunas < centros[i].n_gente_esperando)){
                centros[i].n_vacunas += num_vacunas;
                num_vacunas = 0;

                printf("Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, num_vacunas, centros[i].numero_centro);

            }
        }
        //si no hay gente en cola las repartimos de una en una
        if(num_vacunas > 0){
            //Tiempo de reparto de la tanda de vacunas
            sleep (t_reparto);

            while(num_vacunas > 0){
                for (int i = 0; i < n_centros_vacunacion; i++) {
                    centros[i].n_vacunas++;
                    num_vacunas--;

                    printf("Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, 1, centros[i].numero_centro);
                }
            }
        }

        //Enviamos señal de que hay vacunas
        pthread_cond_broadcast(&hay_vacunas);

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
    int t_ver_citacion = 1 + rand() % (t_hab_ver_citacion - 1 + 1);
    sleep(t_ver_citacion);

    //Seleccion del centro de vacunacion
    int centro_selec = 1 + rand() % (n_centros_vacunacion - 1 + 1);

    printf("Habitante %d elige el centro %d para vacunarse\n", num_habitante, centro_selec);

    //Tiempo en desplazarse al centro de vacunacion
    int t_desplazarse = 1 + rand() % (t_max_desplazamiento_hab - 1 + 1);
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
            pthread_cond_wait(&hay_vacunas, &mutex);
        }

        //Restamos uno al numero de vacunas
        centros[centro_selec-1].n_vacunas--;

        //Sale de la cola de espera del centro
        centros[centro_selec-1].n_gente_esperando--;
        gente_en_espera--;

        printf("Habitante %d vacunado en el centro %d\n", num_habitante, centro_selec);
    }

    //Liberamos el mutex
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
    return NULL;
}
