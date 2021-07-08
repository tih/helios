/*
    CATS_C40.C Direct, low level C40 routines for Helios v1.3x
    Copyright (C) 1993  Ken Blackler, JET Joint Undertaking

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    
    The author can be contacted by EMail as: kb@jet.uk
    or at:
    
    		Ken Blackler
    		JET Joint Undertaking
    		Abingdon
    		Oxfordshire
    		England
    		OX14 3EA
    
*/


                                                                             /*
���������������������������������������������JET Joint Undertaking����������ͻ
�                                                                            �
�                  MODULE: cats_c40.c                                        �
�                                                                            �
�                 PURPOSE: General purpose low-level C40 specific routines   �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      10-Dec-1992 K.Blackler  Original Issue                       �
�      1.01     24-Aug-1993 K.Blackler  Sorted out includes                  �
�      1.1      24-Aug-1993 K.Blackler  Implement DMA snchronization on xCRDY�
�                                                                            �
�         Without this change, when the DMA processor writes to a COM port   �
�         that has a full FIFO it stops and waits until it isn't full        �
�         NO DMA TRANSFERS TAKE PLACE ON ANY CHANNEL SO LONG AS THAT         �
�         FIFO IS FULL.                                                      �         
�         By synch'ing COM port accesses to the COM port ready lines the DMA �
�         doesn't even try and write until it gets the xCRDY interrupt.      �
�         This enables the processor to communicate via its other links and  �
�         stops Helios thinking the processor has locked up. Yuck.           �
�                                                                            �
�      1.2      24-Oct-1993 K.Blackler  Change to using Helios's MP_xxxxxxx  �
�                                       functions 'cause direct C pointers   �
�                                       don't work with my large >32Mb C40.  �
�     **** Second public release version 22/11/1993 ****                     �
���������������������������������������������������������������kb@jet.uk����ͼ */

#include "catsutil.h"
#include "cats_c40.h"

#include <stdio.h>
#include <stdlib.h>
#include <syslib.h>
#include <process.h>
#include <config.h>
#include <link.h>
#include <nonansi.h>

#if defined(_DEBUG_C40)
#define C40_SIMPLE_RW_TIMEOUT 10000
#endif


static const char ModuleName[]="CATS_C40 Module";
                                                                             /*
��������������������������������������������JET Joint Undertaking�����������Ŀ
�                                                                            �
�               PROCEDURE: MachineInfo                                       �
�                                                                            �
�                 PURPOSE: Gets the cluster and name of the current processor�
�                          pCluster and pName must be at least NameLength    �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0       5-May-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */
void MachineInfo(char *pCluster,char *pName)
{
 char pMachineName[NameMax*2+3];
 char *pString;
 MachineName(pMachineName);
 
 ASSERT(pMachineName[0]==c_dirchar);
 
 pString=pMachineName+1; /* Skip the initial '/' */
 if (pCluster!=NULL)
   {
     pString+=splitname(pCluster,c_dirchar,pString);
   }
 else
   {
     while ( (*pString!='/') && *pString )
       pString++;
   }
 if (pName!=NULL)
   {
     splitname(pName,c_dirchar,pString);
   }
}


                                                                             /*
��������������������������������������������JET Joint Undertaking�����������Ŀ
�                                                                            �
�               PROCEDURE: C40MemoryDump                                     �
�                                                                            �
�                 PURPOSE: Displays the contents of memory                   �
�                                                                            �
�                          The routine uses machine pointers to refer        �
�                          to any location in memory, the contents of        �
�                          which are displayed in both hexadecimal and       �
�                          character form.                                   �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0       3-Nov-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */
void C40MemoryDump(C40ADDRESS SourceAddress,int nWords)
{
  int i,j,k;
  union
    {
      WORD32 wData;
      char   cData[4];
    } Data;
  
  for (i=0; i<nWords; i+=5)
    {
      printf("%08lX ",SourceAddress+i);
      for (j=i; j<i+5; j++)
        {
          if (j>nWords)
            {
              printf(".. .. .. .. ");
            }
          else
            {
		          Data.wData=MP_GetWord(SourceAddress,j);
		          for (k=1; k<4; k++)
		            {
		              printf("%02x ",((int)Data.cData[k])&0xff);
		            }
		        }
		    }   
		    
      putchar('\"');  
      for (j=i; j<i+5; j++)
        {
          if (j>nWords)
            {
              printf("....");
            }
					else
					  {
		          Data.wData=MP_GetWord(SourceAddress,j);
			        for (k=1; k<4; k++)
			          {
					        (Data.cData[k]<' ') ? putchar('.') : putchar(Data.cData[k]);
			          }
					  }            
        }
      putchar('\"'); putchar('\n');
    }
}



static LinkInfo saveLinkConf[6];  /* NLinks LinkConf structs  */

                                                                             /*
��������������������������������������������JET Joint Undertaking�����������Ŀ
�                                                                            �
�               PROCEDURE: AllocateLink                                      �
�                                                                            �
�                 PURPOSE: Sets a specified link to dumb mode                �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      16-Mar-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */

BOOL AllocateLink(C40COMPORT nComPort)
{
  LinkConf newconf;

  if (nComPort>c40com5)
    {
      ModuleMessage(ModuleName,"ERROR - Attempted to Allocate and Invalid comport");
      ASSERT(FALSE);
      return(FALSE);
    }
  /* First save the current link configuration */
  LinkData(nComPort,&saveLinkConf[nComPort]);

#if defined(_DEBUG_C40)
  printf("%s\tAllocating COM port #%d",ModuleName,nComPort);
#endif
  newconf.Id = nComPort;
  newconf.Mode = Link_Mode_Dumb;
  newconf.State = Link_State_Dumb;
  if (Configure(newconf)!=0)
    {
      return(FALSE);
    }

  while (AllocLink(nComPort)==0)
    {
#if defined(_DEBUG_C40)
      printf("\\\b/\b-\b");
#else
      ;        
#endif
    }
#if defined(_DEBUG_C40)
      printf(": Done\n");
#endif      
  return(TRUE);
}
                                                                              /*
��������������������������������������������JET Joint Undertaking�����������Ŀ
�                                                                            �
�               PROCEDURE: DeAllocateLink                                    �
�                                                                            �
�                 PURPOSE: Returns a link from dump to active mode           �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      16-Mar-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */

BOOL DeAllocateLink(C40COMPORT nComPort)
{
  LinkConf oldconf;

#if defined(_DEBUG_C40)
  printf("%s\tDeAllocating COM port #%d\n",ModuleName,nComPort);
#endif

  if (FreeLink(nComPort)!=0) return(FALSE);

  /* Now restore the old link configuration */
  
  oldconf.Id =    nComPort;
  oldconf.Mode =  saveLinkConf[nComPort].Mode;
  oldconf.State = saveLinkConf[nComPort].State;
  
  if (Configure(oldconf)!=0) return(FALSE);

return(TRUE);
}

                                                                              /*
��������������������������������������������JET Joint Undertaking�����������Ŀ
�                                                                            �
�               PROCEDURE: C40GetDIE                                         �
�                                                                            �
�                 PURPOSE: Returns the DMA Interrupt Enable register         �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      16-Mar-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */
/* Helios uses R0 as the first parameter AND the return value */
static UNIFIEDDIE C40GetDIE(long dummy) /* dummy gives me a way to get at register R0 */
{
  long DIE;
                             
  _word(0x8000016);   /* LDI DIE,R0 */
  DIE=dummy;          /* Which is magically now DIE, given a favourable optimizer! */
  return *((UNIFIEDDIE *)&DIE);
}

                                                                              /*
��������������������������������������������JET Joint Undertaking�����������Ŀ
�                                                                            �
�               PROCEDURE: C40SetDIE                                         �
�                                                                            �
�                 PURPOSE: Sets the DMA Interrupt Enable register            �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      16-Mar-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */
static void C40SetDIE(UNIFIEDDIE newDIE)
{
  _word(0x8160000);   /* LDI R0,DIE */

#if defined(_DEBUG_C40)
  printf("%s - Setting DIE Register: %lx\n",ModuleName,newDIE);
#endif
}

                                                                              /*
��������������������������������������������JET Joint Undertaking�����������Ŀ
�                                                                            �
�               PROCEDURE: C40GetIIE                                         �
�                                                                            �
�                 PURPOSE: Returns the Interrupt Enable register             �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      16-Mar-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */
static IIE C40GetIIE(long dummy) /* dummy gives me a way to get at register R0 */
{
  long iie;
                             
  _word(0x8000017);   /* LDI IIE,R0 */
  iie=dummy;          /* Which is magically now IIE, given a favourable optimizer! */
  return *((IIE *)&iie);
}

                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40CreateUnifinedModeTransferExC40                �
�                                                                            �
�                 PURPOSE: Sets up a complex unified DMA transfer            �
�                          The buffer is specified as a C40ADDRESS           �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0       3-Nov-1993 K.Blackler  Modification of routine              �
�                                       C40CreateUnifinedModeTransferEx      �
�                                                                            �
������������������������������������������������������������������������������ */
void C40CreateUnifiedModeTransferExC40(int fDirection,PUNIFIEDCHANNELCONTROL pControl,C40ADDRESS pBuffer,
	                                     int nBytes,unsigned idCom, word wPriority,
	                                     BOOL bInterrupt, word wCarryOn,int nSkip)
{
  ASSERT(idCom<6);
  
  ASSERT( (nBytes & BIN_11) == 0); /* Check no one tries to send bytes..... */
  ASSERT( ( nSkip & BIN_11) == 0);
  
  pControl->TransferCounter         = nBytes/4;
  pControl->LinkPointer             = NULL;

  if (fDirection==AT_TO_COMPORT)
    { 
      pControl->SourceAddress           = pBuffer;
      pControl->SourceAddressIndex      = nSkip/4;
      pControl->DestinationAddress      = C40_COMPORT_BASEADDRESS+2 + idCom * 0x10L;
      pControl->DestinationAddressIndex = 0;  /* A single address so no inc. */
      pControl->ControlReg.bSourceSynch=FALSE;
      pControl->ControlReg.bDestinationSynch=TRUE; /* Don't write until the COM port is ready */
    }
  else /* from the COM port */
    {
      pControl->SourceAddress           = C40_COMPORT_BASEADDRESS+1 + idCom * 0x10L;
      pControl->SourceAddressIndex      = 0;  /* A single address so no inc. */
      pControl->DestinationAddress      = pBuffer; 
      pControl->DestinationAddressIndex = nSkip/4;
      pControl->ControlReg.bSourceSynch=TRUE; /* Don't read until the COM port is ready */
      pControl->ControlReg.bDestinationSynch=FALSE;
    }

  pControl->ControlReg.Priority = (int)wPriority&BIN_11;

  pControl->ControlReg.TransferMode = (int)wCarryOn&BIN_11; /* What to do after this transfer is over */

  pControl->ControlReg.bIntOnFinish=bInterrupt;


  pControl->ControlReg.bAutoInitStatic=FALSE;
  pControl->ControlReg.bAutoInitSynch=FALSE; /* Don't need this,init from memory */

  pControl->ControlReg.bReadBitReversed=FALSE;
  pControl->ControlReg.bWriteBitReversed=FALSE;

  pControl->ControlReg.bSplitMode=FALSE;
  pControl->ControlReg.bStart=DMA_START_START;
  pControl->ControlReg.bFixedPriority=FALSE;


}
                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40CreateUnifinedModeTransferEx                   �
�                                                                            �
�                 PURPOSE: Sets up a complex unified DMA transfer            �
�                          The buffer is specified as a C pointer            �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      21-Sep-1993 K.Blackler  Modification of routine              �
�                                       C40CreateUnifinedModeTransfer        �
�      1.1      24-Oct-1993 K.Blackler  Change to using MP_ fuctions         �
�      1.2       3-Nov-1993 K.Blackler  Use C40ADDRESS routine above -       �
�                                       getting deeper all the time!         �
������������������������������������������������������������������������������ */
void C40CreateUnifiedModeTransferEx(int fDirection,PUNIFIEDCHANNELCONTROL pControl,void *pBuffer,
                                    int nBytes,unsigned idCom, word wPriority,
                                    BOOL bInterrupt, word wCarryOn,int nSkip)
{
  C40CreateUnifiedModeTransferExC40(fDirection,pControl,C40WordAddress(pBuffer),
	                                  nBytes,idCom, wPriority,
	                                  bInterrupt, wCarryOn,nSkip);
}
                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40CreateUnifinedModeTransfer                     �
�                                                                            �
�                 PURPOSE: Sets up a unified DMA transfer                    �
�                          The buffer increment defaults to a word           �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      09-Mar-1993 K.Blackler  Original Issue                       �
�      1.1      24-Aug-1993 K.Blackler  Implement DMA snchronization on xCRDY�
�      1.2      21-Sep-1993 K.Blackler  Use Extended routine above           �
�                                                                            �
������������������������������������������������������������������������������ */


void C40CreateUnifiedModeTransfer(int fDirection,PUNIFIEDCHANNELCONTROL pControl,
                                  void *pBuffer,int nBytes,unsigned idCom,
                                  word wPriority, BOOL bInterrupt, word wCarryOn)
{
  C40CreateUnifiedModeTransferEx(fDirection,pControl,pBuffer,nBytes,idCom,
                                 wPriority, bInterrupt, wCarryOn,4);
}

                                                                              /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40SetUnifiedTransferLink                         �
�                                                                            �
�                 PURPOSE: Links two DMA transfers so the engine goes from   �
�                          one to the other automatically                    �
�                          A null link makes the transfer non-initiailising  �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      09-Mar-1993 K.Blackler  Original Issue                       �
�      1.1      22-Sep-1993 K.Blackler  Make a NULL link change prevent the  �
�                                       transfer from auto initialising      �
�                                                                            �
������������������������������������������������������������������������������ */

void C40SetUnifiedTransferLink(PUNIFIEDCHANNELCONTROL pControl,
                               PUNIFIEDCHANNELCONTROL pNextControl)
{
  if (pNextControl!=NULL)
    pControl->LinkPointer=C40WordAddress(pNextControl);
  else
    { /* Will simply stop after this one */
      pControl->ControlReg.TransferMode = DMA_MODE_STOPNOAUTOINIT; 
      pControl->LinkPointer=C40WordAddress(NULL);
    }
}

                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40StartLinkedTransfers                           �
�                                                                            �
�                 PURPOSE: Starts the linked list of transfers               �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      09-Mar-1993 K.Blackler  Original Issue                       �
�      1.1      24-Aug-1993 K.Blackler  Implement DMA snchronization on xCRDY�
�                                                                            �
������������������������������������������������������������������������������ */


void C40StartLinkedTransfers(C40COMPORT DMAChanNo,
                             PUNIFIEDCHANNELCONTROL pLinkedControlList)
{
  union
    {
      UNIFIEDDMACONTROLREG DMAcontrolReg;
      WORD32 Overlay;
    } ControlRegister;

  UNIFIEDDIE DIE;

  ASSERT (DMAChanNo >=0 && DMAChanNo<=5);

  ControlRegister.Overlay=MP_GetWord(C40_DMA_BASEADDRESS, (word)DMAChanNo * 0x10);

  /* First set the control Register so that this DMA is stopped */
  ControlRegister.DMAcontrolReg.bStart         = DMA_START_RESET;
                           
  MP_PutWord(C40_DMA_BASEADDRESS, (word)DMAChanNo * 0x10, ControlRegister.Overlay);

  /* Now set it up to a good starting point so it settles down.... */

  ControlRegister.DMAcontrolReg.bSplitMode        = FALSE;
  ControlRegister.DMAcontrolReg.bFixedPriority    = FALSE;
  ControlRegister.DMAcontrolReg.Priority          = DMA_PRIORITY_TODMA;  /*Get it finished*/

  ControlRegister.DMAcontrolReg.TransferMode      = DMA_MODE_STOPNOAUTOINIT;
  ControlRegister.DMAcontrolReg.bSourceSynch      = TRUE; /* Just to prevent any lock ups*/
  ControlRegister.DMAcontrolReg.bDestinationSynch = TRUE;
  ControlRegister.DMAcontrolReg.bAutoInitStatic   = FALSE;
  ControlRegister.DMAcontrolReg.bAutoInitSynch    = FALSE;
  ControlRegister.DMAcontrolReg.bWriteBitReversed = FALSE;
  ControlRegister.DMAcontrolReg.bReadBitReversed  = FALSE;
  ControlRegister.DMAcontrolReg.bIntOnFinish      = FALSE;
  ControlRegister.DMAcontrolReg.sTransferFinished = 0;
  ControlRegister.DMAcontrolReg.bStart            = DMA_START_RESET;
  ControlRegister.DMAcontrolReg.sStatus           = 0;

  MP_PutWord(C40_DMA_BASEADDRESS, (word)DMAChanNo * 0x10, ControlRegister.Overlay);

  DIE=C40GetDIE(0);
  
  switch(DMAChanNo) /* Enable the DMA (non-vectored) xCRDY interrupt. Note that */
    {               /* I do both here, 'cause I don't want to have to check the */
      case 0:       /* direction of all the transfers. Don't see any problems...*/
        DIE.DMA0Write=UDIE_ENABLE_COMPORTREADY;
        DIE.DMA0Read=UDIE_ENABLE_COMPORTREADY;
        break;
      case 1:
        DIE.DMA1Write=UDIE_ENABLE_COMPORTREADY;
        DIE.DMA1Read=UDIE_ENABLE_COMPORTREADY;
        break;
      case 2:
        DIE.DMA2Write=UDIE_ENABLE_COMPORTREADY;
        DIE.DMA2Read=UDIE_ENABLE_COMPORTREADY;
        break;
      case 3:
        DIE.DMA3Write=UDIE_ENABLE_COMPORTREADY;
        DIE.DMA3Read=UDIE_ENABLE_COMPORTREADY;
        break;
      case 4:
        DIE.DMA4Write=UDIE_ENABLE_COMPORTREADY;
        DIE.DMA4Read=UDIE_ENABLE_COMPORTREADY;
        break;
      case 5:
        DIE.DMA5Write=UDIE_ENABLE_COMPORTREADY;
        DIE.DMA5Read=UDIE_ENABLE_COMPORTREADY;
        break;
    }

  C40SetDIE(DIE); /* Enable the DMA interrupts now */
  

  /* Now 'manually' enter all the values for the first transfer */
  ControlRegister.DMAcontrolReg=pLinkedControlList[0].ControlReg;
  
  /*
  1: pLinkedControlList[0].SourceAddress;
  2: pLinkedControlList[0].SourceAddressIndex;
  3: pLinkedControlList[0].TransferCounter;
  4: pLinkedControlList[0].DestinationAddress;
  5: pLinkedControlList[0].DestinationAddressIndex;
  6: pLinkedControlList[0].LinkPointer;
  */
  MP_PutData(C40_DMA_BASEADDRESS+1, (word)DMAChanNo * 0x10, &(pLinkedControlList[0].SourceAddress),6);

  /* Start the transfer */
  ControlRegister.DMAcontrolReg.bStart = DMA_START_START;

  MP_PutWord(C40_DMA_BASEADDRESS, (word)DMAChanNo * 0x10, ControlRegister.Overlay); /* And here they are started */
  return;
}
                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40StopLinkedTransfers                            �
�                                                                            �
�                 PURPOSE: Stops the DMA engine dead                         �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      09-Mar-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */

void C40StopLinkedTransfers(C40COMPORT DMAChanNo)
{
  union
    {
      UNIFIEDDMACONTROLREG DMAcontrolReg;
      WORD32 Overlay;
    } ControlRegister;
  
  ASSERT (DMAChanNo >=0 && DMAChanNo<=5);
      
  ControlRegister.Overlay=MP_GetWord(C40_DMA_BASEADDRESS, (word)DMAChanNo * 0x10);
  
  /* Now set the control Register so that this DMA is stopped */
  ControlRegister.DMAcontrolReg.bStart         = DMA_START_RESET;

  MP_PutWord(C40_DMA_BASEADDRESS, (word)DMAChanNo * 0x10,ControlRegister.Overlay);

  return;
}

                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40IsDMABusy                                      �
�                                                                            �
�                 PURPOSE: Checks to see if the DMA channel is busy          �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      13-Mar-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */

word C40IsDMABusy(int DMAChanNo)
{
  WORD32 Counter;
  
  ASSERT (DMAChanNo>=0 && DMAChanNo<=5);
  Counter=MP_GetWord(C40_DMA_BASEADDRESS+3, (word)DMAChanNo * 0x10);

  return(Counter);
}
                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40IsThereSpaceToWrite                            �
�                                                                            �
�                 PURPOSE: Checks to see if the comport has space in its     �
�                          output FIFO so a simple write doesn't hang the    �
�                          processor until the FIFO empties...               �
�                          Useful for a low level, non-blocking message      �
�                          protocol between the C40 and external hardware.   �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      17-Aug-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */

int C40IsThereSpaceToWrite(int nComPort)
{
  union
    {
      COMPORTCONTROLREG ControlReg;
      WORD32 Overlay;
    } ControlRegister;

  ASSERT (nComPort>=0 && nComPort<=5);

  ControlRegister.Overlay=MP_GetWord(C40_COMPORT_BASEADDRESS, (word)nComPort * 0x10);
  
  return (ControlRegister.ControlReg.OutputLevel);
}

                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40IsThereDataToRead                              �
�                                                                            �
�                 PURPOSE: Checks to see if the com port has data waiting    �
�                          in the input FIFO. Useful for a low level         �
�                          non-blocking protocol between external hardware   �
�                          abd the C40.                                      �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      25-May-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */

int C40IsThereDataToRead(int nComPort)
{
  union
    {
      COMPORTCONTROLREG ControlReg;
      WORD32 Overlay;
    } ControlRegister;

  ASSERT (nComPort>=0 && nComPort<=5);

  ControlRegister.Overlay=MP_GetWord(C40_COMPORT_BASEADDRESS, (word)nComPort * 0x10);
  
  return (ControlRegister.ControlReg.InputLevel);
}


                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40SimpleWrite                                    �
�                                                                            �
�                 PURPOSE: Performs a very simple blocking write to a        �
�                          COM port.                                         �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      25-May-1993 K.Blackler  Original Issue                       �
�      2.0      29-Jun-1993 K.Blackler  Implement a proper DMA transfer      �
�                                                                            �
������������������������������������������������������������������������������ */

void C40SimpleWrite(int nCommPort, void *pBuffer, unsigned nWords)
{
#if defined(_DEBUG_C40)
  int iTimeout=C40_SIMPLE_RW_TIMEOUT;
#endif
  static UNIFIEDCHANNELCONTROL TransferControl;

  ASSERT (nCommPort>=0 && nCommPort<=5);

  C40CreateUnifiedModeTransfer(AT_TO_COMPORT,&TransferControl,(void *)pBuffer,
  														 nWords*4,nCommPort,DMA_PRIORITY_EQUAL,FALSE,DMA_MODE_STOPNOAUTOINIT);

  C40StartLinkedTransfers(nCommPort,&TransferControl);
  
#if defined(_DEBUG_C40)
  printf("%s - Comport still busy",ModuleName);
#endif
  while(C40IsDMABusy(nCommPort))
    {
#if defined(_DEBUG_C40)
      if (--iTimeout==0) break;
      printf("\\\b/\b-\b");
#endif
      IdleTillInterrupt();
    } 
#if defined(_DEBUG_C40)
 if (iTimeout==0)
   {
 		 printf("\r%s - Comport timed out. ",ModuleName);
 		 ASSERT(FALSE);
   }
 else
   {  
 		 printf("\r%s - Comport finished.  ",ModuleName);
   }
#endif
}
                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40SimpleRead                                     �
�                                                                            �
�                 PURPOSE: Performs a very simple blocking read from a       �
�                          COM port.                                         �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      25-May-1993 K.Blackler  Original Issue                       �
�      2.0      29-Jun-1993 K.Blackler  Implement a proper DMA transfer      �
�                                                                            �
������������������������������������������������������������������������������ */
void C40SimpleRead(int nCommPort, void *pBuffer, unsigned nWords)
{
#if defined(_DEBUG_C40)
  int iTimeout=C40_SIMPLE_RW_TIMEOUT;
#endif
  static UNIFIEDCHANNELCONTROL TransferControl;

  ASSERT (nCommPort>=0 && nCommPort<=5);

  C40CreateUnifiedModeTransfer(AT_FROM_COMPORT,
                               &TransferControl,
                               pBuffer,nWords*4,
                               nCommPort,
                               DMA_PRIORITY_EQUAL,
                               DMA_NOINTERRUPT_ON_COMPLETE,
                               DMA_MODE_STOPNOAUTOINIT);
                               
  C40SetUnifiedTransferLink(&TransferControl,NULL); /* A null link also prevents auto-initialisation */
  C40StartLinkedTransfers(nCommPort,&TransferControl);
  
#if defined(_DEBUG_C40)
  printf("%s - Comport still busy",ModuleName);
#endif
  while(C40IsDMABusy(nCommPort))
    {
#if defined(_DEBUG_C40)
      if (--iTimeout==0) break;
      printf("\\\b/\b-\b");
#endif
      IdleTillInterrupt();
    } 
#if defined(_DEBUG_C40)
 if (iTimeout==0)
   {
 		 printf("\r%s - Comport timed out. ",ModuleName);
 		 ASSERT(FALSE);
   }
 else
   {  
 		 printf("\r%s - Comport finished.  ",ModuleName);
   }
#endif
}

                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: C40ResetComport                                   �
�                                                                            �
�                 PURPOSE: Clears out the contents of a comport FIFO         �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      16-Nov-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */
void C40ResetComport(int nComPort)
{
  static JunkBuffer[8];
  int nWords=C40IsThereDataToRead(nComPort);

  if (nWords==15) nWords=8;
  
  if (nWords>0)
    {
      C40SimpleRead(nComPort, JunkBuffer, nWords);
      nWords=C40IsThereDataToRead(nComPort); /* Now clear out the other processor's output FIFO */
		  if (nWords==15) nWords=8;
		  
		  if (nWords>0)
		    {
		      C40SimpleRead(nComPort, JunkBuffer, nWords);
		    }
    }

}






















                                                                             /*
����������������������������������������������������������������������������Ŀ
�                                                                            �
�               PROCEDURE: Various debug routines                            �
�                                                                            �
�                 PURPOSE: These are varios routines written when testing    �
�                          the above.....                                    �
�                          Useful for seeing what is happening.              �
�                                                                            �
�    MODIFICATION HISTORY:                                                   �
�                                                                            �
�    Version        Date       Author    Comments                            �
�    -------     -----------   ------    --------                            �
�      1.0      24-Oct-1993 K.Blackler  Original Issue                       �
�                                                                            �
������������������������������������������������������������������������������ */
void C40ShowComportControlReg(C40COMPORT nComport)
{
  COMPORTCONTROLREG ControlReg;
  
  MP_GetData(&ControlReg,C40_COMPORT_BASEADDRESS,(word)nComport * 0x10, wordsizeof(COMPORTCONTROLREG));
  
  printf("COMPORT CONTROL REGISTER CONTENTS[%d]\n",nComport);
  printf("\n"
         "\t    PortDirection:%d\n",ControlReg.PortDirection);
  printf("\t InputChannelHalt:%d\n",ControlReg.InputChannelHalt);
  printf("\tOutputChannelHalt:%d\n",ControlReg.OutputChannelHalt);
  printf("\t       InputLevel:%d\n",ControlReg.InputLevel);
  printf("\t      OutputLevel:%d\n",ControlReg.OutputLevel);
}

void C40ShowDMAControlReg(C40COMPORT nComport)
{            
  CHANNELREGISTER ControlRegisters;
  UNIFIEDDMACONTROLREG ControlReg;
  
  MP_GetData(&ControlRegisters,C40_DMA_BASEADDRESS, (word)nComport * 0x10, wordsizeof(CHANNELREGISTER));
  
  ControlReg=ControlRegisters.ControlRegister.UnifiedControlReg;

  printf("DMA CONTROL REGISTER CONTENTS[%d]\n",nComport);
  printf("\n"
         "\t          Priority %d",ControlReg.Priority);
  printf("\t      TransferMode %d",ControlReg.TransferMode);
  printf("\t      bSourceSynch %d\n",ControlReg.bSourceSynch);
  printf("\t bDestinationSynch %d",ControlReg.bDestinationSynch);
  printf("\t   bAutoInitStatic %d",ControlReg.bAutoInitStatic);
  printf("\t    bAutoInitSynch %d\n",ControlReg.bAutoInitSynch);
  printf("\t  bReadBitReversed %d",ControlReg.bReadBitReversed);
  printf("\t bWriteBitReversed %d",ControlReg.bWriteBitReversed);
  printf("\t        bSplitMode %d\n",ControlReg.bSplitMode);
  printf("\t      bIntOnFinish %d",ControlReg.bIntOnFinish);
  printf("\t sTransferFinished %d",ControlReg.sTransferFinished);
  printf("\t            bStart %d\n",ControlReg.bStart);
  printf("\t           sStatus %d",ControlReg.sStatus);
  printf("\t    bFixedPriority %d\n",ControlReg.bFixedPriority);
  
  printf("\n\t           SourceAddress %lx",ControlRegisters.SourceAddress);
  printf("\t      SourceAddressIndex %d\n",ControlRegisters.SourceAddressIndex);
  printf("\t         TransferCounter %d",ControlRegisters.TransferCounter);
  printf("\t      DestinationAddress %lx\n",ControlRegisters.DestinationAddress);
  printf("\t DestinationAddressIndex %d",ControlRegisters.DestinationAddressIndex);
  printf("\t             LinkPointer %lx\n",ControlRegisters.LinkPointer);
  printf("\t      AuxTransferCounter %d",ControlRegisters.AuxTransferCounter);
  printf("\t          AuxLinkPointer %lx\n",ControlRegisters.AuxLinkPointer);
}

void C40ShowIIERegister(void)
{
  IIE iie=C40GetIIE(0);
  WORD32 wordiie=*(WORD32 *)&iie;

  printf("INTERRUPT ENABLE REGISTER CONTENTS: %lx\n",wordiie);
}


