


#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <ctype.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>

char buf[256] = "dede4,10;azerty5,10;sdfgh4,10;wxcvbn5,10;poiuytr4,10;.";

int main(){
  char * joueur_i;
  joueur_i = strtok(buf, ";");
  char ** tab;
  tab= malloc(5*sizeof(char*));
  char ** tab2;
  tab2= malloc(5*sizeof(char*));
  int i = 0;
  do{
    tab[i] = joueur_i;
    joueur_i = strtok(NULL, ";");
    i++;
   }while(strcmp(joueur_i ,".")!=0 &&joueur_i != NULL);
   
   int j;
   for(j=0; j<i;j++){
    joueur_i = strtok(tab[j], ",");
    strcpy(tab[j], joueur_i);
    //printf("1-joueur i: %s\n", joueur_i);
    joueur_i = strtok(NULL, ",");
    tab2[j]= joueur_i;
    //printf("1-joueur i: %s\n", joueur_i);
   }
   
   for(j=0; j<i;j++){
    printf("tab 1 : %s\n", tab[j]);
    printf("tab 2 : %s\n", tab2[j]);
   }
   
   
   
  return 0;
}
