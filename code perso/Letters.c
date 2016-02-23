#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initLetters(char letters[])
{
	int i;

	for(i = 0; i < 26; i++)
	{
		letters[i] = ' ';
	}

	letters[27] = '\0';
}

int checkLetter(char letter, char letters[])
{
	unsigned int i;
	int isTheSame = -1;

	for(i = 0; i <= strlen(letters); i++)
	{
		if(letters[i] == letter)
		{
			isTheSame = i;
		}
	}

	return isTheSame;
}

void addLetter(char letter, char letters[])
{
	int i;
	int j = 0;

	for(i = 'A'; i <= 'Z'; i++)
	{
		if (letter == i)
		{
			letters[j] = letter;
		}

		j++;
	}
}