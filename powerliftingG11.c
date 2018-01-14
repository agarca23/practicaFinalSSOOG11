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
int colaFuente[2];
int atletasIntroducidos=0;
int mejoresPuntuaciones[3]={0,0,0};
int mejoresAtletas[3]={100,100,100};
int fuenteOcupada;
char id[10];
char msg[100];
pthread_t juez1;
pthread_t juez2;

void nuevoCompetidorATarima1(int a);
void nuevoCompetidorATarima2(int a);
void inicializarAtleta(int posicionPuntero, int numeroAtleta, int deshidratado, int ha_competido, int tarimaAsignada);
void writeLogMessage(char *id,char *msg);
void *accionesJuez(void* manejadora);
int calculoAleatorio(int max, int min);
void accionesFuente();
void *accionesAtleta(void* manejadora);


struct atletas{
	pthread_t hiloAtleta;
	int numeroAtleta;
	int deshidratado;
	int ha_competido;
	int tarimaAsignada;
};

struct atletas *punteroAtletas;



/*Mutex para los jueces y sus colas*/
pthread_mutex_t controladorEntrada;/*Controla que dos atletas no entren al mismo tiempo*/
pthread_mutex_t controladorColaJueces;/*controla que no entren a la cola dos atletas y  evita que tengan la misma posicion en la cola*/
pthread_mutex_t controladorJuez1;/*contralara que dos atletas de la cola no intenten entrar al mismo tarima/juez*/
pthread_mutex_t controladorJuez2;/*contralara que dos atletas de la cola no intenten entrar al mismo tarima/juez*/
pthread_mutex_t controladorPodium;
pthread_mutex_t controladorEscritura;/*controlara que no mas de dos atletas o jueces intenten escribir en el fichero*/

FILE *logFile;
char* logFileName ="registro.log";


int main(){
	if(signal(SIGUSR1,nuevoCompetidorATarima1)==SIG_ERR){
		exit(-1);
	}
	if(signal(SIGUSR2,nuevoCompetidorATarima2)==SIG_ERR){
		exit(-1);
	}



	/*Inicializamos los semaforos*/
	pthread_mutex_init(&controladorEntrada, NULL);
	pthread_mutex_init(&controladorColaJueces,NULL);
	pthread_mutex_init(&controladorJuez1,NULL);
	pthread_mutex_init(&controladorJuez2,NULL);
	pthread_mutex_init(&controladorPodium,NULL);
	pthread_mutex_init(&controladorEscritura,NULL);

	/*Reservamos memoria para los atletas*/
	punteroAtletas = (struct atletas*)malloc(sizeof(struct atletas)*NUMATLETAS);
   	int i;
   	for(i=0;i<NUMATLETAS;i++){
    	punteroAtletas[i].numeroAtleta=0;
		punteroAtletas[i].deshidratado=0;
		punteroAtletas[i].ha_competido=0;
		punteroAtletas[i].tarimaAsignada=0;
  	}

   	/*Inicializamos la cola, como no tenemos 100 atletas será nuestro valor de control*/

	for(i=0;i<NUMATLETAS;i++){
		colaJuez[i]=100;
	}

   	/*Lanzamos losjueces*/
	i=1;
	pthread_create(&juez1, NULL, accionesJuez,(void*)&i);
	i=2;	
	pthread_create(&juez2, NULL, accionesJuez,(void*)&i);
	while(1){
		pause();
	}


}

void nuevoCompetidorATarima1(int a){
	if(signal(SIGUSR1,nuevoCompetidorATarima1)==SIG_ERR){
		exit(-1);
	}
	if(signal(SIGUSR2,nuevoCompetidorATarima2)==SIG_ERR){
		exit(-1);
	}
	pthread_mutex_lock(&controladorEntrada);
	if(atletasIntroducidos<NUMATLETAS){
		atletasIntroducidos++;
		int i,j;

		for(i=0;i<NUMATLETAS;i++){
			if(punteroAtletas[i].numeroAtleta==0){
				inicializarAtleta(i,atletasIntroducidos,0,0,1);

				pthread_create(&punteroAtletas[i].hiloAtleta,NULL,accionesAtleta,(void*)&punteroAtletas[i].numeroAtleta);
				/*Añadimos el atleta a la cola*/
				pthread_mutex_lock(&controladorColaJueces);
				for(j=0;j<10;j++){
					if(colaJuez[j]!=100){
						colaJuez[j]=i;
						break;
					}
				}
				pthread_mutex_unlock(&controladorColaJueces);

				pthread_mutex_lock(&controladorEscritura);
				sprintf(id,"atleta_%d",punteroAtletas[i].numeroAtleta);
				sprintf(msg,"entra el ");
				writeLogMessage(msg,id);
				pthread_mutex_unlock(&controladorEscritura);

				break;
			}
		}
	}
	
}

void nuevoCompetidorATarima2(int a){
	if(signal(SIGUSR1,nuevoCompetidorATarima1)==SIG_ERR){
		exit(-1);
	}
	if(signal(SIGUSR2,nuevoCompetidorATarima2)==SIG_ERR){
		exit(-1);
	}
	pthread_mutex_lock(&controladorEntrada);
	if(atletasIntroducidos<NUMATLETAS){
		atletasIntroducidos++;
		int i,j;

		for(i=0;i<NUMATLETAS;i++){
			if(punteroAtletas[i].numeroAtleta==0){
				inicializarAtleta(i,atletasIntroducidos,0,0,2);

				pthread_create(&punteroAtletas[i].hiloAtleta,NULL,accionesAtleta,(void*)&punteroAtletas[i].numeroAtleta);

				pthread_mutex_lock(&controladorColaJueces);
				for(j=0;j<10;j++){
					if(colaJuez[j]!=100){
						colaJuez[j]=i;
						break;
					}
				}
				pthread_mutex_unlock(&controladorColaJueces);
				pthread_mutex_lock(&controladorEscritura);
				sprintf(id,"atleta_%d",punteroAtletas[i].numeroAtleta);
				sprintf(msg,"entra el ");
				writeLogMessage(msg,id);
				pthread_mutex_unlock(&controladorEscritura);

				break;
			}
		}
	}
	
}

void inicializarAtleta(int posicionPuntero, int numeroAtleta, int deshidratado, int ha_competido, int tarimaAsignada){
	
	punteroAtletas[posicionPuntero].numeroAtleta = numeroAtleta;
	punteroAtletas[posicionPuntero].deshidratado = deshidratado;
	punteroAtletas[posicionPuntero].ha_competido = ha_competido;
	punteroAtletas[posicionPuntero].tarimaAsignada = tarimaAsignada;
	
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

void accionesFuente(){
	
}


int calculoAleatorio(int max, int min){
	
	srand(time(NULL));
	int numeroAleatorio = rand()%((max+1)-min)+min;
	return numeroAleatorio;
}

void *accionesAtleta(void* manejadora){

	int comportamiento;
	int tiempoEspera;
	int subeTarima = 0;
	int enEspera=0; //Este es el tiempo que el atleta lleva en espera en una tarima

	pthread_mutex_lock(&controladorEscritura);
	sprintf(msg, "Ha llegado a la tarima %d el atleta",punteroAtletas[atletaActual].tarimaAsignada);
	sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
	writeLogMessage(id,msg);
	pthread_mutex_unlock(&controladorEscritura);

	//Si el atleta llega a la tarima, espera 4 segundos para realizar su levantamiento.
	sleep(4);

	while(subeTarima == 0){

		comportamiento = numeroAleatorio(0,19);
		if(comportamiento<3){
			
			pthread_mutex_lock(&controladorEscritura);
			sprintf(msg, "Un atleta ha tenido un problema y no va a poder subir a la tarima: ");
			sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
			writeLogMessage(id,msg);
			pthread_mutex_unlock(&controladorEscritura);

			exit(0);

		}else{

			if(tarima está libre){
				subeTarima=1;
			}

			sleep(3);
			enEspera = enEspera+3;
		}	
	}

	//Si llega hasta aquí es que no ha tenido ningún problema de salud.

	pthread_mutex_lock(&controladorEscritura);
	sprintf(msg, "Va a competir el atleta");
	sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
	writeLogMessage(id,msg);
	pthread_mutex_unlock(&controladorEscritura);

	comportamiento = numeroAleatorio(0,9);
	if(comportamiento<8){
		//movimiento válido
		pthread_mutex_lock(&controladorEscritura);
		sprintf(msg, "Movimiento válido por parte del atleta ");
		sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
		writeLogMessage(id,msg);
		pthread_mutex_unlock(&controladorEscritura);
		punteroAtletas[atletaActual].ha_competido = 1;
		tiempoEspera = numeroAleatorio(2,6);

	}

	if(comportamiento==9){
		//falta de fuerza
		tiempoEspera = numeroAleatorio(6,10);
		pthread_mutex_lock(&controladorEscritura);
		sprintf(msg, "No ha sido capaz de realizar la prueba correctamente el atleta ");
		sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
		writeLogMessage(id,msg);
		pthread_mutex_unlock(&controladorEscritura);


	}

	if(comportamiento==8){
	//incumplimiento de normativa
		tiempoEspera = numeroAleatorio(1,4);
		pthread_mutex_lock(&controladorEscritura);
		sprintf(msg, "Incumplimiento de la normativa por parte del atleta ");
		sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
		writeLogMessage(id,msg);
		pthread_mutex_unlock(&controladorEscritura);
	}

	sleep(tiempoEspera);
	pthread_mutex_lock(&controladorEscritura);
	sprintf(msg, "Finalizado el ejercicio por parte del atleta ");
	sprintf(id,"atleta_%d",punteroAtletas[atletaActual].numeroAtleta);
	writeLogMessage(id,msg);
	pthread_mutex_unlock(&controladorEscritura);

	exit(0);

}
