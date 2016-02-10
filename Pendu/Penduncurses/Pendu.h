#ifndef DEF_PENDU
#define DEF_PENDU

#if defined(_WIN32) || defined(_WIN64)
#define CLEAR_SCREEN "cls"
#elif defined(__linux__)
#define CLEAR_SCREEN "clear"
#endif

#define HANGMAN_HEIGHT 12
#define HANGMAN_WIDTH 20

#define true 1
#define false 0

#endif