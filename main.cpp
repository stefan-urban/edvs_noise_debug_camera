
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#include "pseudoterminal.h"


using namespace std;


const string helptext =
"Folgende Befehle stehen zur Verfügung: \n\r\
 ?? - Diese Hilfe aufrufen \n\r\
 E+ - Anfangen serielle Daten zu Senden \n\r\
 E- - Aufhören serielle Daten zu Senden \n\r\
 !Q - Programm beenden";


int global_stop = 0;
int stop = 0;

int enable_serial_data = 0;

PseudoTerminal *pts = NULL;


void termination_handler(int signum)
{
    global_stop = 1;
}

string serial_interface(string command)
{
    string out = "";

    // On empty input, just display console again
    if (command.length() == 0)
    {
        out = string("\r> ");
        return out;
    }

    printf("Command: %s\n", command.c_str());

    // Various commands
    if (command == "??")
    {
        out = helptext;
    }
    else if (command == "E+")
    {
        enable_serial_data = 1;
        out = string("enabled serial data stream");
    }
    else if (command == "E-")
    {
        enable_serial_data = 0;
        out = string("disabled serial data stream");
    }
    else if (command == "!Q")
    {
        global_stop = 1;
        out = string("Exiting ...");
        return out;
    }
    else
    {
        out = string("Command not found!");
    }

    // Append command line
    out = "\r" + out;
    out += "\n\r> ";

    return out;
}

void *serial_output_thread(void *threadid)
{
    int byte1, byte2;
    string send_str = "";

    while (stop == 0 && global_stop == 0)
    {
        usleep(100);

        // Output only works if terminal is available
        if (pts == NULL)
        {
            continue;
        }

        // Run only if requested
        if (enable_serial_data == 0)
        {
            continue;
        }

        // Generator two random coordinates
        byte1 = rand() % 128;
        byte2 = rand() % 128;

        byte1 |= 0x80;
        byte2 |= (rand() % 2) << 7;

        send_str.clear();
        send_str += byte1;
        send_str += byte2;

        printf("Send: %s\n", send_str.c_str());

        pts->writeLine(send_str);
    }

    return threadid;
}

char device_name[100] = "/dev/edvs_camera_debug";

void* serial(void* ptr)
{
    // Read from pseudo terminal as long as stop condition is not fullfilled
    string input, command, ret_str;

    pts->writeLine("> ");

    while (stop == 0 && global_stop == 0)
    {
        command.erase();

        // Read complete line
        do
        {
            input = pts->readChar();

            // Do not accept return
            if (input == "\r")
            {
                input = '\n';
            }
            command += input;

            // For the user to see his input, return it
            pts->writeLine(input);
        }
        while (command.at(command.length()-1) != '\n');

        // Cut off last char
        command.erase(command.length()-1, 1);

        // Process command ...
        ret_str = serial_interface(command);

        // ... and write answer back onto pseudo terminal
        pts->writeLine(ret_str);

        usleep(10000);
    }

    unlink(device_name);

    return (void*) 0;
}


int main(int argc, char** argv)
{
    // Get command line options
    int c;

    while ((c = getopt(argc, argv, "c:")) != -1)
    {
        switch (c)
        {
        case 'c':
            sscanf(optarg, "%s", device_name);
            printf("Device path: %s\n", (char*)device_name);
            break;

        default:
            break;
        }
    }


    // Setup right singal handling (allows to safely terminate applicaiton with Ctrl+C)
    // ... as described in http://www.gnu.org/software/libc/manual/html_node/Sigaction-Function-Example.html
//    struct sigaction new_action, old_action;

//    new_action.sa_handler = termination_handler;
//    sigemptyset (&new_action.sa_mask);
//    new_action.sa_flags = 0;

//    sigaction (SIGINT, NULL, &old_action);
//    if (old_action.sa_handler != SIG_IGN)
//    {
//        sigaction (SIGINT, &new_action, NULL);
//    }

//    sigaction (SIGKILL, NULL, &old_action);
//    if (old_action.sa_handler != SIG_IGN)
//    {
//        sigaction (SIGKILL, &new_action, NULL);
//    }

//    sigaction (SIGTERM, NULL, &old_action);
//    if (old_action.sa_handler != SIG_IGN)
//    {
//        sigaction (SIGTERM, &new_action, NULL);
//    }

    // Open up pseudo terminal
    try
    {
        pts = new PseudoTerminal();
    }
    catch (const std::exception &e)
    {
        cout << "Could not create pseudo terminal!" << endl;
        return -1;
    }

    // Create symlink
    string pts_path = pts->getPath();

    unlink(device_name);

    if (symlink(pts_path.c_str(), device_name) < 0)
    {
        cout << "Could not create symlink!" << endl;
        //return -1;
    }
    else
    {
        printf("Symlink %s created!\n", (char*)device_name);
    }

    // Open up serial output thread
    pthread_t thread, thread2;

    pthread_create(&thread, NULL, serial_output_thread, NULL);
    pthread_create(&thread2, NULL, serial, NULL);

    // Debug output
    cout << "PTS-Path: " << pts->getPath() << endl;


    char  a;

    while(global_stop == 0)
    {
        cin >> a;

        if (a == 'q')
        {
             global_stop = 1;
        }

        usleep(100000);
    }

    return 0;
}

