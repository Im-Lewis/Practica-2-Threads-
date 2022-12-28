#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>


int n_habitantes;
int n_centros_vacunacion;
int n_fabricas;
int n_vacunados_tanda;
int n_vacunas_ini;
int n_vacunas_x_fabrica;
int n_min_tanda;
int n_max_tanda;
int t_min_fabricacion_x_tanda;
int t_max_fabricacion_x_tanda;
int t_max_reparto;
int t_hab_ver_citacion;
int t_max_desplazamiento_hab;


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

//Estructura para informe final fabricas
typedef struct {
    int vacunas_fabricadas;
    int *vacunas_entregadas_centros;
} tStatsFabricas;

tStatsFabricas *stats_fabricas;

//Estructura para informe final centros
typedef struct {
    int vacunas_recibidas;
    int habitantes_vacunados;
    int vacunas_sobrantes;
}tStatsCentros;

tStatsCentros * stats_centros;

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

//Creacion o apertura del fichero para escribir la salida
FILE *out_file;

//Lectura de datos del fichero
FILE *in_file;


//-------------------------------------------------------------------------------------------------------------
//COMIENZO DEL PROGRAMA
//-------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]){
    //Entrada y salida dependiendo de los argumentos
    if (argc == 1){
        //Apertura del fichero para escribir la salida
        out_file= fopen("salida_vacunacion.txt", "w");

        //Asignar los valores a las variables
        n_habitantes = 1200;
        n_centros_vacunacion = 5;
        n_fabricas = 3;
        n_vacunados_tanda = 120;
        n_vacunas_ini = 15;
        n_vacunas_x_fabrica = 400;
        n_min_tanda = 25;
        n_max_tanda = 50;
        t_min_fabricacion_x_tanda = 20;
        t_max_fabricacion_x_tanda = 40;
        t_max_reparto = 3;
        t_hab_ver_citacion = 4;
        t_max_desplazamiento_hab = 2;
    }
    else if(argc == 2){
        //Apertura del fichero para escribir la salida
        out_file= fopen("salida_vacunacion.txt", "w");

        //Fichero de lectura de datos
        in_file = fopen(argv[1], "r");

        //Asignar los valores a las variables
        fscanf(in_file, "%d %d %d %d %d %d %d %d %d %d %d %d %d", &n_habitantes, &n_centros_vacunacion, &n_fabricas,
               &n_vacunados_tanda, &n_vacunas_ini, &n_vacunas_x_fabrica, &n_min_tanda, &n_max_tanda, &t_min_fabricacion_x_tanda,
               &t_max_fabricacion_x_tanda, &t_max_reparto, &t_hab_ver_citacion, &t_max_desplazamiento_hab);

        fclose(in_file);
    }
    else if(argc == 3){
        //Apertura del fichero para escribir la salida
        out_file= fopen(argv[2], "w");

        //Fichero de lectura de datos
        in_file = fopen(argv[1], "r");

        //Asignar los valores a las variables
        fscanf(in_file, "%d %d %d %d %d %d %d %d %d %d %d %d %d", &n_habitantes, &n_centros_vacunacion, &n_fabricas,
               &n_vacunados_tanda, &n_vacunas_ini, &n_vacunas_x_fabrica, &n_min_tanda, &n_max_tanda, &t_min_fabricacion_x_tanda,
               &t_max_fabricacion_x_tanda, &t_max_reparto, &t_hab_ver_citacion, &t_max_desplazamiento_hab);

        fclose(in_file);
    }
    else{
        printf("Numero de argumentos incorrecto\n");
    }

    //Configuracion inicial del problema
    printf("VACUNACION EN PANDEMIA: CONFIGURACION INICIAL\n");
    fprintf(out_file, "VACUNACION EN PANDEMIA: CONFIGURACION INICIAL\n");
    printf("Habitantes: %d\n", n_habitantes);
    fprintf(out_file, "Habitantes: %d\n", n_habitantes);
    printf("Centros de vacunacion: %d\n", n_centros_vacunacion);
    fprintf(out_file, "Centros de vacunacion: %d\n", n_centros_vacunacion);
    printf("Fabricas: %d\n", n_fabricas);
    fprintf(out_file, "Fabricas: %d\n", n_fabricas);
    printf("Vacunados por tanda: %d\n", n_vacunados_tanda);
    fprintf(out_file, "Vacunados por tanda: %d\n", n_vacunados_tanda);
    printf("Vacunas inicales en cada centro: %d\n", n_vacunas_ini);
    fprintf(out_file, "Vacunas inicales en cada centro: %d\n", n_vacunas_ini);
    printf("Vacunas totales por fabrica: %d\n", n_vacunas_x_fabrica);
    fprintf(out_file, "Vacunas totales por fabrica: %d\n", n_vacunas_x_fabrica);
    printf("Minimo numero de vacunas fabricadas en cada tanda: %d\n", n_min_tanda);
    fprintf(out_file, "Minimo numero de vacunas fabricadas en cada tanda: %d\n", n_min_tanda);
    printf("Maximo numero de vacunas fabricadas en cada tanda: %d\n", n_max_tanda);
    fprintf(out_file, "Maximo numero de vacunas fabricadas en cada tanda: %d\n", n_max_tanda);
    printf("Tiempo minimo de fabricacion de una tanda de vacunas: %d\n", t_min_fabricacion_x_tanda);
    fprintf(out_file, "Tiempo minimo de fabricacion de una tanda de vacunas: %d\n", t_min_fabricacion_x_tanda);
    printf("Tiempo maximo de fabricacion de una tanda de vacunas: %d\n", t_max_fabricacion_x_tanda);
    fprintf(out_file, "Tiempo maximo de fabricacion de una tanda de vacunas: %d\n", t_max_fabricacion_x_tanda);
    printf("Tiempo maximo de reparto de vacunas a los centros: %d\n", t_max_reparto);
    fprintf(out_file, "Tiempo maximo de reparto de vacunas a los centros: %d\n", t_max_reparto);
    printf("Tiempo maximo que un habitante tarda en ver que esta citado para vacunarse: %d\n", t_hab_ver_citacion);
    fprintf(out_file, "Tiempo maximo que un habitante tarda en ver que esta citado para vacunarse: %d\n", t_hab_ver_citacion);
    printf("Tiempo maximo de desplazamineto del habitante al centro de vacunacion: %d\n", t_max_desplazamiento_hab);
    fprintf(out_file, "Tiempo maximo de desplazamineto del habitante al centro de vacunacion: %d\n", t_max_desplazamiento_hab);
    printf("\nPROCESO DE VACUNACION\n\n");
    fprintf(out_file, "\nPROCESO DE VACUNACION\n\n");

    //Iniciamos el generador de numeros aleatorios
    srand(time(NULL));

    //iniciamos el mutex
    pthread_mutex_init(&mutex, NULL);

    //iniciamos la condicion del mutex
    pthread_cond_init(&hay_vacunas, NULL);

    //Arrays para informe final y array de vacunas entregadas a cada centro
    stats_fabricas = (tStatsFabricas *)calloc(n_fabricas, sizeof(tStatsFabricas));
    for(int i = 0; i < n_fabricas; i++){
        stats_fabricas[i].vacunas_entregadas_centros = (int *) calloc(n_centros_vacunacion, sizeof(int));
    }

    stats_centros = (tStatsCentros *)calloc(n_centros_vacunacion, sizeof(tStatsCentros));

    //Array de centrosde vacunacion
    centros = (tCentro *)calloc(n_centros_vacunacion, sizeof(tCentro));

    //Array de fabricas y argumentos que le pasaremos a la fabrica
    fabricas = (pthread_t *)calloc(n_fabricas, sizeof(pthread_t));
    args_fabricas = (int *)calloc(n_fabricas, sizeof(int));

    //Array de habitantes y argumentos que le pasaremos a los habitantes
    habitantes = (pthread_t *)calloc(n_habitantes, sizeof(pthread_t));
    args_habitantes = (int *)calloc(n_habitantes, sizeof(int));

    //Asignar el numero de centro
    for(int i = 0; i < n_centros_vacunacion; i++){
        centros[i].numero_centro = i+1;
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

    //Vacunas sobrantes
    for(int i = 0; i < n_centros_vacunacion; i++){
        stats_centros[i].vacunas_sobrantes += centros[i].n_vacunas;
    }

    printf("Vacunacion finalizada\n");
    fprintf(out_file, "Vacunacion finalizada\n");

    //Informe de la vacunacion
    printf("\nINFORME DE LA VACUNACION\n");
    fprintf(out_file, "\nINFORME DE LA VACUNACION\n");
    printf("Fabrica 1: \n");
    fprintf(out_file, "Fabrica 1: \n");
    printf("   Fabricadas: %d\n", stats_fabricas[0].vacunas_fabricadas);
    fprintf(out_file, "   Fabricadas: %d\n", stats_fabricas[0].vacunas_fabricadas);
    printf("   Entregadas: %d | %d | %d | %d | %d\n", stats_fabricas[0].vacunas_entregadas_centros[0],
           stats_fabricas[0].vacunas_entregadas_centros[1], stats_fabricas[0].vacunas_entregadas_centros[2],
           stats_fabricas[0].vacunas_entregadas_centros[3], stats_fabricas[0].vacunas_entregadas_centros[4]);
    fprintf(out_file, "   Entregadas: %d | %d | %d | %d | %d\n", stats_fabricas[0].vacunas_entregadas_centros[0],
            stats_fabricas[0].vacunas_entregadas_centros[1], stats_fabricas[0].vacunas_entregadas_centros[2],
            stats_fabricas[0].vacunas_entregadas_centros[3], stats_fabricas[0].vacunas_entregadas_centros[4]);

    printf("Fabrica 2: \n");
    fprintf(out_file, "Fabrica 2: \n");
    printf("   Fabricadas: %d\n", stats_fabricas[1].vacunas_fabricadas);
    fprintf(out_file, "   Fabricadas: %d\n", stats_fabricas[1].vacunas_fabricadas);
    printf("   Entregadas: %d | %d | %d | %d | %d\n", stats_fabricas[1].vacunas_entregadas_centros[0],
           stats_fabricas[1].vacunas_entregadas_centros[1], stats_fabricas[1].vacunas_entregadas_centros[2],
           stats_fabricas[1].vacunas_entregadas_centros[3], stats_fabricas[1].vacunas_entregadas_centros[4]);
    fprintf(out_file, "   Entregadas: %d | %d | %d | %d | %d\n", stats_fabricas[1].vacunas_entregadas_centros[0],
            stats_fabricas[1].vacunas_entregadas_centros[1], stats_fabricas[1].vacunas_entregadas_centros[2],
            stats_fabricas[1].vacunas_entregadas_centros[3], stats_fabricas[1].vacunas_entregadas_centros[4]);

    printf("Fabrica 3: \n");
    fprintf(out_file, "Fabrica 3: \n");
    printf("   Fabricadas: %d\n", stats_fabricas[2].vacunas_fabricadas);
    fprintf(out_file, "   Fabricadas: %d\n", stats_fabricas[2].vacunas_fabricadas);
    printf("   Entregadas: %d | %d | %d | %d | %d\n", stats_fabricas[2].vacunas_entregadas_centros[0],
           stats_fabricas[2].vacunas_entregadas_centros[1], stats_fabricas[2].vacunas_entregadas_centros[2],
           stats_fabricas[2].vacunas_entregadas_centros[3], stats_fabricas[2].vacunas_entregadas_centros[4]);
    fprintf(out_file, "   Entregadas: %d | %d | %d | %d | %d\n", stats_fabricas[2].vacunas_entregadas_centros[0],
            stats_fabricas[2].vacunas_entregadas_centros[1], stats_fabricas[2].vacunas_entregadas_centros[2],
            stats_fabricas[2].vacunas_entregadas_centros[3], stats_fabricas[2].vacunas_entregadas_centros[4]);

    printf("\nCentro 1: \n");
    fprintf(out_file, "\nCentro 1: \n");
    printf("   Recibidas: %d", stats_centros[0].vacunas_recibidas);
    fprintf(out_file, "   Recibidas: %d", stats_centros[0].vacunas_recibidas);
    printf("   Vacunados: %d", stats_centros[0].habitantes_vacunados);
    fprintf(out_file, "   Vacunados: %d", stats_centros[0].habitantes_vacunados);
    printf("   Sobrantes : %d", stats_centros[0].vacunas_sobrantes);
    fprintf(out_file, "   Sobrantes : %d", stats_centros[0].vacunas_sobrantes);

    printf("\nCentro 2: \n");
    fprintf(out_file, "\nCentro 2: \n");
    printf("   Recibidas: %d", stats_centros[1].vacunas_recibidas);
    fprintf(out_file, "   Recibidas: %d", stats_centros[1].vacunas_recibidas);
    printf("   Vacunados: %d", stats_centros[1].habitantes_vacunados);
    fprintf(out_file, "   Vacunados: %d", stats_centros[1].habitantes_vacunados);
    printf("   Sobrantes : %d", stats_centros[1].vacunas_sobrantes);
    fprintf(out_file, "   Sobrantes : %d", stats_centros[1].vacunas_sobrantes);

    printf("\nCentro 3: \n");
    fprintf(out_file, "\nCentro 3: \n");
    printf("   Recibidas: %d", stats_centros[2].vacunas_recibidas);
    fprintf(out_file, "   Recibidas: %d", stats_centros[2].vacunas_recibidas);
    printf("   Vacunados: %d", stats_centros[2].habitantes_vacunados);
    fprintf(out_file, "   Vacunados: %d", stats_centros[2].habitantes_vacunados);
    printf("   Sobrantes : %d", stats_centros[2].vacunas_sobrantes);
    fprintf(out_file, "   Sobrantes : %d", stats_centros[2].vacunas_sobrantes);

    printf("\nCentro 4: \n");
    fprintf(out_file, "\nCentro 4: \n");
    printf("   Recibidas: %d", stats_centros[3].vacunas_recibidas);
    fprintf(out_file, "   Recibidas: %d", stats_centros[3].vacunas_recibidas);
    printf("   Vacunados: %d", stats_centros[3].habitantes_vacunados);
    fprintf(out_file, "   Vacunados: %d", stats_centros[3].habitantes_vacunados);
    printf("   Sobrantes : %d", stats_centros[3].vacunas_sobrantes);
    fprintf(out_file, "   Sobrantes : %d", stats_centros[3].vacunas_sobrantes);

    printf("\nCentro 5: \n");
    fprintf(out_file, "\nCentro 5: \n");
    printf("   Recibidas: %d", stats_centros[4].vacunas_recibidas);
    fprintf(out_file, "   Recibidas: %d", stats_centros[4].vacunas_recibidas);
    printf("   Vacunados: %d", stats_centros[4].habitantes_vacunados);
    fprintf(out_file, "   Vacunados: %d", stats_centros[4].habitantes_vacunados);
    printf("   Sobrantes : %d", stats_centros[4].vacunas_sobrantes);
    fprintf(out_file, "   Sobrantes : %d", stats_centros[4].vacunas_sobrantes);

    fclose(out_file);

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
        fprintf(out_file, "Fabrica %d prepara %d vacunas\n", num_fabrica, num_vacunas);

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
                centros[i].n_vacunas = centros[i].n_vacunas + centros[i].n_gente_esperando;
                num_vacunas = num_vacunas - centros[i].n_gente_esperando;

                printf("Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, centros[i].n_gente_esperando, centros[i].numero_centro);
                fprintf(out_file, "Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, centros[i].n_gente_esperando, centros[i].numero_centro);

                //vacunas entregadas al centro i
                stats_fabricas[num_fabrica-1].vacunas_entregadas_centros[i] = stats_fabricas[num_fabrica-1].vacunas_entregadas_centros[i] + centros[i].n_gente_esperando;

                //Vacunas recibidas en el centro i
                stats_centros[i].vacunas_recibidas = stats_centros[i].vacunas_recibidas + centros[i].n_gente_esperando;

            }
            else if ((centros[i].n_gente_esperando > 0) && (num_vacunas < centros[i].n_gente_esperando)){
                centros[i].n_vacunas = centros[i].n_vacunas + num_vacunas;

                printf("Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, num_vacunas, centros[i].numero_centro);
                fprintf(out_file, "Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, num_vacunas, centros[i].numero_centro);

                //Vacunas entregadas al centro i
                stats_fabricas[num_fabrica-1].vacunas_entregadas_centros[i] += num_vacunas;

                //Vacunas recibidas en el centro i
                stats_centros[i].vacunas_recibidas += centros[i].n_gente_esperando;
                num_vacunas = 0;
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
                    fprintf(out_file, "Fabrica %d entrega %d vacunas en el centro %d\n", num_fabrica, 1, centros[i].numero_centro);

                    //Vacunas entregadas al centro i
                    stats_fabricas[num_fabrica-1].vacunas_entregadas_centros[i]++;

                    //Vacunas recibidas en el centro i
                    stats_centros[i].vacunas_recibidas++;
                }
            }
        }

        //Enviamos señal de que hay vacunas
        pthread_cond_broadcast(&hay_vacunas);

        //Liberamos el mutex
        pthread_mutex_unlock(&mutex);
    }

    //vacunas fabricadas en cada fabrica
    stats_fabricas[num_fabrica-1].vacunas_fabricadas = vac_fabricadas;

    printf("Fabrica %d ha fabricado todas sus vacunas\n", num_fabrica);
    fprintf(out_file, "Fabrica %d ha fabricado todas sus vacunas\n", num_fabrica);

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
    fprintf(out_file, "Habitante %d elige el centro %d para vacunarse\n", num_habitante, centro_selec);

    //Tiempo en desplazarse al centro de vacunacion
    int t_desplazarse = 1 + rand() % (t_max_desplazamiento_hab - 1 + 1);
    sleep(t_desplazarse);

    //Adquirimos el mutex para restar uno a las vacunas del centro
    pthread_mutex_lock(&mutex);

    if (centros[centro_selec-1].n_vacunas > 0){
        //El habitante se vacuna
        centros[centro_selec-1].n_vacunas--;

        //Vacunado en el centro centro_selec-1
        stats_centros[centro_selec-1].habitantes_vacunados += 1;

        printf("Habitante %d vacunado en el centro %d\n", num_habitante, centro_selec);
        fprintf(out_file, "Habitante %d vacunado en el centro %d\n", num_habitante, centro_selec);
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

        //Vacunado en el centro centro_selec-1
        stats_centros[centro_selec-1].habitantes_vacunados += 1;

        printf("Habitante %d vacunado en el centro %d\n", num_habitante, centro_selec);
        fprintf(out_file, "Habitante %d vacunado en el centro %d\n", num_habitante, centro_selec);
    }

    //Liberamos el mutex
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
    return NULL;
}
