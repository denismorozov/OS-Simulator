/* Sim01
*
* Filename: main.cpp
*
* Description: This operating system simulator runs a single program. Simulator
* settings are specified in the configuration file.
*
* Compilation: Use the provided makefile.
*   $ make
*
* Usage: The executable requires the configuration file as a command line 
* argument.
*   $ ./sim01 config_file.cnf
*/

/* Dependencies */
#include <iostream>
#include "Simulator.h"

/* Main */
int main(const int argc, char const *argv[])
{
    // Check to see if a configuration file was provided
    if( argc != 1 )
    {
        std::cerr 
        << "Error: Incorrect number of command line arguments" << std::endl
        << "Example usage: " << argv[0] << " config_file.cnf" << std::endl;        
        return EXIT_FAILURE;
    }

    // Run the simulation with provided config file
    Simulator simulator( argv[1] );
    simulator.run();

    return EXIT_SUCCESS;
}