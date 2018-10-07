/** @file
  Copyright (C) 2016 - 2017, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Base.h>

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 mOcPrintLibHexStr[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *CONST mOcStatusString[] = {
  "Success",                   //  RETURN_SUCCESS               = 0
  "Warning Unknown Glyph",     //  RETURN_WARN_UNKNOWN_GLYPH    = 1
  "Warning Delete Failure",    //  RETURN_WARN_DELETE_FAILURE   = 2
  "Warning Write Failure",     //  RETURN_WARN_WRITE_FAILURE    = 3
  "Warning Buffer Too Small",  //  RETURN_WARN_BUFFER_TOO_SMALL = 4
  "Warning Stale Data",        //  RETURN_WARN_STALE_DATA       = 5
  "Load Error",                //  RETURN_LOAD_ERROR            = 1  | MAX_BIT
  "Invalid Parameter",         //  RETURN_INVALID_PARAMETER     = 2  | MAX_BIT
  "Unsupported",               //  RETURN_UNSUPPORTED           = 3  | MAX_BIT
  "Bad Buffer Size",           //  RETURN_BAD_BUFFER_SIZE       = 4  | MAX_BIT
  "Buffer Too Small",          //  RETURN_BUFFER_TOO_SMALL,     = 5  | MAX_BIT
  "Not Ready",                 //  RETURN_NOT_READY             = 6  | MAX_BIT
  "Device Error",              //  RETURN_DEVICE_ERROR          = 7  | MAX_BIT
  "Write Protected",           //  RETURN_WRITE_PROTECTED       = 8  | MAX_BIT
  "Out of Resources",          //  RETURN_OUT_OF_RESOURCES      = 9  | MAX_BIT
  "Volume Corrupt",            //  RETURN_VOLUME_CORRUPTED      = 10 | MAX_BIT
  "Volume Full",               //  RETURN_VOLUME_FULL           = 11 | MAX_BIT
  "No Media",                  //  RETURN_NO_MEDIA              = 12 | MAX_BIT
  "Media changed",             //  RETURN_MEDIA_CHANGED         = 13 | MAX_BIT
  "Not Found",                 //  RETURN_NOT_FOUND             = 14 | MAX_BIT
  "Access Denied",             //  RETURN_ACCESS_DENIED         = 15 | MAX_BIT
  "No Response",               //  RETURN_NO_RESPONSE           = 16 | MAX_BIT
  "No mapping",                //  RETURN_NO_MAPPING            = 17 | MAX_BIT
  "Time out",                  //  RETURN_TIMEOUT               = 18 | MAX_BIT
  "Not started",               //  RETURN_NOT_STARTED           = 19 | MAX_BIT
  "Already started",           //  RETURN_ALREADY_STARTED       = 20 | MAX_BIT
  "Aborted",                   //  RETURN_ABORTED               = 21 | MAX_BIT
  "ICMP Error",                //  RETURN_ICMP_ERROR            = 22 | MAX_BIT
  "TFTP Error",                //  RETURN_TFTP_ERROR            = 23 | MAX_BIT
  "Protocol Error",            //  RETURN_PROTOCOL_ERROR        = 24 | MAX_BIT
  "Incompatible Version",      //  RETURN_INCOMPATIBLE_VERSION  = 25 | MAX_BIT
  "Security Violation",        //  RETURN_SECURITY_VIOLATION    = 26 | MAX_BIT
  "CRC Error",                 //  RETURN_CRC_ERROR             = 27 | MAX_BIT
  "End of Media",              //  RETURN_END_OF_MEDIA          = 28 | MAX_BIT
  "Reserved (29)",             //  RESERVED                     = 29 | MAX_BIT
  "Reserved (30)",             //  RESERVED                     = 30 | MAX_BIT
  "End of File",               //  RETURN_END_OF_FILE           = 31 | MAX_BIT
  "Invalid Language",          //  RETURN_INVALID_LANGUAGE      = 32 | MAX_BIT
  "Compromised Data"           //  RETURN_COMPROMISED_DATA      = 33 | MAX_BIT
};
