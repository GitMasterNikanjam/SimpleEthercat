#include "SimpleEthercat.h"

/*
This macro defines the timeout value (in milliseconds) used for 
monitoring EtherCAT communication. It determines the maximum time the program 
will wait for responses from slaves before considering them as not responding.
*/
#define EC_TIMEOUTMON 500


bool SIMPLE_ETHERCAT::init(const char* port_name)
{
    // Start the thread_errorCheck using a member function
    //thread_errorCheck = std::thread(&SIMPLE_ETHERCAT::ecatcheck, this);

    /* initialise SOEM, bind socket to port_name */
    /*
    The function ec_init initializes the SOEM library and binds a socket to the specified network interface (port_name). 
    If successful, it returns a non-zero value, indicating success.
    If ec_init succeeds, the code inside the if block is executed, indicating that the initialization was successful.
    It prints a message confirming the successful initialization.
    */
    if(!ec_init(port_name))
    {
        return false;
    }

    _state = EC_STATE_INIT;

    // update read states of slaves.
    _readStates();

    return true;
}

bool SIMPLE_ETHERCAT::configSlaves(void)
{
    /* find and auto-config slaves */
    /*
    The function ec_config_init is called to auto-configure the EtherCAT slaves connected to the network.
    If at least one slave is found and configured (ec_config_init returns a value greater than 0), 
    the code proceeds with slave mapping and configuration.
    When ec_config_init finishes it will have requested all slaves to state PRE_OP.
    */
    if ( ec_config_init(FALSE) > 0 )
    {
        /*
        ec_slavecount is a variable used to store the number of EtherCAT slaves that have been found 
        and configured during the auto-configuration process. In the context of the 
        SOEM (Simple Open EtherCAT Master) library, ec_slavecount provides the count of EtherCAT slaves 
        that are present and recognized on the EtherCAT network.
        This variable is typically initialized and updated during the auto-configuration phase, 
        where the SOEM library scans the network, identifies connected EtherCAT slaves, 
        and configures them for communication.
        */
        _slaveCount = ec_slavecount;

        _state = EC_STATE_PRE_OP;
    }
    else
    {
        errorMessage = "Failed to config slaves. No slaves detected!";
        // update read states of slaves.
        _readStates();
        return false;
    }

    // update read states of slaves.
    _readStates();

    return true;
}

bool SIMPLE_ETHERCAT::configMap(void)
{
    /*
    Depending on whether forceByteAlignment is set, the IOmap is configured either with byte alignment 
    (ec_config_map_aligned) or without byte alignment (ec_config_map).
    The data from the slaves is mapped to the IOmap

    In the provided code, there are two configurations for mapping the IOmap: with byte alignment 
    (ec_config_map_aligned) and without byte alignment (ec_config_map).

    With Byte Alignment:
    When IOmap is configured with byte alignment, it means that each byte of the process data is aligned to 
    byte boundaries. This ensures that each byte of data is located at a memory address that is a multiple of the byte size (typically 8 bits).
    Byte alignment ensures that data access operations are efficient and can be performed directly without 
    any byte manipulation or shifting.

    Without Byte Alignment:
    When IOmap is configured without byte alignment, the data mapping may not necessarily align to byte 
    boundaries. This means that the process data could span across byte boundaries, leading to potential 
    data fragmentation or inefficient access patterns.
    Without byte alignment, accessing specific bits or bytes within the process data might require additional 
    bitwise operations or masking to extract the desired data.
    In summary, byte alignment ensures that data within the IOmap is organized in a structured and 
    efficient manner, aligning to byte boundaries for optimal access. However, depending on the 
    specific requirements of the EtherCAT network and the connected slaves, byte alignment may or 
    may not be necessary.
    */
    if (_forceByteAlignment)
    {
    _IOmapSize = ec_config_map_aligned(&_IOmap);
    }
    else
    {
    _IOmapSize = ec_config_map(&_IOmap);
    }

    if(_IOmapSize < 1)
    {
        errorMessage = "simpleEthercat error: configMap() failed!";
        return false;
    }

    return true;
}

bool SIMPLE_ETHERCAT::configDc(void)
{
    // distributed clocks are configured using ec_configdc.
    if(!ec_configdc())
    {
        errorMessage = "simpleEthercat error: configDc() failed!";
        // update read states of slaves.
        _readStates();
        return false;
    }

    _state = EC_STATE_SAFE_OP;

    // update read states of slaves.
    _readStates();

    return true;
}

void SIMPLE_ETHERCAT::listSlaves(void)
{
    _readStates();
    for (int cnt = 1; cnt <= ec_slavecount; cnt++) 
    {
        std::string str_state;
        str_state = _slaveStateNum2Str(ec_slave[cnt].state);

        printf("\nSlave:%2d Name:%s\t RXsize: %3dbytes, TXsize: %3dbytes\t State: %8s\t Delay: %8d[ns]\t Has DC: %1d\n",
                cnt, ec_slave[cnt].name, ec_slave[cnt].Obits/8, ec_slave[cnt].Ibits/8,
                str_state.c_str(), ec_slave[cnt].pdelay, ec_slave[cnt].hasdc);
    }
}

bool SIMPLE_ETHERCAT::setOperationalState(void)
{
    ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
    ec_readstate();
    /*
    By setting ec_slave[0].state to EC_STATE_OPERATIONAL, the code is indicating that the 
    first EtherCAT slave is ready to operate and exchange data with the master. This typically 
    happens after the configuration phase and before the actual data exchange starts in an EtherCAT network.
    */
    ec_slave[0].state = EC_STATE_OPERATIONAL;
    /* send one valid process data to make outputs in slaves happy*/
    /*
    ec_send_processdata(): 
    This function sends process data to all EtherCAT slaves in the network. 
    Process data consists of input and output data exchanged between the master and slaves.

    ec_receive_processdata(EC_TIMEOUTRET): 
    This function receives process data from all EtherCAT slaves in the network. 
    It waits for a specified timeout (EC_TIMEOUTRET) for the data to be received. 
    If data is not received within the timeout period, the function may return an error or timeout status.

    Together, these two functions facilitate the exchange of process data between the master and slaves 
    in an EtherCAT network. This exchange typically occurs cyclically and is essential for real-time 
    communication and control in industrial automation applications.
    */
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    /* request OP state for all slaves */
    /*
    The code requests the operational state for all slaves by setting their state 
    to EC_STATE_OPERATIONAL using ec_writestate.
    */
    /*
    EC_STATE_INIT (0): 
    This state represents the initialization state. When the slaves are in this state, 
    they are typically not operational, and initialization tasks such as configuration and setup are performed.

    EC_STATE_PRE_OP (1): 
    This state represents the pre-operational state. In this state, the slaves are initialized 
    and configured, but they are not actively participating in the real-time control loop. 
    This state allows for additional setup and testing before transitioning to full operation.

    EC_STATE_SAFE_OP (2): 
    This state represents the safe operational state. In this state, the slaves are operational 
    and ready to participate in the real-time control loop. However, certain safety features or 
    restrictions may still be active to ensure safe operation.

    EC_STATE_OPERATIONAL (8): 
    This state represents the fully operational state. In this state, the slaves are actively 
    participating in the real-time control loop and performing their designated tasks.

    EC_STATE_BOOT (0x20): 
    This state represents the boot state. It typically indicates that the slaves are starting up or rebooting.

    EC_STATE_BOOTSTRAP (0x40): 
    This state represents the bootstrap state. It typically indicates that the slaves are 
    undergoing bootstrap initialization.
    */
    ec_writestate(0);
    int chk = 200;
    /* wait for all slaves to reach OP state */
    do
    {
        ec_send_processdata();
        ec_receive_processdata(EC_TIMEOUTRET);
        /*
        It ensures that all slaves transition to the operational state by repeatedly 
        checking their state with ec_statecheck. Once all slaves reach the operational state, 
        the code continues with the cyclic data exchange loop.
        */
        ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
    }
    while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

    ec_readstate();

    if(ec_slave[0].state != EC_STATE_OPERATIONAL)
    {
        errorMessage = "Slaves state can not set to operational state.";
        return FALSE;
    }
        

    _state = EC_STATE_OPERATIONAL;

    return true;
}

void SIMPLE_ETHERCAT::setInitState(void)
{
    ec_statecheck(0, EC_STATE_INIT, 50000);
    ec_readstate();

    ec_slave[0].state = EC_STATE_INIT;

    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    
    ec_writestate(0);

    int chk = 200;
    do
    {
        ec_send_processdata();
        ec_receive_processdata(EC_TIMEOUTRET);
        ec_statecheck(0, EC_STATE_INIT, 50000);
    }
    while (chk-- && (ec_slave[0].state != EC_STATE_INIT));

    _state = EC_STATE_INIT;
}

bool SIMPLE_ETHERCAT::setPreOperationalState(void)
{
    ec_statecheck(0, EC_STATE_PRE_OP, 50000);
    ec_readstate();

    ec_slave[0].state = EC_STATE_PRE_OP;

    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    
    ec_writestate(0);

    int chk = 200;
    do
    {
        ec_send_processdata();
        ec_receive_processdata(EC_TIMEOUTRET);
        ec_statecheck(0, EC_STATE_PRE_OP, 50000);
    }
    while (chk-- && (ec_slave[0].state != EC_STATE_PRE_OP));

    _state = EC_STATE_PRE_OP;

    return true;
}

bool SIMPLE_ETHERCAT::setSafeOperationalState(void)
{
    bool flag = false;
    // Wait for all slaves to reach SAFE_OP state
    ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 3);

    ec_readstate();

    /* wait for all slaves to reach SAFE_OP state */
    /*
    The function ec_statecheck is used to transition all slaves to the SAFE_OP (safe operational) state.
    It waits for all slaves to reach the SAFE_OP state within a specified timeout.
    */
    /*
    Here's a breakdown of its parameters and functionality:
    ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

    Parameters:
    0: 
    This parameter specifies the group index of the EtherCAT slaves to be checked. 
    In this case, 0 typically refers to the default group containing all slaves.
    EC_STATE_SAFE_OP: 
    This parameter specifies the target operational state to check for. 
    In this case, it's EC_STATE_SAFE_OP, which indicates the "Safe Operation" state.
    EC_TIMEOUTSTATE * 4: 
    This parameter sets the timeout duration for the state check. 
    EC_TIMEOUTSTATE is a predefined constant representing the default timeout value for state checks. 
    Multiplying it by 4 extends the timeout duration.
    */
    flag = (ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE * 4) == EC_STATE_SAFE_OP);

    /*
    expectedWKC represents the expected total number of working counters for both output 
    and input process data frames in the EtherCAT network, and it is used for monitoring and 
    synchronization purposes within the network.
    */
    _expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;

    if(flag == TRUE)
    {
        _state = EC_STATE_SAFE_OP;
    }
    else
    {
        errorMessage = "Slave state can not set to SafeOperation state.";
    }

    return flag;
}

void SIMPLE_ETHERCAT::close(void)
{   
    /* stop SOEM, close socket */
    /*
    Finally, regardless of the outcome, the EtherCAT connection is closed using ec_close.
    */
    ec_close();
    _joinThreadErrorCheck();
}

void SIMPLE_ETHERCAT::_readStates(void)
{
    ec_readstate();
}

int SIMPLE_ETHERCAT::getState(void) 
{
    _readStates();
    return ec_slave[1].state;
}

int SIMPLE_ETHERCAT::getState(uint16_t slave_id) 
{
    _readStates();
    return ec_slave[slave_id].state;
}

void SIMPLE_ETHERCAT::showStates(void)
{
    _readStates();
    for(int i = 1; i<=ec_slavecount ; i++)
    {
        std::string str_state;
        str_state = _slaveStateNum2Str(ec_slave[i].state);
        
        printf("Slave %2d, State=%8s, StatusCode=0x%4.4x : %s\n",
            i, str_state.c_str(), ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));

    }
}

bool SIMPLE_ETHERCAT::isAllStatesOPT(void)
{
    _readStates();
    
    for(int i = 1; i<=ec_slavecount ; i++)
    {
        if(ec_slave[i].state != EC_STATE_OPERATIONAL)
        {
            errorMessage = "Not all slaves reached operational state.";
            return false;
        } 
    }
    return true;
}

OSAL_THREAD_FUNC SIMPLE_ETHERCAT::_ecatcheck(/*void* ptr*/)
{
    int slave;
    //(void)ptr;                  /* Not used */

   /*
   continuously monitor the state of EtherCAT slaves.
   */
    while(1)
    {
         /*
         State Monitoring: 
         Inside the loop, it checks if the system is in operational mode (inOP) and if there are any 
         issues detected (wkc < expectedWKC or ec_group[currentgroup].docheckstate).
         */
        if( (_state == EC_STATE_OPERATIONAL) && ((_wkc < _expectedWKC) || ec_group[_currentgroup].docheckstate))
        {
            if (_needlf)
            {
               _needlf = FALSE;
               printf("\n");
            }
            /* one ore more slaves are not responding */
            ec_group[_currentgroup].docheckstate = FALSE;
            ec_readstate();

            /*
            Error Handling:
            If any slave is not in the operational state, it attempts to rectify the situation:
            If a slave is in SAFE_OP + ERROR state, it attempts to acknowledge the error by transitioning it 
            to SAFE_OP + ACK state.
            If a slave is in SAFE_OP state, it tries to transition it to the operational state.
            If a slave is in a state greater than EC_STATE_NONE, it reconfigures the slave.
            If a slave is not lost but still not in the operational state, it rechecks the state.
            If a slave is lost (its state is EC_STATE_NONE), it attempts to recover it.
            */
            for (slave = 1; slave <= ec_slavecount; slave++)
            {
               if ((ec_slave[slave].group == _currentgroup) && (ec_slave[slave].state != EC_STATE_OPERATIONAL))
               {
                  ec_group[_currentgroup].docheckstate = TRUE;
                  if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
                  {
                     printf("ERROR : slave %d is in SAFE_OP + ERROR, attempting ack.\n", slave);
                     ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state == EC_STATE_SAFE_OP)
                  {
                     printf("WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
                     ec_slave[slave].state = EC_STATE_OPERATIONAL;
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state > EC_STATE_NONE)
                  {
                     if (ec_reconfig_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d reconfigured\n",slave);
                     }
                  }
                  else if(!ec_slave[slave].islost)
                  {
                     /* re-check state */
                     ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                     if (ec_slave[slave].state == EC_STATE_NONE)
                     {
                        ec_slave[slave].islost = TRUE;
                        printf("ERROR : slave %d lost\n",slave);
                     }
                  }
               }
               if (ec_slave[slave].islost)
               {
                  if(ec_slave[slave].state == EC_STATE_NONE)
                  {
                     if (ec_recover_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d recovered\n",slave);
                     }
                  }
                  else
                  {
                     ec_slave[slave].islost = FALSE;
                     printf("MESSAGE : slave %d found\n",slave);
                  }
               }
            }
            if(!ec_group[_currentgroup].docheckstate)
               printf("OK : all slaves resumed OPERATIONAL.\n");
        }
        /*
        Sleep: The function sleeps for a short duration (osal_usleep(10000)) before the next 
        iteration of the loop to avoid busy waiting and reduce CPU usage.
        */
        osal_usleep(10000);
    }
}

void SIMPLE_ETHERCAT::_joinThreadErrorCheck(void)
{
    if(_thread_errorCheck.joinable())
    {
        _thread_errorCheck.join();
    }
}

int SIMPLE_ETHERCAT::readSDO(uint16 slave_num, uint16 index, uint8 subindex, int size, void *buffer)
{
    int wkc;
    wkc = ec_SDOread(slave_num, index, subindex, FALSE, &size, buffer, EC_TIMEOUTRXM);
    return wkc;
}

int SIMPLE_ETHERCAT::writeSDO(uint16 slave_num, uint16 index, uint8 subindex, int size, void *buffer)
{
    int wkc;
    wkc = ec_SDOwrite(slave_num, index, subindex, FALSE, size, buffer, EC_TIMEOUTRXM);
    return wkc;
}

int SIMPLE_ETHERCAT::writeSDO(uint16 slave_num, uint16 index, uint8 subindex, int size, uint8_t buffer)
{
    int wkc;
    wkc = ec_SDOwrite(slave_num, index, subindex, FALSE, size, &buffer, EC_TIMEOUTRXM);
    return wkc;
}

std::string SIMPLE_ETHERCAT::_slaveStateNum2Str(int num_state)
{
    std::string str_state;

    switch(num_state)
    {
        case EC_STATE_BOOT:
            str_state = "Boot";
        break;
        case EC_STATE_INIT:
            str_state = "INIT";
        break;
        case EC_STATE_PRE_OP:
            str_state = "PRE_OP";
        break;
        case EC_STATE_SAFE_OP:
            str_state = "SAFE_OP";
        break;
        case EC_STATE_OPERATIONAL:
            str_state = "OP";
        break;
        default:
            str_state = "NONE";
    }

    return  str_state;
}

bool SIMPLE_ETHERCAT::updateProccess(void)
{
    ec_send_processdata();
    int wkc = ec_receive_processdata(EC_TIMEOUTRET);
    
    if(wkc < _expectedWKC)
        return FALSE;
    
    return TRUE;
}