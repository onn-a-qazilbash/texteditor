/* INCLUDES  */
#include <ctype.h>   /* iscntrl() */
#include <errno.h>   /* errno, EAGAIN */
#include <stdio.h>   /* printf(), perror() */
#include <stdlib.h>  /* atexit(), exit() */
#include <sys/ioctl.h>
#include <termios.h> /* struct termios, tcgetattr(), tcsetattr, ECHO, TCSAFLUSH, ISIG, IXON, IEXTEN, ICRNL, OPOST */
#include <unistd.h>  /* read(), STDIN_FILENO */

/* DEFINES */
#define CTRL_KEY(k) ((k) & 0x1f)


/* DATA */
struct editorConfig {
    int screenrows;
    int screencols;
    struct termios original_termios;
};
struct editorConfig E;


/* TERMINAL */
void die(const char * s) {
    // Clear the screen and reposition cursor
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode(){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios) == -1){
        die("tcsetattr");
    }
}

void enableRawMode(){
    /* Switching terminal from canonical  mode to raw mode.
     * We get termianl attributes using tcgetarrt() and store them
     * in the struct terminos. We then flip the ECHO bit to 0.
     * We then store the modified sturct back in memory. 
     * */
    if (tcgetattr(STDIN_FILENO, &E.original_termios) == -1){
        die("tcgetattr");
    }
    atexit(disableRawMode);

    struct termios raw = E.original_termios;
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

char editorReadKey(){
    int nread;
    char c;
    while ( (nread = read(STDIN_FILENO, &c, 1)) != 1 ){
        if (nread == -1 && errno != EAGAIN) {
            die("read");
        }
    }
    return c;
}

int getWindowSize(int * rows, int * cols){
    struct winsize ws;

    if (1 ||ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if ( write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12 ) return -1;
        editorReadKey();
        return -1;
    }
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0; 
}


/* OUTPUT  */

void editorDrawRows(){
    for (int y = 0; y < E.screenrows; y++){
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}


/* INPUT */
void editorProcessKeypress(){
    char c  = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

/* INIT */

void initEditor(){
    if ( getWindowSize(&E.screenrows, &E.screencols) == -1 ){
        die("getWindowSize");
    } 
}

int main(){
    enableRawMode();
    initEditor(); 
    while (1){  
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
