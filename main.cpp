
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

#include "pseudoterminal.h"


using namespace std;


typedef struct {
    uint8_t x, y;
    uint8_t parity;
    uint64_t time;
} event_t;


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

#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>


int file_id = -1;
std::vector<event_t> events;

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

void read_events(std::string filename)
{
    std::string line;
    std::ifstream infile(filename.c_str(), std::ifstream::in);

    while (std::getline(infile, line))
    {
        std::istringstream iss(line);

        std::vector<std::string> a = split(line.c_str(), ' ');

        event_t event;

        int i = 0;
        for (int j = 0; j < a.size(); j++)
        {
            if (i > a.size())
                break;

            if (a[j].size() == 0)
                continue;

            if (i == 0)
            {
                event.x = std::atoi(a[j].c_str());
                i++;
            }
            else if (i == 1)
            {
                event.y = std::atoi(a[j].c_str());
                i++;
            }
            else if (i == 2)
            {
                event.parity = std::atoi(a[j].c_str());
                i++;
            }
            else if (i == 3)
            {
                event.time = std::atoi(a[j].c_str());
                break;
            }
        }

        if (events.size() >= events.max_size())
            return;

        events.push_back(event);
    }
}


void *serial_output_thread(void *threadid)
{
    if (file_id < 0)
    {
        return threadid;
    }

    char buf[2] = "";
    sprintf(buf, "%d", file_id);

    // Create filename
    std::string filename("data_");
    filename.append(buf);
    filename.append(".dvs");

    // Change size of events vector
    events.reserve(200000);

    // Parse events
    read_events(filename);


    // Start time
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    uint64_t start_time = 1000000ull*(uint64_t)(t.tv_sec) + (uint64_t)(t.tv_nsec)/1000ull;

    // First timestamp in file
    uint64_t first_timestamp = events.front().time;



    int byte1, byte2;
    string send_str = "";
    int current = 0;

    while (stop == 0 && global_stop == 0)
    {
        usleep(1000); continue;

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


        // Calc current time
        struct timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);
        uint64_t current_time = 1000000ull*(uint64_t)(t.tv_sec) + (uint64_t)(t.tv_nsec)/1000ull;
        uint64_t time_diff = current_time - start_time;

        // Display all events to current time
        while (events[current].time < (first_timestamp + time_diff))
        {
            byte1 = events[current].x;
            byte1 |= 0x80;

            byte2 = events[current].y;
            byte2 |= (events[current].parity & 0x01) << 7;

            send_str.clear();
            send_str += byte1;
            send_str += byte2;

            printf("Send: %s\n", send_str.c_str());

            pts->writeLine(send_str);


            // Iterator
            current++;

            if (current == (events.size() - 1))
            {
                current = 0;
            }
        }

//        // Generator two random coordinates
//        byte1 = rand() % 128;
//        byte2 = rand() % 128;

//        byte1 |= 0x80;
//        byte2 |= (rand() % 2) << 7;

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

    while ((c = getopt(argc, argv, "c:i:")) != -1)
    {
        switch (c)
        {
        case 'c':
            sscanf(optarg, "%s", device_name);
            printf("Device path: %s\n", (char*)device_name);
            break;

        case 'i':
            sscanf(optarg, "%d", &file_id);
            printf("Filename: data_%d.dvs\n", file_id);
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

