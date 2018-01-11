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

struct atletas{
	int numeroAtleta
	bool deshidratado;
	int puntuacion;
};

struct atletas *punteroAtletas;


struct jueces{
	pthread_t juez;
	bool ocupado;/*indica si el juez esta con algun atleta*/
	int identificadorJuez;
	bool descansando;/*si el juez ha atendido a 4 atletas se pone a true*/
};

/*puntero que contiene los jueces*/

struct jueces *punteroJueces;


pthread_mutex_t controladorColaJueces;/*controla que no entren a la cola dos atletas y  evita que tengan la misma posicion en la cola*/
pthread_mutex_t controladorJuez;/*contralara que dos atletas de la cola no intenten entrar al mismo tarima/juez*/



int main(){
	/*Reservo memoria para los struct de los jueces*/

	punteroJueces = (struct jueces*)malloc(sizeof(struct jueces)*NUMJUECES);

	/*Inicializamos los jueces*/
	for(i=0;i<NUMJUECES;i++){
   		punteroJueces[i].ocupado = false;
    	punteroJueces[i].identificadorJuez = i+1;
    	punteroJueces[i].descansando = false;
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