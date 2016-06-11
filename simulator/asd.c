#include "ncurses.h"

int main(void)
{
    initscr();
    clear();
    refresh();
    endwin();
    return 0;
}