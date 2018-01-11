#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>

#define NUMJUECES 2
#define NUMATLETAS 10

int colaJuez1[10];/*La cola guarda una referencia en cada posicion al coche que la ocupa*/
int colaJuez2[10];
char id[10];
char msg[100];


void writeLogMessage(char *id,char *msg);
void *accionesJuez(void* manejadora);
int calculoAleatorio(int max, int min);


struct atletas{
	int numeroAtleta;
	int deshidratado;
	int puntuacion;
};

struct atletas *punteroAtletas;


struct jueces{
	pthread_t juez;
	int ocupado;/*indica si el juez esta con algun atleta*/
	int identificadorJuez;
	int descansando;/*si el juez ha atendido a 4 atletas se pone a true*/
};

/*puntero que contiene los jueces*/

struct jueces *punteroJueces;

/*Mutex para los jueces y sus colas*/
pthread_mutex_t controladorColaJueces;/*controla que no entren a la cola dos atletas y  evita que tengan la misma posicion en la cola*/
pthread_mutex_t controladorJuez1;/*contralara que dos atletas de la cola no intenten entrar al mismo tarima/juez*/
pthread_mutex_t controladorJuez2;/*contralara que dos atletas de la cola no intenten entrar al mismo tarima/juez*/


pthread_mutex_t controladorEscritura;/*controlara que no mas de dos coches intenten escribir en el fichero*/


FILE *logFile;
char* logFileName ="registro.log";


int main(){
	/*Reservo memoria para los struct de los jueces*/

	punteroJueces = (struct jueces*)malloc(sizeof(struct jueces)*NUMJUECES);

	/*Inicializamos los jueces*/
	int i;
	for(i=0;i<NUMJUECES;i++){
   		punteroJueces[i].ocupado = 0;
    	punteroJueces[i].identificadorJuez = i+1;
    	punteroJueces[i].descansando = 0;
   	}

   	/*Inicializamos las colas, como no tenemos 100 atletas será nuestro valor de control*/
	for(i=0;i<NUMATLETAS;i++){
		colaJuez1[i]=100;
		colaJuez2[i]=100;
	}

   	/*Lanzamos losjueces*/
   	for(i=0;i<NUMJUECES;i++){
		pthread_create(&punteroJueces[i].juez,NULL,accionesJuez,(void*)&punteroJueces[i].identificadorJuez);
		
	}


}

void *accionesJuez(void* manejadora){
	int idJuez= *(int*)manejadora;
	int i=1;
	int tiempoEnTarima;
	int puntuacionEjercicio;
	int atletasAtendidos=0;
	int atletaActual;
	int j;
	int probabilidadMovimiento;
	int probabilidadAgua;

	while(i==1){
		if(idJuez==1){
			pthread_mutex_lock(&controladorColaJueces);
			if(colaJuez1[0]!=100){
				atletaActual = colaJuez1[0];
				for(j=1;j<10;j++){
					colaJuez1[j-1]=colaJuez1[j];
				}
				colaJuez1[9]=100;
			}else if(colaJuez2[0]!=100){
				atletaActual = colaJuez2[0];
				for(j=1;j<10;j++){
					colaJuez2[j-1]=colaJuez2[j];
				}
				colaJuez1[9]=100;
			}
			pthread_mutex_unlock(&controladorColaJueces);


		}else{
			pthread_mutex_lock(&controladorColaJueces);
			if(colaJuez2[0]!=100){
				atletaActual = colaJuez2[0];
				for(j=1;j<10;j++){
					colaJuez2[j-1]=colaJuez2[j];
				}
				colaJuez2[9]=100;
			}else if(colaJuez1[0]!=100){
				atletaActual = colaJuez1[0];
				for(j=1;j<10;j++){
					colaJuez1[j-1]=colaJuez1[j];
				}
				colaJuez1[9]=100;
			}
			pthread_mutex_unlock(&controladorColaJueces);

		}
		if(atletaActual!=100){
			probabilidadMovimiento=calculoAleatorio(10,1);
			pthread_mutex_lock(&controladorEscritura);
			sprintf(msg,"entra en la tarima %d",idJuez);
			sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
			writeLogMessage(id,msg);
			pthread_mutex_unlock(&controladorEscritura);
			/*Movimiento valido*/
			if(probabilidadMovimiento<9){
				tiempoEnTarima=calculoAleatorio(6,2);
				puntuacionEjercicio=calculoAleatorio(300,60);
				punteroAtletas[atletaActual].puntuacion=puntuacionEjercicio;
				pthread_mutex_lock(&controladorEscritura);
				sprintf(msg,"tiene una puntuacion de %d",punteroAtletas[atletaActual].puntuacion);
				sprintf(id,"coche_%d",punteroAtletas[atletaActual].numeroAtleta);
				writeLogMessage(id,msg);
				pthread_mutex_unlock(&controladorEscritura);
				probabilidadAgua=calculoAleatorio(10,1);
				if(probabilidadAgua==1){
					punteroAtletas[atletaActual].deshidratado=1;
				}

			}
			/*Descalificado normativa*/
			if(probabilidadMovimiento==9){
				tiempoEnTarima=calculoAleatorio(4,1);
				sleep(tiempoEnTarima);

			}
			/*Levantamiento fallido*/
			if(probabilidadMovimiento==10){
				tiempoEnTarima=calculoAleatorio(10,6);
				sleep(tiempoEnTarima);

			}
			if(atletasAtendidos%4){
				punteroJueces[idJuez].descansando=1;
					/*escribo en el fichero*/
				pthread_mutex_lock(&controladorEscritura);
				sprintf(msg,"descansa");
				sprintf(id,"juez_%d",idJuez);
				writeLogMessage(id,msg);
				pthread_mutex_unlock(&controladorEscritura);
				sleep(10);
				punteroJueces[idJuez].descansando=0;

			}
		}
	}

}

void writeLogMessage(char *id,char *msg){
	
	/*calculamos la hora actual*/
	time_t now = time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[19];
	strftime(stnow,19,"%d %m %y %H: %M: %S",tlocal);
	
	/*escribimos en el log*/
	logFile = fopen(logFileName, "a");
	fprintf(logFile, "[%s] %s: %s\n", stnow, id, msg);
	fclose(logFile);
		
}




int calculoAleatorio(int max, int min){
	
	srand(time(NULL));
	int numeroAleatorio = rand()%((max+1)-min)+min;
	return numeroAleatorio;
}