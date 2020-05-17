/** @file
  When installed, the Framework MP Services Protocol produces a collection of
  services that are needed for MP management, such as initialization and management
  of application processors.

  @par Note:
    This protocol has been deprecated and has been replaced by the MP Services
    Protocol from the UEFI Platform Initialization Specification 1.2, Volume 2:
    Driver Execution Environment Core Interface.

  The MP Services Protocol provides a generalized way of performing following tasks:
    - Retrieving information of multi-processor environment and MP-related status of
      specific processors.
    - Dispatching user-provided function to APs.
    - Maintain MP-related processor status.

  The MP Services Protocol must be produced on any system with more than one logical
  processor.

  The Protocol is available only during boot time.

  MP Services Protocol is hardware-independent. Most of the logic of this protocol
  is architecturally neutral. It abstracts the multi-processor environment and
  status of processors, and provides interfaces to retrieve information, maintain,
  and dispatch.

  MP Services Protocol may be consumed by ACPI module. The ACPI module may use this
  protocol to retrieve data that are needed for an MP platform and report them to OS.
  MP Services Protocol may also be used to program and configure processors, such
  as MTRR synchronization for memory space attributes setting in DXE Services.
  MP Services Protocol may be used by non-CPU DXE drivers to speed up platform boot
  by taking advantage of the processing capabilities of the APs, for example, using
  APs to help test system memory in parallel with other device initialization.
  Diagnostics applications may also use this protocol for multi-processor.

Copyright (c) 1999 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FRAMEWORK_MP_SERVICE_PROTOCOL_H_
#define _FRAMEWORK_MP_SERVICE_PROTOCOL_H_

///
/// Global ID for the FRAMEWORK_EFI_MP_SERVICES_PROTOCOL.
///
#define FRAMEWORK_EFI_MP_SERVICES_PROTOCOL_GUID \
  { \
    0xf33261e7, 0x23cb, 0x11d5, {0xbd, 0x5c, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81} \
  }

///
/// Forward declaration for the EFI_MP_SERVICES_PROTOCOL.
///
typedef struct _FRAMEWORK_EFI_MP_SERVICES_PROTOCOL FRAMEWORK_EFI_MP_SERVICES_PROTOCOL;

///
/// Fixed delivery mode that may be used as the DeliveryMode parameter in SendIpi().
///
#define DELIVERY_MODE_FIXED           0x0

///
/// Lowest priority delivery mode that may be used as the DeliveryMode parameter in SendIpi().
///
#define DELIVERY_MODE_LOWEST_PRIORITY 0x1

///
/// SMI delivery mode that may be used as the DeliveryMode parameter in SendIpi().
///
#define DELIVERY_MODE_SMI             0x2

///
/// Remote read delivery mode that may be used as the DeliveryMode parameter in SendIpi().
///
#define DELIVERY_MODE_REMOTE_READ     0x3

///
/// NMI delivery mode that may be used as the DeliveryMode parameter in SendIpi().
///
#define DELIVERY_MODE_NMI             0x4

///
/// INIT delivery mode that may be used as the DeliveryMode parameter in SendIpi().
///
#define DELIVERY_MODE_INIT            0x5

///
/// Startup IPI delivery mode that may be used as the DeliveryMode parameter in SendIpi().
///
#define DELIVERY_MODE_SIPI            0x6

///
/// The DeliveryMode parameter in SendIpi() must be less than this maximum value.
///
#define DELIVERY_MODE_MAX             0x7

///
/// IPF specific value for the state field of the Self Test State Parameter.
///
#define EFI_MP_HEALTH_FLAGS_STATUS_HEALTHY                  0x0

///
/// IPF specific value for the state field of the Self Test State Parameter.
///
#define EFI_MP_HEALTH_FLAGS_STATUS_PERFORMANCE_RESTRICTED   0x1

///
/// IPF specific value for the state field of the Self Test State Parameter.
///
#define EFI_MP_HEALTH_FLAGS_STATUS_FUNCTIONALLY_RESTRICTED  0x2

typedef union {
  ///
  /// Bitfield structure for the IPF Self Test State Parameter.
  ///
  struct {
    UINT32  Status:2;
    UINT32  Tested:1;
    UINT32  Reserved1:13;
    UINT32  VirtualMemoryUnavailable:1;
    UINT32  Ia32ExecutionUnavailable:1;
    UINT32  FloatingPointUnavailable:1;
    UINT32  MiscFeaturesUnavailable:1;
    UINT32  Reserved2:12;
  } Bits;
  ///
  /// IA32 and X64 BIST data of the processor.
  ///
  UINT32  Uint32;
} EFI_MP_HEALTH_FLAGS;

typedef struct {
  ///
  /// @par IA32, X64:
  ///   BIST (built-in self-test) data of the processor.
  ///
  /// @par IPF:
  ///   Lower 32 bits of the self-test state parameter. For definition of self-test
  ///   state parameter, please refer to Intel(R) Itanium(R) Architecture Software
  ///   Developer's Manual, Volume 2: System Architecture.
  ///
  EFI_MP_HEALTH_FLAGS  Flags;
  ///
  /// @par IA32, X64:
  ///   Not used.
  ///
  /// @par IPF:
  ///   Higher 32 bits of self test state parameter.
  ///
  UINT32               TestStatus;
} EFI_MP_HEALTH;

typedef enum {
  EfiCpuAP                = 0,  ///< The CPU is an AP (Application Processor).
  EfiCpuBSP,                    ///< The CPU is the BSP (Boot-Strap Processor).
  EfiCpuDesignationMaximum
} EFI_CPU_DESIGNATION;

typedef struct {
  ///
  /// @par IA32, X64:
  ///   The lower 8 bits contains local APIC ID, and higher bits are reserved.
  ///
  /// @par IPF:
  ///   The lower 16 bits contains id/eid as physical address of local SAPIC
  ///   unit, and higher bits are reserved.
  ///
  UINT32               ApicID;
  ///
  /// This field indicates whether the processor is enabled.  If the value is
  /// TRUE, then the processor is enabled. Otherwise, it is disabled.
  ///
  BOOLEAN              Enabled;
  ///
  /// This field indicates whether the processor is playing the role of BSP.
  /// If the value is EfiCpuAP, then the processor is AP. If the value is
  /// EfiCpuBSP, then the processor is BSP.
  ///
  EFI_CPU_DESIGNATION  Designation;
  ///
  /// @par IA32, X64:
  ///   The Flags field of this EFI_MP_HEALTH data structure holds BIST (built-in
  ///   self test) data of the processor. The TestStatus field is not used, and
  ///   the value is always zero.
  ///
  /// @par IPF:
  ///   Bit format of this field is the same as the definition of self-test state
  ///   parameter, in Intel(R) Itanium(R) Architecture Software Developer's Manual,
  ///   Volume 2: System Architecture.
  ///
  EFI_MP_HEALTH        Health;
  ///
  /// Zero-based physical package number that identifies the cartridge of the
  /// processor.
  ///
  UINTN                PackageNumber;
  ///
  /// Zero-based physical core number within package of the processor.
  ///
  UINTN                NumberOfCores;
  ///
  /// Zero-based logical thread number within core of the processor.
  ///
  UINTN                NumberOfThreads;
  ///
  /// This field is reserved.
  ///
  UINT64               ProcessorPALCompatibilityFlags;
  ///
  /// @par IA32, X64:
  ///   This field is not used, and the value is always zero.
  ///
  /// @par IPF:
  ///   This field is a mask number that is handed off by the PAL about which
  ///   processor tests are performed and which are masked.
  ///
  UINT64               ProcessorTestMask;
} EFI_MP_PROC_CONTEXT;

/**
  Functions of this type are used with the Framework MP Services Protocol and
  the  SMM Services Table to execute a procedure on enabled APs.  The context
  the AP should use durng execution is specified by Buffer.

  @param[in]  Buffer   The pointer to the procedure's argument.
**/
typedef
VOID
(EFIAPI *FRAMEWORK_EFI_AP_PROCEDURE)(
  IN  VOID  *Buffer
  );

/**
  This service retrieves general information of multiprocessors in the system.

  This function is used to get the following information:
    - Number of logical processors in system
    - Maximal number of logical processors supported by system
    - Number of enabled logical processors.
    - Rendezvous interrupt number (IPF-specific)
    - Length of the rendezvous procedure.

  @param[in]  This                   The pointer to the FRAMEWORK_EFI_MP_SERVICES_PROTOCOL
                                     instance.
  @param[out] NumberOfCPUs           The pointer to the total number of logical processors
                                     in the system, including the BSP and disabled
                                     APs.  If NULL, this parameter is ignored.
  @param[out] MaximumNumberOfCPUs    Pointer to the maximum number of processors
                                     supported by the system.  If NULL, this
                                     parameter is ignored.
  @param[out] NumberOfEnabledCPUs    The pointer to the number of enabled logical
                                     processors that exist in system, including
                                     the BSP.  If NULL, this parameter is ignored.
  @param[out] RendezvousIntNumber    This parameter is only meaningful for IPF.
                                     - IA32, X64: The returned value is zero.
                                     If NULL, this parameter is ignored.
                                     - IPF: Pointer to the rendezvous interrupt
                                     number that is used for AP wake-up.
  @param[out] RendezvousProcLength   The pointer to the length of rendezvous procedure.
                                     - IA32, X64: The returned value is 0x1000.
                                     If NULL, this parameter is ignored.
                                     - IPF: The returned value is zero.

  @retval EFI_SUCCESS   Multiprocessor general information was successfully retrieved.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_MP_SERVICES_GET_GENERAL_MP_INFO)(
  IN  FRAMEWORK_EFI_MP_SERVICES_PROTOCOL  *This,
  OUT UINTN                               *NumberOfCPUs          OPTIONAL,
  OUT UINTN                               *MaximumNumberOfCPUs   OPTIONAL,
  OUT UINTN                               *NumberOfEnabledCPUs   OPTIONAL,
  OUT UINTN                               *RendezvousIntNumber   OPTIONAL,
  OUT UINTN                               *RendezvousProcLength  OPTIONAL
  );

/**
  This service gets detailed MP-related information of the requested processor.

  This service gets detailed MP-related information of the requested processor
  at the instant this call is made. Note the following:
    - The processor information may change during the course of a boot session.
    - The data of information presented here is entirely MP related.
  Information regarding the number of caches and their sizes, frequency of operation,
  slot numbers is all considered platform-related information and will not be
  presented here.

  @param[in]  This                     The pointer to the FRAMEWORK_EFI_MP_SERVICES_PROTOCOL
                                       instance.
  @param[in]  ProcessorNumber          The handle number of the processor. The range
                                       is from 0 to the total number of logical
                                       processors minus 1. The total number of
                                       logical processors can be retrieved by
                                       GetGeneralMPInfo().
  @param[in,out] BufferLength          On input, pointer to the size in bytes of
                                       ProcessorContextBuffer.  On output, if the
                                       size of ProcessorContextBuffer is not large
                                       enough, the value pointed by this parameter
                                       is updated to size in bytes that is needed.
                                       If the size of ProcessorContextBuffer is
                                       sufficient, the value is not changed from
                                       input.
  @param[out] ProcessorContextBuffer   The pointer to the buffer where the data of
                                       requested processor will be deposited.
                                       The buffer is allocated by caller.

  @retval EFI_SUCCESS             Processor information was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL    The size of ProcessorContextBuffer is too small.
                                  The value pointed by BufferLength has been updated
                                  to size in bytes that is needed.
  @retval EFI_INVALID_PARAMETER   IA32, X64:BufferLength is NULL.
  @retval EFI_INVALID_PARAMETER   IA32, X64:ProcessorContextBuffer is NULL.
  @retval EFI_INVALID_PARAMETER   IA32, X64:Processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_NOT_FOUND           IPF: Processor with the handle specified by
                                  ProcessorNumber does not exist.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_MP_SERVICES_GET_PROCESSOR_CONTEXT)(
  IN     FRAMEWORK_EFI_MP_SERVICES_PROTOCOL  *This,
  IN     UINTN                               ProcessorNumber,
  IN OUT UINTN                               *BufferLength,
  OUT    EFI_MP_PROC_CONTEXT                 *ProcessorContextBuffer
  );

/**
  This function is used to dispatch all enabled APs to the function specified
  by Procedure. APs can run either simultaneously or one by one. The caller can
  also configure the BSP to either wait for APs or just proceed with the next
  task.  It is the responsibility of the caller of the StartupAllAPs() to make
  sure that the nature of the code that will be run on the BSP and the dispatched
  APs is well controlled. The MP Services Protocol does not guarantee that the
  function that either processor is executing is MP-safe. Hence, the tasks that
  can be run in parallel are limited to certain independent tasks and well-
  controlled exclusive code. EFI services and protocols may not be called by APs
  unless otherwise specified.

  @param[in]  This                    The pointer to the FRAMEWORK_EFI_MP_SERVICES_PROTOCOL
                                      instance.
  @param[in]  Procedure               A pointer to the function to be run on enabled
                                      APs of the system.
  @param[in]  SingleThread            Flag that requests APs to execute one at a
                                      time or simultaneously.
                                      - IA32, X64:
                                      If TRUE, then all the enabled APs execute
                                      the function specified by Procedure one by
                                      one, in ascending order of processor handle
                                      number.  If FALSE, then all the enabled APs
                                      execute the function specified by Procedure
                                      simultaneously.
                                      - IPF:
                                      If TRUE, then all the enabled APs execute
                                      the function specified by Procedure simultaneously.
                                      If FALSE, then all the enabled APs execute the
                                      function specified by Procedure one by one, in
                                      ascending order of processor handle number. The
                                      time interval of AP dispatching is determined
                                      by WaitEvent and TimeoutInMicrosecs.
  @param[in]  WaitEvent               Event to signal when APs have finished.
                                      - IA32, X64:
                                      If not NULL, when all APs finish after timeout
                                      expires, the event will be signaled.  If NULL,
                                      the parameter is ignored.
                                      - IPF:
                                      If SingleThread is TRUE, this parameter
                                      is ignored.  If SingleThread is FALSE (i.e.
                                      dispatch APs one by one), this parameter
                                      determines whether the BSP waits after each
                                      AP is dispatched. If it is NULL, the BSP
                                      does not wait after each AP is dispatched.
                                      If it is not NULL, the BSP waits after each
                                      AP is dispatched, and the time interval is
                                      determined by TimeoutInMicrosecs.  Type
                                      EFI_EVENT is defined in CreateEvent() in
                                      the Unified Extensible Firmware Interface
                                      Specification.
  @param[in]  TimeoutInMicroSecs   Time to wait for APs to finish.
                                      - IA32, X64:
                                      If the value is zero, it means no timeout
                                      limit. The BSP waits until all APs finish.
                                      If the value is not zero, the BSP waits
                                      until all APs finish or timeout expires.
                                      If timeout expires, EFI_TIMEOUT is returned,
                                      and the BSP will then check APs?status
                                      periodically, with time interval of 16
                                      microseconds.
                                      - IPF:
                                      If SingleThread is TRUE and FailedCPUList
                                      is NULL, this parameter is ignored.  If
                                      SingleThread is TRUE and FailedCPUList is
                                      not NULL, this parameter determines whether
                                      the BSP waits until all APs finish their
                                      procedure. If it is zero, the BSP does not
                                      wait for APs. If it is non-zero, it waits
                                      until all APs finish.  If SingleThread is
                                      FALSE and WaitEvent is NULL, this parameter
                                      is ignored.  If SingleThread is FALSE and
                                      WaitEvent is not NULL, the BSP waits after
                                      each AP is dispatched and this value
                                      determines time interval. If the value is
                                      zero, the length of time interval is 10ms.
                                      If the value is non-zero, the BSP waits
                                      until dispatched AP finishes and then
                                      dispatch the next.
  @param[in]  ProcArguments       The pointer to the optional parameter of the
                                      function specified by Procedure.
  @param[out] FailedCPUList           List of APs that did not finish.
                                      - IA32, X64:
                                      If not NULL, it records handle numbers of
                                      all logical processors that fail to accept
                                      caller-provided function (busy or disabled).
                                      If NULL, this parameter is ignored.
                                      - IPF:
                                      If not NULL, it records status of all
                                      logical processors, with processor handle
                                      number as index. If a logical processor
                                      fails to accept caller-provided function
                                      because it is busy, the status is EFI_NOT_READY.
                                      If it fails to accept function due to other
                                      reasons, the status is EFI_NOT_AVAILABLE_YET.
                                      If timeout expires, the status is EFI_TIMEOUT.
                                      Otherwise, the value is EFI_SUCCESS.  If NULL,
                                      this parameter is ignored.

  @retval EFI_SUCCESS	            IA32, X64: All dispatched APs have finished
                                  before the timeout expires.
  @retval EFI_SUCCESS	            IA32, X64: Only 1 logical processor exists
                                  in system.
  @retval EFI_INVALID_PARAMETER	    IA32, X64: Procedure is NULL.
  @retval EFI_TIMEOUT	            IA32, X64: The timeout expires before all
                                  dispatched APs have finished.
  @retval EFI_SUCCESS	            IPF: This function always returns EFI_SUCCESS.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_MP_SERVICES_STARTUP_ALL_APS)(
  IN  FRAMEWORK_EFI_MP_SERVICES_PROTOCOL  *This,
  IN  FRAMEWORK_EFI_AP_PROCEDURE          Procedure,
  IN  BOOLEAN                             SingleThread,
  IN  EFI_EVENT                           WaitEvent           OPTIONAL,
  IN  UINTN                               TimeoutInMicroSecs,
  IN  VOID                                *ProcArguments      OPTIONAL,
  OUT UINTN                               *FailedCPUList      OPTIONAL
  );

/**
  This function is used to dispatch one enabled AP to the function provided by
  the caller. The caller can request the BSP to either wait for the AP or just
  proceed with the next task.

  @param[in] This                    The pointer to the FRAMEWORK_EFI_MP_SERVICES_PROTOCOL
                                     instance.
  @param[in] Procedure               A pointer to the function to be run on the
                                     designated AP.
  @param[in] ProcessorNumber         The handle number of AP. The range is from
                                     0 to the total number of logical processors
                                     minus 1.  The total number of logical
                                     processors can be retrieved by GetGeneralMPInfo().
  @param[in] WaitEvent               Event to signal when APs have finished.
                                     - IA32, X64:
                                     If not NULL, when the AP finishes after timeout
                                     expires, the event will be signaled.  If NULL,
                                     the parameter is ignored.
                                     - IPF:
                                     This parameter determines whether the BSP
                                     waits after the AP is dispatched. If it is
                                     NULL, the BSP does not wait after the AP
                                     is dispatched. If it is not NULL, the BSP
                                     waits after the AP is dispatched, and the
                                     time interval is determined by TimeoutInMicrosecs.
                                     Type EFI_EVENT is defined in CreateEvent()
                                     in the Unified Extensible Firmware Interface
                                     Specification.
  @param[in] TimeoutInMicroSecs   Time to wait for APs to finish.
                                     - IA32, X64:
                                     If the value is zero, it means no timeout
                                     limit. The BSP waits until the AP finishes.
                                     If the value is not zero, the BSP waits until
                                     the AP finishes or timeout expires. If timeout
                                     expires, EFI_TIMEOUT is returned, and the
                                     BSP will then check the AP's status periodically,
                                     with time interval of 16 microseconds.
                                     - IPF:
                                     If WaitEvent is NULL, this parameter is ignored.
                                     If WaitEvent is not NULL, the BSP waits after
                                     the AP is dispatched and this value determines
                                     time interval. If the value is zero, the length
                                     of time interval is 10ms. If the value is
                                     non-zero, the BSP waits until the AP finishes.
  @param[in] ProcArguments       The pointer to the optional parameter of the
                                     function specified by Procedure.

  @retval EFI_SUCCESS             Specified AP has finished before the timeout
                                  expires.
  @retval EFI_TIMEOUT             The timeout expires before specified AP has
                                  finished.
  @retval EFI_INVALID_PARAMETER   IA32, X64: Processor with the handle specified
                                  by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   IA32, X64: Specified AP is busy or disabled.
  @retval EFI_INVALID_PARAMETER   IA32, X64: Procedure is NULL.
  @retval EFI_INVALID_PARAMETER   IA32, X64: ProcessorNumber specifies the BSP
  @retval EFI_NOT_READY           IPF: Specified AP is busy
  @retval EFI_NOT_AVAILABLE_YET   IPF: ProcessorNumber specifies the BSP
  @retval EFI_NOT_AVAILABLE_YET   IPF: Specified AP is disabled.
  @retval EFI_NOT_AVAILABLE_YET   IPF: Specified AP is unhealthy or untested.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_MP_SERVICES_STARTUP_THIS_AP)(
  IN FRAMEWORK_EFI_MP_SERVICES_PROTOCOL  *This,
  IN FRAMEWORK_EFI_AP_PROCEDURE          Procedure,
  IN UINTN                               ProcessorNumber,
  IN EFI_EVENT                           WaitEvent            OPTIONAL,
  IN UINTN                               TimeoutInMicroSecs,
  IN OUT VOID                            *ProcArguments       OPTIONAL
  );

/**
  This service switches the requested AP to be the BSP from that point onward.
  The new BSP can take over the execution of the old BSP and continue seamlessly
  from where the old one left off.  This call can only be performed by the
  current BSP.

  @param[in] This              The pointer to the FRAMEWORK_EFI_MP_SERVICES_PROTOCOL
                               instance.
  @param[in] ProcessorNumber   The handle number of AP. The range is from 0 to
                               the total number of logical processors minus 1.
                               The total number of logical processors can be
                               retrieved by GetGeneralMPInfo().
  @param[in] EnableOldBSP      If TRUE, then the old BSP will be listed as an
                               enabled AP. Otherwise, it will be disabled.

  @retval EFI_SUCCESS             BSP successfully switched.
  @retval EFI_INVALID_PARAMETER   The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP.
  @retval EFI_NOT_READY           IA32, X64: Specified AP is busy or disabled.
  @retval EFI_INVALID_PARAMETER   IPF: Specified AP is disabled.
  @retval EFI_INVALID_PARAMETER   IPF: Specified AP is unhealthy or untested.
  @retval EFI_NOT_READY           IPF: Specified AP is busy.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_MP_SERVICES_SWITCH_BSP)(
  IN FRAMEWORK_EFI_MP_SERVICES_PROTOCOL  *This,
  IN UINTN                               ProcessorNumber,
  IN BOOLEAN                             EnableOldBSP
  );

/**
  This service sends an IPI to a specified AP. Caller can specify vector number
  and delivery mode of the interrupt.

  @param[in] This              The pointer to the FRAMEWORK_EFI_MP_SERVICES_PROTOCOL
                               instance.
  @param[in] ProcessorNumber   The handle number of AP. The range is from 0 to
                               the total number of logical processors minus 1.
                               The total number of logical processors can be
                               retrieved by GetGeneralMPInfo().
  @param[in] VectorNumber      The vector number of the interrupt.
  @param[in] DeliveryMode      The delivery mode of the interrupt.

  @retval EFI_SUCCESS             IPI was successfully sent.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP.
  @retval EFI_INVALID_PARAMETER   IA32, X64: Processor with the handle specified
                                  by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   IA32, X64: VectorNumber is greater than 255.
  @retval EFI_INVALID_PARAMETER   IA32, X64: DeliveryMode is greater than or equal
                                  to DELIVERY_MODE_MAX.
  @retval EFI_NOT_READY           IA32, X64: IPI is not accepted by the target
                                  processor within 10 microseconds.
  @retval EFI_INVALID_PARAMETER   IPF: Specified AP is disabled.
  @retval EFI_INVALID_PARAMETER   IPF: Specified AP is unhealthy or untested.
  @retval EFI_NOT_READY           IPF: Specified AP is busy.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_MP_SERVICES_SEND_IPI)(
  IN FRAMEWORK_EFI_MP_SERVICES_PROTOCOL  *This,
  IN UINTN                               ProcessorNumber,
  IN UINTN                               VectorNumber,
  IN UINTN                               DeliveryMode
  );

/**
  This service lets the caller enable or disable an AP.  The caller can optionally
  specify the health status of the AP by Health. It is usually used to update the
  health status of the processor after some processor test.

  @param[in] This              The pointer to the FRAMEWORK_EFI_MP_SERVICES_PROTOCOL
                               instance.
  @param[in] ProcessorNumber   The handle number of AP. The range is from 0 to
                               the total number of logical processors minus 1.
                               The total number of logical processors can be
                               retrieved by GetGeneralMPInfo().
  @param[in] NewAPState        Indicates whether the new, desired state of the
                               AP is enabled or disabled. TRUE for enabling,
                               FALSE otherwise.
  @param[in] HealthState       If not NULL, it points to the value that specifies
                               the new health status of the AP.  If it is NULL,
                               this parameter is ignored.

  @retval EFI_SUCCESS             AP successfully enabled or disabled.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP.
  @retval EFI_INVALID_PARAMETER   IA32, X64: Processor with the handle specified
                                  by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   IPF: If an unhealthy or untested AP is to be
                                  enabled.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_MP_SERVICES_ENABLEDISABLEAP)(
  IN FRAMEWORK_EFI_MP_SERVICES_PROTOCOL  *This,
  IN UINTN                               ProcessorNumber,
  IN BOOLEAN                             NewAPState,
  IN EFI_MP_HEALTH                       *HealthState  OPTIONAL
  );

/**
  This service lets the caller processor get its handle number, with which any
  processor in the system can be uniquely identified. The range is from 0 to the
  total number of logical processors minus 1. The total number of logical
  processors can be retrieved by GetGeneralMPInfo(). This service may be called
  from the BSP and APs.

  @param[in]  This              The pointer to the FRAMEWORK_EFI_MP_SERVICES_PROTOCOL
                                instance.
  @param[out] ProcessorNumber   A pointer to the handle number of AP. The range is
                                from 0 to the total number of logical processors
                                minus 1. The total number of logical processors
                                can be retrieved by GetGeneralMPInfo().

@retval EFI_SUCCESS   This function always returns EFI_SUCCESS.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_MP_SERVICES_WHOAMI)(
  IN  FRAMEWORK_EFI_MP_SERVICES_PROTOCOL  *This,
  OUT UINTN                               *ProcessorNumber
  );

///
/// Framework MP Services Protocol structure.
///
struct _FRAMEWORK_EFI_MP_SERVICES_PROTOCOL {
  FRAMEWORK_EFI_MP_SERVICES_GET_GENERAL_MP_INFO    GetGeneralMPInfo;
  FRAMEWORK_EFI_MP_SERVICES_GET_PROCESSOR_CONTEXT  GetProcessorContext;
  FRAMEWORK_EFI_MP_SERVICES_STARTUP_ALL_APS        StartupAllAPs;
  FRAMEWORK_EFI_MP_SERVICES_STARTUP_THIS_AP        StartupThisAP;
  FRAMEWORK_EFI_MP_SERVICES_SWITCH_BSP             SwitchBSP;
  FRAMEWORK_EFI_MP_SERVICES_SEND_IPI               SendIPI;
  FRAMEWORK_EFI_MP_SERVICES_ENABLEDISABLEAP        EnableDisableAP;
  FRAMEWORK_EFI_MP_SERVICES_WHOAMI                 WhoAmI;
};

extern EFI_GUID gFrameworkEfiMpServiceProtocolGuid;

#endif
