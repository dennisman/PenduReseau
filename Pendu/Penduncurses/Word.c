#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void initWord(char word[])
{
	int i = 0;

	for (i = 0; word[i] != '\0'; i++)
	{
		if (word[i] != ' ')
		{
			word[i] = '_';
		}
	}
}

int verifyChar(char word[], const char charToVerify, const char mysteryWord[])
{
	unsigned int i;
	int found = 0;

	for (i = 0; i < strlen(word); i++)
	{
		if (charToVerify == mysteryWord[i])
		{
			word[i] = mysteryWord[i];
			found = 1;
		}
	}

	return found;
}

void selectWord(char word[])
{
	FILE *wordFile;

	char wordWithSpaces[27] = {'\0'};

	int numberOfWords = 0;
	unsigned int randWord;
	unsigned int i;
	unsigned int j;

	wordFile = fopen("./Data/Dico", "r");

	if(wordFile == NULL)
	{
		fprintf(stderr, "Error: Unable to open file.\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		while(fgets(word, 27, wordFile) != NULL && strcmp(word, "<EOF>\n"))
		{
			numberOfWords++;
		}

		while(strlen(word) < 7)
		{
			fseek(wordFile, 0, SEEK_SET);

			randWord = rand()%(numberOfWords)+1;

			for(i = 0; i < randWord; i++)
			{
				fgets(word, 27, wordFile);
			}

			word[strlen(word)-1] = '\0';
		}

		for(i = 0; i <= strlen(word); i++)
		{
			j = 2 * i;

			wordWithSpaces[j] = word[i];
			wordWithSpaces[j + 1] = ' ';
		}

		wordWithSpaces[j + 1] = '\0';
		strcpy(word, wordWithSpaces);

	}

	fclose(wordFile);
}