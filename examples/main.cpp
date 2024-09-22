// For complie:
// g++ -o main main.cpp simple_ethercat.cpp -lsoem
// ###############################################
// Header Includes:
#include <iostream>              // standard I/O operations
#include <string.h>              // string manipulation     
#include <cinttypes>             // integer types
#include <thread>
#include <chrono>                // system clock functions
#include <ctime>                 // system clock functions
#include "simple_ethercat.h"     // EtherCAT functionality 

using namespace std;
// ###############################################
// Global Variables
SIMPLE_ETHERCAT ETHERCAT;
const char port_name[] = "enp2s0";
bool ethercat_init_port_flag = FALSE;
// ################################################

int main(void)
{

    if (ETHERCAT.init_port(port_name))
    {
        ethercat_init_port_flag = TRUE;
        printf("Ethercat on %s succeeded.\n",port_name);
    }
    else
    {
        printf("No socket connection on %s\nExecute as root maybe solve problem \n",port_name);
    }

if(ethercat_init_port_flag)
{
    if(ETHERCAT.init_slaves())
    {
        printf("Slaves mapped, state to SAFE_OP.\n");
    }
    else
    {
        printf("Failed to reach Safe-operational state\n");
    }

    printf("%d slaves found and configured.\n",ETHERCAT.getSlaveCount());

    ETHERCAT.listSlaves();

    if(ETHERCAT.setOperationalState())
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
        ETHERCAT.showStates();
    }

    printf("\nRequest init state for all slaves\n");
    ETHERCAT.setInitState();

    printf("close ethercat socket\n");
    ETHERCAT.close();
}

    return 0;
}