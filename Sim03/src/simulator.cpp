#include "simulator.h"

/* Constructor for the Simulator class
* Loads the config file for the simulator, if no problems occured then it
* creates the program object (which reads the meta-data file). Also opens the
* output file if needed, and sets precision of double/float outputs
* @param file path for configuration file
*/
Simulator::Simulator( const std::string filePath )
{
    try
    {
        load_config( filePath );
        load_meta_data( metaDataFilePath_ );
    }
    catch( const std::runtime_error& e )
    {
        std::cerr << e.what();
        throw;
    }


    std::cout.precision(6); // precision for printing doubles/floats

    if( logLocation_ == BOTH || logLocation_ == FILE )
    {
        fout_.open( logFilePath_ );
    }
}

Simulator::~Simulator()
{
    if(fout_.is_open())
    {
        fout_.close();
    }
}

/* Run the simulator and all its programs
*/
void Simulator::run()
{
    // Announce beginning of sim and set starting time point
    start_ = std::chrono::system_clock::now();
    print("Simulator program starting");  

    // First In First Out scheduling
    if( schedulingCode_ == "FIFO" )
    {
        print("OS: preparing all processes");
        for( Program program : programs_ )
        {
            program.state = READY;
        }

        int programCounter = 0;
        for( Program program : programs_ )
        {
            print("OS: selecting next process");
            programCounter++;

            program.id = programCounter;            
            program.state = RUNNING;

            while( !program.done() )
            {
                process_operation( program );
            }

            program.state = EXIT;
        }
    }

    // Shortest Remaining Time First - Non Preemptive
    // Also satisfies Shortest Job First
    else
    {
        print("OS: preparing all processes");
        for( Program program : programs_ )
        {
            program.state = READY;
            SRTF_queue_.push(program);
        }

        int programCounter = 0;
        while( !SRTF_queue_.empty() )
        {
            print("OS: selecting next process");

            // remove program with shortest remaining time from queue
            Program program = SRTF_queue_.top(); 
            SRTF_queue_.pop();

            // simple way of telling whether the program ran before (so id
            // doesn't get changed)
            if( program.id == 0 )
            {
                programCounter++;
                program.id = programCounter;
            }

            program.state = RUNNING;
            process_operation( program );

            if( program.done() )
            {
                program.state = EXIT;
            }

            else
            {
                program.state = READY;
                SRTF_queue_.push( program );
            }
        }
    }

    print("Simulator program ending");
}

/* Process program operations. Create a thread for each I/O operation.
* @param program = current program that is being processed
*/
void Simulator::process_operation( Program &program )
{
    const int programID = program.id;
    Operation operation = program.next();

    // If the process is just starting, announce then go on to printing first operation
    if( operation.type == 'A' && operation.description == "start" )
    {
        print("OS: starting process " + std::to_string(programID));
        operation = program.next();
    }


    // Processing operation
    if( operation.type == 'P' )
    {
        print("Process " + std::to_string(programID) + ": start processing action");
        std::this_thread::sleep_for(
            std::chrono::milliseconds( operation.duration )
        );
        print("Process " + std::to_string(programID) + ": end processing action");

    }

    // Input/Output operation
    else if( operation.type == 'I' || operation.type == 'O' )
    {
        // create a thread for any I/O operation
        std::thread IO_thread( [this, operation, programID](){
            process_IO(operation, programID);
        });

        // wait for IO to finish execution
        if( IO_thread.joinable() )
        {
            IO_thread.join();
        }
    }


    // if only one operation remains, it must be the program end announcement
    if( program.remaining_operations() == 1 )
    {
        program.next(); // just to pop the last item off
        print("OS: removing process " + std::to_string(programID));
    }
}

/* Process I/O operation. This function is always called in a separate thread
* @param operation = current operation that is being processed
* @param programID = id of current program for printing
*/
void Simulator::process_IO( const Operation& operation, const int programID )
{
    if( operation.description == "hard drive" )
    {
        std::string accessType;
        if( operation.type == 'I' )
        {
            accessType = "input";
        }

        else
        {
            accessType = "output";
        }

        print("Process " + std::to_string(programID) + ": start hard drive " + accessType );
        std::this_thread::sleep_for(
            std::chrono::milliseconds( operation.duration )
        );
        print("Process " + std::to_string(programID) + ": end hard drive " + accessType );
    }
    else if( operation.description == "keyboard" )
    {
        print("Process " + std::to_string(programID) + ": start keyboard input");
        std::this_thread::sleep_for(
            std::chrono::milliseconds( operation.duration )
        );
        print("Process " + std::to_string(programID) + ": end keyboard input");
    }
    else if( operation.description == "monitor" )
    {
        print("Process " + std::to_string(programID) + ": start monitor output");
        std::this_thread::sleep_for(
            std::chrono::milliseconds( operation.duration )
        ); 
        print("Process " + std::to_string(programID) + ": end monitor output");           
    }
    else if( operation.description == "printer" )
    {
        print("Process " + std::to_string(programID) + ": start printer output");
        std::this_thread::sleep_for(
            std::chrono::milliseconds( operation.duration )
        ); 
        print("Process " + std::to_string(programID) + ": end printer output");               
    }
}

/* Prints OS action to file, screen, or both, with elapsed time
* @param Message to be printed
*/
void Simulator::print( const std::string message )
{
    auto time = elapsed_seconds().count();
    if( logLocation_ == BOTH || logLocation_ == SCREEN )
    {
        std::cout << std::fixed << time << " - " << message << std::endl;
    }
    if( logLocation_ == BOTH || logLocation_ == FILE )
    {
        fout_ << std::fixed << time << " - " << message << std::endl;
    }    
}

/* Calculates elapsed time from the beginning of the simulation
* @return Time difference
*/
std::chrono::duration<double> Simulator::elapsed_seconds()
{
    return std::chrono::system_clock::now()-start_;
}

/* Loads data from the config file
* @param Path to config file
* @except Throws exception if file wasn't found or file format isn't correct
*/
void Simulator::load_config( const std::string filePath )
{
    // attempt opening the file
    std::ifstream fin( filePath, std::ifstream::in );
    if(!fin)
    {
        std::string error = "Error: Unable to open file " + filePath + "\n";
        throw std::runtime_error( error );
    }

    // make sure the first line of the config file is correct
    std::string configFormatLine;
    std::getline(fin, configFormatLine, '\n');
    if( configFormatLine != "Start Simulator Configuration File" )
    {
        throw std::runtime_error( "Error: Incorrect config file format\n" );
    }

    // ignoring everything up to the ':' 
    auto limit = std::numeric_limits<std::streamsize>::max();
    fin.ignore( limit, ':' );

    // make sure the configuration file is for the correct simulator version
    float simVersion_;
    fin >> simVersion_;
    if( simVersion_ != simulatorVersion_ )
    {
        throw std::runtime_error( "Error: Wrong simulator version\n" );
    }
    fin.ignore( limit, ':' );

    // get the rest of the data from the config file
    fin >> metaDataFilePath_;
    fin.ignore( limit, ':' );

    fin >> schedulingCode_;
    fin.ignore( limit, ':' );
    if( schedulingCode_ != "FIFO" &&
        schedulingCode_ != "SJF" &&
        schedulingCode_ != "SRTF-N" )
    {
        throw std::runtime_error( "Error: Unrecognized scheduling code\n" );
    }

    fin >> quantum_;
    fin.ignore( limit, ':' );

    fin >> processorCycleTime_;
    fin.ignore( limit, ':' );

    fin >> monitorDisplayTime_;
    fin.ignore( limit, ':' );

    fin >> hardDriveCycleTime_;
    fin.ignore( limit, ':' );

    fin >> printerCycleTime_;
    fin.ignore( limit, ':' );

    fin >> keyboardCycleTime_;
    fin.ignore( limit, ':' );

    std::string logString;
    fin >> std::ws; // ignore the space after :
    std::getline(fin, logString, '\n');

    // transform the log location to enum for easier processing later
    if( logString == "Log to Both")
    {
        logLocation_ = BOTH;
    }
    else if( logString == "Log to File" )
    {
        logLocation_ = FILE;
    }
    else
    {
        logLocation_ = SCREEN;
    }
    fin.ignore( limit, ':' );

    fin >> logFilePath_;

    // make sure the config file ends here
    fin >> configFormatLine;
    if( configFormatLine != "End" )
    {
        throw std::runtime_error( "Error: Incorrect config file format\n" );
    }

    fin.close();
}

/* Loads each operation specified in the meta-data file into queue 
* @param file path for the meta data file
*/
void Simulator::load_meta_data( const std::string filePath )
{

    // string object that's used throughout this function
    std::string input;

    // attempt opening the file
    std::ifstream fin( filePath, std::ifstream::in );
    if(!fin)
    {
        std::string error = "Error: Unable to open file " + filePath + "\n";
        throw std::runtime_error( error );
    }

    // make sure the beginning of the file is correct
    std::getline(fin, input, ';');
    fin >> std::ws;
    if( input != "Start Program Meta-Data Code:\nS(start)0" )
    {
        throw std::runtime_error( "Error: Incorrect meta-data file format: \
            Simulator start flag is missing\n" );
    }

    Operation operation;
    int paranthesisLocation;


    while( fin.peek() != 'S' )
    {
        Program newProgram;

        // Get all program data
        do
        {
            // after getline, string looks like this: "A(start)0"   
            std::getline(fin, input, ';');
            paranthesisLocation = input.find(')');

            // parsing Operation object
            operation.type = input.front();        
            operation.description = input.substr(2, paranthesisLocation-2);
            operation.cycles = std::stoi(
                std::string( input.begin()+paranthesisLocation+1, input.end()) 
            );

            // find and set cycle time of the operation
            set_operation_cycle_time(operation);

            // insert operation into queue
            newProgram.add_operation(operation);

            // eat whitespace
            fin >> std::ws;
        } while( input != "A(end)0" );

        // insert the complete program into list of programs
        programs_.push_back(newProgram);
    }

    // Make sure the simulator end flag is there
    std::getline(fin, input, '.');
    fin >> std::ws;
    if( input != "S(end)0" )
    {
        throw std::runtime_error( "Error: Incorrect meta-data file format: \
            Simulator end flag is missing\n" );
    }

    // make sure the last line of the file is correct
    std::getline(fin, input, '.');
    if( input != "End Program Meta-Data Code" )
    {
        throw std::runtime_error( "Error: Incorrect meta-data file format: \
            Meta-Data file does not end after simulator operations end\n" );
    }

    fin.close();
}

void Simulator::set_operation_cycle_time( Operation &operation )
{
    if( operation.type == 'P' )
    {
        operation.cycleTime = processorCycleTime_;
    }

    else if( operation.type == 'I' || operation.type == 'O' )
    {
        if( operation.description == "hard drive" )
        {
            operation.cycleTime = hardDriveCycleTime_;
        }
        
        else if( operation.description == "keyboard" )
        {
            operation.cycleTime = keyboardCycleTime_;
        }
        else if( operation.description == "monitor" )
        {
            operation.cycleTime = monitorDisplayTime_;
        }

        else if( operation.description == "printer" )
        {
            operation.cycleTime = printerCycleTime_;
        }
    }

    else if( operation.type == 'A' || operation.type == 'S' )
    {
        operation.cycleTime = 0;
    }

    else throw std::runtime_error( "Error: Unrecognized operation type, \
        check meta-data file" );
}
