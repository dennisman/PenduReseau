#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initLetters(char letters[])
{
	int i;

	for(i = 0; i < 26; i++)
	{
		letters[i] = '_';
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

int checkLetter2(char letter)
{
	char i;
	int isTheSame = -1;

	for(i = 'A'; i <= 'Z'; i++)
	{
		if(i == letter)
		{
			isTheSame = 0;
		}
	}

	return isTheSame;
}

void addLetter(char letter, char* letters)
{
	int i;
	int j = 0;

	for(i = 'A'; i <= 'Z'; i++)
	{
	  //printf("lettre : %c, i = %c\n",letter, i);
		if (letter == i)
		{ 
			letters[j] = letter;
			break;
		}

		j++;
	}
}
