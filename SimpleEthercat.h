#ifndef _SIMPLEETHERCAT_H
#define _SIMPLEETHERCAT_H

#include <iostream>        // standard I/O operations
#include "ethercat.h"
#include <thread>

class SIMPLE_ETHERCAT
{
public:
    // Initial ethercat port. find and auto-config slaves.
    // Return true if successed.
    bool init(const char* port_name);

    // find and auto-config slaves.
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
    int writeSDO(uint16 slave_num, uint16 index, uint8 subindex, int size, uint8_t buffer);

    // Return number of slaves that detected.
    int getSlaveCount(void) {return slaveCount;};
    
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

    int32_t getExpectedWKC(void) {return expectedWKC;}
    
    // Print last error accured in comminication.
    void printError(void);

    // Close ethercat port.
    void close(void);
    
private:
    /* This array represents the input/output (I/O) map used for EtherCAT communication. 
    It provides a memory area where data exchanged with the EtherCAT slaves is mapped.*/
    /* The value 4096 is suitable for most applications.*/
    /* Hint: Not all cells in the array are sent to the slaves; only those that are needed.*/
    /* Array for Input/Output mapping.*/
    char IOmap[4096];  

    int IOmapSize;

    // Number of slave that detect on ethertcat port.
    int slaveCount;

    bool forceByteAlignment = TRUE;
    int expectedWKC;

    // Indicate current state of ethercat slaves.
    int state = EC_STATE_NONE;

    /*
    This volatile integer variable holds the actual working counter value for the current EtherCAT communication cycle. 
    It is updated dynamically during communication to track the amount of process data received from the slaves.
    */
    volatile int wkc;

    /*
    This variable holds the index of the current group of EtherCAT slaves being processed. 
    It is used in conjunction with the ec_group array to manage communication with multiple groups of slaves.
    */
    uint8 currentgroup = 0;

    /*
    This boolean variable indicates whether a line feed (LF) character is needed for printing output. 
    It helps in formatting the output by determining when to add new lines.
    */
    boolean needlf = FALSE;

    // thread for ethercat error handling.
    std::thread thread_errorCheck;

    // Error messgage string that is last error event on simpleEthercat oject.
    std::string errorMessage;

    // Reteurn last error accured for simpleEthercat object.
    std::string getError(void);

    // Refresh states of all slaves state and read them.
    void readStates(void);

    // thread function for ethercat error handling.
    OSAL_THREAD_FUNC ecatcheck();

    // Attach thread for function ethercat error handling. 
    void joinThreadErrorCheck(void);

    std::string slaveStateNum2Str(int slave_num);

};

#endif