#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "Pendu.h"

void readHangman(char hangman[], int hangmanIndex)
{
	FILE *hangmanFile;
	char line[100];
	int end = 0;
	int count = 0;

	hangman[0] = '\0';

	hangmanFile = fopen("./Data/Pendus", "r");

	if (hangmanFile == NULL)
	{
		fprintf(stderr, "Error: Unable to open file.\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		while (count != hangmanIndex)
		{
			fgets(line, 100, hangmanFile);
			if (!strcmp(line, "<Pendu>\n"))
			{
				count++;
			}
		}

		fgets(line, 100, hangmanFile);

		while (strcmp(line, "<Pendu>\n") && end == 0)
		{
			strcat(hangman, line);
			if (fgets(line, 100, hangmanFile) == NULL)
			{
				end = 1;
			}
		}
	}

	fclose(hangmanFile);
}

void printHangman(WINDOW *whereToPrint, int yPos, int xPos, char hangman[])
{
	int i;
	int j;

	wmove(whereToPrint, yPos, xPos);

	for(i = 0; hangman[i] != '\0'; i++)
	{
		if (hangman[i] != '\n' && hangman[i] != '/' && hangman[i] != '0')
		{
			waddch(whereToPrint, NCURSES_ACS(hangman[i]));
		}
		else if (hangman[i] != '\0')
		{
			waddch(whereToPrint, hangman[i]);
		}
		
		if (hangman[i] == '\n')
		{
			for(j = 0; j < xPos; j++)
			{
				waddch(whereToPrint, ' ');
			}
		}
	}

	wrefresh(whereToPrint);
}