#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

char * getMyTime(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char * myTime = malloc(20 * sizeof(char));
    sprintf (myTime, "%dH:%dM:%dS", tm.tm_hour, tm.tm_min, tm.tm_sec);    
    return myTime;
}

void log(char * message, char * mess2){
    FILE *f = fopen("error.log", "a");
    if (f == NULL)
    {
        perror("ERROR DURING OPENING FILE :");
        exit(errno);
    }
    char * ti = getMyTime();
    fprintf(f, "%s : %s %s\n", ti, message, mess2);
    free(ti);
    fclose(f);
}
