/*----------------------------------------------
Serveur à lancer avant le client
------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h> 	/* pour les sockets */
#include <sys/socket.h>
#include <signal.h>
#include <netdb.h> 		/* pour hostent, servent */
#include <string.h> 		/* pour bcopy, ... */
#include <pthread.h>
#include <ctype.h>

#include "../Pendu/dico.c"
#include "Letters.c"

#define TAILLE_MAX_NOM 256

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;


typedef struct lettre_commun {

    char mot[27];
    char lettre_trouve_fausse[26];
    char motHache[27];

}lettre_commun;


typedef struct thread_socket {

    pthread_t id;
    int socket;
    char * pseudo;
	unsigned int points;

}thread_socket;

thread_socket* socket_tab[10];
int socket_tab_size = 0;
lettre_commun lettres;

/*------------------FONCTION INTERRUCTION------------------------------------*/


/*------------------------------------------------------*/
/*------------------------------------------------------*/


//ETAPE 1 de l'initialisation: pseudo
void init_pseudo(thread_socket* tSock, char* buffer){

  char buffer_pseudo[10];
  char pseudo[12]="";
  if (read(tSock->socket, buffer_pseudo, sizeof(buffer_pseudo)) <= 0){
      write(tSock->socket,"erreur pseudo trop court",25);
  }else{
    // creation du pseudo concaténé avec son numéro de socket
    strcpy(pseudo, buffer_pseudo);
    char str[5];
    sprintf(str,"%d",tSock->socket);
    strcat(pseudo, str);
    //tSock->pseudo = malloc(15*sizeof(char));
    strcpy(tSock->pseudo, pseudo);

    char repPseudo[50] = "Votre pseudo pour le jeu sera :";
    strcat(repPseudo, pseudo);
	  strcpy(buffer, pseudo);
	  printf("ok\n");

    //write(tSock->socket,repPseudo,sizeof(repPseudo));
  }
}

void init_others(char* buffer2){
	char buffer[256]="others:";
	int user;
	thread_socket *tmp;

	//Message : others:pseudoA,pointsA;pseudoB,pointsB;.
	for(user=0; user<socket_tab_size; user++){
		tmp = socket_tab[user];
		strcat(buffer,tmp->pseudo);
		strcat(buffer,",");
		char str[5];
		sprintf(str,"%d",tmp->points);
		strcat(buffer,str);
		strcat(buffer,";");
	}
	strcat(buffer,".");
	printf ("users : %s\n", buffer);

	strcat(buffer2,buffer);
	//write(tSock->socket,buffer2,sizeof(buffer2));
	  printf("ok\n");

}
void init_mot(char* buffer){
  char buff[50] ="taille du mot:";
  char str[5];
	sprintf(str,"%d",strlen(lettres.mot));
  strcat(buff, str);
  strcat(buffer, buff);
  //write(tSock->socket,buff,sizeof(buff));
  printf("%s\n", buff);

}

void init_lettres(char* buffer){

    char buffer2[200]="lettresTrouvees:";

   strcat(buffer2,lettres.motHache);
   //strcat(buffer2,"ABC"); //pour debugage client
   strcat(buffer2,".lettresFausses:");
   strcat(buffer2,lettres.lettre_trouve_fausse);
   //strcat(buffer2,"DEF");//pour debugage client
   strcat(buffer2,".");
   strcat(buffer, buffer2);

    //write(tSock->socket,buffer,sizeof(buffer));
}

void initialisation(thread_socket* tSock){
  tSock->points = 10;
  char buffer[500]="";
  char dollar[1] = "$";
	init_pseudo(tSock, buffer);
	strcat(buffer,"$");
	//ETAPE 2: envoie des données des autres utilisateurs
	//pour chaque personne dans le tableau de socket, on envoie
	//son pseudo et ses points
	init_others(buffer);
	strcat(buffer,"$");
	//init_mot(buffer);
	//strcat(buffer,dollar);
	init_lettres(buffer);
	strcat(buffer,"$");
	printf(" buffer final : %s\n", buffer);
	write(tSock->socket,buffer,strlen(buffer));
	
	char message[20];
	bzero(message,20);
	strcpy(message,"c:");
	strcat(message,tSock->pseudo);
	strcat(message,".\0");
	
	int i = 0;
    for(i; i < socket_tab_size; i++){
        
        if(socket_tab[i]->socket != tSock->socket)
	    write(socket_tab[i]->socket,message,strlen(message));
		
	}
	
	
  //read(tSock->socket,buffer,sizeof(buffer));
	//Etape 3: envoie des lettres fausses et lettre trouvées + indices
	//dans le mot


}

void initialisationMot(){

	piocherMot(lettres.mot);

	printf(lettres.mot);
	int i =0;
	while( i < 27){
		if(checkLetter2(lettres.mot[i]) != -1){
			lettres.motHache[i] = '_';
		}else{
			lettres.motHache[i] = '\0';
		}
		//lettres.lettre_trouve_fausse[i] = '%';
		i++;
	}
	//printf(lettres.motHache);
}

char* pendu(char lettrePropose, char res[]){
	unsigned int i;
	int trouver = 0;
	for(i = 0; i < strlen(lettres.mot); i++)
	{
		if(lettres.mot[i] == lettrePropose)
		{
			lettres.motHache[i] = lettres.mot[i];
			trouver = 1;
		}
	}
	if(trouver == 0){
		for(i = 0; i < 26; i++) {

			if(lettrePropose == lettres.lettre_trouve_fausse[i]){
				trouver = 2;
				res[0] = 'e';
				res [1] = '\0';
			}

			if(trouver == 0 && lettres.lettre_trouve_fausse[i] == '\0'){

				lettres.lettre_trouve_fausse[i] = lettrePropose;
				i = 27;
				res[0] = 'f';
				res [1] = ':';
				res [2] = lettrePropose;
				res [3] = '\0';

			}

		}

	} else {
		res[0] = 'v';
		res [1] = ':';
		res [2] = lettrePropose;
		res [3] = ',';
		res [4] = '\0';
		strcat(res,lettres.motHache);
	}


	strcat(res,",\0");

	return res;


}

//envoie a tout les clients des données
void renvoi(char message[]){

		int i = 0;
		for(i; i < socket_tab_size; i++){

			write(socket_tab[i]->socket,message,strlen(message)
    );
		
	}
}


char finJeu(thread_socket* tSock){
    char buffer[50];
    char res = 'F';
    
    if(read(tSock->socket, buffer, sizeof(buffer)) > 0){
        if (buffer[1] == 'Y'){
            res ='Y';
        }
    }
    
    return res;

}

//fonction qui renvoie t si le jeu est fini sinon f
char jeuFini(){
    char res = 'f';
    if(lettres.lettre_trouve_fausse[9] >= 'A' && lettres.lettre_trouve_fausse[9] <= 'Z'){
        res = 't';
        return res;
    }
    res = 't';
    int i;
    for(i = 0; i < strlen(lettres.mot); i++)
	{
		if(lettres.mot[i] != lettres.motHache[i])
		{
			res = 'f';
			return res;
		}
	}
    return res;
}

char jeu(thread_socket* tSock){
    char res;
	char buffer[50];

	char envoi[50];

	strcpy(envoi,"");
	char * pseudo = tSock->pseudo ;
	int fin = 0;
    while(fin == 0){
      bzero(envoi,50);
      bzero(buffer,50);
		sleep(1);
     if(read(tSock->socket, buffer, sizeof(buffer)) > 0){
		if(buffer[0] != 27){
			printf("buff:%s \n",buffer);
			char tmp[50];
			pendu(buffer[0],tmp);
			printf("tmp:%s \n",tmp);
			strcat(envoi,tmp);
			strcat(envoi,pseudo);
			strcat(envoi,".\0");

			renvoi(envoi);
            printf("envoie:%s \n",envoi);
            
            
		} else {
			fin = 1;
			strcat(envoi,"d:");
			strcat(envoi,pseudo);
			renvoi(envoi);
		}
		
		if(jeuFini() == 't'){
		    res = finJeu(tSock);
		    fin = 1;
		}

     }


    }
    
    return res;

}


/*------------------------------------------------------*/

/*------------------------------------------------------*/
main(int argc, char **argv) {

  initialisationMot();



    printf("\n     __________\n");
    printf("     | //      |\n");
    printf("     |//       |\n");
    printf("     |/       (_)\n");
    printf("     |         |\n");
    printf("     |        /|\\\n");
    printf("     |         |\n");
    printf("     |        / \\\n");
    printf("    /|\\\n");
    printf("   / | \\\n");
    printf("#####################\n");


    int             socket_descriptor, 		    /* descripteur de socket */
		            nouv_socket_descriptor, 	/* [nouveau] descripteur de socket */
		            longueur_adresse_courante; 	/* longueur d'adresse courante d'un client */
    sockaddr_in 	adresse_locale, 		    /* structure d'adresse locale*/
		            adresse_client_courant; 	/* adresse client courant */
    hostent*		ptr_hote; 			        /* les infos recuperees sur la machine hote */
    servent*		ptr_service; 			    /* les infos recuperees sur le service de la machine */
    char 		    machine[TAILLE_MAX_NOM+1]; 	/* nom de la machine locale */

    gethostname(machine,TAILLE_MAX_NOM);		/* recuperation du nom de la machine */

    void *fct_thread(void * p_data){
        int num_thread_sock = p_data;
        num_thread_sock--;
        //printf("fct_thread-----  : %s\n",);

        thread_socket *thread_sock= socket_tab[num_thread_sock];
        printf("reception d'un message sur sock %d\n",thread_sock->socket );


        //------------1ere etape------------------
        //envoi des données pour que le client puisse initialiser le jeu
		printf("initialisation debut\n");
        initialisation(thread_sock);
        //------------2e etape------------------
        // echanges avec le client.
        char tmp[256];
		//read(thread_sock->socket,tmp,sizeof(tmp));//petit read pour annuler le write d'on ne sait pas ou
		printf("initialisation fini\n");

		//printf("%s \n",tmp);
		//write(thread_sock->socket,tmp,sizeof(tmp));
        while(jeu(thread_sock) == 'Y'){
            initialisationMot();
            
            
        }
        sleep(5);
        
        int i;
        for(i=0; i<socket_tab_size;i++){
          if(strcmp(thread_sock->pseudo, socket_tab[i]->pseudo)==0){
            socket_tab[i]=socket_tab[socket_tab_size-1];
            socket_tab_size--;
            break;
          }
          
        }
        
        close(thread_sock->socket );
        (void) p_data;
        return NULL;

	}
    /* recuperation de la structure d'adresse en utilisant le nom */
    if ((ptr_hote = gethostbyname(machine)) == NULL) {
		perror("erreur : impossible de trouver le serveur a partir de son nom.");
		exit(1);
    }

    /* initialisation de la structure adresse_locale avec les infos recuperees */

    /* copie de ptr_hote vers adresse_locale */
    bcopy((char*)ptr_hote->h_addr, (char*)&adresse_locale.sin_addr, ptr_hote->h_length);
    adresse_locale.sin_family		= ptr_hote->h_addrtype; 	/* ou AF_INET */
    adresse_locale.sin_addr.s_addr	= INADDR_ANY; 			/* ou AF_INET */

    /* 2 facons de definir le service que l'on va utiliser a distance */
    /* (commenter l'une ou l'autre des solutions) */

    /*-----------------------------------------------------------*/
    /* SOLUTION 1 : utiliser un service existant, par ex. "irc" */
    /*
    if ((ptr_service = getservbyname("irc","tcp")) == NULL) {
		perror("erreur : impossible de recuperer le numero de port du service desire.");
		exit(1);
    }
    adresse_locale.sin_port = htons(ptr_service->s_port);
    */
    /*-----------------------------------------------------------*/
    /* SOLUTION 2 : utiliser un nouveau numero de port */
    adresse_locale.sin_port = htons(5042);
    /*-----------------------------------------------------------*/

    printf("numero de port pour la connexion au serveur : %d \n",ntohs(adresse_locale.sin_port) /*ntohs(ptr_service->s_port)*/);

    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("erreur : impossible de creer la socket de connexion avec le client.");
		exit(1);
    }

    /* association du socket socket_descriptor à la structure d'adresse adresse_locale */
    if ((bind(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de lier la socket a l'adresse de connexion.");
		exit(1);
    }

    /* initialisation de la file d'ecoute */
    listen(socket_descriptor,5);

    void closeEcoute(){
        int i;
        for(i =0; i<socket_tab_size; i++){
          close(socket_tab[i]->socket);
        }
        close(socket_descriptor);

    }

    void sig_handler(int signo)
    {
      if (signo == SIGINT)
      // fermer les clients avant
        closeEcoute();
        exit(1);
    }

    if (signal(SIGINT, sig_handler) == SIG_ERR){
        printf("\ncan't catch SIGINT\n");
    }

    //On va créer reserver N thread pour les N clients


    /* attente des connexions et traitement des donnees recues */
    for(;;) {
    	if(socket_tab_size<11){
			longueur_adresse_courante = sizeof(adresse_client_courant);
			/* adresse_client_courant sera renseigné par accept via les infos du connect */
			thread_socket *nouv_socket = malloc(sizeof(thread_socket));

			if ((nouv_socket->socket= accept(socket_descriptor,(sockaddr*)(&adresse_client_courant),&longueur_adresse_courante))< 0) {
				perror("erreur : impossible d'accepter la connexion avec le client.");
				exit(1);
			}
			socket_tab[socket_tab_size] = nouv_socket;
			socket_tab[socket_tab_size]->pseudo= malloc(15*sizeof(char));
			strcpy(socket_tab[socket_tab_size]->pseudo, "NoPseudoYet");
			socket_tab[socket_tab_size]->points=10;
			socket_tab_size++;
			printf("nouveau client, taille tab=%d\n",socket_tab_size);
			// on veut 1 thread qui s'occupe de chaque client

			if( pthread_create( &(nouv_socket->id) , NULL ,  fct_thread , (void*) socket_tab_size) < 0)
		    {
		        perror("could not create thread");
		        return 1;
		    }
		}
    }


    exit(0);

}
