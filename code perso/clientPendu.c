/*-----------------------------------------------------------
Client a lancer apres le serveur avec la commande :
client <adresse-serveur> <message-a-transmettre>
------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <ctype.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <errno.h> 
#include "Letters.c"
typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;
typedef struct joueur{
  char points[5];
  char nom[15];
}joueur;


int str_to_int(const char *s, int *p) 
{ 
    int ret = 0; /* code d'erreur 0 : succès. */ 
    if (*s == '\0') 
        ret = 1; /* code d'erreur 1 : la chaine a convertir est vide. */ 
    else 
    { 
        char *end; 
        *p = (int)strtol(s, &end, 10); 
  
        if (errno != 0) 
            ret = 2; /* code d'erreur 2 : la fonction strtol a rencontre une erreur. */ 
        else if (*end != '\0') 
            ret = 3; /* code d'erreur 3 : la conversion a échoué car un caractère invalide a été détecté. */ 
    } 
  
    return ret; 
}

int main(int argc, char **argv) {
  
    int 	socket_descriptor, 	/* descripteur de socket */
		longueur; 		/* longueur d'un buffer utilisé */
    sockaddr_in adresse_locale; 	/* adresse de socket local */
    hostent *	ptr_host; 		/* info sur une machine hote */
    servent *	ptr_service; 		/* info sur service */
    char 	buffer[256];
    char *	prog; 			/* nom du programme */
    char *	host; 			/* nom de la machine distante */
    char	mesg[256]; 			/* message envoyé */
    
    void closeEcoute(){
      close(socket_descriptor);
    }
    void sig_handler(int signo)
    {
      if (signo == SIGINT)
        closeEcoute();
        endwin();
        exit(1);
    }
    
  if (signal(SIGINT, sig_handler) == SIG_ERR){
    printf("\ncan't catch SIGINT\n");
  }
  if (argc != 2) {
		perror("usage : client <adresse-serveur>");
		exit(1);
  }
 
  prog = argv[0];
  host = argv[1];
  
  //--------------------DEFINITION DES FENETRES----------------------- 
	WINDOW *winHangman;
	WINDOW *winWord;
	WINDOW *winLives;
	WINDOW *winLetters;
	WINDOW *others;
  int rows = 0;
  int cols = 0;
  initscr();
  start_color();
  getmaxyx(stdscr, rows, cols);
  
  //----------------Verification taille min-----------------
  if (rows < 16 || cols < 55)
  {
	  endwin();
	  fprintf(stderr, "ERROR : Too small terminal. Try resizing it. (min 16 rows and 55 colums)\n"
					  "I recommand a size of 80x24.\n");
	  exit(EXIT_FAILURE);
  }
	  
   //----------------Verification couleur-----------------
  if(has_colors() == FALSE)
  {	endwin();
	  printf("Your terminal does not support color\n");
	  exit(1);
  }
	  
	//--------------------DEFINITION DES COULEURS-----------------------
	#define WHITE_B 1
	init_pair(WHITE_B, COLOR_WHITE, COLOR_BLACK);
	#define RED_B 2
	init_pair(RED_B, COLOR_RED, COLOR_BLACK);
	#define GREEN_B 3
	init_pair(GREEN_B, COLOR_GREEN, COLOR_BLACK);
	#define CYAN_B 4
	init_pair(CYAN_B, COLOR_CYAN, COLOR_BLACK);
	//color_set(RED_B, NULL);
	 
	
	
	//--------------------DEFINITION DES VARIABLES DE JEU----------------------- 
	char actualWord[27];
	int bool_mot_incomplet = 1;
	int taille_mot;
	char word[27];
	char hangman[300] = {0};
	char letters[27] = {0};
	char pseudo[15];
	char msg[50]="Entrez votre pseudo : ";
	char scannedChar = 0;
	char oldScannedChar = 0;
	char alreadyWroteLetter = 0;
	int lives = 10;
	int alreadyWroteIndex = 0;
	int nbJoueurs = 0;
	joueur** tabJoueurs= malloc(10*sizeof(joueur*));
	bool isWarningDone = true;
	
	void err(char * aff){
	  color_set(RED_B, NULL);
	  mvprintw(rows/2,0,"%s", aff);
	  mvprintw(rows - 2, cols - (33), "Pressez une touche pour terminer");
	  getch();
	  endwin();
	  exit(1);
	 }
  //-------------------- message d'accueil----------------------- 
	mvprintw(4, (cols - 32)/2, "Voici un jeu de Pendu en Reseau !");
	color_set(GREEN_B, NULL);
	mvprintw(9, (cols - 32)/2, "Par COURAUD Thao & BORDET Dennis");
	color_set(WHITE_B, NULL);
	mvprintw(rows - 2, cols - (33), "Pressez une touche pour continuer");
	refresh();
	getch(); /* Pause */
	
	//-------------------- CONNEXION AU SERVEUR -----------------------
	clear();
	refresh();
	color_set(CYAN_B, NULL);
	mvprintw(0,0,"Connexion au Serveur");
	color_set(WHITE_B, NULL);
	mvprintw(2, 0,"adresse du serveur  : %s \n", host);
	
	if ((ptr_host = gethostbyname(host)) == NULL) {
		err("erreur : impossible de trouver le serveur a partir de son adresse.");
	}
	/* copie caractere par caractere des infos de ptr_host vers adresse_locale */
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; /* ou ptr_host->h_addrtype; */
	adresse_locale.sin_port = htons(5042);
    
    mvprintw(3, 0,"numero de port pour la connexion au serveur : %d \n", ntohs(adresse_locale.sin_port));
    
    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    err("erreur : impossible de creer la socket de connexion avec le serveur.");
	  
    }
    
    /* tentative de connexion au serveur dont les infos sont dans adresse_locale */
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
    err("erreur : impossible de se connecter au serveur.");
	  
    }
	
	
	color_set(GREEN_B, NULL);
	mvprintw(rows/2,cols/2,"Connexion effectué avec succès.");
	color_set(WHITE_B, NULL);
	mvprintw(rows - 2, cols - (33), "Pressez une touche pour continuer.");
	getch();
	
	
	//-------------------- DEMANDE PSEUDO -----------------------
	clear();
	refresh();
	
	mvprintw(0,(cols-strlen(msg))/2,"%s",msg);
	int taille_pseudo;
	do{
	  refresh();
	  getstr(pseudo);
	  taille_pseudo = strlen(pseudo);
	  if(taille_pseudo < 1|| taille_pseudo>10){
	    color_set(RED_B, NULL);
	    mvprintw(1,0,"erreur : Taille incorrecte, votre pseudo doit avoir de 1 à 10 caractères");
	    color_set(WHITE_B, NULL);
	  }
	}while(taille_pseudo < 1|| taille_pseudo>10);
	
 	mvprintw(2, 0, "Envoie de votre pseudo au serveur: %s", pseudo);
 	if ((write(socket_descriptor, pseudo, sizeof(pseudo))) < 0) {
 	  err("erreur : impossible d'ecrire le message destine au serveur.");
	  
  }
  
  char pseudoRenv[50];
  read(socket_descriptor, pseudoRenv, sizeof(pseudoRenv));
  strcpy(pseudo, strchr(pseudoRenv,':')+1);
  mvprintw(rows - 2, 0, ">%s", pseudo);
  
  
  

  
  
 	getch();
	
	
	//-------------------- DEMANDE JOUEURS -----------------------
	clear();
	refresh();
	strcpy(msg, "Reception des données des autres joueurs");
	mvprintw(0,(cols-strlen(msg))/2,"%s",msg);
	char * joueur_i;
	char * joueur_points;
	char bufJoueurs[256];
	char buffer2[256];
	//buffer2 = malloc(256*sizeof(char));
	bzero(buffer2,256);
	printf("-\n");
	if(read(socket_descriptor, buffer2, sizeof(buffer2))<1){
	  printf("error\n");
	};
	
	//printf("-\n");
	//Message : others:pseudoA,pointsA;pseudoB,pointsB;.
	mvprintw(2, 0,"users : %s\n", buffer2);
	getch();
	if(buffer2[0]!='o' || buffer2[6]!=':'){
	  err("erreur : Veuilliez compiler avec -g pour eviter de mauvaises optimisations de compilation");  
	}
	//printf("-\n");
	//COMPILER AVEC -g sinon erreur compil
	strcpy(bufJoueurs,strchr(buffer2, ':'));
	if(strlen(buffer2)==strlen(bufJoueurs)){
	  err("erreur : mauvaise reception des autres joueurs");
	  
	}

	joueur_i = strtok(bufJoueurs, ";");
	do{
	  tabJoueurs[nbJoueurs]= malloc(sizeof(joueur));
	  strcpy(tabJoueurs[nbJoueurs]->nom, joueur_i);
	  nbJoueurs++;
	  joueur_i = strtok(NULL, ";");
	}while (strcmp(joueur_i ,".")!=0 &&joueur_i != NULL);
	
	
	int j;
	for(j=0; j<nbJoueurs;j++){
	  joueur_i = strtok(tabJoueurs[j]->nom, ",");
	  strcpy(tabJoueurs[j]->nom, joueur_i);
	  joueur_i = strtok(NULL, ",");
    strcpy(tabJoueurs[j]->points,joueur_i);
  }
  
  //Ca marche
  /*
  for(j=0; j<nbJoueurs;j++){
    printf("nom : %s\n", tabJoueurs[j]->nom);
    printf("points : %s\n", tabJoueurs[j]->points);
    printf("-------\n");
  }
  */
  
  
  //---------------------Taille mot-----------------------
  char taille_mot_str[51];
  bzero(taille_mot_str,51);
  if(read(socket_descriptor, taille_mot_str, sizeof(taille_mot_str))<1){
	  printf("error\n");
	};
	if(taille_mot_str[0]!='t' || taille_mot_str[13]!=':'){
	  err("erreur taille mot: Veuilliez compiler avec -g pour eviter de mauvaises optimisations de compilation");
	  
	}
	strcpy(taille_mot_str,strchr(taille_mot_str, ':')+1);
	
	int ret = str_to_int(taille_mot_str, &taille_mot); 
    if (ret == 0){
        printf("taille_mot = %d\n", taille_mot); 
    }else{
      err("erreur : la taille du mot est invalide, probleme serveur");

	}
	
	
	
	
	//----------------Lettres données-----------------------
	//lettresTrouvees:lettre1{pos1,pos2,0,0...,},lettre2{pos1,pos2,0,0...,},;lettresFausses:lettre1,lettre2,.
	
	
	char buff_lettres[200];
	if(read(socket_descriptor, buff_lettres, sizeof(buff_lettres))<1){
	  printf("error\n");
	};
	if(buff_lettres[0]!='l' || buff_lettres[15]!=':'){
	  err("erreur lettres: Veuilliez compiler avec -g pour eviter de mauvaises optimisations de compilation");	  
	}
	strcpy(buff_lettres,strchr(buff_lettres, ':')+1);
	
	char lettresTrouvees[150];
	lettresTrouvees[0]= strtok(buff_lettres, ";");
	char lettresFausses[50];
	lettresFausses[0]= strtok(NULL, ".");
	
	strcpy(lettresFausses,strchr(lettresFausses, ':')+1);
	
	printf("Trouvées %s/n",lettresTrouvees);
	printf("Fausses %s/n",lettresFausses);
	
	//LETTRES FAUSSES
	char* lettre_fausse_i;
	lettre_fausse_i = malloc(sizeof(char));
	lettre_fausse_i = strtok(lettresFausses, ",");
	while(strcmp(lettre_fausse_i, ".")!=0){
	  //on enleve les vies et on dessine le pendu
	  lives--;
	  //dessin du pendu ici
	  
	  addLetter(lettre_fausse_i[0], letters);
	  lettre_fausse_i = strtok(NULL, ",");
	}
	free(lettre_fausse_i);
	
	//LETTRES OK
	//TODO TROP CHIANT ENVOYER LE MOT _A_F___D plutot
	/*char* lettre_ok_i;
	lettre_ok_i = malloc(15*sizeof(char));
	lettre_ok_i = strtok(lettresTrouvees, ",");
	while(strcmp(lettre_ok_i, ".")!=0){
	  lettre_ok_i = strtok(NULL, ",");
	}
	free(lettre_ok_i);
	*/
	box(winHangman, ACS_VLINE, ACS_HLINE);

	mvwprintw(winWord, 0, 0, "%s", actualWord);
	mvwprintw(winLives, 0, 0, "Lives : %d", lives);

	wrefresh(winHangman);
	wrefresh(winWord);
	wrefresh(winLives);
	
	/*FAIRE 2 THREAD,
	- 1 qui demande a ce client les lettres
	- l'autre qui s'occupe des reponses des autres clients*/
	/*
	void *threadOthers(void * p_data){
	  while(bool_mot_incomplet && lives > 0){
	    read(socket_descriptor, readChar, sizeof(readChar));
	    
	  }
	}
	
	void *threadThisGamer(void * p_data){
	  while(bool_mot_incomplet && lives > 0){
	    scannedChar = toupper(getch());
	    
	  }
	}
	*/
	
  getch();
	endwin();
    
    
    
    
    printf("envoi d'un message au serveur. \n");
     while (strcmp(mesg, "quitter") !=0){
      printf("\nmessage  a envoye   :\n");
      scanf("%s", mesg);
      /* envoi du message vers le serveur */
      if ((write(socket_descriptor, mesg, sizeof(mesg))) < 0) {
	  perror("erreur : impossible d'ecrire le message destine au serveur.");
	  exit(1);
      }
      
      /* mise en attente du prgramme pour simuler un delai de transmission */
     sleep(3);
       
      printf("message envoye au serveur. \n");
                  
      /* lecture de la reponse en provenance du serveur */
      if((longueur = read(socket_descriptor, buffer, sizeof(buffer))) > 0) {
	      printf(">");
	      write(1,buffer,longueur);
      }
      
    }
   
    printf("\nfin de la reception. %d\n", socket_descriptor);
    
    close(socket_descriptor);
    
    printf("connexion avec le serveur fermee, fin du programme.\n");
    
    exit(0);
    
}

