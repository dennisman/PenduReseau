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
#include "log.c"
#include "../Pendu/dico.c"
#include "Letters.c"

#define TAILLE_MAX_NOM 256

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;


typedef struct lettre_commun {

    char mot[27];
    char lettre_trouve_fausse[10];
    char motHache[27];

}lettre_commun;


typedef struct thread_socket {

    pthread_t id;
    int socket;
    char * pseudo;
	unsigned int points;

}thread_socket;

char reponses[10] = {'a','a','a','a','a','a','a','a','a','a'};
int reponses_tab_size = 0;
thread_socket* socket_tab[10];
int socket_tab_size = 0;
lettre_commun lettres;
int phase_rejouer = 0;

/*------------------FONCTION INTERRUCTION------------------------------------*/

//TODO verifier le renvoie des BONNES données quand on rejoue, en particulier les lettresFausses et mot haché"; ou est initialisé le tab de lettres fausses ?
/*------------------------------------------------------*/
/*------------------------------------------------------*/


void deconnexion(thread_socket* thread_sock){
    int i;
    for(i=0; i<socket_tab_size;i++){
      if(strcmp(thread_sock->pseudo, socket_tab[i]->pseudo)==0){
        socket_tab[i]=socket_tab[socket_tab_size-1];
        socket_tab[socket_tab_size-1] = NULL;
        socket_tab_size--;
        break;
      }
      
    }
    
    if(phase_rejouer == 0){
    	char envoi[50];
        strcpy(envoi,"");
        char * pseudo = thread_sock->pseudo ;
        strcat(envoi,"d:");
        strcat(envoi,pseudo);
        renvoi(envoi);
    }
        
}


//ETAPE 1 de l'initialisation: pseudo
void init_pseudo(thread_socket* tSock, char* buffer){

  char buffer_pseudo[10];
  char pseudo[12]="";
  if (read(tSock->socket, buffer_pseudo, sizeof(buffer_pseudo)) < 0){
      //TODO supp ce joueur et envoyer aux autre sa déco !
      printf("deco a l'init \n");
      deconnexion(tSock);
  }else{
    // creation du pseudo concaténé avec son numéro de socket
    strcpy(pseudo, buffer_pseudo);
    char str[5];
    sprintf(str,"%d",tSock->socket);
    strcat(pseudo, str);
    strcpy(tSock->pseudo, pseudo);

    char repPseudo[50] = "Votre pseudo pour le jeu sera :";
    strcat(repPseudo, pseudo);
	  strcpy(buffer, pseudo);

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

	strcat(buffer2,buffer);

}
void init_mot(char* buffer){
  char buff[50] ="taille du mot:";
  char str[5];
  sprintf(str,"%d",strlen(lettres.mot)+1);
  strcat(buff, str);
  strcat(buffer, buff);
  printf("%s\n", buff);

}

void init_lettres(char* buffer){

   char buffer2[200]="lettresTrouvees:";

   strcat(buffer2,lettres.motHache);
   strcat(buffer2,".lettresFausses:");
   strcat(buffer2,lettres.lettre_trouve_fausse);
   strcat(buffer2,".");
   strcat(buffer, buffer2);
   
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
    init_lettres(buffer);
    strcat(buffer,"$");
    printf(" buffer final : %s\n", buffer);
    write(tSock->socket,buffer,strlen(buffer)+1);

    char message[20];
    bzero(message,20);
    strcpy(message,"c:");
    strcat(message,tSock->pseudo);
    strcat(message,".");
    printf("connexion :%s", message);
    int i = 0;
    for(i; i < socket_tab_size; i++){
        
        if(socket_tab[i]->socket != tSock->socket){
        	if(strcmp(socket_tab[i]->pseudo,"NoPseudoYet")!=0 && phase_rejouer == 0)
	        write(socket_tab[i]->socket,message,strlen(message)+1);
		    //+1 evite une erreur d'envoie
		
        }
        	
	
    }
	
	
  //read(tSock->socket,buffer,sizeof(buffer));
	//Etape 3: envoie des lettres fausses et lettre trouvées + indices
	//dans le mot


}

void initialisationMot(){

	piocherMot(lettres.mot);

	printf("mot:%s\n",lettres.mot);
	int i =0;
	while( i < 27){
		if(checkLetter2(lettres.mot[i]) != -1){
			lettres.motHache[i] = '_';
		}else{
			lettres.motHache[i] = '\0';
		}
		if(i<10){lettres.lettre_trouve_fausse[i] = '\0';}
				
		i++;
	}
	phase_rejouer = 0;
}

char* pendu(char lettrePropose, char res[],thread_socket* tSock){
	unsigned int i;
	int trouver = 0;
	for(i = 0; i < strlen(lettres.mot); i++)
	{
		if(lettres.mot[i] == lettrePropose)
		{
			lettres.motHache[i] = lettres.mot[i];
			trouver = 1;
			tSock->points++;
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
			    tSock->points--;

			}

		}

	} else {
		res[0] = 'v';
		res [1] = ':';
		res [2] = lettrePropose;
		res [3] = ',';
		res [4] = '\0';
		strcat(res,lettres.motHache);
		
		int flag = 1;
		
		//on regarde si le mot a ete trouver
		for(i = 0; i < strlen(lettres.mot); i++)
	    {
		    if(lettres.mot[i] != lettres.motHache[i])
		    {
			    flag = 0;
		    }
		
	    }
	    //si oui on lui ajoute 10 point
	    if(flag == 1){
	        tSock->points = tSock->points + 10;
	    }
		
	}

    printf("point: %d \n",tSock->points);

	strcat(res,",\0");

	return res;


}

//envoie a tout les clients des données
void renvoi(char* message){

        printf("envoi:%s\n",message);
		int i = 0;
		for(i; i < socket_tab_size; i++){
            if(strcmp(socket_tab[i]->pseudo,"NoPseudoYet")!=0){
			    if(write(socket_tab[i]->socket,message,strlen(message)+1)<0){
				    //TODO supp ce joueur et envoyer aux autre sa déco !
				    printf("renoie\n");
				    deconnexion(socket_tab[i]);
			    }
			}
		
	}
}


char finJeu(thread_socket* tSock, char buff[], int nbJoueur){


    char buffer[50];
    char res = 'N';
    if(buff[0] != '$'){

        if(read(tSock->socket, buffer, sizeof(buffer)) > 0){
	        printf("buff1:%s \n",buffer);
            if (buffer[1] == 'Y'){
                res ='Y';
            } 
        }else{
        	//TODO supp ce joueur et envoyer aux autre sa déco !
				printf("finjeu\n");
        	deconnexion(tSock);
        	
        }
    } else {
		res = buff[1];
		printf("buff2:%s \n",buff);
	}
	
	if(res != 'Y'){deconnexion(tSock);}
    
    reponses[reponses_tab_size] = res;
    reponses_tab_size ++;
    int i = 0;
    int flag = 1;
    for(i; i < nbJoueur; i++){
        printf("reponses[%d]: %c \n",i,reponses[i]);
        if(reponses[i] !='Y' && reponses[i] !='N' ){flag = 0;}
        printf("flag: %d \n",flag);
        
    }
	//quand tout le monde a rendu la reponse
    if(flag == 1){
        //donnees:motHache$others:j1,pts;j2,pts;.
        char tmp[50] = "donnees:";
        initialisationMot();
        strcat(tmp,lettres.motHache);
        strcat(tmp,"$others:");
        i = 0;
        for(i; i < socket_tab_size; ++i){
            strcat(tmp,socket_tab[i]->pseudo);
            strcat(tmp,",");
			char str[5];
			sprintf(str,"%d",socket_tab[i]->points);
			printf("point: %d \n",socket_tab[i]->points);
            strcat(tmp,str);
            strcat(tmp,";");
        }
        strcat(tmp,".");
        renvoi(tmp);
		
		for(i = 0; i < nbJoueur; i++){
			reponses[i] = 'a';
		}
		reponses_tab_size = 0;
    
        
    }
    return res;

}

//fonction qui renvoie t si le jeu est fini sinon f
char jeuFini(){
    char res = 'f';
    if(lettres.lettre_trouve_fausse[9] >= 'A' && lettres.lettre_trouve_fausse[9] <= 'Z'){
        res = 't';
        
        //on enleve 3 points a tous les joueurs
        int i = 0;
        for(i; i < socket_tab_size; ++i){
            socket_tab[i]->points -= 3;
        }
        
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
    char res = 'F';
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
           
            //si buffer[0] est different d'echap(27) et de $ et qu'on est pas dans la phase rejouer
            if(buffer[0] != 27 && buffer[0] != '$' && phase_rejouer == 0){

                    printf("buffer:%s \n",buffer);
                    char tmp[50];
                    pendu(buffer[0],tmp,tSock);
                    strcat(envoi,tmp);
                    strcat(envoi,pseudo);
                    strcat(envoi,".");

                    renvoi(envoi);

                    if(jeuFini() == 't'){
                        phase_rejouer = 1;
                        res = finJeu(tSock,"_",socket_tab_size);
                        fin = 1;
                    }



                } else if(buffer[0] == 27){
                    printf("echap\n");
                    fin = 1;
                    deconnexion(tSock);
                    res = 'F';
                    return res;
                } else {
                    phase_rejouer = 1;
                    res = finJeu(tSock,buffer,socket_tab_size);
                    fin = 1;
                }

            
            }else{
                //TODO supp ce joueur et envoyer aux autre sa déco !

                    printf("deco sauvage\n");
                fin = 1;
                deconnexion(tSock);
                res = 'F';
                return res;
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
        thread_socket *thread_sock= socket_tab[num_thread_sock];
        printf("reception d'un message sur sock %d\n",thread_sock->socket );


        //------------1ere etape------------------
        //envoi des données pour que le client puisse initialiser le jeu
        initialisation(thread_sock);
        //------------2e etape------------------
        // echanges avec le client.
        char tmp[256];

        while(jeu(thread_sock) == 'Y'){}
        
        
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
    	if(socket_tab_size<110){
			longueur_adresse_courante = sizeof(adresse_client_courant);
			/* adresse_client_courant sera renseigné par accept via les infos du connect */
			thread_socket *nouv_socket = malloc(sizeof(thread_socket));
            while(socket_tab_size>9); 
			if ((nouv_socket->socket= accept(socket_descriptor,(sockaddr*)(&adresse_client_courant),&longueur_adresse_courante))< 0) {
				perror("erreur : impossible d'accepter la connexion avec le client.");
				exit(1);
			}
			////
			socket_tab[socket_tab_size] = nouv_socket;
			///
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
