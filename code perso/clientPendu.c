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
#include <pthread.h> 
#include <string.h>
#include <errno.h>
#include "Letters.c"

#define HANGMAN_HEIGHT 12
#define HANGMAN_WIDTH 20

#define true 1
#define false 0
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
  WINDOW *winOthers;
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
  int bool_mot_incomplet = 1;
  int taille_mot;
  char word[27];
  char hangman[300] = {0};
  char* letters;
  letters=malloc(27*sizeof(char));
  int i;
  for(i=0;i<27;i++){
    letters[i]=' ';
  }
  char pseudo[15];
  char msg[50]="Entrez votre pseudo : ";
  char alreadyWroteLetter = 0;
  int lives = 10;
  int alreadyWroteIndex = 0;
  int nbJoueurs = 0;
  joueur** tabJoueurs= malloc(10*sizeof(joueur*));
  bool isWarningDone = true;
  pthread_t id_threadOthers, id_threadThisGamer;
  time_t startBlinkTime = time(NULL);
  time_t timeOfNow = time(NULL);
  int ligne_init=0;


  //***************************Fonctions ***************************
  void err(char * aff){
    color_set(RED_B, NULL);
    mvprintw(rows/2,0,"%s", aff);
    mvprintw(rows - 2, cols - (33), "Pressez une touche pour terminer");
    getch();
    endwin();
    exit(1);
  }
  void demandePseudo(){
    clear();
    refresh();
    int taille_pseudo;
    do{
      mvprintw(0,(cols-strlen(msg))/2,"%s",msg);
      getstr(pseudo);
      taille_pseudo = strlen(pseudo);
      if(taille_pseudo < 1|| taille_pseudo>10){
        color_set(RED_B, NULL);
        mvprintw(1,0,"erreur : Taille incorrecte, votre pseudo doit avoir de 1 à 10 caractères");
        mvprintw(0,(cols-strlen(msg))/2,"%s",msg);
        color_set(WHITE_B, NULL);
        

      }
    }while(taille_pseudo < 1|| taille_pseudo>10);
    clear();
    refresh();
    strcpy(msg,"Initialisation des données, echange avec le serveur :");
    color_set(CYAN_B, NULL);
    mvprintw(ligne_init,(cols-strlen(msg))/2,"%s",msg);
    ligne_init++;
    ligne_init++;
    color_set(WHITE_B, NULL);
    mvprintw(ligne_init, 0, "Envoie de votre pseudo (%s) au serveur ...", pseudo);
    ligne_init++;
    if ((write(socket_descriptor, pseudo, sizeof(pseudo))) < 0) {
      err("erreur : impossible d'ecrire le message destine au serveur.");

    }
  }

  void demandeJoueurs(char buffer2[]){
    mvprintw(ligne_init, 0, "Reception des données des autres joueurs...");
    ligne_init++;
    char * joueur_i;
    char * joueur_points;
    char bufJoueurs[256];
    //Message : others:pseudoA,pointsA;pseudoB,pointsB;.
    mvprintw(ligne_init, 0,"%s", buffer2);
    ligne_init++;
    if(buffer2[0]!='o' || buffer2[6]!=':'){
      err("erreur : Veuilliez compiler avec -g pour eviter de mauvaises optimisations de compilation");
    }
    //COMPILER AVEC -g sinon erreur compil
    strcpy(bufJoueurs,strchr(buffer2, ':')+1); // pe +1
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
    printf("-------\n");}*/
    

  }
  void demandeLettres(char buff_lettres[]){
    mvprintw(ligne_init, 0, "Reception des infos de jeu...");
    
    ligne_init++;
    if(buff_lettres[0]!='l' || buff_lettres[15]!=':'){
      err("erreur lettres: Veuilliez compiler avec -g pour eviter de mauvaises optimisations de compilation");
    }
    strcpy(buff_lettres,strchr(buff_lettres, ':')+1);
    char* lettresTrouvees;
    lettresTrouvees=malloc(27*sizeof(char));
    lettresTrouvees= strtok(buff_lettres, ".");
    char* lettresFausses;
    strcpy(word, lettresTrouvees);
    lettresFausses=malloc(27*sizeof(char));
    lettresFausses= strtok(NULL, "$");
    mvprintw(ligne_init, 0, "Mot : %s", lettresTrouvees);
    ligne_init++;
    
    strcpy(lettresFausses,strchr(lettresFausses, ':')+1);
    
    int ind_let_i=0;
    char let_i = lettresFausses[ind_let_i];
    while(let_i!='.'){
      lives--;
      addLetter(let_i,letters);
      ind_let_i++;
      let_i = lettresFausses[ind_let_i];
    }
    mvprintw(ligne_init, 0, "Lettres erronees : %s", letters);
    ligne_init++;
    
  }
  void aff_score(){
    werase(winOthers);
    wrefresh(winOthers);
    int i;
    char nomJ[15];
    for(i=0; i<nbJoueurs;i++){
      strcpy(nomJ, tabJoueurs[i]->nom);
      if(strcmp(nomJ, pseudo)==0){
       wcolor_set(winOthers,GREEN_B,NULL);
        mvwprintw(winOthers,i+1,1, "%s: %s",nomJ, tabJoueurs[i]->points);
        wcolor_set(winOthers,WHITE_B,NULL);
      }else{
        mvwprintw(winOthers,i+1,1, "%s: %s",nomJ, tabJoueurs[i]->points);
      }
    }
    box(winOthers, ACS_VLINE, ACS_HLINE);
    wrefresh(winOthers);
  }

  void *threadOthers(){
    
  	char recu[200];
    while(bool_mot_incomplet && lives > 0){
    	if(read(socket_descriptor, recu, sizeof(recu))<1){
      printf("error read sock\n");
      }
    	char typeMsg = recu[0];
    	if (strcmp(recu[1],":")!=0){
    		// erreur de reception
    		break;
    	}
    	strcpy(recu,strchr(recu, ":")+1);
    	switch (typeMsg){
    	
    	case 'c'://----------connection nouveau client
    		//c:nomDuJoueur.
    		
    	break;
    	case 'd'://----------Déconnection client
    		//d:nomDuJoueur.
    	break;
    	case 'r'://----------réponse d'un client (potentiellement nous)
    		//r:LettreEnvoyée,nomDuJoueur,motHache.
    	break;
    	default://----------
    	break;
    	}
    	

  	}
  }
  void initialisation(){
    char grandBuf[500];
   
    
    demandePseudo();
   
    if(read(socket_descriptor, grandBuf, sizeof(grandBuf))<1){
      printf("error\n");
    }
    pthread_create( &id_threadOthers , NULL ,  threadOthers ,NULL);
	  char* copie;
    copie = malloc(500*sizeof(char));
		strcpy(copie , grandBuf);
    char* bufTmp;
    bufTmp = malloc(200*sizeof(char));
    strcpy(bufTmp , strtok(grandBuf,"$"));
    strcpy(pseudo,bufTmp);
    mvprintw(ligne_init,0,"Votre pseudo sera: %s",bufTmp);
    ligne_init++;

    
    strcpy(bufTmp , strtok(NULL,"$"));

    demandeJoueurs(bufTmp);
    strcpy(bufTmp , strtok(copie,"$"));
    strcpy(bufTmp , strtok(NULL,"$"));
    strcpy(bufTmp , strtok(NULL,"$"));
    
    demandeLettres(bufTmp);
    refresh();
    sleep(1);
    
  }

  

//a quoi ca sert d'en faire un thread ? -> mettre dans le prgm principal ?
  void *threadThisGamer(void * p_data){
  	
	

  }


  //***************************PRGM PRINCIPAL***************************
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

  initialisation();
  
  clear();
  refresh();
  winHangman = newwin(HANGMAN_HEIGHT + 4, HANGMAN_WIDTH + 4, 0, cols - 24);
	winWord = newwin(1, strlen(word), rows/2, (cols-strlen(word))/2);
	winLives = newwin(1, 10, HANGMAN_HEIGHT + 2, cols - 11);
	winLetters = newwin(1, 26, 0, 0);
	winOthers = newwin(nbJoueurs+2,17,3,1);
	
  box(winHangman, ACS_VLINE, ACS_HLINE);
  
  mvwprintw(winWord, 0, 0, "%s", word);
  mvwprintw(winLives, 0, 0, "vies : %d", lives);
  mvwprintw(winLetters, 0, 0, "%s", letters);
  wrefresh(winLetters);
  wrefresh(winHangman);
  wrefresh(winWord);
  wrefresh(winLives);
  aff_score();
  refresh();

    char scannedChar = 0;
  	char oldScannedChar = 0;

    while(bool_mot_incomplet && lives > 0){
      nodelay(winHangman, 1);
		  scannedChar = toupper(wgetch(winHangman));
		  nodelay(winHangman, 0);
		  if(scannedChar >= 'A' && scannedChar <= 'Z'){
			  if(checkLetter(scannedChar, letters) == -1){
				  //envoyer au serveur
					
				
			  }else{
				  //faire clignoter la lettre
				  if(isWarningDone){
					  alreadyWroteIndex = checkLetter(scannedChar, letters);
					  alreadyWroteLetter = scannedChar;
					  wattrset(winLetters, A_REVERSE);
					  mvwaddch(winLetters, 0, alreadyWroteIndex, alreadyWroteLetter);
					  wattroff(winLetters, A_REVERSE);
					  wrefresh(winLetters);
					  startBlinkTime = time(NULL);
					  isWarningDone = false;
				  }else{
					  mvwaddch(winLetters, 0, alreadyWroteIndex, alreadyWroteLetter);

					  alreadyWroteIndex = checkLetter(scannedChar, letters);
					  alreadyWroteLetter = scannedChar;
					  wattrset(winLetters, A_REVERSE);
					  mvwaddch(winLetters, 0, alreadyWroteIndex, alreadyWroteLetter);
					  wattroff(winLetters, A_REVERSE);
					  wrefresh(winLetters);
					  startBlinkTime = time(NULL);
				  }
			  }
		  }
		
		  if(isWarningDone==0){
			  timeOfNow = time(NULL);
			  if(difftime(timeOfNow, startBlinkTime) >= 2){
				  wattroff(winLetters, A_REVERSE);
				  mvwaddch(winLetters, 0, alreadyWroteIndex, alreadyWroteLetter);
				  wrefresh(winLetters);
				  isWarningDone = 1;
			  }
		  }
	}


  endwin();
  

  printf("\nfin de la reception. %d\n", socket_descriptor);

  close(socket_descriptor);

  printf("connexion avec le serveur fermee, fin du programme.\n");
  
  exit(0);

}
