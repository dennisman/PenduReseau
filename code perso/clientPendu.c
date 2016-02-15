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

typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;
typedef struct joueur{
  char points[5];
  char nom[15];
}joueur;




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
		color_set(RED_B, NULL);
		mvprintw(rows/2,0,"erreur : impossible de trouver le serveur a partir de son adresse.");
		mvprintw(rows - 2, cols - (33), "Pressez une touche pour terminer");
		getch();
		endwin();
		exit(1);
	}
	/* copie caractere par caractere des infos de ptr_host vers adresse_locale */
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; /* ou ptr_host->h_addrtype; */
	adresse_locale.sin_port = htons(5042);
    
    mvprintw(3, 0,"numero de port pour la connexion au serveur : %d \n", ntohs(adresse_locale.sin_port));
    
    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    color_set(RED_B, NULL);
	  mvprintw(rows/2,0,"erreur : impossible de creer la socket de connexion avec le serveur.");
	  mvprintw(rows - 2, cols - (33), "Pressez une touche pour terminer");
	  getch();
	  endwin();
	  exit(1);
    }
    
    /* tentative de connexion au serveur dont les infos sont dans adresse_locale */
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
    color_set(RED_B, NULL);
	  mvprintw(rows/2,0,"erreur : impossible de se connecter au serveur.");
	  mvprintw(rows - 2, cols - (33), "Pressez une touche pour terminer");
	  getch();
	  endwin();
	  exit(1);
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
 	if ((write(socket_descriptor, pseudo, strlen(pseudo)+1)) < 0) {
 	  color_set(RED_B, NULL);
	  mvprintw(rows/2,0,"erreur : impossible d'ecrire le message destine au serveur.");
	  mvprintw(rows - 2, cols - (33), "Pressez une touche pour terminer");
	  getch();
	  endwin();
	  exit(1);
  }
  
  char pseudoRenv[50];
  read(socket_descriptor, pseudoRenv, sizeof(pseudoRenv));
  strcpy(pseudo, strchr(pseudoRenv,':')+1);
  mvprintw(rows - 2, 0, ">%s", pseudo);
  
  
  /*tabJoueurs[0]->pseudo = pseudo;
  tabJoueurs[0]->points = points;*/
  
 	getch();
	
	
	//-------------------- DEMANDE JOUEURS -----------------------
	clear();
	refresh();
	strcpy(msg, "Reception des données des autres joueurs");
	mvprintw(0,(cols-strlen(msg))/2,"%s",msg);
	char * joueur_i;
	char * joueur_points;
	char bufJoueurs[256];
	read(socket_descriptor, buffer, sizeof(buffer));
	
	mvprintw(2, 0,"users : %s\n", buffer);
	getch();
	//Message : others:pseudoA,pointsA;pseudoB,pointsB;.
	strcpy(bufJoueurs,strchr(buffer, ':')); 
	if(strlen(buffer)==strlen(bufJoueurs)){
	  color_set(RED_B, NULL);
	  mvprintw(rows/2,0,"erreur : mauvaise reception des autres joueurs");
	  mvprintw(rows - 2, cols - (33), "Pressez une touche pour terminer");
	  getch();
	  endwin();
	  exit(1);
	}

	/*
	joueur_i = strtok(bufJoueurs, ";");
	do{
	  
	  strcpy(tabJoueurs[nbJoueurs]->nom, joueur_points);
	  joueur_points = strtok(NULL, ",");
	  strcpy(tabJoueurs[nbJoueurs]->points, joueur_points);
	  nbJoueurs++;
	  joueur_i = strtok(NULL, ";");
	}while (joueur_i !=".");
*/
  getch();
	endwin();
    
    
    
    
    printf("envoi d'un message au serveur. \n");
     while (strcmp(mesg, "quitter") !=0){
      printf("\nmessage  a envoye   :\n");
      scanf("%s", mesg);
      /* envoi du message vers le serveur */
      if ((write(socket_descriptor, mesg, strlen(mesg))) < 0) {
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

