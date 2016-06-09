/*!
    \file       lci_usbav_private.h
    \brief      USB AV private structures definitions
    \details    This files defines the private data structures, function
                declarations that are only used within USBAV module.
    \authors    lucaslin
    \version    0.01a
    \date       May 23, 2013
    \todo       (Optional)
    \bug        (Optional)
    \warning    (Optional)
    \copyright  (c) 2013 Luminon Core Incorporated. All Rights Reserved.

    Revision Log
    + 0.01a;    May 12, 2013;   lucaslin
     - Created.

 */

#ifndef _LCI_USBAV_PRIVATE_H_
#define _LCI_USBAV_PRIVATE_H_

#pragma warning(disable:4127)

//#include <windows.h>
//#include <winioctl.h>
//#include <strsafe.h>
//#include <setupapi.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <usb.h>


#include "lci_vmon.h"
//#include "lci_maanshan.h"
#include "lci_usbav_ioctl.h"
#include "usbav_class_100.h"

/*
 Forward declaration
 */
typedef struct _LCI_USBAV_CTRL_IFC      LCI_USBAV_CTRL_IFC;
typedef struct _LCI_USBAV_CTRL          LCI_USBAV_CTRL;
typedef struct _LCI_USBAV_TERMINAL      LCI_USBAV_TERMINAL;
typedef struct _LCI_USBAV_UNIT          LCI_USBAV_UNIT;
typedef struct _LCI_USBAV_DATA_ENTITY   LCI_USBAV_DATA_ENTITY;
typedef struct _LCI_USBAV_CONTEXT       LCI_USBAV_CONTEXT;


//typedef struct _LCI_COMPRESSION_CTX     LCI_COMPRESSION_CTX;

#define LCI_MAX_NUM_OF_CONTROLS     32
#define LCI_MAX_NUM_OF_UNITS        32
#define LCI_MAX_NUM_OF_TERMINALS    32
#define LCI_MAX_NUM_OF_DATA_ENT     32
#define LCI_MAX_NUM_OF_VIDEO        32
#define LCI_MAX_NUM_OF_AUDIO        32
#define LCI_MAX_NUM_OF_RANGES       32
#define LCI_MAX_NUM_OF_REQUEST      LCI_MAX_NUM_OF_BITMAP_CTX

#define USB_AV_HEADER_SIZE          (USBAV_MESSAGE_INVAR_SIZE + USBAV_FORMAT1_HEADER_SIZE + USBAV_FORMAT1_INFO_BLOCK_SIZE)
#define LCI_RESPONSE_BUF_SIZE       1024

/*
 The AVControl Descriptor describes an AVControl. Whenever necessary, the
 AVControl Descriptor is followed by one or more Ranges Descriptors describing
 the ranges supported by this AVControl.
 */
typedef struct _LCI_USBAV_CTRL
    {
    CONST USBAV_CTRL_DESC *     pCtrlDesc;

    UINT                        NumberOfRanges;
    CONST USBAV_RANGES_DESC *   RangesDescList[LCI_MAX_NUM_OF_RANGES];
    } LCI_USBAV_CTRL;

/*
 the Terminal Descriptor is followed by one or more AVControl Descriptors
 describing the AVControls supported by this Terminal.
 */
typedef struct _LCI_USBAV_TERMINAL
    {
    CONST USBAV_TERM_DESC * pTermDesc;

    UINT                    NumberOfControls;
    LCI_USBAV_CTRL          Controls[LCI_MAX_NUM_OF_CONTROLS];
    } LCI_USBAV_TERMINAL;

/*
 Whenever necessary, the Unit Descriptor is followed by one or more
 AVControl Descriptors describing the AVControls supported by this Unit.
 */
typedef struct _LCI_USBAV_UNIT
    {
    CONST USBAV_UNIT_DESC * pUnitDesc;

    UINT                    NumberOfControls;
    LCI_USBAV_CTRL          Controls[LCI_MAX_NUM_OF_CONTROLS];
    } LCI_USBAV_UNIT;

/*
 Whenever necessary, the AVData Entity Descriptor is followed by one or more
 AVControl Descriptors describing the AVControls supported by this AVData
 Entity.

 If the AVData Entity is an AVData FrameBuffer entity and therefore able to
 interact with VideoBulkStreams, then the AVData Entity Descriptor is followed
 by one or more VideoBulkStreamConfig Descriptors.

 If the AVData Entity is an AVData Video Streaming Interface and therefore able
 to interact with VideoIsoStreams, then the AVData Entity Descriptor is followed
 by one or more VideoIsoStreamConfig Descriptors.

 If the AVData Entity is an AVData Audio Streaming Interface and therefore able
 to interact with AudioStreams, then the AVData Entity Descriptor is followed by
 one or more AudioStreamConfig Descriptors.

 In Legacy View, an AVData Entity that is not an AVData Streaming Interface
 shall only support the (mandatory) Alternate Setting 0 and one Active Alternate
 Setting 1. An AVData Streaming Interface may support one or more Active
 Alternate Settings. However, since there is no class-specific information to
 convey for the Active Alternate Setting(s), there is no need for an Alternate
 Setting Descriptor.
 */
typedef struct _LCI_USBAV_DATA_ENTITY
    {
    CONST USBAV_AVDATA_ENTITY_DESC * pDataEntityDesc;


    UINT                    NumberOfControls;
    LCI_USBAV_CTRL          Controls[LCI_MAX_NUM_OF_CONTROLS];

    UINT                    NumberOfVideoBulkStreams;
    CONST USBAV_VIDEO_BULK_STREAM_CFG_DESC *
                            VideoBulkStreamDescList[LCI_MAX_NUM_OF_VIDEO];

    UINT                    NumberOfVideoIsoStreams;
    CONST USBAV_VIDEO_ISO_STREAM_CFG_DESC *
                            VideoIsoStreamDescList[LCI_MAX_NUM_OF_VIDEO];

    UINT                    NumberOfAudioStreams;
    CONST USBAV_AUDIO_STREAM_CFG_DESC *
                            AudioStreamDescList[LCI_MAX_NUM_OF_AUDIO];

    } LCI_USBAV_DATA_ENTITY;

/*
 USB AV control interface abstraction.
 */
typedef struct _LCI_USBAV_CTRL_IFC
    {
    CONST USBAV_CTRL_IFC_DESC *     pCtrlIfcDesc;

    /*
     AVControl
     */
    UINT                    NumberOfControls;
    LCI_USBAV_CTRL          Controls[LCI_MAX_NUM_OF_CONTROLS];

    /*
     Terminal
     */
    UINT                    NumberOfTerminals;
    LCI_USBAV_TERMINAL      Termals[LCI_MAX_NUM_OF_TERMINALS];

    /*
     Unit
     */
    UINT                    NumberOfUnits;
    LCI_USBAV_UNIT          Units[LCI_MAX_NUM_OF_UNITS];

    /*
     AVData Entity
     */
    UINT                    NumberOfDataEntities;
    LCI_USBAV_DATA_ENTITY   DataEntities[LCI_MAX_NUM_OF_DATA_ENT];
    LCI_USBAV_DATA_ENTITY * pFrameBufferInEntity;

    } LCI_USBAV_CTRL_IFC;


typedef VOID
LCI_USBAV_GENERIC_DONE(
    __in LCI_USBAV_CONTEXT *    pUsbAvCtx,
    __in PVOID                  pDoneCtx
    );


/*
 Generic USB AV request management
 */
typedef struct _LCI_USBAV_REQUEST
    {
    LIST_ENTRY                  ListEntry;

    LCI_USBAV_CONTEXT *         pUsbAvCtx;
    ULONG                       CmdBufferSize;
    PVOID                       pCmdBuffer;
    ULONG                       RspBufferSize;
    PVOID                       pRspBuffer;

    OVERLAPPED                  Overlapped;

    LCI_USBAV_GENERIC_DONE *    pDoneFn;
    PVOID                       pDoneCtx;

    ULONG                       BytesReturned;
	UCHAR                      	IoRspBuffer[LCI_RESPONSE_BUF_SIZE];
    } LCI_USBAV_REQUEST;

/*
 USBAV context area
 */
#define LCI_USBAV_EDID_DATA_SIZE        256
typedef struct _LCI_USBAV_CONTEXT
    {
    HANDLE                              hDevice; // maybe not needed. has a copy in LCI_VMON_DEV_CTX.


    UCHAR CONST *                       pFullConfigDesc;
    USHORT                              FullConfigDescSize;
    ULONG                               MaximumPacketSize;

    UCHAR                               EdidData[LCI_USBAV_EDID_DATA_SIZE];
    LCI_USBAV_CTRL_IFC                  CtrlIfc;

    LCI_USBAV_REQUEST                   AvRequestPool[LCI_MAX_NUM_OF_REQUEST];

    LIST_ENTRY                          FreeRequestListHead;
    HANDLE                              FreeRequestListMutex;

    LIST_ENTRY                  		AvIoPendingListHead;
    HANDLE                      		AvIoPendingListMutex;
	
	PVOID								pClientCompressionCtx;
    } LCI_USBAV_CONTEXT;

/*!
 Name : LCI_VMON_REQ_CTX

 \details   LCI_VMON_REQ_CTX is used in 3 stages:
    Stage1: The pixel grabber prepares a frame for encoding. During this stage,
            a LCI_VMON_REQ_CTX structure is allocated, and the following fields
            are updated: {BitmapSize, BitmapBuffer, Width, Height, Format}.
            The output buffer is also allocated, and the following fields are
            updated : {OutBufferSize, OutBuffer}
            The final step of this stage is to kick off encoding process, by
            submitting the frame to compressor thread.

    Stage2: The compressor thread receives the a LCI_VMON_REQ_CTX, and calls
            2.1) MSTAR_WRAPPER_JpeInit to initialize the JPE context. The input
            information includes Width, Height, USB_AV_preserved header, and
            Format.
            2.2) MSTAR_WRAPPER_JpeEncodeOneFrame to kick off the compression
            engine. The compression engine executes the job in another thread
            which was created inside MSTAR library.

    Stage3: Once the compression job is done, a MSTAR_WRAPPER_JPE_CALLBACK
            is triggered. Inside the MSTAR_WRAPPER_JPE_CALLBACK routine,
            MSTAR_WRAPPER_JpeFreeBuffer is called to inform the compression that
            current compression context is completed, and compression engine
            is now ready to receive another compression job via
            MSTAR_WRAPPER_JpeEncodeOneFrame.

            By looking at MSTAR API design, the MSTAR library isn't multi-thread
            safe in which MSTAR_WRAPPER_JpeEncodeOneFrame must be paired with
            MSTAR_WRAPPER_JpeFreeBuffer. The thread that initiates
            MSTAR_WRAPPER_JpeEncodeOneFrame must insure that previous compression
            job is completed, and indicating the compression done event to MSTAR
            engine via MSTAR_WRAPPER_JpeFreeBuffer.
 */

/*
 Function declaration
 */
__checkReturn
BOOL
LCI_USBAV_Create(
    __in PVOID  					pClientCompressCtx,
	__in HANDLE						hDevice
    );

VOID
LCI_USBAV_Destroy(
    __in PVOID   					pClientCompressCtx
    );

CONST UCHAR *
LCI_USBAV_FindDesc(
	__in UCHAR CONST *              pConfigDesc,
    __in SIZE_T                     ConfigDescSize,
    __in UCHAR CONST *              pStartPos,
    __in UCHAR CONST *              pEndPos,
    __in UCHAR                      bDescriptorType
	);

__checkReturn
BOOL
LCI_USBAV_ParseCtrlIfc(
    __in LCI_USBAV_CONTEXT *            pUsbAvCtx
    );

VOID
LCI_USBAV_ParseCtrlDesc(
    __in LCI_USBAV_CONTEXT *            pUsbAvCtx,
    __in LCI_USBAV_CTRL *               pControl,
    __in USBAV_CTRL_DESC CONST *        pCtrlDesc,
    __in CONST UCHAR *                  pEndPos,
    __out ULONG *                       pNumberOfBytesParsed
    );

VOID
LCI_USBAV_ParseDataEntityDesc(
    __in LCI_USBAV_CONTEXT *            pUsbAvCtx,
    __in LCI_USBAV_DATA_ENTITY *        pDataEntity,
    __in USBAV_AVDATA_ENTITY_DESC CONST *pDataEntityDesc,
    __in CONST UCHAR *                  pEndPos,
    __out ULONG *                       pNumberOfBytesParsed
    );

VOID
LCI_USBAV_ParseTermDesc(
    __in LCI_USBAV_CONTEXT *            pUsbAvCtx,
    __in LCI_USBAV_TERMINAL *           pTerminal,
    __in USBAV_TERM_DESC CONST *        pTermDesc,
    __in CONST UCHAR *                  pEndPos,
    __out ULONG *                       pNumberOfBytesParsed
    );

VOID
LCI_USBAV_ParseUnitDesc(
    __in LCI_USBAV_CONTEXT *            pUsbAvCtx,
    __in LCI_USBAV_UNIT *               pUnit,
    __in USBAV_UNIT_DESC CONST *        pUnitDesc,
    __in CONST UCHAR *                  pEndPos,
    __out ULONG *                       pNumberOfBytesParsed
    );



VOID
LCI_USBAV_CheckBitmapCompletion(
    __in PVOID	pClientCompressionCtx
    );

VOID
LCI_USBAV_WaitForAllBitmapCompletion(
    __in PVOID	pClientCompressionCtx
    );
	
__checkReturn
PVOID
LCI_USBAV_AllocateAvRequest(
    __in PVOID   			pBusCtx
    );

VOID
LCI_USBAV_ReleaseAvRequest(
    __in LCI_USBAV_CONTEXT *   pUsbAvCtx,
    __in LCI_USBAV_REQUEST *   pAvRequest
    );

__checkReturn
BOOL
LCI_USBAV_GetEdid(
    __in LCI_USBAV_CONTEXT *            pUsbAvCtx,
    __out PVOID                         pEdidData,
    __in USHORT                         wEdidOffset,
    __in USHORT                         wEdidLength,
    __out ULONG *                       pBytesReturned
    );

VOID
LCI_USBAV_DestroyVirtualMem(
    __in LCI_USBAV_CONTEXT *    pUsbAvCtx
    );

VOID
LCI_USBAV_OutputFrame(
    __in UCHAR *    				pOutputBuffer,
    __in ULONG      				nOffset,
    __in ULONG      				nSize,
	__in LCI_MAANSHAN_CTX *			pMaanshanCtx,
	__in LCI_VMON_REQ_CTX *			pBitmapReq
    );



#endif /* _LCI_USBAV_PRIVATE_H_ */