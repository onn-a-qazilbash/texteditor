#include <ctype.h>   /* iscntrl() */
#include <stdio.h>   /* printf() */
#include <unistd.h>  /* read(), STDIN_FILENO */
#include <termios.h> /* struct termios, tcgetattr(), tcsetattr, ECHO, TCSAFLUSH, ISIG, IXON, IEXTEN, ICRNL, OPOST */
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


    raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
    raw.c_iflag &= ~(OPOST);
    /* Set 8 bits per byte, if not already. */
    raw.c_iflag &= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


int main(){
    enableRawMode();

    char c;
    while ( read(STDIN_FILENO, &c, 1) == 1  && c != 'q'){
        if ( iscntrl(c)  ){
            printf("%d\r\n", c); /* Print ASCII code of char */
        }
        else{
            printf("%d ('%c')\r\n", c, c); /* Print ASCII code and actual character */
        }
    }
    return 0;
}
