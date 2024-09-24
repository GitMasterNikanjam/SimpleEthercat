#ifndef _SIMPLEETHERCAT_H
#define _SIMPLEETHERCAT_H

// ##################################################################################
// Include libraries:

#include <iostream>        // standard I/O operations
#include "ethercat.h"
#include <thread>

// ###################################################################################
// SimpleEthercat class:

class SimpleEthercat
{
public:

    // Last error message accured for object.
    std::string errorMessage;

    /**
     * @brief Initial ethercat port.   
     * initialise SOEM, bind socket to port_name.   
     * Start the thread_errorCheck.  
     * @return true if successed.
     *  */  
    bool init(const char* port_name);

    /**
     * Find and auto-config slaves.
     * Set all slave operation state to Pre operational.
     * @return true if successed.
     *  */ 
    bool configSlaves(void);

    // Print list of slaves that detected. show slave number, name, RX size,TX size, state, Pdelay and distrubution clock ability.
    void listSlaves(void);

    // set Byte Alignment for IOmap buffer.
    bool configMap(void);

    // set distrubution clock for all slaves. 
    bool configDc(void);

    // Read proccess for SDO objects dictionary.
    int readSDO(uint16 slave_num, uint16 index, uint8 subindex, int size, void *buffer);

    // Write proccess for SDO objects dictionary.
    int writeSDO(uint16 slave_num, uint16 index, uint8 subindex, int size, void *buffer);

    // Write proccess for SDO objects dictionary.
    int writeSDO(uint16 slave_num, uint16 index, uint8 subindex, int size, uint8_t buffer);

    // Return number of slaves that detected.
    int getSlaveCount(void) {return _slaveCount;};
    
    // Set init state for all slaves.
    void setInitState(void);

    // Set pre operational state for all slaves.
    bool setPreOperationalState(void);

    // Set safe operational state for all slaves. 
    bool setSafeOperationalState(void);

    // Set operational state for all slaves.
    bool setOperationalState(void);

    // Check if all detected slaves are in operational mode. return true/false.
    bool isAllStatesOPT(void);
    
    // Print current state slaves.
    void showStates(void);

    // Send and recieve proccess data (PDO) in blocking mode.
    bool updateProccess(void);

    // Return ethercat state for slave 1.
    int getState(void);

    // Return certain slave ethercat state.
    int getState(uint16_t slave_id);

    // Return expectedWKC.
    int32_t getExpectedWKC(void) {return _expectedWKC;}

    // Close ethercat port.
    void close(void);
    
private:
    /* This array represents the input/output (I/O) map used for EtherCAT communication. 
    It provides a memory area where data exchanged with the EtherCAT slaves is mapped.*/
    /* The value 4096 is suitable for most applications.*/
    /* Hint: Not all cells in the array are sent to the slaves; only those that are needed.*/
    /* Array for Input/Output mapping.*/
    char _IOmap[4096];  

    int _IOmapSize;

    // Number of slave that detect on ethertcat port.
    int _slaveCount;

    bool _forceByteAlignment = TRUE;
    int _expectedWKC;

    // Indicate current ethercat operational state of SimpleEthercat object.
    int _state = EC_STATE_NONE;

    /*
    This volatile integer variable holds the actual working counter value for the current EtherCAT communication cycle. 
    It is updated dynamically during communication to track the amount of process data received from the slaves.
    */
    volatile int _wkc;

    /*
    This variable holds the index of the current group of EtherCAT slaves being processed. 
    It is used in conjunction with the ec_group array to manage communication with multiple groups of slaves.
    */
    uint8 _currentgroup = 0;

    /*
    This boolean variable indicates whether a line feed (LF) character is needed for printing output. 
    It helps in formatting the output by determining when to add new lines.
    */
    boolean _needlf = FALSE;

    // thread for ethercat error handling.
    std::thread _thread_errorCheck;

    // Refresh states of all slaves state and read them.
    void _readStates(void);

    // thread function for ethercat error handling.
    OSAL_THREAD_FUNC _ecatcheck();

    // Attach thread for function ethercat error handling. 
    void _joinThreadErrorCheck(void);

    std::string _slaveStateNum2Str(int slave_num);

};

#endif