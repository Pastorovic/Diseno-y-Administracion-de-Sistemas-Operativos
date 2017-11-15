//este archivo es el fichero fuente que al compilarse produce el ejecutable Ej3

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>

#define MAX 256 		//tamaño maximo del mensaje

//imprime la tarea realizada por un proceso
void printTask(int process, char *task)
{
	printf("El proceso P%d (PID=%d, Ej3) %s\n", process, getpid(), task);
}

//imprime el error causado y termina el programa
void printError(char *error)
{
	perror(error);
	exit(-1);
}

int main()
{
	char *vc1;		//mensaje de la memoria compartida
	int shmid;		//identificador de la zona de memoria compartida asociado a llave
	int sem1;		//identificador del semaforo
	int msqid;		//identificador de la cola de mensajes
	key_t llaveCM;		//llave para la cola de mensajes
	key_t llave;		//llave para abrir el semaforo y la memoria compartida
	struct
	{
		long tipo;
		pid_t pid;
	} msgQueue;		//almacena el mensaje y el tipo de una cola de mensajes
	msgQueue.tipo=2;	//se asigna el tipo 2
	msgQueue.pid=getpid();

	//se crrea la misma llave que se utilizo en Ej2 para crear el semaforo y la MC
	if((llave=ftok("fichero1", 'P'))==-1) printError("error al crear la llave");

	//se carga la region de memoria compartida creada en Ej2
	if((shmid=shmget(llave, MAX*sizeof(char), 0600))==-1) printError("error al cargar la memoria compartida");

	//se carga el semaforo creado en Ej2
	if((sem1=semget(llave, 1, 0666))==-1) printError("error al cargar el semaforo");
	//se crea la operacion P()
	struct sembuf opP[1];
	opP[0].sem_num=0;
	opP[0].sem_op=-1; 
	opP[0].sem_flg=0;
	printTask(3, "espera a que se levante el semáforo");
	//se realiza la operacion P() del semaforo
	if(semop(sem1, opP, 1)==-1) printError("error al realizar operacion V del semaforo");

	//se une la zona de memoria compartida al espacio de direcciones virtuales de vc1 para leer el mensaje
	if((vc1=shmat(shmid, 0, 0))==(char *)-1)
		printError("error al asignar el espacio de direcciones virtuales a la zona de memoria compartida");
	printTask(3, "lee el mensaje de la region de memoria compartida");
	//se muestra el mensaje
	printf("\nMensaje: %s\n\n", vc1);
	//se separa la MC del espacio de direcciones virtuales
	if(shmdt((int *)vc1)==-1)
		printError("error al separar el espacio de direcciones virtuales de la region de memoria compartida");
	//se borra la zona de memoria compartida
	if(shmctl(shmid, IPC_RMID, 0)==-1) printError("error al borrar la zona de memoria compartida");
	//se libera el semaforo
	if(semctl(sem1, 0, IPC_RMID)==-1) printError("error al liberar el semaforo");;

	//se crea la misma llave que se utilizo en Ej1 para crear la cola de mensajes
	if((llaveCM=ftok("./Trabajo2/Ej1", 'D'))==-1) printError("error al crear la llave");

	//se abre la cola de mensajes creada en Ej1
	if((msqid=msgget(llaveCM, 0600))==-1) printError("error al abrir la cola de mensajes");
	//se envia el pid de P3 por la cola de mensajes
	printTask(3, "envia su pid por la cola de mensajes");
	if(msgsnd(msqid, &msgQueue, MAX, 0)==-1) printError("error al enviar por la cola de mensajes");
	
	//se suspende la ejecucion de P3
	pause();
}
