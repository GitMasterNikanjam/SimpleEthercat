// For compile: g++ -o slaveinfo slaveinfo.cpp -lsoem
// For run: sudo ./slaveinfo enp2s0

/** \file
 * \brief Example code for Simple Open EtherCAT master
 *
 * Usage : slaveinfo [ifname] [-sdo] [-map]
 * Ifname is NIC interface, f.e. eth0.
 * Optional -sdo to display CoE object dictionary.
 * Optional -map to display slave PDO mapping
 *
 * This shows the configured slave data.
 *
 * (c)Arthur Ketels 2010 - 2011
 */
 /*
 It provides functionalities to gather information about EtherCAT slaves connected to the network 
 and their configuration.
 */

// ###############################################################
// Header Includes:

#include <iostream>         // the standard input-output library, which provides functions like printf() and scanf()
#include <string.h>         // string manipulation library, which provides functions like strcpy() and strcat()
#include <inttypes.h>       // handling integer types with specific widths, such as int8_t and uint64_t
#include <unistd.h> // Include this for usleep()
#include "ethercat.h"       // Ethernet communication (EtherCAT)

using namespace std;
// ###############################################################
// Global Variables

/* This array represents the input/output (I/O) map used for EtherCAT communication. 
It provides a memory area where data exchanged with the EtherCAT slaves is mapped.*/
/* The value 4096 is suitable for most applications.*/
/* Hint: Not all cells in the array are sent to the slaves; only those that are needed.*/
/* Array for Input/Output mapping.*/
char IOmap[4096];

/*Structures for EtherCAT object dictionary.*/
ec_ODlistt ODlist;
ec_OElistt OElist;

/*Boolean flags for printing SDO and PDO mapping.*/
boolean printSDO = FALSE;
boolean printMAP = FALSE;

/*a buffer or array used to store data received from an EtherCAT slave device 
during an SDO (Service Data Object) read operation.
*/
char usdo[128];

/*
Type Definitions (OTYPE_VAR, OTYPE_ARRAY, OTYPE_RECORD): These constants likely represent different 
data types within the software.
*/
#define OTYPE_VAR               0x0007      // could represent a simple variable.
#define OTYPE_ARRAY             0x0008      // might represent an array data structure.
#define OTYPE_RECORD            0x0009      // ould represent a record or struct, which is a composite data type that groups together variables under one name.

/*
Attribute Definitions (ATYPE_Rpre, ATYPE_Rsafe, ATYPE_Rop, ATYPE_Wpre, ATYPE_Wsafe, ATYPE_Wop): 
These constants represent different attributes that can be associated with the types defined above. 
It seems like they're used to describe the accessibility or permissions associated with reading and 
writing these types of data.
*/
/*
ATYPE_Rpre, ATYPE_Wpre: 
These could represent attributes indicating that reading or writing a variable requires 
some precondition to be met.
ATYPE_Rsafe, ATYPE_Wsafe: 
These might represent attributes indicating that reading or writing a variable can be done 
safely without causing unintended side effects.
ATYPE_Rop, ATYPE_Wop: 
These could represent attributes indicating that reading or writing a variable can be done 
with any operation, regardless of safety or preconditions.
*/
#define ATYPE_Rpre              0x01
#define ATYPE_Rsafe             0x02
#define ATYPE_Rop               0x04
#define ATYPE_Wpre              0x08
#define ATYPE_Wsafe             0x10
#define ATYPE_Wop               0x20

/*Converts EtherCAT data types to strings.*/
/*
This function, dtype2string, converts EtherCAT data types represented by a uint16 value into corresponding strings. 
The function takes two arguments:
dtype: This is the data type identifier represented as a uint16.
bitlen: This represents the bit length associated with certain string representations like "VISIBLE_STR" or "OCTET_STR". It's used for these particular data types to indicate the length of the string.
*/
/*
This function seems to be useful for debugging or logging purposes, allowing developers to easily 
convert data type identifiers into human-readable strings.
*/
char* dtype2string(uint16 dtype, uint16 bitlen)
{
    static char str[32] = { 0 };

    /*
    It defines a static character array str of size 32 to hold the resulting string.
    It switches on the dtype value and populates the str array accordingly using sprintf to 
    format the string representation based on the EtherCAT data type.
    For data types like "VISIBLE_STRING" and "OCTET_STRING", it includes the bitlen value in the 
    string to indicate the length of the string.
    For any other unrecognized data type, it formats a string indicating the hexadecimal representation 
    of dtype and includes bitlen as well.
    */
    switch(dtype)
    {
        case ECT_BOOLEAN:
            sprintf(str, "BOOLEAN");
            break;
        case ECT_INTEGER8:
            sprintf(str, "INTEGER8");
            break;
        case ECT_INTEGER16:
            sprintf(str, "INTEGER16");
            break;
        case ECT_INTEGER32:
            sprintf(str, "INTEGER32");
            break;
        case ECT_INTEGER24:
            sprintf(str, "INTEGER24");
            break;
        case ECT_INTEGER64:
            sprintf(str, "INTEGER64");
            break;
        case ECT_UNSIGNED8:
            sprintf(str, "UNSIGNED8");
            break;
        case ECT_UNSIGNED16:
            sprintf(str, "UNSIGNED16");
            break;
        case ECT_UNSIGNED32:
            sprintf(str, "UNSIGNED32");
            break;
        case ECT_UNSIGNED24:
            sprintf(str, "UNSIGNED24");
            break;
        case ECT_UNSIGNED64:
            sprintf(str, "UNSIGNED64");
            break;
        case ECT_REAL32:
            sprintf(str, "REAL32");
            break;
        case ECT_REAL64:
            sprintf(str, "REAL64");
            break;
        case ECT_BIT1:
            sprintf(str, "BIT1");
            break;
        case ECT_BIT2:
            sprintf(str, "BIT2");
            break;
        case ECT_BIT3:
            sprintf(str, "BIT3");
            break;
        case ECT_BIT4:
            sprintf(str, "BIT4");
            break;
        case ECT_BIT5:
            sprintf(str, "BIT5");
            break;
        case ECT_BIT6:
            sprintf(str, "BIT6");
            break;
        case ECT_BIT7:
            sprintf(str, "BIT7");
            break;
        case ECT_BIT8:
            sprintf(str, "BIT8");
            break;
        case ECT_VISIBLE_STRING:
            sprintf(str, "VISIBLE_STR(%d)", bitlen);
            break;
        case ECT_OCTET_STRING:
            sprintf(str, "OCTET_STR(%d)", bitlen);
            break;
        default:
            sprintf(str, "dt:0x%4.4X (%d)", dtype, bitlen);
    }
    /*
    Finally, it returns the str array containing the formatted string representation of the EtherCAT data type.
    */
    return str;
}

/*Converts EtherCAT object types to strings.*/
/*
Like dtype2string, it defines a static character array str of size 32 to hold the resulting string.
It switches on the otype value and populates the str array accordingly using sprintf to format the string 
representation based on the object type.
For known object types (OTYPE_VAR, OTYPE_ARRAY, OTYPE_RECORD), it assigns the corresponding string representation.
For any other unrecognized object type, it formats a string indicating the hexadecimal representation of otype.
*/
char* otype2string(uint16 otype)
{
    static char str[32] = { 0 };

    switch(otype)
    {
        case OTYPE_VAR:
            sprintf(str, "VAR");
            break;
        case OTYPE_ARRAY:
            sprintf(str, "ARRAY");
            break;
        case OTYPE_RECORD:
            sprintf(str, "RECORD");
            break;
        default:
            sprintf(str, "ot:0x%4.4X", otype);
    }
    return str;
}

/*Converts EtherCAT access types to strings.*/
/*
This function seems designed to generate a string representing the access permissions for an EtherCAT object. 
For example, if access is ATYPE_Rpre | ATYPE_Wsafe, the resulting string would be "RW__W_". This indicates that the object has read and write access with pre-operation safety and write operation safety.
*/
/*
It defines a static character array str of size 32 to hold the resulting string.
Using bitwise AND operations (&), it checks each access type flag (ATYPE_Rpre, ATYPE_Wpre, ATYPE_Rsafe, 
ATYPE_Wsafe, ATYPE_Rop, ATYPE_Wop) against the access parameter.
If the corresponding flag is set, it appends either "R" (for read access) or "W" (for write access) to the str array.
If the flag is not set, it appends an underscore "_" to indicate that the access type is not present.
After processing all flags, it returns the constructed string representation.
*/
/*
Here are the common states of an EtherCAT slave device:

Init: 
This is the initial state of a slave device after power-up or reset. In this state, 
the slave device is waiting to be configured by the EtherCAT master.

Pre-Operational: 
After initialization, the slave device transitions to the pre-operational state. 
In this state, the device is configured by the master, but it does not participate in real-time communication. 
It may perform self-tests, parameterization, or other setup procedures.

Safe-Operational: 
Upon successful configuration and verification, the slave device can transition to the safe-operational state. 
In this state, the device is fully configured and can exchange real-time data with the master. However, it may 
still be restricted from performing certain critical operations.

Operational: 
The operational state is the final state of a properly configured slave device. In this state, the device is 
actively participating in real-time communication, exchanging data with the master and other slave devices. 
It can perform its intended control or automation tasks.

Boot: 
Some EtherCAT slave devices may also have a boot state, which occurs immediately after power-up or reset and 
before entering the init state. In this state, the device performs internal initialization and self-checks 

before entering the init state.
*/
char* access2string(uint16 access)
{
    static char str[32] = { 0 };

    sprintf(str, "%s%s%s%s%s%s",
            ((access & ATYPE_Rpre) != 0 ? "R" : "_"),
            ((access & ATYPE_Wpre) != 0 ? "W" : "_"),
            ((access & ATYPE_Rsafe) != 0 ? "R" : "_"),
            ((access & ATYPE_Wsafe) != 0 ? "W" : "_"),
            ((access & ATYPE_Rop) != 0 ? "R" : "_"),
            ((access & ATYPE_Wop) != 0 ? "W" : "_"));
    return str;
}

/*Converts EtherCAT SDO (Service Data Object) to strings.*/
/*
This SDO2string function converts EtherCAT data read from a slave into a string representation based on its 
data type. Here's a breakdown of how it works:
It takes the slave number (slave), index, sub-index, and data type (dtype) as input parameters.
It initializes various variables for different data types such as u8 (uint8), i8 (int8), u16 (uint16), 
i16 (int16), and so on, to interpret the received data correctly.
It initializes a static buffer str of size 64 to hold the resulting string.
It clears the buffer using memset.
It reads the data from the slave using ec_SDOread function and stores it in the usdo buffer.
It checks for any EtherCAT errors. If there's an error, it returns a string representation of the error 
using ec_elist2string.
If there are no errors, it interprets the data based on its data type (dtype) using a switch statement.
For each data type, it formats the data into a string representation and stores it in the str buffer.
Finally, it returns the constructed string.
*/
char* SDO2string(uint16 slave, uint16 index, uint8 subidx, uint16 dtype)
{
   int l = sizeof(usdo) - 1, i;
   uint8 *u8;
   int8 *i8;
   uint16 *u16;
   int16 *i16;
   uint32 *u32;
   int32 *i32;
   uint64 *u64;
   int64 *i64;
   float *sr;
   double *dr;

   /*
   character array used to store formatted string representations of individual bytes read from an EtherCAT 
   slave device during an SDO (Service Data Object) read operation.
   */
   char es[32];

   memset(&usdo, 0, 128);
   /*
   &l: This parameter is a pointer to an integer variable l, which likely represents the length of 
   the data to be read. Upon completion of the read operation, this variable will hold the actual length 
   of the data read.

   &usdo: This parameter is a pointer to the buffer (usdo) where the data read from the slave device will be stored. 
   It's likely an array or a memory location allocated to store the read data.
   */
   ec_SDOread(slave, index, subidx, FALSE, &l, &usdo, EC_TIMEOUTRXM);

    /*
    EcatError is likely a variable or a flag used to indicate whether an error has occurred during the execution of 
    EtherCAT (Ethernet for Control Automation Technology) communication operations.
    In the provided code snippet, it seems that EcatError is used to check if an error occurred during the 
    execution of the ec_SDOread function, which reads data from the SDO (Service Data Object) of an EtherCAT
    slave device. If EcatError is non-zero after calling ec_SDOread, it likely means that an error occurred during the read operation.
    */
   if (EcatError)
   {
      return ec_elist2string();
   }
   else
   {
      static char str[64] = { 0 };
      switch(dtype)
      {
         case ECT_BOOLEAN:
            u8 = (uint8*) &usdo[0];
            if (*u8) sprintf(str, "TRUE");
            else sprintf(str, "FALSE");
            break;
         case ECT_INTEGER8:
            i8 = (int8*) &usdo[0];
            sprintf(str, "0x%2.2x / %d", *i8, *i8);
            break;
         case ECT_INTEGER16:
            i16 = (int16*) &usdo[0];
            sprintf(str, "0x%4.4x / %d", *i16, *i16);
            break;
         case ECT_INTEGER32:
         case ECT_INTEGER24:
            i32 = (int32*) &usdo[0];
            sprintf(str, "0x%8.8x / %d", *i32, *i32);
            break;
         case ECT_INTEGER64:
            i64 = (int64*) &usdo[0];
            sprintf(str, "0x%16.16" PRIx64 " / %" PRId64, *i64, *i64);
            break;
         case ECT_UNSIGNED8:
            u8 = (uint8*) &usdo[0];
            sprintf(str, "0x%2.2x / %u", *u8, *u8);
            break;
         case ECT_UNSIGNED16:
            u16 = (uint16*) &usdo[0];
            sprintf(str, "0x%4.4x / %u", *u16, *u16);
            break;
         case ECT_UNSIGNED32:
         case ECT_UNSIGNED24:
            u32 = (uint32*) &usdo[0];
            sprintf(str, "0x%8.8x / %u", *u32, *u32);
            break;
         case ECT_UNSIGNED64:
            u64 = (uint64*) &usdo[0];
            sprintf(str, "0x%16.16" PRIx64 " / %" PRIu64, *u64, *u64);
            break;
         case ECT_REAL32:
            sr = (float*) &usdo[0];
            sprintf(str, "%f", *sr);
            break;
         case ECT_REAL64:
            dr = (double*) &usdo[0];
            sprintf(str, "%f", *dr);
            break;
         case ECT_BIT1:
         case ECT_BIT2:
         case ECT_BIT3:
         case ECT_BIT4:
         case ECT_BIT5:
         case ECT_BIT6:
         case ECT_BIT7:
         case ECT_BIT8:
            u8 = (uint8*) &usdo[0];
            sprintf(str, "0x%x / %u", *u8, *u8);
            break;
         case ECT_VISIBLE_STRING:
            strcpy(str, "\"");
            strcat(str, usdo);
            strcat(str, "\"");
            break;
         case ECT_OCTET_STRING:
            str[0] = 0x00;
            for (i = 0 ; i < l ; i++)
            {
               sprintf(es, "0x%2.2x ", usdo[i]);
               strcat( str, es);
            }
            break;
         default:
            sprintf(str, "Unknown type");
      }
      return str;
   }
}

/** Read PDO assign structure */
/*Reads PDO (Process Data Object) assignments.*/
/*
Overall, this function is responsible for configuring PDOs for an EtherCAT slave, reading their configurations, 
and printing out relevant information about the mapped SDOs. It seems to be part of a larger system for 
configuring and managing EtherCAT communication.
*/
/*
Variable Initialization: 
It initializes various variables to hold data such as indices (idxloop, subidxloop), read data (rdat, rdat2), and work counter (wkc). It also initializes variables to keep track of byte and bit offsets (mapoffset, bitoffset), among others.

Reading PDO Assignment Information: 
It reads the PDO assignment from the slave. This includes reading the number of PDOs available and then iterating through each PDO.

Reading PDO Configuration: 
For each PDO, it reads its configuration, including the number of subindexes. Then, it iterates through each subindex to read the SDO (Service Data Object) mapped in the PDO.

Processing and Printing: 
For each SDO, it extracts information such as the bit length, object index, and subindex. It calculates absolute offsets and reads object entries from the dictionary if applicable. Finally, it prints out the information, including the data type and name of the object.

Return Value: 
It returns the total found bit length (PDO).
*/
int si_PDOassign(uint16 slave, uint16 PDOassign, int mapoffset, int bitoffset)
{
    uint16 idxloop, nidx, subidxloop, rdat, idx, subidx;
    uint8 subcnt;
    int wkc, bsize = 0, rdl;
    int32 rdat2;
    uint8 bitlen, obj_subidx;
    uint16 obj_idx;
    int abs_offset, abs_bit;

    rdl = sizeof(rdat); rdat = 0;
    /* read PDO assign subindex 0 ( = number of PDO's) */
    wkc = ec_SDOread(slave, PDOassign, 0x00, FALSE, &rdl, &rdat, EC_TIMEOUTRXM);
    rdat = etohs(rdat);
    /* positive result from slave ? */
    if ((wkc > 0) && (rdat > 0))
    {
        /* number of available sub indexes */
        nidx = rdat;
        bsize = 0;
        /* read all PDO's */
        for (idxloop = 1; idxloop <= nidx; idxloop++)
        {
            rdl = sizeof(rdat); rdat = 0;
            /* read PDO assign */
            /*
            ec_SDOread: This function is used to read data from an object dictionary (SDO - Service Data Object) 
            of an EtherCAT slave device. It's a part of the SOEM library and is used for communication over the 
            EtherCAT network.

            slave: This parameter specifies the slave device from which data is being read.
            slave is most likely a 16-bit unsigned integer (uint16_t) representing the identifier or address of 
            the EtherCAT slave device.
            */
            wkc = ec_SDOread(slave, PDOassign, (uint8)idxloop, FALSE, &rdl, &rdat, EC_TIMEOUTRXM);
            /* result is index of PDO */
            idx = etohs(rdat);
            if (idx > 0)
            {
                rdl = sizeof(subcnt); subcnt = 0;
                /* read number of subindexes of PDO */
                /*
                idx: This parameter represents the index of the object within the object dictionary of the 
                specified slave device.
                */
                wkc = ec_SDOread(slave,idx, 0x00, FALSE, &rdl, &subcnt, EC_TIMEOUTRXM);
                subidx = subcnt;
                /* for each subindex */
                for (subidxloop = 1; subidxloop <= subidx; subidxloop++)
                {
                    rdl = sizeof(rdat2); rdat2 = 0;
                    /* read SDO that is mapped in PDO */
                    /*
                    (uint8)subidxloop: This parameter represents the sub-index of the object within the object dictionary. 
                    It's likely being cast to uint8 to ensure it's passed correctly.

                    FALSE: This parameter likely indicates whether the SDO read operation should be performed asynchronously 
                    or not. Here, FALSE suggests that it's a synchronous operation.
                    
                    &rdl: This parameter is a pointer to an integer variable where the length of the data read will be stored.

                    &rdat2: This parameter is a pointer to where the data read from the object dictionary will be stored.
                    */
                    wkc = ec_SDOread(slave, idx, (uint8)subidxloop, FALSE, &rdl, &rdat2, EC_TIMEOUTRXM);
                    rdat2 = etohl(rdat2);
                    /* extract bitlength of SDO */
                    bitlen = LO_BYTE(rdat2);
                    bsize += bitlen;
                    obj_idx = (uint16)(rdat2 >> 16);
                    obj_subidx = (uint8)((rdat2 >> 8) & 0x000000ff);
                    abs_offset = mapoffset + (bitoffset / 8);
                    abs_bit = bitoffset % 8;
                    ODlist.Slave = slave;
                    ODlist.Index[0] = obj_idx;
                    OElist.Entries = 0;
                    wkc = 0;
                    /* read object entry from dictionary if not a filler (0x0000:0x00) */
                    if(obj_idx || obj_subidx)
                        wkc = ec_readOEsingle(0, obj_subidx, &ODlist, &OElist);
                    printf("  [0x%4.4X.%1d] 0x%4.4X:0x%2.2X 0x%2.2X", abs_offset, abs_bit, obj_idx, obj_subidx, bitlen);
                    if((wkc > 0) && OElist.Entries)
                    {
                        printf(" %-12s %s\n", dtype2string(OElist.DataType[obj_subidx], bitlen), OElist.Name[obj_subidx]);
                    }
                    else
                        printf("\n");
                    bitoffset += bitlen;
                };
            };
        };
    };
    /* return total found bitlength (PDO) */
    return bsize;
}

/*Maps SDO based on CoE (CANopen over EtherCAT).*/
/*
Variable Initialization: 
Various variables are initialized, including counters (nSM, iSM), synchronization manager types (tSM), 
and sizes (Tsize, outputs_bo, inputs_bo). Additionally, a flag SMt_bug_add is set to 0.

Reading SyncManager Communication Type Object Count: 
It reads the number of SyncManager Communication Type objects from the slave.

Iterating Over SyncManager Types: 
For each SyncManager type, it reads its communication type and performs specific actions based on the type.

Correcting SyncManager Type Bug: 
If SyncManager type 2 is encountered for SyncManager 2, it adjusts SMt_bug_add by 1 and prints a message about 
the workaround.

Mapping Outputs and Inputs: 
For each SyncManager type, it determines whether it corresponds to outputs or inputs. It then prints out the 
corresponding PDO mapping information and calls the si_PDOassign function to assign PDOs accordingly.

Adjusting SyncManager Type: 
If the SyncManager type is non-zero, it adds SMt_bug_add to the type.
*/
int si_map_sdo(int slave)
{
    int wkc, rdl;
    int retVal = 0;
    uint8 nSM, iSM, tSM;
    int Tsize, outputs_bo, inputs_bo;
    uint8 SMt_bug_add;

    printf("PDO mapping according to CoE :\n");
    SMt_bug_add = 0;
    outputs_bo = 0;
    inputs_bo = 0;
    rdl = sizeof(nSM); nSM = 0;
    /* read SyncManager Communication Type object count */
    wkc = ec_SDOread(slave, ECT_SDO_SMCOMMTYPE, 0x00, FALSE, &rdl, &nSM, EC_TIMEOUTRXM);
    /* positive result from slave ? */
    if ((wkc > 0) && (nSM > 2))
    {
        /* make nSM equal to number of defined SM */
        nSM--;
        /* limit to maximum number of SM defined, if true the slave can't be configured */
        if (nSM > EC_MAXSM)
            nSM = EC_MAXSM;
        /* iterate for every SM type defined */
        for (iSM = 2 ; iSM <= nSM ; iSM++)
        {
            rdl = sizeof(tSM); tSM = 0;
            /* read SyncManager Communication Type */
            wkc = ec_SDOread(slave, ECT_SDO_SMCOMMTYPE, iSM + 1, FALSE, &rdl, &tSM, EC_TIMEOUTRXM);
            if (wkc > 0)
            {
                if((iSM == 2) && (tSM == 2)) // SM2 has type 2 == mailbox out, this is a bug in the slave!
                {
                    SMt_bug_add = 1; // try to correct, this works if the types are 0 1 2 3 and should be 1 2 3 4
                    printf("Activated SM type workaround, possible incorrect mapping.\n");
                }
                if(tSM)
                    tSM += SMt_bug_add; // only add if SMt > 0

                if (tSM == 3) // outputs
                {
                    /* read the assign RXPDO */
                    printf("  SM%1d outputs\n     addr b   index: sub bitl data_type    name\n", iSM);
                    Tsize = si_PDOassign(slave, ECT_SDO_PDOASSIGN + iSM, (int)(ec_slave[slave].outputs - (uint8 *)&IOmap[0]), outputs_bo );
                    outputs_bo += Tsize;
                }
                if (tSM == 4) // inputs
                {
                    /* read the assign TXPDO */
                    printf("  SM%1d inputs\n     addr b   index: sub bitl data_type    name\n", iSM);
                    Tsize = si_PDOassign(slave, ECT_SDO_PDOASSIGN + iSM, (int)(ec_slave[slave].inputs - (uint8 *)&IOmap[0]), inputs_bo );
                    inputs_bo += Tsize;
                }
            }
        }
    }

    /* found some I/O bits ? */
    if ((outputs_bo > 0) || (inputs_bo > 0))
        retVal = 1;
    return retVal;
}

/* Maps PDO based on SII (Slave Information Interface).*/
int si_siiPDO(uint16 slave, uint8 t, int mapoffset, int bitoffset)
{
    uint16 a , w, c, e, er;
    uint8 eectl;
    uint16 obj_idx;
    uint8 obj_subidx;
    uint8 obj_name;
    uint8 obj_datatype;
    uint8 bitlen;
    int totalsize;
    ec_eepromPDOt eepPDO;
    ec_eepromPDOt *PDO;
    int abs_offset, abs_bit;
    char str_name[EC_MAXNAME + 1];

    eectl = ec_slave[slave].eep_pdi;

    totalsize = 0;
    PDO = &eepPDO;
    PDO->nPDO = 0;
    PDO->Length = 0;
    PDO->Index[1] = 0;
    for (c = 0 ; c < EC_MAXSM ; c++) PDO->SMbitsize[c] = 0;
    if (t > 1)
        t = 1;
    PDO->Startpos = ec_siifind(slave, ECT_SII_PDO + t);
    if (PDO->Startpos > 0)
    {
        a = PDO->Startpos;
        w = ec_siigetbyte(slave, a++);
        w += (ec_siigetbyte(slave, a++) << 8);
        PDO->Length = w;
        c = 1;
        /* traverse through all PDOs */
        do
        {
            PDO->nPDO++;
            PDO->Index[PDO->nPDO] = ec_siigetbyte(slave, a++);
            PDO->Index[PDO->nPDO] += (ec_siigetbyte(slave, a++) << 8);
            PDO->BitSize[PDO->nPDO] = 0;
            c++;
            /* number of entries in PDO */
            e = ec_siigetbyte(slave, a++);
            PDO->SyncM[PDO->nPDO] = ec_siigetbyte(slave, a++);
            a++;
            obj_name = ec_siigetbyte(slave, a++);
            a += 2;
            c += 2;
            if (PDO->SyncM[PDO->nPDO] < EC_MAXSM) /* active and in range SM? */
            {
                str_name[0] = 0;
                if(obj_name)
                  ec_siistring(str_name, slave, obj_name);
                if (t)
                  printf("  SM%1d RXPDO 0x%4.4X %s\n", PDO->SyncM[PDO->nPDO], PDO->Index[PDO->nPDO], str_name);
                else
                  printf("  SM%1d TXPDO 0x%4.4X %s\n", PDO->SyncM[PDO->nPDO], PDO->Index[PDO->nPDO], str_name);
                printf("     addr b   index: sub bitl data_type    name\n");
                /* read all entries defined in PDO */
                for (er = 1; er <= e; er++)
                {
                    c += 4;
                    obj_idx = ec_siigetbyte(slave, a++);
                    obj_idx += (ec_siigetbyte(slave, a++) << 8);
                    obj_subidx = ec_siigetbyte(slave, a++);
                    obj_name = ec_siigetbyte(slave, a++);
                    obj_datatype = ec_siigetbyte(slave, a++);
                    bitlen = ec_siigetbyte(slave, a++);
                    abs_offset = mapoffset + (bitoffset / 8);
                    abs_bit = bitoffset % 8;

                    PDO->BitSize[PDO->nPDO] += bitlen;
                    a += 2;

                    /* skip entry if filler (0x0000:0x00) */
                    if(obj_idx || obj_subidx)
                    {
                       str_name[0] = 0;
                       if(obj_name)
                          ec_siistring(str_name, slave, obj_name);

                       printf("  [0x%4.4X.%1d] 0x%4.4X:0x%2.2X 0x%2.2X", abs_offset, abs_bit, obj_idx, obj_subidx, bitlen);
                       printf(" %-12s %s\n", dtype2string(obj_datatype, bitlen), str_name);
                    }
                    bitoffset += bitlen;
                    totalsize += bitlen;
                }
                PDO->SMbitsize[ PDO->SyncM[PDO->nPDO] ] += PDO->BitSize[PDO->nPDO];
                c++;
            }
            else /* PDO deactivated because SM is 0xff or > EC_MAXSM */
            {
                c += 4 * e;
                a += 8 * e;
                c++;
            }
            if (PDO->nPDO >= (EC_MAXEEPDO - 1)) c = PDO->Length; /* limit number of PDO entries in buffer */
        }
        while (c < PDO->Length);
    }
    if (eectl) ec_eeprom2pdi(slave); /* if eeprom control was previously pdi then restore */
    return totalsize;
}

/*Maps SII.*/
int si_map_sii(int slave)
{
    int retVal = 0;
    int Tsize, outputs_bo, inputs_bo;

    printf("PDO mapping according to SII :\n");

    outputs_bo = 0;
    inputs_bo = 0;
    /* read the assign RXPDOs */
    Tsize = si_siiPDO(slave, 1, (int)(ec_slave[slave].outputs - (uint8*)&IOmap), outputs_bo );
    outputs_bo += Tsize;
    /* read the assign TXPDOs */
    Tsize = si_siiPDO(slave, 0, (int)(ec_slave[slave].inputs - (uint8*)&IOmap), inputs_bo );
    inputs_bo += Tsize;
    /* found some I/O bits ? */
    if ((outputs_bo > 0) || (inputs_bo > 0))
        retVal = 1;
    return retVal;
}

/*Reads CoE Object Description.*/
void si_sdo(int cnt)
{
    int i, j;

    ODlist.Entries = 0;
    memset(&ODlist, 0, sizeof(ODlist));
    if( ec_readODlist(cnt, &ODlist))
    {
        printf(" CoE Object Description found, %d entries.\n",ODlist.Entries);
        for( i = 0 ; i < ODlist.Entries ; i++)
        {
            uint8_t max_sub;
            char name[128] = { 0 };

            ec_readODdescription(i, &ODlist);
            while(EcatError) printf(" - %s\n", ec_elist2string());
            snprintf(name, sizeof(name) - 1, "\"%s\"", ODlist.Name[i]);
            if (ODlist.ObjectCode[i] == OTYPE_VAR)
            {
                printf("0x%04x      %-40s      [%s]\n", ODlist.Index[i], name,
                       otype2string(ODlist.ObjectCode[i]));
            }
            else
            {
                printf("0x%04x      %-40s      [%s  maxsub(0x%02x / %d)]\n",
                       ODlist.Index[i], name, otype2string(ODlist.ObjectCode[i]),
                       ODlist.MaxSub[i], ODlist.MaxSub[i]);
            }
            memset(&OElist, 0, sizeof(OElist));
            ec_readOE(i, &ODlist, &OElist);
            while(EcatError) printf("- %s\n", ec_elist2string());

            if(ODlist.ObjectCode[i] != OTYPE_VAR)
            {
                int l = sizeof(max_sub);
                ec_SDOread(cnt, ODlist.Index[i], 0, FALSE, &l, &max_sub, EC_TIMEOUTRXM);
            }
            else {
                max_sub = ODlist.MaxSub[i];
            }

            for( j = 0 ; j < max_sub+1 ; j++)
            {
                if ((OElist.DataType[j] > 0) && (OElist.BitLength[j] > 0))
                {
                    snprintf(name, sizeof(name) - 1, "\"%s\"", OElist.Name[j]);
                    printf("    0x%02x      %-40s      [%-16s %6s]      ", j, name,
                           dtype2string(OElist.DataType[j], OElist.BitLength[j]),
                           access2string(OElist.ObjAccess[j]));
                    if ((OElist.ObjAccess[j] & 0x0007))
                    {
                        printf("%s", SDO2string(cnt, ODlist.Index[i], j, OElist.DataType[j]));
                    }
                    printf("\n");
                }
            }
        }
    }
    else
    {
        while(EcatError) printf("%s", ec_elist2string());
    }
}

/* Main function for gathering slave information.*/
void slaveinfo(char *ifname)
{
   int cnt, i, j, nSM;
    uint16 ssigen;
    int expectedWKC;

   printf("Starting slaveinfo\n");

   /* initialise SOEM, bind socket to ifname */
   if (ec_init(ifname))
   {
      usleep(100000);
      printf("ec_init on %s succeeded.\n",ifname);
      /* find and auto-config slaves */
      /*
      the function ec_config() is used to configure the EtherCAT network.
      In your case, ec_config(FALSE, &IOmap) would likely be configuring the EtherCAT network with verbose output 
      disabled (FALSE) and using the provided IOmap object dictionary for the configuration. The IOmap is typically 
      a data structure representing the mapping of the input/output process data areas of the EtherCAT slaves.
      */
      if ( ec_config(FALSE, &IOmap) > 0 )
      {
        /*
        ec_configdc() is a function used to set up and configure distributed clocks within an EtherCAT network, 
        ensuring synchronized timing across all connected slave devices.
        */
         ec_configdc();
         while(EcatError) printf("%s", ec_elist2string());
         printf("%d slaves found and configured.\n",ec_slavecount);
         expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
         printf("Calculated workcounter %d\n", expectedWKC);
         /* wait for all slaves to reach SAFE_OP state */
         ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE * 5);
         usleep(100000);
         if (ec_slave[0].state != EC_STATE_SAFE_OP )
         {
            printf("Not all slaves reached safe operational state.\n");
            ec_readstate();
            for(i = 1; i<=ec_slavecount ; i++)
            {
               if(ec_slave[i].state != EC_STATE_SAFE_OP)
               {
                  printf("Slave %d State=%2x StatusCode=%4x : %s\n",
                     i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
               }
            }
         }


         ec_readstate();

         for( cnt = 1 ; cnt <= ec_slavecount ; cnt++)
         {
            usleep(10000); // 10 ms delay for readability
            printf("\nSlave:%d\n Name:%s\n Output size: %dbits\n Input size: %dbits\n State: %d\n Delay: %d[ns]\n Has DC: %d\n",
                  cnt, ec_slave[cnt].name, ec_slave[cnt].Obits, ec_slave[cnt].Ibits,
                  ec_slave[cnt].state, ec_slave[cnt].pdelay, ec_slave[cnt].hasdc);
            if (ec_slave[cnt].hasdc) printf(" DCParentport:%d\n", ec_slave[cnt].parentport);
            printf(" Activeports:%d.%d.%d.%d\n", (ec_slave[cnt].activeports & 0x01) > 0 ,
                                         (ec_slave[cnt].activeports & 0x02) > 0 ,
                                         (ec_slave[cnt].activeports & 0x04) > 0 ,
                                         (ec_slave[cnt].activeports & 0x08) > 0 );
            printf(" Configured address: %4.4x\n", ec_slave[cnt].configadr);
            printf(" Man: %8.8x ID: %8.8x Rev: %8.8x\n", (int)ec_slave[cnt].eep_man, (int)ec_slave[cnt].eep_id, (int)ec_slave[cnt].eep_rev);
            for(nSM = 0 ; nSM < EC_MAXSM ; nSM++)
            {
               if(ec_slave[cnt].SM[nSM].StartAddr > 0)
                  printf(" SM%1d A:%4.4x L:%4d F:%8.8x Type:%d\n",nSM, etohs(ec_slave[cnt].SM[nSM].StartAddr), etohs(ec_slave[cnt].SM[nSM].SMlength),
                         etohl(ec_slave[cnt].SM[nSM].SMflags), ec_slave[cnt].SMtype[nSM]);
            }
            for(j = 0 ; j < ec_slave[cnt].FMMUunused ; j++)
            {
               printf(" FMMU%1d Ls:%8.8x Ll:%4d Lsb:%d Leb:%d Ps:%4.4x Psb:%d Ty:%2.2x Act:%2.2x\n", j,
                       etohl(ec_slave[cnt].FMMU[j].LogStart), etohs(ec_slave[cnt].FMMU[j].LogLength), ec_slave[cnt].FMMU[j].LogStartbit,
                       ec_slave[cnt].FMMU[j].LogEndbit, etohs(ec_slave[cnt].FMMU[j].PhysStart), ec_slave[cnt].FMMU[j].PhysStartBit,
                       ec_slave[cnt].FMMU[j].FMMUtype, ec_slave[cnt].FMMU[j].FMMUactive);
            }
            printf(" FMMUfunc 0:%d 1:%d 2:%d 3:%d\n",
                     ec_slave[cnt].FMMU0func, ec_slave[cnt].FMMU1func, ec_slave[cnt].FMMU2func, ec_slave[cnt].FMMU3func);
            printf(" MBX length wr: %d rd: %d MBX protocols : %2.2x\n", ec_slave[cnt].mbx_l, ec_slave[cnt].mbx_rl, ec_slave[cnt].mbx_proto);
            ssigen = ec_siifind(cnt, ECT_SII_GENERAL);
            /* SII general section */
            if (ssigen)
            {
               ec_slave[cnt].CoEdetails = ec_siigetbyte(cnt, ssigen + 0x07);
               ec_slave[cnt].FoEdetails = ec_siigetbyte(cnt, ssigen + 0x08);
               ec_slave[cnt].EoEdetails = ec_siigetbyte(cnt, ssigen + 0x09);
               ec_slave[cnt].SoEdetails = ec_siigetbyte(cnt, ssigen + 0x0a);
               if((ec_siigetbyte(cnt, ssigen + 0x0d) & 0x02) > 0)
               {
                  ec_slave[cnt].blockLRW = 1;
                  ec_slave[0].blockLRW++;
               }
               ec_slave[cnt].Ebuscurrent = ec_siigetbyte(cnt, ssigen + 0x0e);
               ec_slave[cnt].Ebuscurrent += ec_siigetbyte(cnt, ssigen + 0x0f) << 8;
               ec_slave[0].Ebuscurrent += ec_slave[cnt].Ebuscurrent;
            }
            printf(" CoE details: %2.2x FoE details: %2.2x EoE details: %2.2x SoE details: %2.2x\n",
                    ec_slave[cnt].CoEdetails, ec_slave[cnt].FoEdetails, ec_slave[cnt].EoEdetails, ec_slave[cnt].SoEdetails);
            printf(" Ebus current: %d[mA]\n only LRD/LWR:%d\n",
                    ec_slave[cnt].Ebuscurrent, ec_slave[cnt].blockLRW);
            if ((ec_slave[cnt].mbx_proto & ECT_MBXPROT_COE) && printSDO)
                    si_sdo(cnt);
                if(printMAP)
            {
                    if (ec_slave[cnt].mbx_proto & ECT_MBXPROT_COE)
                        si_map_sdo(cnt);
                    else
                        si_map_sii(cnt);
            }
         }
      }
      else
      {
         printf("No slaves found!\n");
      }
      printf("End slaveinfo, close socket\n");
      /* stop SOEM, close socket */
      ec_close();
   }
   else
   {
      printf("No socket connection on %s\nExcecute as root\n",ifname);
   }
}

char ifbuf[1024];

/*
Main Function:
It initializes EtherCAT, configures slaves, and prints their information.
Additionally, it sets flags (printSDO and printMAP) based on command-line arguments.*/
int main(int argc, char *argv[])
{
   ec_adaptert * adapter = NULL;
   printf("SOEM (Simple Open EtherCAT Master)\nSlaveinfo\n");

   if (argc > 1)
   {
      if ((argc > 2) && (strncmp(argv[2], "-sdo", sizeof("-sdo")) == 0)) printSDO = TRUE;
      if ((argc > 2) && (strncmp(argv[2], "-map", sizeof("-map")) == 0)) printMAP = TRUE;
      /* start slaveinfo */
      strcpy(ifbuf, argv[1]);
      slaveinfo(ifbuf);
   }
   else
   {
      printf("Usage: slaveinfo ifname [options]\nifname = eth0 for example\nOptions :\n -sdo : print SDO info\n -map : print mapping\n");

      printf ("Available adapters\n");
      adapter = ec_find_adapters ();
      while (adapter != NULL)
      {
         printf ("Description : %s, Device to use for wpcap: %s\n", adapter->desc,adapter->name);
         adapter = adapter->next;
      }
      ec_free_adapters(adapter);
   }

   printf("End program\n");
   return (0);
}