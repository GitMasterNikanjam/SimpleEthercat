// For complie and build:
// mkdir -p ./bin && g++ -o ./bin/ex1 ex1.cpp ../SimpleEthercat.cpp -lsoem -Wall -Wextra -std=c++17

// For run:
// sudo ./bin/ex1

// ########################################################################################
// Header Includes:

#include <iostream>              // standard I/O operations
#include <string.h>              // string manipulation     
#include <cinttypes>             // integer types
#include <thread>
#include <chrono>                // system clock functions
#include <ctime>                 // system clock functions
#include "../SimpleEthercat.h"     // EtherCAT functionality 

using namespace std;

// ###############################################
// Global Variables

SimpleEthercat ethercat;
const char port_name[] = "enp2s0";

// ################################################

int main(void)
{

    if (ethercat.init(port_name))
    {
        printf("Ethercat on %s succeeded.\n",port_name);
    }
    else
    {
        cout << ethercat.errorMessage << endl;
        return 1;
    }

    if(ethercat.configSlaves())
    {
        printf("Slaves mapped, state to SAFE_OP.\n");
    }
    else
    {
        cout << ethercat.errorMessage << endl;
    }

    printf("%d slaves found and configured.\n",ethercat.getSlaveCount());

    ethercat.configMap();
    ethercat.configDc();

    ethercat.listSlaves();

    if(ethercat.setOperationalState())
    {
        printf("Operational state reached for all slaves.\n");
    }
    else
    {
        /*
        If not all slaves reach the operational state within the specified timeout, 
        the code prints a message indicating which slaves failed to reach the operational state.
        */
        printf("Not all slaves reached operational state.\n");
        ethercat.showStates();
    }

    printf("\nRequest init state for all slaves\n");
    ethercat.setInitState();

    printf("close ethercat socket\n");
    ethercat.close();
    
    printf("debug\n");
    return 0;
}