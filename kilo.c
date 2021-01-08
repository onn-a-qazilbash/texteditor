#include <ctype.h>   /* iscntrl() */
#include <stdio.h>   /* printf() */
#include <unistd.h>  /* read(), STDIN_FILENO */
#include <termios.h> /* struct termios, tcgetattr(), tcsetattr, ECHO, TCSAFLUSH */
#include <stdlib.h>  /* atexit() */

struct termios original_termios;

void disableRawMode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}


void enableRawMode(){
    /* Switching terminal from canonical  mode to raw mode.
     * We get termianl attributes using tcgetarrt() and store them
     * in the struct terminos. We then flip the ECHO bit to 0.
     * We then store the modified sturct back in memory. 
     * */
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(disableRawMode);
    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


int main(){
    enableRawMode();

    char c;
    while ( read(STDIN_FILENO, &c, 1) == 1  && c != 'q'){
        if ( iscntrl(c)  ){
            printf("%d\n", c); /* Print ASCII code of char */
        }
        else{
            printf("%d ('%c')\n", c, c); /* Print ASCII code and actual character */
        }
    }
    return 0;
}
