/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef __INCLUDE_SCSI_H
#define __INCLUDE_SCSI_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <aic_core.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SCSI commands ************************************************************/

#define SCSI_CMD_TESTUNITREADY            0x00
#define SCSI_CMD_REZEROUNIT               0x01
#define SCSI_CMD_REQUESTSENSE             0x03
#define SCSI_CMD_FORMAT_UNIT              0x04
#define SCSI_CMD_REASSIGNBLOCKS           0x07
#define SCSI_CMD_READ6                    0x08
#define SCSI_CMD_WRITE6                   0x0a
#define SCSI_CMD_SEEK6                    0x0b
#define SCSI_CMD_SPACE6                   0x11
#define SCSI_CMD_INQUIRY                  0x12
#define SCSI_CMD_MODESELECT6              0x15
#define SCSI_CMD_RESERVE6                 0x16
#define SCSI_CMD_RELEASE6                 0x17
#define SCSI_CMD_COPY                     0x18
#define SCSI_CMD_MODESENSE6               0x1a
#define SCSI_CMD_STARTSTOPUNIT            0x1b
#define SCSI_CMD_RECEIVEDIAGNOSTICRESULTS 0x1c
#define SCSI_CMD_SENDDIAGNOSTIC           0x1d
#define SCSI_CMD_PREVENTMEDIAREMOVAL      0x1e
#define SCSI_CMD_READFORMATCAPACITIES     0x23
#define SCSI_CMD_READCAPACITY10           0x25
#define SCSI_CMD_READ10                   0x28
#define SCSI_CMD_WRITE10                  0x2a
#define SCSI_CMD_SEEK10                   0x2b
#define SCSI_CMD_WRITEANDVERIFY           0x2e
#define SCSI_CMD_VERIFY10                 0x2f
#define SCSI_CMD_SEARCHDATAHIGH           0x30
#define SCSI_CMD_SEARCHDATAEQUAL          0x31
#define SCSI_CMD_SEARCHDATALOW            0x32
#define SCSI_CMD_SETLIMITS10              0x33
#define SCSI_CMD_PREFETCH10               0x34
#define SCSI_CMD_SYNCHCACHE10             0x35
#define SCSI_CMD_LOCKCACHE                0x36
#define SCSI_CMD_READDEFECTDATA10         0x37
#define SCSI_CMD_COMPARE                  0x39
#define SCSI_CMD_COPYANDVERIFY            0x3a
#define SCSI_CMD_WRITEBUFFER              0x3b
#define SCSI_CMD_READBUFFER               0x3c
#define SCSI_CMD_READLONG10               0x3e
#define SCSI_CMD_WRITELONG10              0x3f
#define SCSI_CMD_CHANGEDEFINITION         0x40
#define SCSI_CMD_WRITESAME10              0x41
#define SCSI_CMD_LOGSELECT                0x4c
#define SCSI_CMD_LOGSENSE                 0x4d
#define SCSI_CMD_XDWRITE10                0x50
#define SCSI_CMD_XPWRITE10                0x51
#define SCSI_CMD_XDREAD10                 0x52
#define SCSI_CMD_MODESELECT10             0x55
#define SCSI_CMD_RESERVE10                0x56
#define SCSI_CMD_RELEASE10                0x57
#define SCSI_CMD_MODESENSE10              0x5a
#define SCSI_CMD_PERSISTENTRESERVEIN      0x5e
#define SCSI_CMD_PERSISTENTRESERVEOUT     0x5f
#define SCSI_CMD_32                       0x7f
#define SCSI_CMD_XDWRITEEXTENDED          0x80
#define SCSI_CMD_REBUILD                  0x82
#define SCSI_CMD_REGENERATE               0x82
#define SCSI_CMD_EXTENDEDCOPY             0x83
#define SCSI_CMD_COPYRESULTS              0x84
#define SCSI_CMD_ACCESSCONTROLIN          0x86
#define SCSI_CMD_ACCESSCONTROLOUT         0x87
#define SCSI_CMD_READ16                   0x88
#define SCSI_CMD_WRITE16                  0x8a
#define SCSI_CMD_READATTRIBUTE            0x8c
#define SCSI_CMD_WRITEATTRIBUTE           0x8d
#define SCSI_CMD_WRITEANDVERIFY16         0x8e
#define SCSI_CMD_PREFETCH16               0x90
#define SCSI_CMD_SYNCHCACHE16             0x91
#define SCSI_CMD_LOCKUNLOCKACACHE         0x92
#define SCSI_CMD_WRITESAME16              0x93
#define SCSI_CMD_READCAPACITY16           0x9e
#define SCSI_CMD_READLONG16               0x9e
#define SCSI_CMD_WRITELONG106             0x9f
#define SCSI_CMD_REPORTLUNS               0xa0
#define SCSI_CMD_MAINTENANCEIN            0xa3
#define SCSI_CMD_MAINTENANCEOUT           0xa4
#define SCSI_CMD_MOVEMEDIUM               0xa5
#define SCSI_CMD_MOVEMEDIUMATTACHED       0xa7
#define SCSI_CMD_READ12                   0xa8
#define SCSI_CMD_WRITE12                  0xaa
#define SCSI_CMD_READMEDIASERIALNUMBER    0xab
#define SCSI_CMD_WRITEANDVERIFY12         0xae
#define SCSI_CMD_VERIFY12                 0xaf
#define SCSI_CMD_SETLIMITS12              0xb3
#define SCSI_CMD_READELEMENTSTATUS        0xb4
#define SCSI_CMD_READDEFECTDATA12         0xb7
#define SCSI_CMD_REDUNDANCYGROUPIN        0xba
#define SCSI_CMD_REDUNDANCYGROUPOUT       0xbb
#define SCSI_CMD_SPAREIN                  0xbc
#define SCSI_CMD_SPAREOUT                 0xbd
#define SCSI_CMD_VOLUMESETIN              0xbe
#define SCSI_CMD_VOLUMESETOUT             0xbf

/* Common SCSI KCQ values (sense Key/additional sense Code/ASC Qualifier) ***
 *
 *   0xnn0386  Write Fault Data Corruption
 *   0xnn0500  Illegal request
 *   0xnn0600  Unit attention
 *   0xnn0700  Data protect
 *   0xnn0800  LUN communication failure
 *   0xnn0801  LUN communication timeout
 *   0xnn0802  LUN communication parity error
 *   0xnn0803  LUN communication CRC error
 *   0xnn0900  vendor specific sense key
 *   0xnn0901  servo fault
 *   0xnn0904  head select fault
 *   0xnn0a00  error log overflow
 *   0xnn0b00  aborted command
 *   0xnn0c00  write error
 *   0xnn0c02  write error - auto-realloc failed
 *   0xnn0e00  data miscompare
 *   0xnn1200  address mark not founf for ID field
 *   0xnn1400  logical block not found
 *   0xnn1500  random positioning error
 *   0xnn1501  mechanical positioning error
 *   0xnn1502  positioning error detected by read of medium
 *   0xnn2700  write protected
 *   0xnn2900  POR or bus reset occurred
 *   0xnn3101  format failed
 *   0xnn3191  format corrupted
 *   0xnn3201  defect list update error
 *   0xnn3202  no spares available
 *   0xnn3501  unspecified enclosure services failure
 *   0xnn3700  parameter rounded
 *   0xnn3d00  invalid bits in identify message
 *   0xnn3e00  LUN not self-configured yet
 *   0xnn4001  DRAM parity error
 *   0xnn4002  DRAM parity error
 *   0xnn4200  power-on or self-test failure
 *   0xnn4c00  LUN failed self-configuration
 *   0xnn5c00  RPL status change
 *   0xnn5c01  spindles synchronized
 *   0xnn5c02  spindles not synchronized
 *   0xnn6500  voltage fault
 *   0xnn8000  general firmware error
 */

/* No sense KCQ values */

#define SCSI_KCQ_NOSENSE 0x000000 /* No error */
#define SCSI_KCQ_PFATHRESHOLDREACHED \
    0x005c00 /* No sense - PFA threshold reached */

/* Soft error KCQ values */

#define SCSI_KCQSE_RWENOINDEX 0x010100 /* Recovered Write error - no index */
#define SCSI_KCQSE_RECOVEREDNOSEEKCOMPLETION \
    0x010200 /* Recovered no seek completion */
#define SCSI_KCQSE_RWEWRITEFAULT \
    0x010300 /* Recovered Write error - write fault */
#define SCSI_KCQSE_TRACKFOLLOWINGERROR 0x010900 /* Track following error */
#define SCSI_KCQSE_TEMPERATUREWARNING  0x010b01 /* Temperature warning */
#define SCSI_KCQSE_RWEWARREALLOCATED \
    0x010c01 /* Recovered Write error with auto-realloc - reallocated */
#define SCSI_KCQSE_RWERECOMMENDREASSIGN \
    0x010c03 /* Recovered Write error - recommend reassign */
#define SCSI_KCQSE_RDWOEUSINGPREVLBI \
    0x011201 /* Recovered data without ECC using prev logical block ID */
#define SCSI_KCQSE_RDWEUSINGPREVLBI \
    0x011202 /* Recovered data with ECC using prev logical block ID */
#define SCSI_KCQSE_RECOVEREDRECORDNOTFOUND \
    0x011401 /* Recovered Record Not Found */
#define SCSI_KCQSE_RWEDSME \
    0x011600 /* Recovered Write error - Data Sync Mark Error */
#define SCSI_KCQSE_RWEDSEDATAREWRITTEN \
    0x011601 /* Recovered Write error - Data Sync Error - data rewritten */
#define SCSI_KCQSE_RWEDSERECOMMENDREWRITE \
    0x011602 /* Recovered Write error - Data Sync Error - recommend rewrite */
#define SCSI_KCQSE_RWEDSEDATAAUTOREALLOCATED \
    0x011603 /* Recovered Write error - Data Sync Error - data auto-reallocated */
#define SCSI_KCQSE_RWEDSERECOMMENDREASSIGNMENT \
    0x011604 /* Recovered Write error - Data Sync Error - recommend reassignment */
#define SCSI_KCQSE_RDWNECORRECTIONAPPLIED \
    0x011700 /* Recovered data with no error correction applied */
#define SCSI_KCQSE_RREWITHRETRIES \
    0x011701 /* Recovered Read error - with retries */
#define SCSI_KCQSE_RDUSINGPOSITIVEOFFSET \
    0x011702 /* Recovered data using positive offset */
#define SCSI_KCQSE_RDUSINGNEGATIVEOFFSET \
    0x011703 /* Recovered data using negative offset */
#define SCSI_KCQSE_RDUSINGPREVIOUSLBI \
    0x011705 /* Recovered data using previous logical block ID */
#define SCSI_KCQSE_RREWOEAUTOREALLOCATED \
    0x011706 /* Recovered Read error - without ECC, auto reallocated */
#define SCSI_KCQSE_RREWOERECOMMENDREASSIGN \
    0x011707 /* Recovered Read error - without ECC, recommend reassign */
#define SCSI_KCQSE_RREWOERECOMMENDREWRITE \
    0x011708 /* Recovered Read error - without ECC, recommend rewrite */
#define SCSI_KCQSE_RREWOEDATAREWRITTEN \
    0x011709 /* Recovered Read error - without ECC, data rewritten */
#define SCSI_KCQSE_RREWE 0x011800 /* Recovered Read error - with ECC */
#define SCSI_KCQSE_RDWEANDRETRIES \
    0x011801 /* Recovered data with ECC and retries */
#define SCSI_KCQSE_RREWEAUTOREALLOCATED \
    0x011802 /* Recovered Read error - with ECC, auto reallocated */
#define SCSI_KCQSE_RREWERECOMMENDREASSIGN \
    0x011805 /* Recovered Read error - with ECC, recommend reassign */
#define SCSI_KCQSE_RDUSINGECCANDOFFSETS \
    0x011806 /* Recovered data using ECC and offsets */
#define SCSI_KCQSE_RREWEDATAREWRITTEN \
    0x011807 /* Recovered Read error - with ECC, data rewritten */
#define SCSI_KCQSE_DLNOTFOUND 0x011c00 /* Defect List not found */
#define SCSI_KCQSE_PRIMARYDLNOTFOUND \
    0x011c01                                /* Primary defect list not found */
#define SCSI_KCQSE_GROWNDLNOTFOUND 0x011c02 /* Grown defect list not found */
#define SCSI_KCQSE_PARTIALDLTRANSFERRED \
    0x011f00 /* Partial defect list transferred */
#define SCSI_KCQSE_INTERNALTARGETFAILURE 0x014400 /* Internal target failure */
#define SCSI_KCQSE_PFATHRESHOLDREACHED   0x015d00 /* PFA threshold reached */
#define SCSI_KCQSE_PFATESTWARNING        0x015dff /* PFA test warning */
#define SCSI_KCQSE_INTERNALLOGICFAILURE  0x018100 /* Internal logic failure */

/* Not Ready / Diagnostic Failure KCQ values */

#define SCSI_KCQNR_CAUSENOTREPORTABLE \
    0x020400 /* Not Ready - Cause not reportable. */
#define SCSI_KCQNR_BECOMINGREADY 0x020401 /* Not Ready - becoming ready */
#define SCSI_KCQNR_NEEDINITIALIZECOMMAND \
    0x020402 /* Not Ready - need initialize command (start unit) */
#define SCSI_KCQNR_MANUALINTERVENTIONREQUIRED \
    0x020403 /* Not Ready - manual intervention required */
#define SCSI_KCQNR_FORMATINPROGRESS \
    0x020404 /* Not Ready - format in progress */
#define SCSI_KCQNR_SELFTESTINPROGRESS \
    0x020409 /* Not Ready - self-test in progress */
#define SCSI_KCQNR_MEDIUMFORMATCORRUPTED \
    0x023100 /* Not Ready - medium format corrupted */
#define SCSI_KCQNR_FORMATCOMMANDFAILED \
    0x023101 /* Not Ready - format command failed */
#define SCSI_KCQNR_ESUNAVAILABLE \
    0x023502 /* Not Ready - enclosure services unavailable */
#define SCSI_KCQNR_MEDIANOTPRESENT 0x023a00 /* Not Ready - media not present */
#define SCSI_KCQDF_BRINGUPFAILORDEGRADEDMODE \
    0x024080 /* Diagnostic Failure - bring-up fail or degraded mode */
#define SCSI_KCQDF_HARDDISKCONTROLLER \
    0x024081 /* Diagnostic Failure - Hard Disk Controller */
#define SCSI_KCQDF_RAMMICROCODENOTLOADED \
    0x024085 /* Diagnostic Failure - RAM microcode not loaded */
#define SCSI_KCQDF_RROCALIBRATION \
    0x024090 /* Diagnostic Failure - RRO Calibration */
#define SCSI_KCQDF_CHANNELCALIBRATION \
    0x024091 /* Diagnostic Failure - Channel Calibration */
#define SCSI_KCQDF_HEADLOAD 0x024092 /* Diagnostic Failure - Head Load */
#define SCSI_KCQDF_WRITEAE  0x024093 /* Diagnostic Failure - Write AE */
#define SCSI_KCQDF_12VOVERCURRENT \
    0x024094 /* Diagnostic Failure - 12V over current */
#define SCSI_KCQDF_OTHERSPINDLEFAILURE \
    0x024095 /* Diagnostic Failure - Other spindle failure */
#define SCSI_KCQDF_SELFRESET 0x0240b0 /* Diagnostic Failure - self-reset */
#define SCSI_KCQDF_CONFIGNOTLOADED \
    0x024c00 /* Diagnostic Failure - config not loaded */

/* Medium error KCQ values */

#define SCSI_KCQME_WRITEFAULT 0x030300 /* Medium Error - write fault */
#define SCSI_KCQME_WRITEFAULTAUTOREALLOCFAILED \
    0x030c02 /* Medium Error - write error - auto-realloc failed */
#define SCSI_KCQME_WRITERTLIMITEXCEEDED \
    0x030cbb /* Medium Error - write recovery time limit exceeded */
#define SCSI_KCQME_IDCRCERROR 0x031000 /* Medium Error - ID CRC error */
#define SCSI_KCQME_UNRRE1     0x031100 /* Medium Error - unrecovered read error */
#define SCSI_KCQME_READRETRIESEXHAUSTED \
    0x031101 /* Medium Error - read retries exhausted */
#define SCSI_KCQME_ERRORTOOLONGTOCORRECT \
    0x031102 /* Medium Error - error too long to correct */
#define SCSI_KCQME_UREAUTOREALLOCFAILED \
    0x031104 /* Medium Error - unrecovered read error - auto re-alloc failed */
#define SCSI_KCQME_URERECOMMENDREASSIGN \
    0x03110b /* Medium Error - unrecovered read error - recommend reassign */
#define SCSI_KCQME_READRTLIMITEXCEEDED \
    0x0311ff /* Medium Error - read recovery time limit exceeded */
#define SCSI_KCQME_RECORDNOTFOUND 0x031401 /* Medium Error - record not found */
#define SCSI_KCQME_DSME           0x031600 /* Medium Error - Data Sync Mark error */
#define SCSI_KCQME_DSERECOMMENDREASSIGN \
    0x031604 /* Medium Error - Data Sync Error - recommend reassign */
#define SCSI_KCQME_DLE 0x031900 /* Medium Error - defect list error */
#define SCSI_KCQME_DLNOTAVAILABLE \
    0x031901 /* Medium Error - defect list not available */
#define SCSI_KCQME_DLEINPRIMARYLIST \
    0x031902 /* Medium Error - defect list error in primary list */
#define SCSI_KCQME_DLEINGROWNLIST \
    0x031903 /* Medium Error - defect list error in grown list */
#define SCSI_KCQME_FEWERTHAN50PCTDLCOPIES \
    0x03190e /* Medium Error - fewer than 50% defect list copies */
#define SCSI_KCQME_MEDIUMFORMATCORRUPTED \
    0x033100 /* Medium Error - medium format corrupted */
#define SCSI_KCQME_FORMATCOMMANDFAILED \
    0x033101 /* Medium Error - format command failed */
#define SCSI_KCQME_DATAAUTOREALLOCATED \
    0x038000 /* Medium Error - data auto-reallocated */

/* Hardware Error KCQ values */

#define SCSI_KCQHE_NOINDEXORSECTOR \
    0x040100 /* Hardware Error - no index or sector */
#define SCSI_KCQHE_NOSEEKCOMPLETE \
    0x040200                           /* Hardware Error - no seek complete */
#define SCSI_KCQHE_WRITEFAULT 0x040300 /* Hardware Error - write fault */
#define SCSI_KCQHE_COMMUNICATIONFAILURE \
    0x040800 /* Hardware Error - communication failure */
#define SCSI_KCQHE_TRACKFOLLOWINGERROR \
    0x040900 /* Hardware Error - track following error */
#define SCSI_KCQHE_UREINRESERVEDAREA \
    0x041100 /* Hardware Error - unrecovered read error in reserved area */
#define SCSI_KCQHE_DSMEINRESERVEDAREA \
    0x041600 /* Hardware Error - Data Sync Mark error in reserved area */
#define SCSI_KCQHE_DLE 0x041900 /* Hardware Error - defect list error */
#define SCSI_KCQHE_DLEINPRIMARYLIST \
    0x041902 /* Hardware Error - defect list error in Primary List */
#define SCSI_KCQHE_DLEINGROWNLIST \
    0x041903 /* Hardware Error - defect list error in Grown List */
#define SCSI_KCQHE_REASSIGNFAILED \
    0x043100 /* Hardware Error - reassign failed */
#define SCSI_KCQHE_NODEFECTSPAREAVAILABLE \
    0x043200 /* Hardware Error - no defect spare available */
#define SCSI_KCQHE_UNSUPPORTEDENCLOSUREFUNCTION \
    0x043501 /* Hardware Error - unsupported enclosure function */
#define SCSI_KCQHE_ESUNAVAILABLE \
    0x043502 /* Hardware Error - enclosure services unavailable */
#define SCSI_KCQHE_ESTRANSFERFAILURE \
    0x043503 /* Hardware Error - enclosure services transfer failure */
#define SCSI_KCQHE_ESREFUSED \
    0x043504 /* Hardware Error - enclosure services refused */
#define SCSI_KCQHE_SELFTESTFAILED \
    0x043e03 /* Hardware Error - self-test failed */
#define SCSI_KCQHE_UNABLETOUPDATESELFTEST \
    0x043e04 /* Hardware Error - unable to update self-test */
#define SCSI_KCQHE_DMDIAGNOSTICFAIL \
    0x044080 /* Hardware Error - Degrade Mode. Diagnostic Fail */
#define SCSI_KCQHE_DMHWERROR \
    0x044081 /* Hardware Error - Degrade Mode. H/W Error */
#define SCSI_KCQHE_DMRAMMICROCODENOTLOADED \
    0x044085 /* Hardware Error - Degrade Mode. RAM microcode not loaded */
#define SCSI_KCQHE_SEEKTESTFAILURE \
    0x044090 /* Hardware Error - seek test failure */
#define SCSI_KCQHE_READWRITETESTFAILURE \
    0x0440a0 /* Hardware Error - read/write test failure */
#define SCSI_KCQHE_DEVICESELFRESET \
    0x0440b0 /* Hardware Error - device self-reset */
#define SCSI_KCQHE_COMPONENTMISMATCH \
    0x0440d0 /* Hardware Error - component mismatch */
#define SCSI_KCQHE_INTERNALTARGETFAILURE \
    0x044400 /* Hardware Error - internal target failure */
#define SCSI_KCQHE_INTERNALLOGICERROR \
    0x048100 /* Hardware Error - internal logic error */
#define SCSI_KCQHE_COMMANDTIMEOUT \
    0x048200 /* Hardware Error - command timeout */

/* Illegal Request KCQ values */

#define SCSI_KCQIR_PARMLISTLENGTHERROR \
    0x051a00 /* Illegal Request - parm list length error */
#define SCSI_KCQIR_INVALIDCOMMAND \
    0x052000 /* Illegal Request - invalid/unsupported command code */
#define SCSI_KCQIR_LBAOUTOFRANGE \
    0x052100 /* Illegal Request - LBA out of range */
#define SCSI_KCQIR_INVALIDFIELDINCBA \
    0x052400 /* Illegal Request - invalid field in CDB (Command Descriptor Block) */
#define SCSI_KCQIR_INVALIDLUN 0x052500 /* Illegal Request - invalid LUN */
#define SCSI_KCQIR_INVALIDFIELDSINPARMLIST \
    0x052600 /* Illegal Request - invalid fields in parm list */
#define SCSI_KCQIR_PARAMETERNOTSUPPORTED \
    0x052601 /* Illegal Request - parameter not supported */
#define SCSI_KCQIR_INVALIDPARMVALUE \
    0x052602 /* Illegal Request - invalid parm value */
#define SCSI_KCQIR_IFPTHRESHOLDPARAMETER \
    0x052603 /* Illegal Request - invalid field parameter - threshold parameter */
#define SCSI_KCQIR_INVALIDRELEASEOFPR \
    0x052604 /* Illegal Request - invalid release of persistent reservation */
#define SCSI_KCQIR_IFPTMSFIRMWARETAG \
    0x052697 /* Illegal Request - invalid field parameter - TMS firmware tag */
#define SCSI_KCQIR_IFPCHECKSUM \
    0x052698 /* Illegal Request - invalid field parameter - check sum */
#define SCSI_KCQIR_IFPFIRMWARETAG \
    0x052699 /* Illegal Request - invalid field parameter - firmware tag */
#define SCSI_KCQIR_COMMANDSEQUENCEERROR \
    0x052c00 /* Illegal Request - command sequence error */
#define SCSI_KCQIR_UNSUPPORTEDENCLOSUREFUNCTION \
    0x053501 /* Illegal Request - unsupported enclosure function */
#define SCSI_KCQIR_SAVINGPARMSNOTSUPPORTED \
    0x053900 /* Illegal Request - Saving parameters not supported */
#define SCSI_KCQIR_INVALIDMESSAGE \
    0x054900 /* Illegal Request - invalid message */
#define SCSI_KCQIR_MEDIALOADOREJECTFAILED \
    0x055300 /* Illegal Request - media load or eject failed */
#define SCSI_KCQIR_UNLOADTAPEFAILURE \
    0x055301 /* Illegal Request - unload tape failure */
#define SCSI_KCQIR_MEDIUMREMOVALPREVENTED \
    0x055302 /* Illegal Request - medium removal prevented */
#define SCSI_KCQIR_SYSTEMRESOURCEFAILURE \
    0x055500 /* Illegal Request - system resource failure */
#define SCSI_KCQIR_SYSTEMBUFFERFULL \
    0x055501 /* Illegal Request - system buffer full */
#define SCSI_KCQIR_INSUFFICIENTRR \
    0x055504 /* Illegal Request - Insufficient Registration Resources */

/* Unit Attention KCQ values */

#define SCSI_KCQUA_NOTREADYTOTRANSITION \
    0x062800 /* Unit Attention - not-ready to ready transition (format complete) */
#define SCSI_KCQUA_DEVICERESETOCCURRED \
    0x062900 /* Unit Attention - POR or device reset occurred */
#define SCSI_KCQUA_POROCCURRED 0x062901 /* Unit Attention - POR occurred */
#define SCSI_KCQUA_SCSIBUSRESETOCCURRED \
    0x062902 /* Unit Attention - SCSI bus reset occurred */
#define SCSI_KCQUA_TARGETRESETOCCURRED \
    0x062903 /* Unit Attention - TARGET RESET occurred */
#define SCSI_KCQUA_SELFINITIATEDRESETOCCURRED \
    0x062904 /* Unit Attention - self-initiated-reset occurred */
#define SCSI_KCQUA_TRANSCEIVERMODECHANGETOSE \
    0x062905 /* Unit Attention - transceiver mode change to SE */
#define SCSI_KCQUA_TRANSCEIVERMODECHANGETOLVD \
    0x062906 /* Unit Attention - transceiver mode change to LVD */
#define SCSI_KCQUA_PARAMETERSCHANGED \
    0x062a00 /* Unit Attention - parameters changed */
#define SCSI_KCQUA_MODEPARAMETERSCHANGED \
    0x062a01 /* Unit Attention - mode parameters changed */
#define SCSI_KCQUA_LOGSELECTPARMSCHANGED \
    0x062a02 /* Unit Attention - log select parms changed */
#define SCSI_KCQUA_RESERVATIONSPREEMPTED \
    0x062a03 /* Unit Attention - Reservations pre-empted */
#define SCSI_KCQUA_RESERVATIONSRELEASED \
    0x062a04 /* Unit Attention - Reservations released */
#define SCSI_KCQUA_REGISTRATIONSPREEMPTED \
    0x062a05 /* Unit Attention - Registrations pre-empted */
#define SCSI_KCQUA_COMMANDSCLEARED \
    0x062f00 /* Unit Attention - commands cleared by another initiator */
#define SCSI_KCQUA_OPERATINGCONDITIONSCHANGED \
    0x063f00 /* Unit Attention - target operating conditions have changed */
#define SCSI_KCQUA_MICROCODECHANGED \
    0x063f01 /* Unit Attention - microcode changed */
#define SCSI_KCQUA_CHANGEDOPERATINGDEFINITION \
    0x063f02 /* Unit Attention - changed operating definition */
#define SCSI_KCQUA_INQUIRYPARAMETERSCHANGED \
    0x063f03 /* Unit Attention - inquiry parameters changed */
#define SCSI_KCQUA_DEVICEIDENTIFIERCHANGED \
    0x063f05 /* Unit Attention - device identifier changed */
#define SCSI_KCQUA_INVALIDAPMPARAMETERS \
    0x063f90 /* Unit Attention - invalid APM parameters */
#define SCSI_KCQUA_WORLDWIDENAMEMISMATCH \
    0x063f91 /* Unit Attention - world-wide name mismatch */
#define SCSI_KCQUA_PFATHRESHOLDREACHED \
    0x065d00 /* Unit Attention - PFA threshold reached */
#define SCSI_KCQUA_PFATHRESHOLDEXCEEDED \
    0x065dff /* Unit Attention - PFA threshold exceeded */

/* Write Protect KCQ values */

#define SCSI_KCQWP_COMMANDNOTALLOWED \
    0x072700 /* Write Protect - command not allowed */

/* Aborted Command KCQ values */

#define SCSI_KCQAC_NOADDITIONALSENSECODE \
    0x0b0000 /* Aborted Command - no additional sense code */
#define SCSI_KCQAC_SYNCDATATRANSFERERROR \
    0x0b1b00 /* Aborted Command - sync data transfer error (extra ACK) */
#define SCSI_KCQAC_UNSUPPORTEDLUN \
    0x0b2500 /* Aborted Command - unsupported LUN */
#define SCSI_KCQAC_ECHOBUFFEROVERWRITTEN \
    0x0b3f0f /* Aborted Command - echo buffer overwritten */
#define SCSI_KCQAC_MESSAGEREJECTERROR \
    0x0b4300 /* Aborted Command - message reject error */
#define SCSI_KCQAC_INTERNALTARGETFAILURE \
    0x0b4400 /* Aborted Command - internal target failure */
#define SCSI_KCQAC_SELECTIONFAILURE \
    0x0b4500 /* Aborted Command - Selection/Reselection failure */
#define SCSI_KCQAC_SCSIPARITYERROR \
    0x0b4700 /* Aborted Command - SCSI parity error */
#define SCSI_KCQAC_INITIATORDETECTEDERRORECEIVED \
    0x0b4800 /* Aborted Command - initiator-detected error message received */
#define SCSI_KCQAC_ILLEGALMESSAGE \
    0x0b4900 /* Aborted Command - inappropriate/illegal message */
#define SCSI_KCQAC_DATAPHASEERROR \
    0x0b4b00 /* Aborted Command - data phase error */
#define SCSI_KCQAC_OVERLAPPEDCOMMANDSATTEMPTED \
    0x0b4e00 /* Aborted Command - overlapped commands attempted */
#define SCSI_KCQAC_LOOPINITIALIZATION \
    0x0b4f00 /* Aborted Command - due to loop initialization */

/* Other KCQ values: */

#define SCSO_KCQOTHER_MISCOMPARE \
    0x0e1d00 /* Miscompare - during verify byte check operation */

/* SSCSI Status Codes *******************************************************/

#define SCSI_STATUS_OK               0x00 /* OK */
#define SCSI_STATUS_CHECKCONDITION   0x02 /* Check condition */
#define SCSI_STATUS_CONDITIONMET     0x04 /* Condition met */
#define SCSI_STATUS_BUSY             0x08 /* Busy */
#define SCSI_STATUS_INTERMEDIATE     0x10 /* Intermediate */
#define SCSI_STATUS_DATAOVERUNDERRUN 0x12 /* Data Under/Over Run? */
#define SCSI_STATUS_INTERMEDIATECONDITIONMET \
    0x14                                     /* Intermediate - Condition met */
#define SCSI_STATUS_RESERVATIONCONFLICT 0x18 /* Reservation conflict */
#define SCSI_STATUS_COMMANDTERMINATED   0x22 /* Command terminated */
#define SCSI_STATUS_QUEUEFULL           0x28 /* Queue (task set) full */
#define SCSI_STATUS_ACAACTIVE           0x30 /* ACA active */
#define SCSI_STATUS_TASKABORTED         0x40 /* Task aborted */

/* Definitions for selected SCSI commands ***********************************/

/* Inquiry */

#define SCSICMD_INQUIRYFLAGS_EVPD 0x01    /* Bit 0: EVPD */
                                          /* Bits 5-7: Peripheral Qualifier */
#define SCSIRESP_INQUIRYPQ_CONNECTED 0x00 /*   000: Device is connected */
#define SCSIRESP_INQUIRYPQ_NOTCONNECTED \
    0x20                                   /*   001: Device is NOT connected */
#define SCSIRESP_INQUIRYPQ_NOTCAPABLE 0x60 /*   011: LUN not supported */
                                           /* Bits 0-4: Peripheral Device */
#define SCSIRESP_INQUIRYPD_DIRECTACCESS 0x00 /*   Direct-access block device */
#define SCSIRESP_INQUIRYPD_SEQUENTIALACCESS \
    0x01                                  /*   Sequential-access block device */
#define SCSIRESP_INQUIRYPD_PRINTER   0x02 /*   Printer device */
#define SCSIRESP_INQUIRYPD_PROCESSOR 0x03 /*   Processor device */
#define SCSIRESP_INQUIRYPD_WRONCE    0x04 /*   Write once device */
#define SCSIRESP_INQUIRYPD_CDDVD     0x05 /*   CD/DVD device */
#define SCSIRESP_INQUIRYPD_SCANNER   0x06 /*   Scanner device (obsolete) */
#define SCSIRESP_INQUIRYPD_OPTICAL   0x07 /*   Optical memory device */
#define SCSIRESP_INQUIRYPD_MEDIUMCHANGER \
    0x08 /*   Medium changer device (Jukebox) */
#define SCSIRESP_INQUIRYPD_COMMUNICATIONS \
    0x09 /*   Communications device (obsolete) */
#define SCSIRESP_INQUIRYPD_STORAGEARRAY \
    0x0c /*   Storage array controller device */
#define SCSIRESP_INQUIRYPD_ENCLOSURESERVICES \
    0x0d                                /*   Enclosure services device */
#define SCSIRESP_INQUIRYPD_RBC     0x0e /*   Simplified direct-access device */
#define SCSIRESP_INQUIRYPD_OCRW    0x0f /*   Optical reader/writer device */
#define SCSIRESP_INQUIRYPD_BCC     0x10 /*   Bridge controller commands */
#define SCSIRESP_INQUIRYPD_OSD     0x11 /*   Object-based storage device */
#define SCSIRESP_INQUIRYPD_ADC     0x12 /*   Automation/drive interface */
#define SCSIRESP_INQUIRYPD_WKLU    0x1e /*   Well-known logical unit */
#define SCSIRESP_INQUIRYPD_UNKNOWN 0x1f /*   Direct-access block device */

#define SCSIRESP_INQUIRYFLAGS1_RMB     0x80 /* Bit 7: RMB */
#define SCSIRESP_INQUIRYFLAGS2_NORMACA 0x20 /* Bit 5: NormACA */
#define SCSIRESP_INQUIRYFLAGS2_HISUP   0x10 /* Bit 4: HiSup */
#define SCSIRESP_INQUIRYFLAGS2_FMTMASK 0x0f /* Bits 0-3: Response data format */

#define SCSIRESP_INQUIRYFLAGS3_SCCS     0x80 /* Bit 8: SCCS */
#define SCSIRESP_INQUIRYFLAGS3_ACC      0x40 /* Bit 7: ACC */
#define SCSIRESP_INQUIRYFLAGS3_TPGSMASK 0x30 /* Bits 4-5: TPGS */
#define SCSIRESP_INQUIRYFLAGS3_3PC      0x08 /* Bit 3: 3PC */
#define SCSIRESP_INQUIRYFLAGS3_PROTECT  0x01 /* Bit 0: Protect */

#define SCSIRESP_INQUIRYFLAGS4_BQUE    0x80 /* Bit 7: BQue */
#define SCSIRESP_INQUIRYFLAGS4_ENCSERV 0x40 /* Bit 6: EncServ */
#define SCSIRESP_INQUIRYFLAGS4_VS      0x20 /* Bit 5: VS */
#define SCSIRESP_INQUIRYFLAGS4_MULTIP  0x10 /* Bit 4: MultIP */
#define SCSIRESP_INQUIRYFLAGS4_MCHNGR  0x08 /* Bit 3: MChngr */
#define SCSIRESP_INQUIRYFLAGS4_ADDR16  0x01 /* Bit 0: Addr16 */

#define SCSIRESP_INQUIRYFLAGS5_WBUS16   0x20 /* Bit 5: WBus16 */
#define SCSIRESP_INQUIRYFLAGS5_SYNC     0x10 /* Bit 4: SYNC */
#define SCSIRESP_INQUIRYFLAGS5_LINKED   0x08 /* Bit 3: LINKED */
#define SCSIRESP_INQUIRYFLAGS5_CMDQUEUE 0x02 /* Bit 1: CmdQue */
#define SCSIRESP_INQUIRYFLAGS5_VS       0x01 /* Bit 0: VS */

#define SCSIRESP_INQUIRYFLAGS6_CLOCKINGMASK 0xc0 /* Bits 2-3: Clocking */
#define SCSIRESP_INQUIRYFLAGS6_QAS          0x02 /* Bit 1: QAS */
#define SCSIRESP_INQUIRYFLAGS6_IUS          0x01 /* Bit 0: IUS */

/* Sense data */

/* Sense data response codes */

#define SCSIRESP_SENSEDATA_CURRENTFIXED \
    0x70 /* Byte 1 is always the response code */
#define SCSIRESP_SENSEDATA_DEFERREDFIXED 0x71
#define SCSIRESP_SENSEDATA_CURRENTDESC   0x72
#define SCSIRESP_SENSEDATA_DEFERREDDESC  0x73

#define SCSIRESP_SENSEDATA_RESPVALID 0x80

/* Fixed sense data flags */

#define SCSIRESP_SENSEDATA_FILEMARK     0x80 /* Bit 7: FileMark */
#define SCSIRESP_SENSEDATA_EOM          0x40 /* Bit 6: EOM */
#define SCSIRESP_SENSEDATA_ILI          0x20 /* Bit 5: ILI */
#define SCSIRESP_SENSEDATA_SENSEKEYMASK 0x0f /* Bits 0-3: Sense key */
#define SCSIRESP_SENSEDATA_NOSENSE      0x00 /*   Nothing to be reported */
#define SCSIRESP_SENSEDATA_RECOVEREDERROR \
    0x01 /*   Successful after recovery action */
#define SCSIRESP_SENSEDATA_NOTREADY 0x02 /*   Logical unit is not accessible */
#define SCSIRESP_SENSEDATA_MEDIUMERROR \
    0x03 /*   Error possibly caused by flaw in medium */
#define SCSIRESP_SENSEDATA_HARDWAREERROR \
    0x04 /*   Non-recoverable hardware error */
#define SCSIRESP_SENSEDATA_ILLEGALREQUEST 0x05 /*   Error in received request */
#define SCSIRESP_SENSEDATA_UNITATTENTION  0x06 /*   Unit attention condition */
#define SCSIRESP_SENSEDATA_DATAPROTECT \
    0x07 /*   Action failed, medium protected */
#define SCSIRESP_SENSEDATA_BLANKCHECK     0x08 /*   Encountered blank media */
#define SCSIRESP_SENSEDATA_VENDORSPECIFIC 0x09 /*   Vendor specific condition */
#define SCSIRESP_SENSEDATA_ABORTEDCOMMAND 0x0b /*   Command was aborted */

#define SCSIRESP_SENSEDATA_KEYVALID 0x80 /* Sense-specific data valid */

/* Mode Select 6 */

#define SCSICMD_MODESELECT6_PF 0x10 /* Bit 4: PF */
#define SCSICMD_MODESELECT6_SP 0x01 /* Bit 0: SP */

/* Mode Sense 6 */

#define SCSICMD_MODESENSE6_DBD 0x08 /* Bit 3: PF */

#define SCSICMD_MODESENSE_PCMASK       0xc0 /* Bits 6-7: Page control (PC) */
#define SCSICMD_MODESENSE_PCCURRENT    0x00 /*   Current values */
#define SCSICMD_MODESENSE_PCCHANGEABLE 0x40 /*   Changeable values */
#define SCSICMD_MODESENSE_PCDEFAULT    0x80 /*   Default values */
#define SCSICMD_MODESENSE_PCSAVED      0xc0 /*   Saved values */
#define SCSICMD_MODESENSE_PGCODEMASK   0x3f /* Bits 0-5: Page code */

#define SCSICMD_MODESENSE6_PCDEFAULT 0x80 /*   Default values */
                                          /* Direct-access device page codes */
#define SCSIRESP_MODESENSE_PGCCODE_VENDOR 0x00 /*   Vendor-specific */
#define SCSIRESP_MODESENSE_PGCCODE_RWERROR \
    0x01 /*   Read/Write error recovery mode page */
#define SCSIRESP_MODESENSE_PGCCODE_RECONNECT \
    0x02 /*   Disconnect-reconnect mode page */
#define SCSIRESP_MODESENSE_PGCCODE_FORMATDEV \
    0x03 /*   Format device mode page (obsolete) */
#define SCSIRESP_MODESENSE_PGCCODE_RIGID \
    0x04 /*   Rigid disk geometry mode page (obsolete) */
#define SCSIRESP_MODESENSE_PGCCODE_FLEXIBLE \
    0x05 /*   Flexible disk geometry mode page (obsolete) */
#define SCSIRESP_MODESENSE_PGCCODE_VERIFY \
    0x07 /*   Verify error recovery mode page */
#define SCSIRESP_MODESENSE_PGCCODE_CACHING 0x08 /*   Caching mode page */
#define SCSIRESP_MODESENSE_PGCCODE_CONTROL \
    0x0a /*   Control mode page (0x0a/0x00) */
#define SCSIRESP_MODESENSE_PGCCODE_CONTROLEXT \
    0x0a /*   Control extension mode page (0x0a/0x01) */
#define SCSIRESP_MODESENSE_PGCCODE_MEDIUMTYPES \
    0x0b /*   Medum types supported mode page (obsolete) */
#define SCSIRESP_MODESENSE_PGCCODE_NP \
    0x0c /*   Notch and partition mode page (obsolete) */
#define SCSIRESP_MODESENSE_PGCCODE_XOR 0x10 /*   XOR control mode page */
#define SCSIRESP_MODESENSE_PGCCODE_ES  0x14 /*   Enclosure services mode page */
#define SCSIRESP_MODESENSE_PGCCODE_PSLUN \
    0x18 /*   Protocol-specific LUN mode page */
#define SCSIRESP_MODESENSE_PGCCODE_PSPORT \
    0x19 /*   Protocol-specific port mode page */
#define SCSIRESP_MODESENSE_PGCCODE_POWER 0x1a /*   Power condition mode page */
#define SCSIRESP_MODESENSE_PGCCODE_IE \
    0x1c /*   Informational exceptions control mode page (0x1c/0x00) */
#define SCSIRESP_MODESENSE_PGCCODE_BC \
    0x1c /*   Background control mode page (0x1c/0x01) */
#define SCSIRESP_MODESENSE_PGCCODE_RETURNALL 0x3f /*   Return all mode pages */
/* Direct-access caching mode page */
#define SCSIRESP_CACHINGMODEPG_PS   0x80 /*   Byte 0, Bit 7: PS */
#define SCSIRESP_CACHINGMODEPG_SPF  0x60 /*   Byte 0, Bit 6: SPF */
#define SCSIRESP_CACHINGMODEPG_IC   0x80 /*   Byte 2, Bit 7: IC */
#define SCSIRESP_CACHINGMODEPG_ABPF 0x40 /*   Byte 2, Bit 6: ABPF */
#define SCSIRESP_CACHINGMODEPG_CAP  0x20 /*   Byte 2, Bit 5: CAP */
#define SCSIRESP_CACHINGMODEPG_DISC 0x10 /*   Byte 2, Bit 4: DISC */
#define SCSIRESP_CACHINGMODEPG_SIZE 0x08 /*   Byte 2, Bit 3: SIZE */
#define SCSIRESP_CACHINGMODEPG_WCE \
    0x04 /*   Byte 2, Bit 2: Write cache enable (WCE) */
#define SCSIRESP_CACHINGMODEPG_MF 0x02 /*   Byte 2, Bit 1: MF */
#define SCSIRESP_CACHINGMODEPG_RCD \
    0x01 /*   Byte 2, Bit 0: Read cache disable (RCD) */

#define SCSIRESP_MODEPARMHDR_DAPARM_WP \
    0x80 /* Bit 7: WP (Direct-access block devices only) */
#define SCSIRESP_MODEPARMHDR_DAPARM_DBPFUA \
    0x10 /* Bit 4: DBOFUA (Direct-access block devices only) */

#define SCSIRESP_PAGEFMT_PS         0x80 /* Bit 7: PS */
#define SCSIRESP_PAGEFMT_SPF        0x40 /* Bit 6: SPF */
#define SCSIRESP_PAGEFMT_PGCODEMASK 0x3f /* Bits 0-5: Page code */

/* Prevent / Allow Medium Removal */

#define SCSICMD_PREVENTMEDIUMREMOVAL_TRANSPORT \
    0x01 /* Removal prohibited from data transport */
#define SCSICMD_PREVENTMEDIUMREMOVAL_MCHANGER \
    0x02 /* Removal prohibited from medium changer */

/* Read format capacities */

#define SCIRESP_RDFMTCAPACITIES_UNFORMATED 0x01 /* Unformatted media */
#define SCIRESP_RDFMTCAPACITIES_FORMATED   0x02 /* Formatted media */
#define SCIRESP_RDFMTCAPACITIES_NOMEDIA    0x03 /* No media */

/* Read 6 */

#define SCSICMD_READ6_MSLBAMASK 0x1f

/* Write 6 */

#define SCSICMD_WRITE6_MSLBAMASK 0x1f

/* Mode Select 10 */

#define SCSICMD_MODESELECT10_PF 0x10 /* Bit 4: PF */
#define SCSICMD_MODESELECT10_SP 0x01 /* Bit 0: SP */

/* Mode Sense 10 */

#define SCSICMD_MODESENSE10_LLBAA 0x10 /* Bit 4: LLBAA */
#define SCSICMD_MODESENSE10_DBD   0x08 /* Bit 3: PF */

/* Read 10 */

#define SCSICMD_READ10FLAGS_RDPROTECTMASK 0xe0
#define SCSICMD_READ10FLAGS_DPO           0x10 /* Disable Page Out */
#define SCSICMD_READ10FLAGS_FUA           0x08
#define SCSICMD_READ10FLAGS_FUANV         0x02

/* Write 10 */

#define SCSICMD_WRITE10FLAGS_WRPROTECTMASK 0xe0
#define SCSICMD_WRITE10FLAGS_DPO           0x10 /* Disable Page Out */
#define SCSICMD_WRITE10FLAGS_FUA           0x08
#define SCSICMD_WRITE10FLAGS_FUANV         0x02

/* Verify 10 */

#define SCSICMD_VERIFY10_VRPROTECTMASK 0xe0 /* Byte 1: Bits 5-7: VRPROTECT */
#define SCSICMD_VERIFY10_DPO           0x10 /* Byte 1: Bit 4: Disable Page Out (DPO) */
#define SCSICMD_VERIFY10_BYTCHK        0x02 /* Byte 1: Bit 2: BytChk */

/* Read 12 */

#define SCSICMD_READ12FLAGS_RDPROTECTMASK 0xe0
#define SCSICMD_READ12FLAGS_DPO           0x10 /* Disable Page Out */
#define SCSICMD_READ12FLAGS_FUA           0x08
#define SCSICMD_READ12FLAGS_FUANV         0x02

/* Write 12 */

#define SCSICMD_WRITE12FLAGS_WRPROTECTMASK 0xe0
#define SCSICMD_WRITE12FLAGS_DPO           0x10 /* Disable Page Out */
#define SCSICMD_WRITE12FLAGS_FUA           0x08
#define SCSICMD_WRITE12FLAGS_FUANV         0x02

/* Verify 12 */

#define SCSICMD_VERIFY12_VRPROTECTMASK 0xe0 /* Byte 1: Bits 5-7: VRPROTECT */
#define SCSICMD_VERIFY12_DPO           0x10 /* Byte 1: Bit 4: Disable Page Out (DPO) */
#define SCSICMD_VERIFY12_BYTCHK        0x02 /* Byte 1: Bit 2: BytChk */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Format structures for selected SCSI primary commands */

#define SCSICMD_TESTUNITREADY_SIZEOF 6

struct scsicmd_requestsense_s {
    u8 opcode;      /* 0: 0x03 */
    u8 flags;       /* 1: See SCSICMD_REQUESTSENSE_FLAGS_* */
    u8 reserved[2]; /* 2-3: Reserved */
    u8 alloclen;    /* 4: Allocation length */
    u8 control;     /* 5: Control */
};
#define SCSICMD_REQUESTSENSE_SIZEOF 6
#define SCSICMD_REQUESTSENSE_MSSIZEOF \
    12 /* MS-Windows REQUEST SENSE with cbw->cdblen == 12 */

struct scsiresp_fixedsensedata_s {
    u8 code;       /* 0: Response code See  SCSIRESP_SENSEDATA_*FIXED defns */
    u8 obsolete;   /* 1: */
    u8 flags;      /* 2: See SCSIRESP_SENSEDATA_* definitions */
    u8 info[4];    /* 3-6: Information */
    u8 len;        /* 7: Additional length */
    u8 cmdinfo[4]; /* 8-11: Command-specific information */
    u8 code2;      /* 12: Additional sense code */
    u8 qual2;      /* 13: Additional sense code qualifier */
    u8 fru;        /* 14: Field replacement unit code */
    u8 key[3];     /* 15-17: Sense key specific */
                   /* 18-: Additional bytes may follow */
};
#define SCSIRESP_FIXEDSENSEDATA_SIZEOF 18 /* Minimum size */

struct scscicmd_inquiry_s {
    u8 opcode;      /* 0: 0x12 */
    u8 flags;       /* 1: See SCSICMD_INQUIRY_FLAGS_* */
    u8 pagecode;    /* 2: Page code */
    u8 alloclen[2]; /* 3-4: Allocation length */
    u8 control;     /* 5: Control */
};
#define SCSICMD_INQUIRY_SIZEOF 6

struct scsiresp_inquiry_s {
    /* Mandatory */

    u8 qualtype; /* 0: Bits 5-7: Peripheral qualifier; Bits 0-4: Peripheral device type */
    u8 flags1;        /* 1: See SCSIRESP_INQUIRY_FLAGS1_* */
    u8 version;       /* 2: Version */
    u8 flags2;        /* 3: See SCSIRESP_INQUIRY_FLAGS2_* */
    u8 len;           /* 4: Additional length */
    u8 flags3;        /* 5: See SCSIRESP_INQUIRY_FLAGS3_* */
    u8 flags4;        /* 6: See SCSIRESP_INQUIRY_FLAGS4_* */
    u8 flags5;        /* 7: See SCSIRESP_INQUIRY_FLAGS5_* */
    u8 vendorid[8];   /* 8-15: T10 Vendor Identification */
    u8 productid[16]; /* 16-31: Product Identification */
    u8 revision[4];   /* 32-35: Product Revision Level */

    /* Optional */

    u8 vendor[20];    /* 36-55: Vendor specific */
    u8 flags6;        /* 56: See SCSIRESP_INQUIRY_FLAGS6_* */
    u8 reserved1;     /* 57: Reserved */
    u8 version1[2];   /* 58-59: Version Descriptor 1 */
    u8 version2[2];   /* 60-61: Version Descriptor 2 */
    u8 version3[2];   /* 62-63: Version Descriptor 3 */
    u8 version4[2];   /* 64-65: Version Descriptor 4 */
    u8 version5[2];   /* 66-67: Version Descriptor 5 */
    u8 version6[2];   /* 68-69: Version Descriptor 6 */
    u8 version7[2];   /* 70-71: Version Descriptor 7 */
    u8 version8[2];   /* 72-73: Version Descriptor 8 */
    u8 reserved2[22]; /* 74-95: Reserved */
                      /* 96-: Vendor-specific parameters may follow */
};
#define SCSIRESP_INQUIRY_SIZEOF 36 /* Minimum size */

struct scsicmd_modeselect6_s {
    u8 opcode;      /* 0x15 */
    u8 flags;       /* 1: See SCSICMD_MODESELECT6_FLAGS_* */
    u8 reserved[2]; /* 2-3: Reserved */
    u8 plen;        /* 4: Parameter list length */
    u8 control;     /* 5: Control */
};
#define SCSICMD_MODESELECT6_SIZEOF 6

struct scsicmd_modesense6_s {
    u8 opcode;    /* 0x1a */
    u8 flags;     /* 1: See SCSICMD_MODESENSE6_FLAGS_* */
    u8 pcpgcode;  /* 2: Bits 6-7: PC, bits 0-5: page code */
    u8 subpgcode; /* 3: subpage code */
    u8 alloclen;  /* 4: Allocation length */
    u8 control;   /* 5: Control */
};
#define SCSICMD_MODESENSE6_SIZEOF 6

struct scsiresp_modeparameterhdr6_s {
    u8 mdlen; /* 0: Mode data length */
    u8 type;  /* 1: Medium type */
    u8 param; /* 2: Device-specific parameter */
    u8 bdlen; /* 3: Block descriptor length */
};
#define SCSIRESP_MODEPARAMETERHDR6_SIZEOF 4

struct scsiresp_blockdesc_s {
    u8 density;    /* 0: density code */
    u8 nblocks[3]; /* 1-3: Number of blocks */
    u8 reserved;   /* 4: reserved */
    u8 blklen[3];  /* 5-7: Block len */
};
#define SCSIRESP_BLOCKDESC_SIZEOF 8

struct scsiresp_pageformat_s {
    u8 pgcode;   /* 0: See SCSIRESP_PAGEFMT_* definitions */
    u8 pglen;    /* 1: Page length (n-1) */
    u8 parms[1]; /* 2-n: Mode parameters */
};

struct scsiresp_subpageformat_s {
    u8 pgcode;    /* 0: See SCSIRESP_PAGEFMT_* definitions */
    u8 subpgcode; /* 1: sub-page code */
    u8 pglen[2];  /* 2-3: Page length (n-3) */
    u8 parms[1];  /* 4-n: Mode parameters */
};

struct scsiresp_cachingmodepage_s {
    u8 pgcode;   /* 0: Bit 7: PS; Bit 6: SPF, Bits 0-5: page code == 8 */
    u8 len;      /* 1: Page length (18) */
    u8 flags1;   /* 2: See SCSIRESP_CACHINGMODEPG_* definitions */
    u8 priority; /* 3: Bits 4-7: Demand read retention priority; Bits 0-3: Write retention priority */
    u8 dpflen[2];   /* 4-5: Disable prefetch transfer length */
    u8 minpf[2];    /* 6-7: Minimum pre-fetch */
    u8 maxpf[2];    /* 8-9: Maximum pre-fetch */
    u8 maxpfc[2];   /* 10-11: Maximum pref-fetch ceiling */
    u8 flags2;      /* 12: See SCSIRESP_CACHINGMODEPG_* definitions */
    u8 nsegments;   /* 13: Number of cache segments */
    u8 segsize[2];  /* 14-15: Cache segment size */
    u8 reserved;    /* 16: Reserved */
    u8 obsolete[3]; /* 17-19: Obsolete */
};

/* Format structures for selected SCSI block commands */

struct scsicmd_read6_s {
    u8 opcode; /* 0: 0x08 */
    u8 mslba; /* 1: Bits 5-7: reserved; Bits 0-6: MS Logical Block Address (LBA) */
    u8 lslba[2]; /* 2-3: LS Logical Block Address (LBA) */
    u8 xfrlen;   /* 4: Transfer length (in contiguous logical blocks) */
    u8 control;  /* 5: Control */
};
#define SCSICMD_READ6_SIZEOF 6

struct scsicmd_write6_s {
    u8 opcode; /* 0: 0x0a */
    u8 mslba; /* 1: Bits 5-7: reserved; Bits 0-6: MS Logical Block Address (LBA) */
    u8 lslba[2]; /* 2-3: LS Logical Block Address (LBA) */
    u8 xfrlen;   /* 4: Transfer length (in contiguous logical blocks) */
    u8 control;  /* 5: Control */
};
#define SCSICMD_WRITE6_SIZEOF 6

struct scsicmd_startstopunit_s {
    u8 opcode;   /* 0: 0x1b */
    u8 immed;    /* 1: Bits 2-7: Reserved, Bit 0: Immed */
    u8 reserved; /* 2: reserved */
    u8 pcm;      /* 3: Bits 4-7: Reserved, Bits 0-3: Power condition modifier */
    u8 pc; /* 4: Bits 4-7: Power condition, Bit 2: NO_FLUSH, Bit 1: LOEJ, Bit 0: START */
    u8 control; /* 5: Control */
};
#define SCSICMD_STARTSTOPUNIT_SIZEOF 6

struct scsicmd_preventmediumremoval_s {
    u8 opcode;      /* 0: 0x1e */
    u8 reserved[3]; /* 1-3: Reserved */
    u8 prevent;     /* 4: Bits 2-7: Reserved, Bits 0:1: prevent */
    u8 control;     /* 5: Control */
};
#define SCSICMD_PREVENTMEDIUMREMOVAL_SIZEOF 6

struct scsicmd_readformatcapcacities_s {
    u8 opcode;      /* 0: 0x23 */
    u8 reserved[6]; /* 1-6: Reserved */
    u8 alloclen[2]; /* 7-8: Allocation length */
    u8 control;     /* 9: Control */
};
#define SCSICMD_READFORMATCAPACITIES_SIZEOF 10

struct scsiresp_readformatcapacities_s {
    /* Current capacity header */

    u8 reserved[3]; /* 0-2: Reserved */
    u8 listlen;     /* 3: Capacity list length */

    /* Current/Maximum Capacity Descriptor (actually a separate structure) */

    u8 nblocks[4];  /* 4-7: Number of blocks */
    u8 type;        /* 8: Bits 2-7: Reserved, Bits 0-1: Descriptor type */
    u8 blocklen[3]; /* 9-11: Block length */
};
#define SCSIRESP_READFORMATCAPACITIES_SIZEOF 12
#define SCSIRESP_CURRCAPACITYDESC_SIZEOF     8

struct scsiresp_formattedcapacitydesc_s {
    u8 nblocks[4]; /* 0-3: Number of blocks */
    u8 type;       /* 4: Bits 2-7: Type, bits 0-1, reserved */
    u8 param[3];   /* 5-7: Type dependent parameter */
};
#define SCSIRESP_FORMATTEDCAPACITYDESC_SIZEOF 8

struct scsicmd_readcapacity10_s {
    u8 opcode;       /* 0: 0x25 */
    u8 reserved1;    /* 1: Bits 1-7: Reserved, Bit 0: Obsolete */
    u8 lba[4];       /* 2-5: Logical block address (LBA) */
    u8 reserved2[2]; /* 6-7: Reserved */
    u8 pmi;          /* 8: Bits 1-7 Reserved; Bit 0: PMI */
    u8 control;      /* 9: Control */
};
#define SCSICMD_READCAPACITY10_SIZEOF 10

struct scsiresp_readcapacity10_s {
    u8 lba[4];    /* 0-3: Returned logical block address (LBA) */
    u8 blklen[4]; /* 4-7: Logical block length (in bytes) */
};
#define SCSIRESP_READCAPACITY10_SIZEOF 8

struct scsicmd_read10_s {
    u8 opcode;    /* 0: 0x28 */
    u8 flags;     /* 1: See SCSICMD_READ10FLAGS_* */
    u8 lba[4];    /* 2-5: Logical Block Address (LBA) */
    u8 groupno;   /* 6: Bits 5-7: reserved; Bits 0-6: group number */
    u8 xfrlen[2]; /* 7-8: Transfer length (in contiguous logical blocks) */
    u8 control;   /* 9: Control */
};
#define SCSICMD_READ10_SIZEOF 10

struct scsicmd_write10_s {
    u8 opcode;    /* 0: 0x2a */
    u8 flags;     /* 1: See SCSICMD_WRITE10FLAGS_* */
    u8 lba[4];    /* 2-5: Logical Block Address (LBA) */
    u8 groupno;   /* 6: Bits 5-7: reserved; Bits 0-6: group number */
    u8 xfrlen[2]; /* 7-8: Transfer length (in contiguous logical blocks) */
    u8 control;   /* 9: Control */
};
#define SCSICMD_WRITE10_SIZEOF 10

struct scsicmd_verify10_s {
    u8 opcode; /* 0: 0x2f */
    u8 flags;  /* 1: See SCSICMD_VERIFY10_* definitions */
    u8 lba[4]; /* 2-5: Logical block address (LBA) */
    u8 groupno; /* 6: Bit 7: restricted; Bits 5-6: Reserved, Bits 0-4: Group number */
    u8 len[2];  /* 7-8: Verification length (in blocks) */
    u8 control; /* 9: Control */
};
#define SCSICMD_VERIFY10_SIZEOF 10

struct scsicmd_synchronizecache10_s {
    u8 opcode; /* 0: 0x35 */
    u8 flags;  /* 1: See SCSICMD_SYNCHRONIZECACHE10_* definitions */
    u8 lba[4]; /* 2-5: Logical block address (LBA) */
    u8 groupno; /* 6: Bit 7: restricted; Bits 5-6: Reserved, Bits 0-4: Group number */
    u8 len[2];  /* 7-8: Number of logical blocks */
    u8 control; /* 9: Control */
};
#define SCSICMD_SYNCHRONIZECACHE10_SIZEOF 10

struct scsicmd_modeselect10_s {
    u8 opcode;      /* 0: 0x55 */
    u8 flags;       /* 1: See SCSICMD_MODESELECT10_FLAGS_* */
    u8 reserved[5]; /* 2-6: Reserved */
    u8 parmlen[2];  /* 7-8: Parameter list length */
    u8 control;     /* 9: Control */
};
#define SCSICMD_MODESELECT10_SIZEOF 10

struct scsiresp_modeparameterhdr10_s {
    u8 mdlen[2];    /* 0-1: Mode data length */
    u8 type;        /* 2: Medium type */
    u8 param;       /* 3: Device-specific parameter */
    u8 reserved[2]; /* 4-5: reserved */
    u8 bdlen[2];    /* 6-7: Block descriptor length */
};
#define SCSIRESP_MODEPARAMETERHDR10_SIZEOF 8

struct scsicmd_modesense10_s {
    u8 opcode;      /* O: 0x5a */
    u8 flags;       /* 1: See SCSICMD_MODESENSE10_FLAGS_* */
    u8 pcpgcode;    /* 2: Bits 6-7: PC, bits 0-5: page code */
    u8 subpgcode;   /* 3: subpage code */
    u8 reserved[3]; /* 4-6: reserved */
    u8 alloclen[2]; /* 7-8: Allocation length */
    u8 control;     /* 9: Control */
};
#define SCSICMD_MODESENSE10_SIZEOF 10

struct scsicmd_readcapacity16_s {
    u8 opcode;   /* 0: 0x9e */
    u8 action;   /* 1: Bits 5-7: Reserved, Bits 0-4: Service action */
    u8 lba[8];   /* 2-9: Logical block address (LBA) */
    u8 len[4];   /* 10-13: Allocation length */
    u8 reserved; /* 14: Reserved */
    u8 control;  /* 15: Control */
};
#define SCSICMD_READCAPACITY16_SIZEOF 16

struct scsicmd_read12_s {
    u8 opcode;    /* 0: 0xa8 */
    u8 flags;     /* 1: See SCSICMD_READ12FLAGS_* */
    u8 lba[4];    /* 2-5: Logical Block Address (LBA) */
    u8 xfrlen[4]; /* 6-9: Transfer length (in contiguous logical blocks) */
    u8 groupno; /* 10: Bit 7: restricted; Bits 5-6: reserved; Bits 0-6: group number */
    u8 control; /* 11: Control */
};
#define SCSICMD_READ12_SIZEOF 12

struct scsicmd_write12_s {
    u8 opcode;    /* 0: 0xaa */
    u8 flags;     /* 1: See SCSICMD_WRITE12FLAGS_* */
    u8 lba[4];    /* 2-5: Logical Block Address (LBA) */
    u8 xfrlen[4]; /* 6-9: Transfer length (in contiguous logical blocks) */
    u8 groupno; /* 10: Bit 7: restricted; Bits 5-6: reserved; Bits 0-6: group number */
    u8 control; /* 11: Control */
};
#define SCSICMD_WRITE12_SIZEOF 12

struct scsicmd_verify12_s {
    u8 opcode; /* 0: 0xaf */
    u8 flags;  /* 1: See SCSICMD_VERIFY12_* definitions */
    u8 lba[4]; /* 2-5: Logical block address (LBA) */
    u8 len[4]; /* 6-9: Verification length */
    u8 groupno; /* 10: Bit 7: restricted; Bits 5-6: Reserved, Bits 0-4: Group number */
    u8 control; /* 11: Control */
};
#define SCSICMD_VERIFY12_SIZEOF 12

/****************************************************************************
 * Public Functions Definitions
 ****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_SCSI_H */
