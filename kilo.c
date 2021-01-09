/* INCLUDES  */
#include <ctype.h>   /* iscntrl() */
#include <errno.h>   /* errno, EAGAIN */
#include <stdio.h>   /* printf(), perror() */
#include <unistd.h>  /* read(), STDIN_FILENO */
#include <termios.h> /* struct termios, tcgetattr(), tcsetattr, ECHO, TCSAFLUSH, ISIG, IXON, IEXTEN, ICRNL, OPOST */
#include <stdlib.h>  /* atexit(), exit() */

/* DATA */
struct termios original_termios;

/* TERMINAL */
void die(const char * s) {
    perror(s);
    exit(1);
}

void disableRawMode(){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1){
        die("tcsetattr");
    }
}

void enableRawMode(){
    /* Switching terminal from canonical  mode to raw mode.
     * We get termianl attributes using tcgetarrt() and store them
     * in the struct terminos. We then flip the ECHO bit to 0.
     * We then store the modified sturct back in memory. 
     * */
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1){
        die("tcgetattr");
    }
    atexit(disableRawMode);

    struct termios raw = original_termios;
    raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
    raw.c_iflag &= ~(OPOST);
    /* Set 8 bits per byte, if not already. */
    raw.c_iflag &= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0; /* number of bytes before read returns */ 
    raw.c_cc[VTIME] = 1; /* seconds before timeout (1/10 of a second) */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
        die("tcsetattr");
    }
}

/* INIT */

int main(){
    enableRawMode();
    
    while (1){  
        char c = '\0';
        if (read( STDIN_FILENO, &c, 1  ) == -1 && errno != EAGAIN){
            die("read");
        }
        if ( iscntrl(c)  ){
                printf("%d\r\n", c); /* Print ASCII code of char */
        }
        else{
                printf("%d ('%c')\r\n", c, c); /* Print ASCII code and actual character */
        }
        if ( c == 'q' ){
            break;
        }
    }
    return 0;
}
