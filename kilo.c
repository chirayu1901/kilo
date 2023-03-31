#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

struct termios orig_termios;

void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    /**
     * BRKINT - When BRKINT is turned on, a break condition will cause a SIGINT 
     *          signal to be sent to the program
     * ICRNL  - disable carriage return, newline
     * INPCK  - INPCK enables parity checking (doesn't apply to modern terminals). 
     *          Traditionally done though for enabling raw mode
     * ISTRIP - causes the 8th bit of each input byte to be stripped, meaning it will set it to 0.
     *          Traditionally done though for enabling raw mode
     * IXON   - to pause transmission (legacy stuff)
    */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST); // disable all output processing
    raw.c_cflag |= (CS8); // sets the character size (CS) to 8 bits per byte
    /**
     * ICANON - allows to read byte by byte
     * ECHO   - turns off the echo after exiting the programo
     * IEXTEN - disables the ctrl-v synchronos idle
     * ISIG   - disable the ctrl-c control character
    */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // flushes input -> no longer feeds to the shell after ctrl-c
}

int main()
{
    enableRawMode();

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
    {
        if (iscntrl(c))
        {
            // checking if the character is a control character
            printf("%d\r\n", c);
        }
        else
        {
            printf("%d ('%c')\r\n", c, c);
        }
    }

    return 0;
}
