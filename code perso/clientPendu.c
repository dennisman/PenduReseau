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
#include "DrawHangman.c"

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
  WINDOW *winInfos;
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
  curs_set(1);
  int pasDeReponse=1;
  int finished = 0;

  //***************************Fonctions ***************************
  void closeExit(){
    close(socket_descriptor);
    curs_set(1);
    clear();
    delwin(winWord);
    delwin(winHangman);
    delwin(winLives);
    delwin(winLetters);
    delwin(winInfos);
    delwin(winOthers);
    endwin();
    exit(1);
  }
  void sig_handler(int signo)
  {
    if (signo == SIGINT)
    closeExit();

  }

  if (signal(SIGINT, sig_handler) == SIG_ERR){
    printf("\ncan't catch SIGINT\n");
  }
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
    if ((write(socket_descriptor, pseudo, strlen(pseudo)+1)) < 0) {
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
  void suppJoueur(char * nomJ){
    int trouve=0;
    for(i=0; i<nbJoueurs;i++){
      if(strcmp(nomJ, tabJoueurs[i]->nom)==0){
        trouve=1;
      }
      if(trouve==1 && i<nbJoueurs-1){
        strcpy(tabJoueurs[i]->nom,tabJoueurs[i+1]->nom);
        strcpy(tabJoueurs[i]->points,tabJoueurs[i+1]->points);
      }
    }
    nbJoueurs--;
  }


  void finDuJeu(){
    //*3
    char scannedChar;
    do{
      //nodelay(winHangman, 1);
      scannedChar = toupper(wgetch(winHangman));
      //nodelay(winHangman, 0);
    }while(scannedChar != 'Y' && scannedChar != 'N');

    if(scannedChar == 'Y'){
      //*4
      char envoi[300];
      strcpy(envoi, "$");
      envoi[1]=scannedChar;
      envoi[2]='\0';
      write(socket_descriptor, envoi, strlen(envoi)+1);
      printf("envoi %s\n",envoi);
      //*5
      bzero(envoi,300);
      /*le serveur a attendu que tout les clients repondent
      ou max 20 sec pour renvoyer les donnees :
      donnees:motHache$others:j1,pts;j2,pts;.
      */
      read(socket_descriptor, envoi, sizeof(envoi));
      //*6
      strcpy(word , strtok(envoi,"$"));
      //surement des erreurs par la, tests a faire
      strcpy(envoi , strtok(NULL,"$"));
      nbJoueurs =0;
      demandeJoueurs(envoi);
      //*7
      finished = 0


      /*
      1*arreter la boucle qui demande les touches au joueur
      (if avec un var globale)
      2*affichage des scores/signalement fin
      3*on demande aux utilisateurs si il veulent rejouer
      4*envoie au serveur
      5*renvoie du mot par le serveur et les mecs connectés (max rep 20 sec)
      6*maj mecs co
      7*reprendre la boucle
      */
      ;
    }else{
      char c=27;
      char mess[10];
      mess[0]=c;
      write(socket_descriptor, mess, 2);
      closeExit();
    }

  }
  void jeuPerdu(){
  	//*1
  	finished=1;


    //*2*3
    char infos[50];
    strcpy(infos, "Le groupe n'a plus de vies, voulez vous rejouer? Y/N");
    mvwprintw(winInfos,1,2,"%s",infos);
    box(winInfos, ACS_VLINE, ACS_HLINE);
    wrefresh(winInfos);
  	finDuJeu();
  }

  void jeuWin(){
  	//*1
    finished=1;


    //*2*3
    char infos[50];
    strcpy(infos, "Le mot a été trouvé, voulez vous rejouer? Y/N");
    mvwprintw(winInfos,1,2,"%s",infos);
    box(winInfos, ACS_VLINE, ACS_HLINE);
    wrefresh(winInfos);
    finDuJeu();
  }

  void aff_scores(){
    werase(winOthers);
    wresize(winOthers,nbJoueurs+2,17);
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
  void addPointWin(char * nomJ, char lettre){
    int i;
    int j;
    int motEntier=1;
    int pts;
    char* ptsStr;
    for(i=0; i<nbJoueurs;i++){
      //printf("nomJ %s\n",nomJ);
      //printf("nomjoueur %s\n",tabJoueurs[i]->nom );
      if(strcmp(nomJ, tabJoueurs[i]->nom)==0){
        j=i;
        pts= atoi( tabJoueurs[i]->points);
        for (i=0;i<strlen(word);i++){
          if(word[i]=='_'){
            motEntier=0;
          }
          if(word[i]==lettre){
            pts++;
          }
        }
        break;
      }

    }
    if(motEntier==1){
      pts = pts +10;
    }
    sprintf(tabJoueurs[j]->points,"%d",pts);
    aff_scores();
    if(motEntier==1){
      jeuWin();
    }


  }
  void addPointLoose(char * nomJ){
    int pts,i;
    for(i=0; i<nbJoueurs;i++){
      if(strcmp(nomJ, tabJoueurs[i]->nom)==0){
        pts= atoi( tabJoueurs[i]->points );
        pts--;
        if(pts<0){
          pts=0;
        }
        sprintf(tabJoueurs[i]->points,"%d",pts);
        break;
      }
    }
    aff_scores();
    if (lives<1){
      for(i=0; i<nbJoueurs;i++){
        pts= atoi( tabJoueurs[i]->points );
        pts -= 3;
        if(pts<0){
          pts=0;
        }
        sprintf(tabJoueurs[i]->points,"%d",pts);
      }
      aff_scores();
      jeuPerdu();
    }
  }


  void aff_hangman(){
    werase(winHangman);
    readHangman(hangman, ((lives-10)*-1));
    printHangman(winHangman, 1, 2, hangman);
    box(winHangman, ACS_VLINE, ACS_HLINE);
    wrefresh(winHangman);
  }
  void aff_word(){
    werase(winWord);
    mvwprintw(winWord, 0, 0, "%s", word);
    wrefresh(winWord);
    //si le mot est rempli, fin du jeu appelé par la fonction addPointsWin
  }

  void *threadOthers(){

    char recu[50];
    char lettre;
    char infos[50];
    char nomJ[20];
    int i=0;
    int boolJoueurInconnu=0;
    while(/*bool_mot_incomplet==1 && lives > 0*/1){

      while(read(socket_descriptor, recu, sizeof(recu))<1){
      }
      //printf("mess recu :%s---\n",recu);
      char typeMsg = recu[0];
      if (recu[1]!=':'){
        printf("erreur de reception\n");
        break;
      }
      char copie[50];
      //la copie evite des erreurs
      strcpy(copie,strchr(recu, ':')+1);
      strcpy(recu,copie);
      //printf("strchr recu :%s---\n",recu);
      switch (typeMsg){
        case 'c'://----------connection nouveau client
        //c:nomDuJoueur.
        //printf("recu :%s\n",recu);
        strcpy(nomJ,strtok(recu,"."));
        sprintf(infos,"%s s'est connecté",nomJ);
        wcolor_set(winInfos,CYAN_B,NULL);
        werase(winInfos);
        box(winInfos, ACS_VLINE, ACS_HLINE);
        mvwprintw(winInfos,1,2,"%s",infos);
        wrefresh(winInfos);
        wcolor_set(winInfos,WHITE_B,NULL);
        boolJoueurInconnu=0;
        for(i=0;i<nbJoueurs;i++){
          if(strcmp(tabJoueurs[i]->nom,"NoPseudoYet")==0){
            boolJoueurInconnu=1;
            strcpy(tabJoueurs[i]->nom,nomJ);
            break;
          }
        }
        if(boolJoueurInconnu==0){
          tabJoueurs[nbJoueurs]= malloc(sizeof(joueur));
          strcpy(tabJoueurs[nbJoueurs]->nom,nomJ);
          strcpy(tabJoueurs[nbJoueurs]->points,"10");
        nbJoueurs++;
        }
        
        aff_scores();
        break;
        case 'd'://----------Déconnection client
        //d:nomDuJoueur.
        strcpy(nomJ,strtok(recu,"."));
        sprintf(infos,"%s s'est déconnecté",nomJ);
        wcolor_set(winInfos,CYAN_B,NULL);
        suppJoueur(nomJ);
        aff_scores();
        break;
        case 'v'://----------réponse d'un client (potentiellement nous) Bonne !
        //v:LettreEnvoyée,motHache,nomDuJoueur.

        lettre=recu[0];
        strtok(recu,",");

        strcpy(word,strtok(NULL,","));
        strcpy(nomJ,strtok(NULL,"."));
        sprintf(infos,"%s propose %c",nomJ,lettre);
        wcolor_set(winInfos,GREEN_B,NULL);
        werase(winInfos);
        box(winInfos, ACS_VLINE, ACS_HLINE);
        mvwprintw(winInfos,1,2,"%s",infos);
        wrefresh(winInfos);
        wcolor_set(winInfos,WHITE_B,NULL);
        aff_word();
        //printf("**3**\n");
        addPointWin(nomJ,lettre);
        //printf("**1**\n");
        if(strcmp(nomJ,pseudo) ==0){
          pasDeReponse=0;
        }
        //printf("**2**\n");
        break;
        case 'f'://----------réponse d'un client (potentiellement nous) Fausse !
        //f:LettreEnvoyée,nomDuJoueur.

        lettre=recu[0];
        addLetter(lettre,letters);
        strtok(recu,",");
        strcpy(nomJ,strtok(NULL,"."));
        sprintf(infos,"%s propose %c",nomJ,lettre);
        wcolor_set(winInfos,RED_B,NULL);
        werase(winInfos);
        box(winInfos, ACS_VLINE, ACS_HLINE);
        mvwprintw(winInfos,1,2,"%s",infos);
        wrefresh(winInfos);
        wcolor_set(winInfos,WHITE_B,NULL);
        lives--;
        aff_hangman();
        addPointLoose(nomJ);
        if(strcmp(nomJ,pseudo) ==0){
          pasDeReponse=0;
        }

        break;
        default://----------
        break;
      }


      werase(winLives);
      mvwprintw(winLives, 0, 0, "vies : %d", lives);
      wrefresh(winLives);
      werase(winLetters);
      mvwprintw(winLetters, 0, 0, "%s", letters);
      wrefresh(winLetters);
      bzero(recu,200);
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
  mvprintw(rows/2,cols/2,"Connexion effectuée avec succès.");
  color_set(WHITE_B, NULL);
  mvprintw(rows - 2, cols - (33), "Pressez une touche pour continuer");
  getch();

  initialisation();

  clear();
  refresh();
  raw();
  noecho();
  curs_set(0);
  winHangman = newwin(HANGMAN_HEIGHT + 4, HANGMAN_WIDTH + 4, 0, cols - 24);
  winWord = newwin(1, strlen(word), rows/2, (cols-strlen(word))/2);
  winLives = newwin(1, 10, HANGMAN_HEIGHT + 2, cols - 11);
  winLetters = newwin(1, 26, 0, 0);
  winOthers = newwin(nbJoueurs+2,17,3,1);
  winInfos = newwin(4,55,rows/2 + 5, (cols-55)/2);

  box(winHangman, ACS_VLINE, ACS_HLINE);
  box(winInfos, ACS_VLINE, ACS_HLINE);
  mvwprintw(winWord, 0, 0, "%s", word);
  mvwprintw(winLives, 0, 0, "vies : %d", lives);
  mvwprintw(winLetters, 0, 0, "%s", letters);
  wrefresh(winLetters);


  aff_hangman();

  wrefresh(winWord);
  wrefresh(winLives);
  aff_scores();
  refresh();

  char scannedChar = 0;
  int scannedInt=0;
  char oldScannedChar = 0;
  char mess2serv[10];
  while(1){
    if(finished==0){
      //nodelay(winHangman, 1);
      scannedInt=wgetch(winHangman);
      scannedChar = toupper(scannedInt);
      //nodelay(winHangman, 0);
      if(scannedChar >= 'A' && scannedChar <= 'Z'){
        if(checkLetter(scannedChar, letters) == -1 ){
          if(checkLetter(scannedChar, word)==-1){
            //envoyer au serveur
            bzero(mess2serv,10);
            mess2serv[0]=scannedChar;
            //printf("envoie\n");

            if ((write(socket_descriptor, mess2serv, 2)) < 0) {
              err("erreur : impossible d'ecrire le message destine au serveur.");
            }
            //attendre une reponse pour repartir

            while(pasDeReponse==1){
              //TODO a garder, c'est juste pour debug server qu'on commente
              //printf("attente\n");
              sleep(1);
            }
            pasDeReponse=1;
        }
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

      }else{
        //si on appuie sur echap
        if(scannedInt==27){
          bzero(mess2serv,10);
          mess2serv[0]=scannedChar;
          //printf("envoie\n");

          if ((write(socket_descriptor, mess2serv, 2)) < 0) {
            err("erreur : impossible d'ecrire le message destine au serveur.");
          }
          closeExit();
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
    }else{
      sleep(2);
      //on attend que les joueurs décident de continuer
    }

  }


  endwin();


  printf("\nfin de la reception. %d\n", socket_descriptor);

  close(socket_descriptor);

  printf("connexion avec le serveur fermee, fin du programme.\n");

  exit(0);

}
