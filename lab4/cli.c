/* ****************************************************************************
This code was adapted from:

"Programming Embedded Systems: With C and GNU Development Tools"
 By Michael Barr, Anthony Massa

"O'Reilly Media, Inc."   (c) 2007, 1999

**************************************************************************** */

#include "includes.h"
#include <string.h>

#define MAX_COMMAND_LEN             (10)
#define MAX_PARAMETER_LEN           (10)
#define COMMAND_TABLE_SIZE          (7)

#define COMMAND				0
#define PARAM1				1
#define PARAM2				2

//#define FALSE 0
//#define TRUE  1


int cliBuildCommand(char nextChar);
void cliProcessCommand(void);
void commandHelp(void);
void commandRate(void);
void commandStopAll(void);
void SerialPrintFooterInfo(void);


extern OS_EVENT     *SerialTxMBox;
extern OS_EVENT     *SR04MBox;

char gCommandBuffer[MAX_COMMAND_LEN + 1];
char gParam1Buffer[MAX_PARAMETER_LEN + 1];
char gParam2Buffer[MAX_PARAMETER_LEN + 1];
long gParam1Value;
long gParam2Value;

#if 1
uint8_t CmdIndx; //index for command buffer
uint8_t Parm1Indx; //index for parameter buffer
uint8_t Parm2Indx; //index for parameter buffer
uint8_t CLIstate;
#endif


typedef struct {
  char const    *name;
  void          (*function)(void);
} command_t;



command_t const gCommandTable[COMMAND_TABLE_SIZE] = {
  {"H",       commandHelp, },
  {"HELP",    commandHelp, },

  {"R",       commandRate, },
  {"RATE",    commandRate, },
  
  {"X",       commandStopAll, },
  {"STOPALL", commandStopAll, },  

  {NULL,      NULL }
};



/**********************************************************************
 * Function:    cliBuildCommand
 * Description: Put received characters into the command buffer or the
 *              parameter buffer. Once a complete command is received
 *              return true.
 * Notes:       
 * Returns:     true if a command is complete, otherwise false.
 **********************************************************************/
int cliBuildCommand(char nextChar) {
#if 0
static uint8_t CmdIndx = 0; //index for command buffer
static uint8_t Parm1Indx = 0; //index for parameter buffer
static uint8_t Parm2Indx = 0; //index for parameter buffer
static uint8_t CLIstate = COMMAND;
#endif

  if ((nextChar == '\n') || (nextChar == ' ') || (nextChar == '\t') || (nextChar == '\r'))  /* Don't store any new line characters or spaces. */
    return FALSE;

  if (nextChar == ';') { /* The completed command has been received. */
    gCommandBuffer[CmdIndx] = '\0';  /* Replace the final carriage return character with a NULL character to help with processing the command */
    gParam1Buffer[Parm1Indx] = '\0';
    gParam2Buffer[Parm2Indx] = '\0';
    CmdIndx = 0; Parm1Indx = 0; Parm2Indx = 0;
    CLIstate = COMMAND;
    return TRUE;
  }

  if (nextChar == ',') {
    if (CLIstate == COMMAND){
      CLIstate = PARAM1;
      return FALSE;
    }
    else if (CLIstate == PARAM1){
      CLIstate = PARAM2;
      return FALSE;
    }
  }

  if (CLIstate == COMMAND) {
    gCommandBuffer[CmdIndx] = toupper(nextChar);     /* Convert the incoming character to upper case. This matches the case of commands in the */
    CmdIndx++;                                        /* command table. Then store the received character in the command buffer. */
    if (CmdIndx > MAX_COMMAND_LEN) {  /* If the command is too long, reset the index and process the current command buffer. */
      CmdIndx = 0;
      return TRUE;
    }
  }
  if (CLIstate == PARAM1) {
    gParam1Buffer[Parm1Indx] = nextChar;    /* Store the received character in the parameter buffer. */
    Parm1Indx++;
    if (Parm1Indx > MAX_PARAMETER_LEN) {     /* If the command is too long, reset the index and process the current parameter buffer. */
      Parm1Indx = 0;
      return TRUE;
    }
  }
  if (CLIstate == PARAM2) {
    gParam2Buffer[Parm2Indx] = nextChar;    /* Store the received character in the parameter buffer. */
    Parm2Indx++;
    if (Parm2Indx > MAX_PARAMETER_LEN) {     /* If the command is too long, reset the index and process the current parameter buffer. */
      Parm2Indx = 0;
      return TRUE;
    }
  }
  return FALSE;
}

/**********************************************************************
 * Function:    cliProcessCommand
 * Description: Look up the command in the command table. If the
 *              command is found, call the command's function. If the
 *              command is not found, output an error message.
 * Notes:       
 * Returns:     None.
 **********************************************************************/
void cliProcessCommand(void)
{
  int bCommandFound = FALSE;
  int idx;
  INT8U  err;
  char  TextMessage[24];
    
  gParam1Value = strtol(gParam1Buffer, NULL, 0);  /* Convert the parameter to an integer value.  If the parameter is empty, gParamValue becomes 0. */
  gParam2Value = strtol(gParam2Buffer, NULL, 0);  /* Convert the parameter to an integer value.  If the parameter is empty, gParamValue becomes 0. */
  for (idx = 0; gCommandTable[idx].name != NULL; idx++) {        /* Search for the command in the command table until it is found or */
    if (strcmp(gCommandTable[idx].name, gCommandBuffer) == 0) {  /* the end of the table is reached. If the command is found, break */
      bCommandFound = TRUE;                                      /* out of the loop. */
      break;
    }
  }

#if 1
  if (bCommandFound == TRUE)  /* If the command was found, call the command function. Otherwise, output an error message. */
  {
    strcpy(TextMessage, "\n");
	err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
    (*gCommandTable[idx].function)();
  }
  else 
  {
    strcpy(TextMessage, "\n");
	err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
	strcpy(TextMessage, "Command not found.\n");
	err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
  }
#endif

}

/**********************************************************************
 * Function:    commandHelp
 * Description: Help command function.
 * Notes:       
 * Returns:     None.
 **********************************************************************/
void commandHelp(void){
  int idx;
  INT8U  err;
  char  TextMessage[24];
    
    strcpy(TextMessage, "-------------------\n");
    err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
    strcpy(TextMessage, "Available commands:\n\n");
    err = OSMboxPost(SerialTxMBox, (void *) TextMessage);

  for (idx = 0; gCommandTable[idx].name != NULL; idx++) {   /* Loop through each command in the table and send out the command  */
    strcpy(TextMessage, "   ");
    err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
    strcpy(TextMessage, gCommandTable[idx].name);           /* name to the serial port. */
    err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
    idx++;
    strcpy(TextMessage, ": ");
    err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
    strcpy(TextMessage, gCommandTable[idx].name);
    err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
    strcpy(TextMessage, "\n");
    err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
  }
  SerialPrintFooterInfo();
  strcpy(TextMessage, "-------------------\n");
  err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
}



void commandRate(void)
{
	char RcmdChar;
	INT8U err;
	
	RcmdChar = 'r';
	err = OSMboxPost(SR04MBox, (void *) RcmdChar);
}

void commandStopAll(void)
{
	char XcmdChar;
	INT8U err;

	XcmdChar = 'x';
	err = OSMboxPost(SR04MBox, (void *) XcmdChar);
}

void SerialPrintFooterInfo(void)
{
  INT8U  err;
  char  TextMessage[34];

  strcpy(TextMessage, "\n");
  err = OSMboxPost(SerialTxMBox, (void *) TextMessage);

  strcpy(TextMessage, "Press ; to terminate a command\n");
  err = OSMboxPost(SerialTxMBox, (void *) TextMessage);
}
