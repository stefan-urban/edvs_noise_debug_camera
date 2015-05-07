
#define _XOPEN_SOURCE

#include <string>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#include "pseudoterminal.h"

using namespace std;

#define BAUDRATE (B115200)

PseudoTerminal::PseudoTerminal()
{
    char *slavedevice;

    masterfd = open("/dev/ptmx", O_RDWR | O_NOCTTY);

    // Opening file failed
    if (masterfd == -1)
    {
        throw new std::exception();
    }

    // Access configuration
    if (grantpt (masterfd) == -1 || unlockpt (masterfd) == -1)
    {
        throw new std::exception();
    }

    // Get path to slave device
    if ((slavedevice = ptsname (masterfd)) == NULL)
    {
        throw new std::exception();
    }

    path = std::string(slavedevice);

    // Setup everything
    struct termios newtio;
    memset(&newtio, 0, sizeof(newtio)); /* clear struct for new port settings */

    /*
        BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
        CRTSCTS : output hardware flow control (only used if the cable has
                  all necessary lines. See sect. 7 of Serial-HOWTO)
        CS8     : 8n1 (8bit,no parity,1 stopbit)
        CLOCAL  : local connection, no modem contol
        CREAD   : enable receiving characters
    */
    newtio.c_cflag = BAUDRATE | CS8 | CREAD; // | CLOCAL;

    /*
        IGNPAR  : ignore bytes with parity errors
        ICRNL   : map CR to NL (otherwise a CR input on the other computer
                  will not terminate input)
                  otherwise make device raw (no other input processing)
    */
    newtio.c_iflag = IGNPAR; // | ICRNL;

    /*
        Raw output.
    */
    newtio.c_oflag = 0;

    /*
        ICANON  : enable canonical input
                  disable all echo functionality, and don't send signals to calling program
    */
    newtio.c_lflag = ICANON;

    /*
        initialize all control characters
        default values can be found in /usr/include/termios.h, and are given
        in the comments, but we don't need them here
    */
    newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */
    newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
    newtio.c_cc[VERASE]   = 0;     /* del */
    newtio.c_cc[VKILL]    = 0;     /* @ */
    newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
    newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
    newtio.c_cc[VSWTC]    = 0;     /* '\0' */
    newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */
    newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
    newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
    newtio.c_cc[VEOL]     = 0;     /* '\0' */
    newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
    newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
    newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
    newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
    newtio.c_cc[VEOL2]    = 0;     /* '\0' */

    // Now clean the modem line and activate the settings for the port
    tcflush(masterfd, TCIFLUSH);
    tcsetattr(masterfd,TCSANOW,&newtio);
}

PseudoTerminal::~PseudoTerminal()
{
    close(masterfd);
}

string PseudoTerminal::getPath()
{
    return path;
}

string PseudoTerminal::readChar()
{
    int size;
    char buffer[1];

    // Read from serial port and make sure the string is terminated
    size = read(masterfd, buffer, 1);

    // Some error occured
    if (size < 1)
    {
        return std::string("a");
        throw new std::exception();
    }

    return std::string(buffer);
}

void PseudoTerminal::writeLine(string str)
{
    write(masterfd, str.c_str(), str.length());
}
