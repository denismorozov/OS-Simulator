/*
* Filename: Program.h
* Specifications for the Program and Operation objects. 
*/

#ifndef PROGRAM_H
#define PROGRAM_H

#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>
#include <queue>

enum State
{
    START, READY, RUNNING, EXIT
};
    
/* Model of program operations, such as processing and I/O */
struct Operation
{
    char type; // S (OS), A (Program), P (Processing), I (Input), or O (Output)
    std::string description; // end, hard drive, keyboard, monitor, run, or start
    int duration; // cycles

};

/* Models a program which the OS can load and run */
class Program
{
public:
    Program();
    ~Program();
    void add_operation( Operation operation );

    /* Queue containing all program operations */
    std::queue<Operation> operations;

    State state = START;
    int running_time = 0;
    int id = 0;

    bool operator>( const Program &other ) const;

private:
     
};

#endif // PROGRAM_H