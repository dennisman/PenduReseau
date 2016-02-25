#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <ctype.h>
#include <time.h>
#include "Pendu.h"
#include "Word.h"
#include "Commands.h"
#include "DrawHangman.h"
#include "Letters.h"

int main()
{
	WINDOW *winHangman;
	WINDOW *winWord;
	WINDOW *winLives;
	WINDOW *winLetters;
	WINDOW *others;

	time_t startBlinkTime = time(NULL);
	time_t timeOfNow = time(NULL);

	char actualWord[27];
	char word[27];
	char hangman[300] = {0};
	char letters[27] = {0};
	char scannedChar = 0;
	char oldScannedChar = 0;
	char alreadyWroteLetter = 0;

	int lives = 10;
	int rows = 0;
	int cols = 0;
	int alreadyWroteIndex = 0;

	bool isWarningDone = true;

	srand(time(NULL));

	selectWord(word);
	strcpy(actualWord, word);

	initWord(actualWord); /*Transforms every actualWord's chars into '_'*/
	initLetters(letters);

	initscr();
	raw();
	noecho();
	curs_set(0);

	getmaxyx(stdscr, rows, cols);

	if (rows < 16 || cols < 55)
	{
		endwin();
		fprintf(stderr, "ERROR : Too small terminal. Try resizing it. (min 16 rows and 55 colums)\n"
						"I recommand a size of 80x24.\n");
		exit(EXIT_FAILURE);
	}

	/* Creating windows for each element of the GUI */
	winHangman = newwin(HANGMAN_HEIGHT + 4, HANGMAN_WIDTH + 4, 0, cols - 24);
	winWord = newwin(1, strlen(actualWord), rows/2, (cols-strlen(actualWord))/2);
	winLives = newwin(1, 10, HANGMAN_HEIGHT + 2, cols - 11);
	winLetters = newwin(1, 26, 0, 0);

	/* Welcome message */
	mvprintw(4, (cols - 32)/2, "Welcome in the \"Hangman's game\"!");
	mvprintw(7, (cols - 37)/2, "You can see the rules in 'RULES.txt'.");
	mvprintw(rows - 2, cols - (3 + 26), "Press enter to continue...");

	refresh();
	getch(); /* Pause */

	clear();
	refresh();

	box(winHangman, ACS_VLINE, ACS_HLINE);

	mvwprintw(winWord, 0, 0, "%s", actualWord);
	mvwprintw(winLives, 0, 0, "Lives : 10");

	wrefresh(winHangman);
	wrefresh(winWord);
	wrefresh(winLives);

	while(strcmp(actualWord, word) && lives > 0)
	{
		nodelay(winHangman, 1);
		scannedChar = toupper(wgetch(winHangman));
		nodelay(winHangman, 0);

		if(scannedChar >= 'A' && scannedChar <= 'Z')
		{
			if(checkLetter(scannedChar, letters) == -1)
			{
				if (!verifyChar(actualWord, scannedChar, word))
				{
					lives--;

					werase(winHangman);
					wrefresh(winHangman);
					readHangman(hangman, ((lives-10)*-1));
					printHangman(winHangman, 1, 2, hangman);
					box(winHangman, ACS_VLINE, ACS_HLINE);
					wrefresh(winHangman);

					mvwprintw(winLives, 0, 0, "Lives : %d ", lives);
					wrefresh(winLives);
				}
				else
				{
					mvwprintw(winWord, 0, 0, "%s", actualWord);
					wrefresh(winWord);
				}

				if(scannedChar != oldScannedChar)
				{
					addLetter(scannedChar, letters);
					
					mvwprintw(winLetters, 0, 0, "%s", letters);
					wrefresh(winLetters);
				}
			}
			else
			{
				if(isWarningDone)
				{
					alreadyWroteIndex = checkLetter(scannedChar, letters);
					alreadyWroteLetter = scannedChar;
					wattrset(winLetters, A_REVERSE);
					mvwaddch(winLetters, 0, alreadyWroteIndex, alreadyWroteLetter);
					wattroff(winLetters, A_REVERSE);
					wrefresh(winLetters);
					startBlinkTime = time(NULL);
					isWarningDone = false;
				}
				else
				{
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

			oldScannedChar = scannedChar;
		}

		if(!isWarningDone)
		{
			timeOfNow = time(NULL);
			if(difftime(timeOfNow, startBlinkTime) >= 2)
			{
				wattroff(winLetters, A_REVERSE);
				mvwaddch(winLetters, 0, alreadyWroteIndex, alreadyWroteLetter);
				wrefresh(winLetters);
				isWarningDone = true;
			}
		}
	}

	wgetch(winWord);

	delwin(winWord);
	delwin(winHangman);
	delwin(winLives);
	delwin(winLetters);
	endwin();

	return EXIT_SUCCESS;
}
