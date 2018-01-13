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

int colaJuez[10];/*La cola guarda una referencia en cada posicion al coche que la ocupa*/
int atletasIntroducidos;
int mejoresPuntuaciones[3]={0,0,0};
int mejoresAtletas[3]={100,100,100};
char id[10];
char msg[100];
pthread_t juez1;
pthread_t juez2;


void writeLogMessage(char *id,char *msg);
void *accionesJuez(void* manejadora);
int calculoAleatorio(int max, int min);


struct atletas{
	int numeroAtleta;
	int deshidratado;
	int ha_competido;
	int tarimaAsignada;
};

struct atletas *punteroAtletas;



/*Mutex para los jueces y sus colas*/
pthread_mutex_t controladorColaJueces;/*controla que no entren a la cola dos atletas y  evita que tengan la misma posicion en la cola*/
pthread_mutex_t controladorJuez1;/*contralara que dos atletas de la cola no intenten entrar al mismo tarima/juez*/
pthread_mutex_t controladorJuez2;/*contralara que dos atletas de la cola no intenten entrar al mismo tarima/juez*/
pthread_mutex_t controladorPodium;
pthread_mutex_t controladorEscritura;/*controlara que no mas de dos coches intenten escribir en el fichero*/


FILE *logFile;
char* logFileName ="registro.log";


int main(){



	/*Inicializamos los semaforos*/
	pthread_mutex_init(&controladorColaJueces,NULL);
	pthread_mutex_init(&controladorJuez1,NULL);
	pthread_mutex_init(&controladorJuez2,NULL);
	pthread_mutex_init(&controladorPodium,NULL);
	pthread_mutex_init(&controladorEscritura,NULL);

   	/*Inicializamos la cola, como no tenemos 100 atletas será nuestro valor de control*/
   	int i;
	for(i=0;i<NUMATLETAS;i++){
		colaJuez[i]=100;
	}

   	/*Lanzamos losjueces*/
	i=1;
	pthread_create(&juez1, NULL, accionesJuez,(void*)&i);
	i=2;	
	pthread_create(&juez2, NULL, accionesJuez,(void*)&i);



}

void *accionesJuez(void* manejadora){

	int idJuez= *(int*)manejadora;
	int i=1;
	int tiempoEnTarima;
	int puntuacionEjercicio;
	int atletasAtendidos=0;
	int atletaActual=100;
	int j, k;
	int probabilidadMovimiento;
	int probabilidadAgua;

	while(i==1){
		
		pthread_mutex_lock(&controladorColaJueces);
		/*Buscamos en la cola el primer atleta que pertenezca a la tarima*/
		for(j=0;j<10;j++){
			if(colaJuez[j]!=100){
				atletaActual=colaJuez[j];
				if(punteroAtletas[atletaActual].tarimaAsignada==idJuez){
					/*avanzamos la cola*/
					for(k=1;k<10;j++){
						colaJuez[k-1]=colaJuez[j];
					}
					break;
				}
			}
		}
		pthread_mutex_unlock(&controladorColaJueces);

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
				sleep(tiempoEnTarima);
				puntuacionEjercicio=calculoAleatorio(300,60);
				/*comprobamos si la posicion pertenece al podium*/
				pthread_mutex_lock(&controladorPodium);
				for(j=0;j<3;j++){
					if(puntuacionEjercicio>mejoresPuntuaciones[j]){
						/*guaradmos la puntuacion y el id en el podium*/
						if(j==2){
							mejoresPuntuaciones[j]=puntuacionEjercicio;
							mejoresAtletas[j]=punteroAtletas[atletaActual].numeroAtleta;
							break;
						}else if(j==1){
							mejoresPuntuaciones[j+1]=mejoresPuntuaciones[j];
							mejoresPuntuaciones[j]=puntuacionEjercicio;

							mejoresAtletas[j+1]=mejoresAtletas[j];
							mejoresAtletas[j]=punteroAtletas[atletaActual].numeroAtleta;
							break;
						}else{
							mejoresPuntuaciones[j+2]=mejoresPuntuaciones[j+1];
							mejoresPuntuaciones[j+1]=mejoresPuntuaciones[j];
							mejoresPuntuaciones[j]=puntuacionEjercicio;

							mejoresAtletas[j+2]=mejoresAtletas[j+1];
							mejoresAtletas[j+1]=mejoresAtletas[j];
							mejoresAtletas[j]=punteroAtletas[atletaActual].numeroAtleta;
							break;
						}
					}
				}
				pthread_mutex_unlock(&controladorPodium);

				pthread_mutex_lock(&controladorEscritura);
				sprintf(msg,"tiene una puntuacion de %d",puntuacionEjercicio);
				sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
				writeLogMessage(id,msg);
				pthread_mutex_unlock(&controladorEscritura);
				probabilidadAgua=calculoAleatorio(10,1);
				if(probabilidadAgua==1){
					punteroAtletas[atletaActual].deshidratado=1;
				}
				atletasAtendidos++;
			}
			/*Descalificado normativa*/
			if(probabilidadMovimiento==9){
				tiempoEnTarima=calculoAleatorio(4,1);
				sleep(tiempoEnTarima);
				pthread_mutex_lock(&controladorEscritura);
				sprintf(msg,"ha sido descalificado por no cumplir la normativa");
				sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
				writeLogMessage(id,msg);
				pthread_mutex_unlock(&controladorEscritura);
				atletasAtendidos++;
			}
			/*Levantamiento fallido*/
			if(probabilidadMovimiento==10){
				tiempoEnTarima=calculoAleatorio(10,6);
				sleep(tiempoEnTarima);
				pthread_mutex_lock(&controladorEscritura);
				sprintf(msg,"no ha realizado un movimiento válido");
				sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
				writeLogMessage(id,msg);
				pthread_mutex_unlock(&controladorEscritura);
				atletasAtendidos++;

			}
			punteroAtletas[atletaActual].ha_competido=1;
			if(atletasAtendidos%4==0){
				pthread_mutex_lock(&controladorEscritura);
				sprintf(msg,"comienza el descanso");
				sprintf(id,"juez_%d",idJuez);
				writeLogMessage(id,msg);
				pthread_mutex_unlock(&controladorEscritura);
				sleep(10);
				pthread_mutex_lock(&controladorEscritura);
				sprintf(msg,"finaliza el descanso");
				sprintf(id,"juez_%d",idJuez);
				writeLogMessage(id,msg);
				pthread_mutex_unlock(&controladorEscritura);
	

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