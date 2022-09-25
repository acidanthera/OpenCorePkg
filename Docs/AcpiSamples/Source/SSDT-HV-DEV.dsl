/*
 * Hyper-V device SSDT to disable unsupported devices under macOS on Windows Server 2019 / Windows 10 and newer.
 *
 * Windows 10 and newer define various objects conditionally, which are
 * unsupported in older versions of macOS. All objects are enabled regardless
 * of state in older versions, causing slow boot behavior and crashes during boot caused by AppleACPIPlatform.
 *
 * Windows 10 and newer also use ACPI0007 Device objects for CPUs, which macOS cannot detect.
 * This SSDT redefines the first 64 as Processor objects.
 * This may cause issues with other operating systems.
 * 
 * This SSDT defines _STA methods for all affected objects and disables them entirely, or enables only if
 * the approprate register is set (enabled in Hyper-V).
 *
 * On Windows Server 2012 R2 / Windows 8.1 and older this SSDT is unnecessary and can be disabled along with associated patches.
 *
 * Requires the following ACPI patches:
 * (1) Base:            \_SB.VMOD.TPM2
 *     Comment:         _STA to XSTA rename (Hyper-V TPM)
 *     Count:           1
 *     Find:            _STA
 *     Replace:         XSTA
 *     TableSignature:  44534454 (DSDT)
 * (2) Base:            \_SB.NVDR
 *     Comment:         _STA to XSTA rename (Hyper-V NVDIMM)
 *     Count:           1
 *     Find:            _STA
 *     Replace:         XSTA
 *     TableSignature:  44534454 (DSDT)
 * (3) Base:            \_SB.EPC
 *     Comment:         _STA to XSTA rename (Hyper-V EPC)
 *     Count:           1
 *     Find:            _STA
 *     Replace:         XSTA
 *     TableSignature:  44534454 (DSDT)
 * (4) Base:            \_SB.VMOD.BAT1
 *     Comment:         _STA to XSTA rename (Hyper-V battery)
 *     Count:           1
 *     Find:            _STA
 *     Replace:         XSTA
 *     TableSignature:  44534454 (DSDT)
 * (5) Base:            \P001
 *     Comment:         _STA to XSTA rename (Hyper-V processors)
 *     Count:           240
 *     Find:            _STA
 *     Replace:         XSTA
 *     TableSignature:  44534454 (DSDT)
 */

DefinitionBlock ("", "SSDT", 2, "ACDT", "HVDEV", 0x00000000)
{
    //
    // EC fields in DSDT
    //
    External (SCFG, FieldUnitObj)
    External (TCFG, FieldUnitObj)
    External (NCFG, FieldUnitObj)
    External (SGXE, FieldUnitObj)
    External (BCFG, FieldUnitObj)
    External (PCNT, FieldUnitObj)

    //
    // Device and method objects in DSDT
    //
    External (\_SB.UAR1, DeviceObj)
    External (\_SB.UAR2, DeviceObj)
    External (\_SB.VMOD.TPM2, DeviceObj)
    External (\_SB.VMOD.TPM2.XSTA, MethodObj)
    External (\_SB.NVDR, DeviceObj)
    External (\_SB.NVDR.XSTA, MethodObj)
    External (\_SB.EPC, DeviceObj)
    External (\_SB.EPC.XSTA, MethodObj)
    External (\_SB.VMOD.BAT1, DeviceObj)
    External (\_SB.VMOD.BAT1.XSTA, MethodObj)
    External (\_SB.VMOD.AC1, DeviceObj)
    External (\P001, DeviceObj)
    External (\P001.XSTA, MethodObj)
    External (\P002, DeviceObj)
    External (\P002.XSTA, MethodObj)
    External (\P003, DeviceObj)
    External (\P003.XSTA, MethodObj)
    External (\P004, DeviceObj)
    External (\P004.XSTA, MethodObj)
    External (\P005, DeviceObj)
    External (\P005.XSTA, MethodObj)
    External (\P006, DeviceObj)
    External (\P006.XSTA, MethodObj)
    External (\P007, DeviceObj)
    External (\P007.XSTA, MethodObj)
    External (\P008, DeviceObj)
    External (\P008.XSTA, MethodObj)
    External (\P009, DeviceObj)
    External (\P009.XSTA, MethodObj)
    External (\P010, DeviceObj)
    External (\P010.XSTA, MethodObj)
    External (\P011, DeviceObj)
    External (\P011.XSTA, MethodObj)
    External (\P012, DeviceObj)
    External (\P012.XSTA, MethodObj)
    External (\P013, DeviceObj)
    External (\P013.XSTA, MethodObj)
    External (\P014, DeviceObj)
    External (\P014.XSTA, MethodObj)
    External (\P015, DeviceObj)
    External (\P015.XSTA, MethodObj)
    External (\P016, DeviceObj)
    External (\P016.XSTA, MethodObj)
    External (\P017, DeviceObj)
    External (\P017.XSTA, MethodObj)
    External (\P018, DeviceObj)
    External (\P018.XSTA, MethodObj)
    External (\P019, DeviceObj)
    External (\P019.XSTA, MethodObj)
    External (\P020, DeviceObj)
    External (\P020.XSTA, MethodObj)
    External (\P021, DeviceObj)
    External (\P021.XSTA, MethodObj)
    External (\P022, DeviceObj)
    External (\P022.XSTA, MethodObj)
    External (\P023, DeviceObj)
    External (\P023.XSTA, MethodObj)
    External (\P024, DeviceObj)
    External (\P024.XSTA, MethodObj)
    External (\P025, DeviceObj)
    External (\P025.XSTA, MethodObj)
    External (\P026, DeviceObj)
    External (\P026.XSTA, MethodObj)
    External (\P027, DeviceObj)
    External (\P027.XSTA, MethodObj)
    External (\P028, DeviceObj)
    External (\P028.XSTA, MethodObj)
    External (\P029, DeviceObj)
    External (\P029.XSTA, MethodObj)
    External (\P030, DeviceObj)
    External (\P030.XSTA, MethodObj)
    External (\P031, DeviceObj)
    External (\P031.XSTA, MethodObj)
    External (\P032, DeviceObj)
    External (\P032.XSTA, MethodObj)
    External (\P033, DeviceObj)
    External (\P033.XSTA, MethodObj)
    External (\P034, DeviceObj)
    External (\P034.XSTA, MethodObj)
    External (\P035, DeviceObj)
    External (\P035.XSTA, MethodObj)
    External (\P036, DeviceObj)
    External (\P036.XSTA, MethodObj)
    External (\P037, DeviceObj)
    External (\P037.XSTA, MethodObj)
    External (\P038, DeviceObj)
    External (\P038.XSTA, MethodObj)
    External (\P039, DeviceObj)
    External (\P039.XSTA, MethodObj)
    External (\P040, DeviceObj)
    External (\P040.XSTA, MethodObj)
    External (\P041, DeviceObj)
    External (\P041.XSTA, MethodObj)
    External (\P042, DeviceObj)
    External (\P042.XSTA, MethodObj)
    External (\P043, DeviceObj)
    External (\P043.XSTA, MethodObj)
    External (\P044, DeviceObj)
    External (\P044.XSTA, MethodObj)
    External (\P045, DeviceObj)
    External (\P045.XSTA, MethodObj)
    External (\P046, DeviceObj)
    External (\P046.XSTA, MethodObj)
    External (\P047, DeviceObj)
    External (\P047.XSTA, MethodObj)
    External (\P048, DeviceObj)
    External (\P048.XSTA, MethodObj)
    External (\P049, DeviceObj)
    External (\P049.XSTA, MethodObj)
    External (\P050, DeviceObj)
    External (\P050.XSTA, MethodObj)
    External (\P051, DeviceObj)
    External (\P051.XSTA, MethodObj)
    External (\P052, DeviceObj)
    External (\P052.XSTA, MethodObj)
    External (\P053, DeviceObj)
    External (\P053.XSTA, MethodObj)
    External (\P054, DeviceObj)
    External (\P054.XSTA, MethodObj)
    External (\P055, DeviceObj)
    External (\P055.XSTA, MethodObj)
    External (\P056, DeviceObj)
    External (\P056.XSTA, MethodObj)
    External (\P057, DeviceObj)
    External (\P057.XSTA, MethodObj)
    External (\P058, DeviceObj)
    External (\P058.XSTA, MethodObj)
    External (\P059, DeviceObj)
    External (\P059.XSTA, MethodObj)
    External (\P060, DeviceObj)
    External (\P060.XSTA, MethodObj)
    External (\P061, DeviceObj)
    External (\P061.XSTA, MethodObj)
    External (\P062, DeviceObj)
    External (\P062.XSTA, MethodObj)
    External (\P063, DeviceObj)
    External (\P063.XSTA, MethodObj)
    External (\P064, DeviceObj)
    External (\P064.XSTA, MethodObj)
    External (\P065, DeviceObj)
    External (\P065.XSTA, MethodObj)
    External (\P066, DeviceObj)
    External (\P066.XSTA, MethodObj)
    External (\P067, DeviceObj)
    External (\P067.XSTA, MethodObj)
    External (\P068, DeviceObj)
    External (\P068.XSTA, MethodObj)
    External (\P069, DeviceObj)
    External (\P069.XSTA, MethodObj)
    External (\P070, DeviceObj)
    External (\P070.XSTA, MethodObj)
    External (\P071, DeviceObj)
    External (\P071.XSTA, MethodObj)
    External (\P072, DeviceObj)
    External (\P072.XSTA, MethodObj)
    External (\P073, DeviceObj)
    External (\P073.XSTA, MethodObj)
    External (\P074, DeviceObj)
    External (\P074.XSTA, MethodObj)
    External (\P075, DeviceObj)
    External (\P075.XSTA, MethodObj)
    External (\P076, DeviceObj)
    External (\P076.XSTA, MethodObj)
    External (\P077, DeviceObj)
    External (\P077.XSTA, MethodObj)
    External (\P078, DeviceObj)
    External (\P078.XSTA, MethodObj)
    External (\P079, DeviceObj)
    External (\P079.XSTA, MethodObj)
    External (\P080, DeviceObj)
    External (\P080.XSTA, MethodObj)
    External (\P081, DeviceObj)
    External (\P081.XSTA, MethodObj)
    External (\P082, DeviceObj)
    External (\P082.XSTA, MethodObj)
    External (\P083, DeviceObj)
    External (\P083.XSTA, MethodObj)
    External (\P084, DeviceObj)
    External (\P084.XSTA, MethodObj)
    External (\P085, DeviceObj)
    External (\P085.XSTA, MethodObj)
    External (\P086, DeviceObj)
    External (\P086.XSTA, MethodObj)
    External (\P087, DeviceObj)
    External (\P087.XSTA, MethodObj)
    External (\P088, DeviceObj)
    External (\P088.XSTA, MethodObj)
    External (\P089, DeviceObj)
    External (\P089.XSTA, MethodObj)
    External (\P090, DeviceObj)
    External (\P090.XSTA, MethodObj)
    External (\P091, DeviceObj)
    External (\P091.XSTA, MethodObj)
    External (\P092, DeviceObj)
    External (\P092.XSTA, MethodObj)
    External (\P093, DeviceObj)
    External (\P093.XSTA, MethodObj)
    External (\P094, DeviceObj)
    External (\P094.XSTA, MethodObj)
    External (\P095, DeviceObj)
    External (\P095.XSTA, MethodObj)
    External (\P096, DeviceObj)
    External (\P096.XSTA, MethodObj)
    External (\P097, DeviceObj)
    External (\P097.XSTA, MethodObj)
    External (\P098, DeviceObj)
    External (\P098.XSTA, MethodObj)
    External (\P099, DeviceObj)
    External (\P099.XSTA, MethodObj)
    External (\P100, DeviceObj)
    External (\P100.XSTA, MethodObj)
    External (\P101, DeviceObj)
    External (\P101.XSTA, MethodObj)
    External (\P102, DeviceObj)
    External (\P102.XSTA, MethodObj)
    External (\P103, DeviceObj)
    External (\P103.XSTA, MethodObj)
    External (\P104, DeviceObj)
    External (\P104.XSTA, MethodObj)
    External (\P105, DeviceObj)
    External (\P105.XSTA, MethodObj)
    External (\P106, DeviceObj)
    External (\P106.XSTA, MethodObj)
    External (\P107, DeviceObj)
    External (\P107.XSTA, MethodObj)
    External (\P108, DeviceObj)
    External (\P108.XSTA, MethodObj)
    External (\P109, DeviceObj)
    External (\P109.XSTA, MethodObj)
    External (\P110, DeviceObj)
    External (\P110.XSTA, MethodObj)
    External (\P111, DeviceObj)
    External (\P111.XSTA, MethodObj)
    External (\P112, DeviceObj)
    External (\P112.XSTA, MethodObj)
    External (\P113, DeviceObj)
    External (\P113.XSTA, MethodObj)
    External (\P114, DeviceObj)
    External (\P114.XSTA, MethodObj)
    External (\P115, DeviceObj)
    External (\P115.XSTA, MethodObj)
    External (\P116, DeviceObj)
    External (\P116.XSTA, MethodObj)
    External (\P117, DeviceObj)
    External (\P117.XSTA, MethodObj)
    External (\P118, DeviceObj)
    External (\P118.XSTA, MethodObj)
    External (\P119, DeviceObj)
    External (\P119.XSTA, MethodObj)
    External (\P120, DeviceObj)
    External (\P120.XSTA, MethodObj)
    External (\P121, DeviceObj)
    External (\P121.XSTA, MethodObj)
    External (\P122, DeviceObj)
    External (\P122.XSTA, MethodObj)
    External (\P123, DeviceObj)
    External (\P123.XSTA, MethodObj)
    External (\P124, DeviceObj)
    External (\P124.XSTA, MethodObj)
    External (\P125, DeviceObj)
    External (\P125.XSTA, MethodObj)
    External (\P126, DeviceObj)
    External (\P126.XSTA, MethodObj)
    External (\P127, DeviceObj)
    External (\P127.XSTA, MethodObj)
    External (\P128, DeviceObj)
    External (\P128.XSTA, MethodObj)
    External (\P129, DeviceObj)
    External (\P129.XSTA, MethodObj)
    External (\P130, DeviceObj)
    External (\P130.XSTA, MethodObj)
    External (\P131, DeviceObj)
    External (\P131.XSTA, MethodObj)
    External (\P132, DeviceObj)
    External (\P132.XSTA, MethodObj)
    External (\P133, DeviceObj)
    External (\P133.XSTA, MethodObj)
    External (\P134, DeviceObj)
    External (\P134.XSTA, MethodObj)
    External (\P135, DeviceObj)
    External (\P135.XSTA, MethodObj)
    External (\P136, DeviceObj)
    External (\P136.XSTA, MethodObj)
    External (\P137, DeviceObj)
    External (\P137.XSTA, MethodObj)
    External (\P138, DeviceObj)
    External (\P138.XSTA, MethodObj)
    External (\P139, DeviceObj)
    External (\P139.XSTA, MethodObj)
    External (\P140, DeviceObj)
    External (\P140.XSTA, MethodObj)
    External (\P141, DeviceObj)
    External (\P141.XSTA, MethodObj)
    External (\P142, DeviceObj)
    External (\P142.XSTA, MethodObj)
    External (\P143, DeviceObj)
    External (\P143.XSTA, MethodObj)
    External (\P144, DeviceObj)
    External (\P144.XSTA, MethodObj)
    External (\P145, DeviceObj)
    External (\P145.XSTA, MethodObj)
    External (\P146, DeviceObj)
    External (\P146.XSTA, MethodObj)
    External (\P147, DeviceObj)
    External (\P147.XSTA, MethodObj)
    External (\P148, DeviceObj)
    External (\P148.XSTA, MethodObj)
    External (\P149, DeviceObj)
    External (\P149.XSTA, MethodObj)
    External (\P150, DeviceObj)
    External (\P150.XSTA, MethodObj)
    External (\P151, DeviceObj)
    External (\P151.XSTA, MethodObj)
    External (\P152, DeviceObj)
    External (\P152.XSTA, MethodObj)
    External (\P153, DeviceObj)
    External (\P153.XSTA, MethodObj)
    External (\P154, DeviceObj)
    External (\P154.XSTA, MethodObj)
    External (\P155, DeviceObj)
    External (\P155.XSTA, MethodObj)
    External (\P156, DeviceObj)
    External (\P156.XSTA, MethodObj)
    External (\P157, DeviceObj)
    External (\P157.XSTA, MethodObj)
    External (\P158, DeviceObj)
    External (\P158.XSTA, MethodObj)
    External (\P159, DeviceObj)
    External (\P159.XSTA, MethodObj)
    External (\P160, DeviceObj)
    External (\P160.XSTA, MethodObj)
    External (\P161, DeviceObj)
    External (\P161.XSTA, MethodObj)
    External (\P162, DeviceObj)
    External (\P162.XSTA, MethodObj)
    External (\P163, DeviceObj)
    External (\P163.XSTA, MethodObj)
    External (\P164, DeviceObj)
    External (\P164.XSTA, MethodObj)
    External (\P165, DeviceObj)
    External (\P165.XSTA, MethodObj)
    External (\P166, DeviceObj)
    External (\P166.XSTA, MethodObj)
    External (\P167, DeviceObj)
    External (\P167.XSTA, MethodObj)
    External (\P168, DeviceObj)
    External (\P168.XSTA, MethodObj)
    External (\P169, DeviceObj)
    External (\P169.XSTA, MethodObj)
    External (\P170, DeviceObj)
    External (\P170.XSTA, MethodObj)
    External (\P171, DeviceObj)
    External (\P171.XSTA, MethodObj)
    External (\P172, DeviceObj)
    External (\P172.XSTA, MethodObj)
    External (\P173, DeviceObj)
    External (\P173.XSTA, MethodObj)
    External (\P174, DeviceObj)
    External (\P174.XSTA, MethodObj)
    External (\P175, DeviceObj)
    External (\P175.XSTA, MethodObj)
    External (\P176, DeviceObj)
    External (\P176.XSTA, MethodObj)
    External (\P177, DeviceObj)
    External (\P177.XSTA, MethodObj)
    External (\P178, DeviceObj)
    External (\P178.XSTA, MethodObj)
    External (\P179, DeviceObj)
    External (\P179.XSTA, MethodObj)
    External (\P180, DeviceObj)
    External (\P180.XSTA, MethodObj)
    External (\P181, DeviceObj)
    External (\P181.XSTA, MethodObj)
    External (\P182, DeviceObj)
    External (\P182.XSTA, MethodObj)
    External (\P183, DeviceObj)
    External (\P183.XSTA, MethodObj)
    External (\P184, DeviceObj)
    External (\P184.XSTA, MethodObj)
    External (\P185, DeviceObj)
    External (\P185.XSTA, MethodObj)
    External (\P186, DeviceObj)
    External (\P186.XSTA, MethodObj)
    External (\P187, DeviceObj)
    External (\P187.XSTA, MethodObj)
    External (\P188, DeviceObj)
    External (\P188.XSTA, MethodObj)
    External (\P189, DeviceObj)
    External (\P189.XSTA, MethodObj)
    External (\P190, DeviceObj)
    External (\P190.XSTA, MethodObj)
    External (\P191, DeviceObj)
    External (\P191.XSTA, MethodObj)
    External (\P192, DeviceObj)
    External (\P192.XSTA, MethodObj)
    External (\P193, DeviceObj)
    External (\P193.XSTA, MethodObj)
    External (\P194, DeviceObj)
    External (\P194.XSTA, MethodObj)
    External (\P195, DeviceObj)
    External (\P195.XSTA, MethodObj)
    External (\P196, DeviceObj)
    External (\P196.XSTA, MethodObj)
    External (\P197, DeviceObj)
    External (\P197.XSTA, MethodObj)
    External (\P198, DeviceObj)
    External (\P198.XSTA, MethodObj)
    External (\P199, DeviceObj)
    External (\P199.XSTA, MethodObj)
    External (\P200, DeviceObj)
    External (\P200.XSTA, MethodObj)
    External (\P201, DeviceObj)
    External (\P201.XSTA, MethodObj)
    External (\P202, DeviceObj)
    External (\P202.XSTA, MethodObj)
    External (\P203, DeviceObj)
    External (\P203.XSTA, MethodObj)
    External (\P204, DeviceObj)
    External (\P204.XSTA, MethodObj)
    External (\P205, DeviceObj)
    External (\P205.XSTA, MethodObj)
    External (\P206, DeviceObj)
    External (\P206.XSTA, MethodObj)
    External (\P207, DeviceObj)
    External (\P207.XSTA, MethodObj)
    External (\P208, DeviceObj)
    External (\P208.XSTA, MethodObj)
    External (\P209, DeviceObj)
    External (\P209.XSTA, MethodObj)
    External (\P210, DeviceObj)
    External (\P210.XSTA, MethodObj)
    External (\P211, DeviceObj)
    External (\P211.XSTA, MethodObj)
    External (\P212, DeviceObj)
    External (\P212.XSTA, MethodObj)
    External (\P213, DeviceObj)
    External (\P213.XSTA, MethodObj)
    External (\P214, DeviceObj)
    External (\P214.XSTA, MethodObj)
    External (\P215, DeviceObj)
    External (\P215.XSTA, MethodObj)
    External (\P216, DeviceObj)
    External (\P216.XSTA, MethodObj)
    External (\P217, DeviceObj)
    External (\P217.XSTA, MethodObj)
    External (\P218, DeviceObj)
    External (\P218.XSTA, MethodObj)
    External (\P219, DeviceObj)
    External (\P219.XSTA, MethodObj)
    External (\P220, DeviceObj)
    External (\P220.XSTA, MethodObj)
    External (\P221, DeviceObj)
    External (\P221.XSTA, MethodObj)
    External (\P222, DeviceObj)
    External (\P222.XSTA, MethodObj)
    External (\P223, DeviceObj)
    External (\P223.XSTA, MethodObj)
    External (\P224, DeviceObj)
    External (\P224.XSTA, MethodObj)
    External (\P225, DeviceObj)
    External (\P225.XSTA, MethodObj)
    External (\P226, DeviceObj)
    External (\P226.XSTA, MethodObj)
    External (\P227, DeviceObj)
    External (\P227.XSTA, MethodObj)
    External (\P228, DeviceObj)
    External (\P228.XSTA, MethodObj)
    External (\P229, DeviceObj)
    External (\P229.XSTA, MethodObj)
    External (\P230, DeviceObj)
    External (\P230.XSTA, MethodObj)
    External (\P231, DeviceObj)
    External (\P231.XSTA, MethodObj)
    External (\P232, DeviceObj)
    External (\P232.XSTA, MethodObj)
    External (\P233, DeviceObj)
    External (\P233.XSTA, MethodObj)
    External (\P234, DeviceObj)
    External (\P234.XSTA, MethodObj)
    External (\P235, DeviceObj)
    External (\P235.XSTA, MethodObj)
    External (\P236, DeviceObj)
    External (\P236.XSTA, MethodObj)
    External (\P237, DeviceObj)
    External (\P237.XSTA, MethodObj)
    External (\P238, DeviceObj)
    External (\P238.XSTA, MethodObj)
    External (\P239, DeviceObj)
    External (\P239.XSTA, MethodObj)
    External (\P240, DeviceObj)
    External (\P240.XSTA, MethodObj)

    //
    // Duplicated from Windows 8.1 Hyper-V Generation 2 DSDT.
    // Defines old-style Processor objects that macOS can use.
    //
    // PSTA will exist on 8.1 and older, and should block this SSDT from
    // being loaded if used on older Hyper-V versions.
    //
    Scope (\_SB)
    {
        Method (PSTA, 1, Serialized)
        {
            If (LAnd (_OSI ("Darwin"), LLessEqual (Arg0, PCNT)))
            {
                Return (0x0F)
            }

            Return (Zero)
        }

        Processor (P001, 0x01, 0x00000000, 0x00)
        {
            Name (PNUM, 0x01)
            Method (_STA, 0, Serialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P002, 0x02, 0x00000000, 0x00)
        {
            Name (PNUM, 0x02)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P003, 0x03, 0x00000000, 0x00)
        {
            Name (PNUM, 0x03)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P004, 0x04, 0x00000000, 0x00)
        {
            Name (PNUM, 0x04)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P005, 0x05, 0x00000000, 0x00)
        {
            Name (PNUM, 0x05)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P006, 0x06, 0x00000000, 0x00)
        {
            Name (PNUM, 0x06)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P007, 0x07, 0x00000000, 0x00)
        {
            Name (PNUM, 0x07)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P008, 0x08, 0x00000000, 0x00)
        {
            Name (PNUM, 0x08)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P009, 0x09, 0x00000000, 0x00)
        {
            Name (PNUM, 0x09)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P010, 0x0A, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0A)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P011, 0x0B, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0B)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P012, 0x0C, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0C)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P013, 0x0D, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0D)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P014, 0x0E, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0E)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P015, 0x0F, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0F)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P016, 0x10, 0x00000000, 0x00)
        {
            Name (PNUM, 0x10)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P017, 0x11, 0x00000000, 0x00)
        {
            Name (PNUM, 0x11)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P018, 0x12, 0x00000000, 0x00)
        {
            Name (PNUM, 0x12)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P019, 0x13, 0x00000000, 0x00)
        {
            Name (PNUM, 0x13)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P020, 0x14, 0x00000000, 0x00)
        {
            Name (PNUM, 0x14)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P021, 0x15, 0x00000000, 0x00)
        {
            Name (PNUM, 0x15)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P022, 0x16, 0x00000000, 0x00)
        {
            Name (PNUM, 0x16)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P023, 0x17, 0x00000000, 0x00)
        {
            Name (PNUM, 0x17)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P024, 0x18, 0x00000000, 0x00)
        {
            Name (PNUM, 0x18)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P025, 0x19, 0x00000000, 0x00)
        {
            Name (PNUM, 0x19)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P026, 0x1A, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1A)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P027, 0x1B, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1B)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P028, 0x1C, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1C)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P029, 0x1D, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1D)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P030, 0x1E, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1E)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P031, 0x1F, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1F)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P032, 0x20, 0x00000000, 0x00)
        {
            Name (PNUM, 0x20)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P033, 0x21, 0x00000000, 0x00)
        {
            Name (PNUM, 0x21)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P034, 0x22, 0x00000000, 0x00)
        {
            Name (PNUM, 0x22)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P035, 0x23, 0x00000000, 0x00)
        {
            Name (PNUM, 0x23)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P036, 0x24, 0x00000000, 0x00)
        {
            Name (PNUM, 0x24)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P037, 0x25, 0x00000000, 0x00)
        {
            Name (PNUM, 0x25)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P038, 0x26, 0x00000000, 0x00)
        {
            Name (PNUM, 0x26)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P039, 0x27, 0x00000000, 0x00)
        {
            Name (PNUM, 0x27)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P040, 0x28, 0x00000000, 0x00)
        {
            Name (PNUM, 0x28)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P041, 0x29, 0x00000000, 0x00)
        {
            Name (PNUM, 0x29)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P042, 0x2A, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2A)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P043, 0x2B, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2B)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P044, 0x2C, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2C)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P045, 0x2D, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2D)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P046, 0x2E, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2E)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P047, 0x2F, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2F)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P048, 0x30, 0x00000000, 0x00)
        {
            Name (PNUM, 0x30)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P049, 0x31, 0x00000000, 0x00)
        {
            Name (PNUM, 0x31)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P050, 0x32, 0x00000000, 0x00)
        {
            Name (PNUM, 0x32)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P051, 0x33, 0x00000000, 0x00)
        {
            Name (PNUM, 0x33)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P052, 0x34, 0x00000000, 0x00)
        {
            Name (PNUM, 0x34)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P053, 0x35, 0x00000000, 0x00)
        {
            Name (PNUM, 0x35)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P054, 0x36, 0x00000000, 0x00)
        {
            Name (PNUM, 0x36)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P055, 0x37, 0x00000000, 0x00)
        {
            Name (PNUM, 0x37)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P056, 0x38, 0x00000000, 0x00)
        {
            Name (PNUM, 0x38)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P057, 0x39, 0x00000000, 0x00)
        {
            Name (PNUM, 0x39)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P058, 0x3A, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3A)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P059, 0x3B, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3B)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P060, 0x3C, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3C)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P061, 0x3D, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3D)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P062, 0x3E, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3E)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P063, 0x3F, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3F)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P064, 0x40, 0x00000000, 0x00)
        {
            Name (PNUM, 0x40)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }
    }

    //
    // Older versions of macOS before 10.6 do not support conditional definitions
    // and will always expose the objects.
    // Re-evaluate in each object's _STA function as a workaround.
    //
    // Serial port devices.
    //
    If (LGreater (SCFG, Zero))
    {
        Scope (\_SB.UAR1)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (SCFG, Zero))
                {
                    Return (0x0F)
                }

                Return (Zero)
            }
        }

        Scope (\_SB.UAR2)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (SCFG, Zero))
                {
                    Return (0x0F)
                }

                Return (Zero)
            }
        }
    }

    //
    // TPM device.
    //
    If (LGreater (TCFG, Zero))
    {
        Scope (\_SB.VMOD.TPM2)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (TCFG, Zero))
                {
                    Return (\_SB.VMOD.TPM2.XSTA())
                }

                Return (Zero)
            }
        }
    }

    //
    // NVDIMM device.
    //
    If (LGreater (NCFG, Zero))
    {
        Scope (\_SB.NVDR)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (NCFG, Zero))
                {
                    Return (\_SB.NVDR.XSTA())
                }

                Return (Zero)
            }
        }
    }

    //
    // Enclave Page Cache device.
    //
       If (LGreater (SGXE, Zero))
    {
        Scope (\_SB.EPC)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (SGXE, Zero))    
                {
                    Return (\_SB.EPC.XSTA())
                }

                Return (Zero)
            }
        }
    }

    //
    // Battery and AC adapter device.
    //
    If (LGreater (BCFG, Zero))
    {
        Scope (\_SB.VMOD.BAT1)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (BCFG, Zero))
                {
                    Return (\_SB.VMOD.BAT1.XSTA())
                }

                Return (Zero)
            }
        }

        Scope (\_SB.VMOD.AC1)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (BCFG, Zero))
                {
                    Return (0x0F)
                }

                Return (Zero)
            }
        }
    }

    //
    // New-style ACPI0007 processor devices are not supported under macOS.
    // Versions of macOS that support conditional definitions seem to always enable the below objects anyway.
    //
       If (LLessEqual (0x01, PCNT))
    {
        Scope (\P001)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01, PCNT))
                {
                    Return (\P001.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02, PCNT))
    {
        Scope (\P002)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02, PCNT))
                {
                    Return (\P002.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03, PCNT))
    {
        Scope (\P003)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03, PCNT))
                {
                    Return (\P003.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04, PCNT))
    {
        Scope (\P004)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04, PCNT))
                {
                    Return (\P004.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05, PCNT))
    {
        Scope (\P005)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05, PCNT))
                {
                    Return (\P005.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06, PCNT))
    {
        Scope (\P006)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06, PCNT))
                {
                    Return (\P006.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07, PCNT))
    {
        Scope (\P007)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07, PCNT))
                {
                    Return (\P007.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x08, PCNT))
    {
        Scope (\P008)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x08, PCNT))
                {
                    Return (\P008.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x09, PCNT))
    {
        Scope (\P009)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x09, PCNT))
                {
                    Return (\P009.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A, PCNT))
    {
        Scope (\P010)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A, PCNT))
                {
                    Return (\P010.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B, PCNT))
    {
        Scope (\P011)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B, PCNT))
                {
                    Return (\P011.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C, PCNT))
    {
        Scope (\P012)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C, PCNT))
                {
                    Return (\P012.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D, PCNT))
    {
        Scope (\P013)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D, PCNT))
                {
                    Return (\P013.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E, PCNT))
    {
        Scope (\P014)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E, PCNT))
                {
                    Return (\P014.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0F, PCNT))
    {
        Scope (\P015)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F, PCNT))
                {
                    Return (\P015.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x010, PCNT))
    {
        Scope (\P016)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x010, PCNT))
                {
                    Return (\P016.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x011, PCNT))
    {
        Scope (\P017)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x011, PCNT))
                {
                    Return (\P017.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x012, PCNT))
    {
        Scope (\P018)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x012, PCNT))
                {
                    Return (\P018.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x013, PCNT))
    {
        Scope (\P019)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x013, PCNT))
                {
                    Return (\P019.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x014, PCNT))
    {
        Scope (\P020)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x014, PCNT))
                {
                    Return (\P020.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x015, PCNT))
    {
        Scope (\P021)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x015, PCNT))
                {
                    Return (\P021.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x016, PCNT))
    {
        Scope (\P022)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x016, PCNT))
                {
                    Return (\P022.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x017, PCNT))
    {
        Scope (\P023)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x017, PCNT))
                {
                    Return (\P023.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x018, PCNT))
    {
        Scope (\P024)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x018, PCNT))
                {
                    Return (\P024.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x019, PCNT))
    {
        Scope (\P025)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x019, PCNT))
                {
                    Return (\P025.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A, PCNT))
    {
        Scope (\P026)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A, PCNT))
                {
                    Return (\P026.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B, PCNT))
    {
        Scope (\P027)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B, PCNT))
                {
                    Return (\P027.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C, PCNT))
    {
        Scope (\P028)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C, PCNT))
                {
                    Return (\P028.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D, PCNT))
    {
        Scope (\P029)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D, PCNT))
                {
                    Return (\P029.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E, PCNT))
    {
        Scope (\P030)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E, PCNT))
                {
                    Return (\P030.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F, PCNT))
    {
        Scope (\P031)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F, PCNT))
                {
                    Return (\P031.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x020, PCNT))
    {
        Scope (\P032)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x020, PCNT))
                {
                    Return (\P032.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x021, PCNT))
    {
        Scope (\P033)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x021, PCNT))
                {
                    Return (\P033.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x022, PCNT))
    {
        Scope (\P034)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x022, PCNT))
                {
                    Return (\P034.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x023, PCNT))
    {
        Scope (\P035)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x023, PCNT))
                {
                    Return (\P035.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x024, PCNT))
    {
        Scope (\P036)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x024, PCNT))
                {
                    Return (\P036.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x025, PCNT))
    {
        Scope (\P037)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x025, PCNT))
                {
                    Return (\P037.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x026, PCNT))
    {
        Scope (\P038)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x026, PCNT))
                {
                    Return (\P038.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x027, PCNT))
    {
        Scope (\P039)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x027, PCNT))
                {
                    Return (\P039.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x028, PCNT))
    {
        Scope (\P040)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x028, PCNT))
                {
                    Return (\P040.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x029, PCNT))
    {
        Scope (\P041)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x029, PCNT))
                {
                    Return (\P041.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A, PCNT))
    {
        Scope (\P042)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A, PCNT))
                {
                    Return (\P042.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B, PCNT))
    {
        Scope (\P043)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B, PCNT))
                {
                    Return (\P043.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C, PCNT))
    {
        Scope (\P044)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C, PCNT))
                {
                    Return (\P044.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D, PCNT))
    {
        Scope (\P045)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D, PCNT))
                {
                    Return (\P045.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E, PCNT))
    {
        Scope (\P046)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E, PCNT))
                {
                    Return (\P046.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F, PCNT))
    {
        Scope (\P047)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F, PCNT))
                {
                    Return (\P047.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x030, PCNT))
    {
        Scope (\P048)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x030, PCNT))
                {
                    Return (\P048.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x031, PCNT))
    {
        Scope (\P049)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x031, PCNT))
                {
                    Return (\P049.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x032, PCNT))
    {
        Scope (\P050)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x032, PCNT))
                {
                    Return (\P050.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x033, PCNT))
    {
        Scope (\P051)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x033, PCNT))
                {
                    Return (\P051.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x034, PCNT))
    {
        Scope (\P052)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x034, PCNT))
                {
                    Return (\P052.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x035, PCNT))
    {
        Scope (\P053)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x035, PCNT))
                {
                    Return (\P053.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x036, PCNT))
    {
        Scope (\P054)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x036, PCNT))
                {
                    Return (\P054.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x037, PCNT))
    {
        Scope (\P055)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x037, PCNT))
                {
                    Return (\P055.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x038, PCNT))
    {
        Scope (\P056)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x038, PCNT))
                {
                    Return (\P056.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x039, PCNT))
    {
        Scope (\P057)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x039, PCNT))
                {
                    Return (\P057.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A, PCNT))
    {
        Scope (\P058)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A, PCNT))
                {
                    Return (\P058.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B, PCNT))
    {
        Scope (\P059)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B, PCNT))
                {
                    Return (\P059.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C, PCNT))
    {
        Scope (\P060)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C, PCNT))
                {
                    Return (\P060.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D, PCNT))
    {
        Scope (\P061)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D, PCNT))
                {
                    Return (\P061.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E, PCNT))
    {
        Scope (\P062)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E, PCNT))
                {
                    Return (\P062.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F, PCNT))
    {
        Scope (\P063)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F, PCNT))
                {
                    Return (\P063.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x040, PCNT))
    {
        Scope (\P064)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x040, PCNT))
                {
                    Return (\P064.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x041, PCNT))
    {
        Scope (\P065)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x041, PCNT))
                {
                    Return (\P065.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x042, PCNT))
    {
        Scope (\P066)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x042, PCNT))
                {
                    Return (\P066.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x043, PCNT))
    {
        Scope (\P067)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x043, PCNT))
                {
                    Return (\P067.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x044, PCNT))
    {
        Scope (\P068)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x044, PCNT))
                {
                    Return (\P068.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x045, PCNT))
    {
        Scope (\P069)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x045, PCNT))
                {
                    Return (\P069.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x046, PCNT))
    {
        Scope (\P070)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x046, PCNT))
                {
                    Return (\P070.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x047, PCNT))
    {
        Scope (\P071)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x047, PCNT))
                {
                    Return (\P071.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x048, PCNT))
    {
        Scope (\P072)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x048, PCNT))
                {
                    Return (\P072.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x049, PCNT))
    {
        Scope (\P073)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x049, PCNT))
                {
                    Return (\P073.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A, PCNT))
    {
        Scope (\P074)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A, PCNT))
                {
                    Return (\P074.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B, PCNT))
    {
        Scope (\P075)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B, PCNT))
                {
                    Return (\P075.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C, PCNT))
    {
        Scope (\P076)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C, PCNT))
                {
                    Return (\P076.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D, PCNT))
    {
        Scope (\P077)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D, PCNT))
                {
                    Return (\P077.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E, PCNT))
    {
        Scope (\P078)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E, PCNT))
                {
                    Return (\P078.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F, PCNT))
    {
        Scope (\P079)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F, PCNT))
                {
                    Return (\P079.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x050, PCNT))
    {
        Scope (\P080)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x050, PCNT))
                {
                    Return (\P080.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x051, PCNT))
    {
        Scope (\P081)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x051, PCNT))
                {
                    Return (\P081.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x052, PCNT))
    {
        Scope (\P082)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x052, PCNT))
                {
                    Return (\P082.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x053, PCNT))
    {
        Scope (\P083)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x053, PCNT))
                {
                    Return (\P083.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x054, PCNT))
    {
        Scope (\P084)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x054, PCNT))
                {
                    Return (\P084.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x055, PCNT))
    {
        Scope (\P085)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x055, PCNT))
                {
                    Return (\P085.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x056, PCNT))
    {
        Scope (\P086)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x056, PCNT))
                {
                    Return (\P086.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x057, PCNT))
    {
        Scope (\P087)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x057, PCNT))
                {
                    Return (\P087.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x058, PCNT))
    {
        Scope (\P088)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x058, PCNT))
                {
                    Return (\P088.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x059, PCNT))
    {
        Scope (\P089)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x059, PCNT))
                {
                    Return (\P089.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A, PCNT))
    {
        Scope (\P090)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A, PCNT))
                {
                    Return (\P090.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B, PCNT))
    {
        Scope (\P091)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B, PCNT))
                {
                    Return (\P091.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C, PCNT))
    {
        Scope (\P092)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C, PCNT))
                {
                    Return (\P092.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D, PCNT))
    {
        Scope (\P093)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D, PCNT))
                {
                    Return (\P093.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E, PCNT))
    {
        Scope (\P094)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E, PCNT))
                {
                    Return (\P094.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F, PCNT))
    {
        Scope (\P095)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F, PCNT))
                {
                    Return (\P095.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x060, PCNT))
    {
        Scope (\P096)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x060, PCNT))
                {
                    Return (\P096.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x061, PCNT))
    {
        Scope (\P097)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x061, PCNT))
                {
                    Return (\P097.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x062, PCNT))
    {
        Scope (\P098)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x062, PCNT))
                {
                    Return (\P098.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x063, PCNT))
    {
        Scope (\P099)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x063, PCNT))
                {
                    Return (\P099.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x064, PCNT))
    {
        Scope (\P100)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x064, PCNT))
                {
                    Return (\P100.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x065, PCNT))
    {
        Scope (\P101)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x065, PCNT))
                {
                    Return (\P101.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x066, PCNT))
    {
        Scope (\P102)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x066, PCNT))
                {
                    Return (\P102.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x067, PCNT))
    {
        Scope (\P103)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x067, PCNT))
                {
                    Return (\P103.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x068, PCNT))
    {
        Scope (\P104)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x068, PCNT))
                {
                    Return (\P104.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x069, PCNT))
    {
        Scope (\P105)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x069, PCNT))
                {
                    Return (\P105.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A, PCNT))
    {
        Scope (\P106)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A, PCNT))
                {
                    Return (\P106.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B, PCNT))
    {
        Scope (\P107)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B, PCNT))
                {
                    Return (\P107.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C, PCNT))
    {
        Scope (\P108)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C, PCNT))
                {
                    Return (\P108.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D, PCNT))
    {
        Scope (\P109)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D, PCNT))
                {
                    Return (\P109.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E, PCNT))
    {
        Scope (\P110)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E, PCNT))
                {
                    Return (\P110.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F, PCNT))
    {
        Scope (\P111)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F, PCNT))
                {
                    Return (\P111.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x070, PCNT))
    {
        Scope (\P112)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x070, PCNT))
                {
                    Return (\P112.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x071, PCNT))
    {
        Scope (\P113)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x071, PCNT))
                {
                    Return (\P113.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x072, PCNT))
    {
        Scope (\P114)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x072, PCNT))
                {
                    Return (\P114.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x073, PCNT))
    {
        Scope (\P115)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x073, PCNT))
                {
                    Return (\P115.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x074, PCNT))
    {
        Scope (\P116)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x074, PCNT))
                {
                    Return (\P116.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x075, PCNT))
    {
        Scope (\P117)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x075, PCNT))
                {
                    Return (\P117.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x076, PCNT))
    {
        Scope (\P118)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x076, PCNT))
                {
                    Return (\P118.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x077, PCNT))
    {
        Scope (\P119)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x077, PCNT))
                {
                    Return (\P119.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x078, PCNT))
    {
        Scope (\P120)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x078, PCNT))
                {
                    Return (\P120.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x079, PCNT))
    {
        Scope (\P121)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x079, PCNT))
                {
                    Return (\P121.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A, PCNT))
    {
        Scope (\P122)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A, PCNT))
                {
                    Return (\P122.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B, PCNT))
    {
        Scope (\P123)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B, PCNT))
                {
                    Return (\P123.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C, PCNT))
    {
        Scope (\P124)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C, PCNT))
                {
                    Return (\P124.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D, PCNT))
    {
        Scope (\P125)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D, PCNT))
                {
                    Return (\P125.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E, PCNT))
    {
        Scope (\P126)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E, PCNT))
                {
                    Return (\P126.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F, PCNT))
    {
        Scope (\P127)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F, PCNT))
                {
                    Return (\P127.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x080, PCNT))
    {
        Scope (\P128)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x080, PCNT))
                {
                    Return (\P128.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x081, PCNT))
    {
        Scope (\P129)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x081, PCNT))
                {
                    Return (\P129.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x082, PCNT))
    {
        Scope (\P130)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x082, PCNT))
                {
                    Return (\P130.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x083, PCNT))
    {
        Scope (\P131)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x083, PCNT))
                {
                    Return (\P131.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x084, PCNT))
    {
        Scope (\P132)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x084, PCNT))
                {
                    Return (\P132.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x085, PCNT))
    {
        Scope (\P133)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x085, PCNT))
                {
                    Return (\P133.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x086, PCNT))
    {
        Scope (\P134)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x086, PCNT))
                {
                    Return (\P134.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x087, PCNT))
    {
        Scope (\P135)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x087, PCNT))
                {
                    Return (\P135.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x088, PCNT))
    {
        Scope (\P136)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x088, PCNT))
                {
                    Return (\P136.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x089, PCNT))
    {
        Scope (\P137)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x089, PCNT))
                {
                    Return (\P137.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x08A, PCNT))
    {
        Scope (\P138)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x08A, PCNT))
                {
                    Return (\P138.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x08B, PCNT))
    {
        Scope (\P139)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x08B, PCNT))
                {
                    Return (\P139.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x08C, PCNT))
    {
        Scope (\P140)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x08C, PCNT))
                {
                    Return (\P140.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x08D, PCNT))
    {
        Scope (\P141)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x08D, PCNT))
                {
                    Return (\P141.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x08E, PCNT))
    {
        Scope (\P142)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x08E, PCNT))
                {
                    Return (\P142.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x08F, PCNT))
    {
        Scope (\P143)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x08F, PCNT))
                {
                    Return (\P143.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x090, PCNT))
    {
        Scope (\P144)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x090, PCNT))
                {
                    Return (\P144.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x091, PCNT))
    {
        Scope (\P145)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x091, PCNT))
                {
                    Return (\P145.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x092, PCNT))
    {
        Scope (\P146)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x092, PCNT))
                {
                    Return (\P146.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x093, PCNT))
    {
        Scope (\P147)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x093, PCNT))
                {
                    Return (\P147.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x094, PCNT))
    {
        Scope (\P148)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x094, PCNT))
                {
                    Return (\P148.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x095, PCNT))
    {
        Scope (\P149)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x095, PCNT))
                {
                    Return (\P149.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x096, PCNT))
    {
        Scope (\P150)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x096, PCNT))
                {
                    Return (\P150.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x097, PCNT))
    {
        Scope (\P151)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x097, PCNT))
                {
                    Return (\P151.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x098, PCNT))
    {
        Scope (\P152)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x098, PCNT))
                {
                    Return (\P152.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x099, PCNT))
    {
        Scope (\P153)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x099, PCNT))
                {
                    Return (\P153.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x09A, PCNT))
    {
        Scope (\P154)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x09A, PCNT))
                {
                    Return (\P154.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x09B, PCNT))
    {
        Scope (\P155)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x09B, PCNT))
                {
                    Return (\P155.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x09C, PCNT))
    {
        Scope (\P156)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x09C, PCNT))
                {
                    Return (\P156.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x09D, PCNT))
    {
        Scope (\P157)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x09D, PCNT))
                {
                    Return (\P157.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x09E, PCNT))
    {
        Scope (\P158)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x09E, PCNT))
                {
                    Return (\P158.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x09F, PCNT))
    {
        Scope (\P159)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x09F, PCNT))
                {
                    Return (\P159.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A0, PCNT))
    {
        Scope (\P160)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A0, PCNT))
                {
                    Return (\P160.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A1, PCNT))
    {
        Scope (\P161)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A1, PCNT))
                {
                    Return (\P161.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A2, PCNT))
    {
        Scope (\P162)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A2, PCNT))
                {
                    Return (\P162.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A3, PCNT))
    {
        Scope (\P163)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A3, PCNT))
                {
                    Return (\P163.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A4, PCNT))
    {
        Scope (\P164)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A4, PCNT))
                {
                    Return (\P164.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A5, PCNT))
    {
        Scope (\P165)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A5, PCNT))
                {
                    Return (\P165.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A6, PCNT))
    {
        Scope (\P166)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A6, PCNT))
                {
                    Return (\P166.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A7, PCNT))
    {
        Scope (\P167)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A7, PCNT))
                {
                    Return (\P167.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A8, PCNT))
    {
        Scope (\P168)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A8, PCNT))
                {
                    Return (\P168.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0A9, PCNT))
    {
        Scope (\P169)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0A9, PCNT))
                {
                    Return (\P169.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0AA, PCNT))
    {
        Scope (\P170)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0AA, PCNT))
                {
                    Return (\P170.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0AB, PCNT))
    {
        Scope (\P171)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0AB, PCNT))
                {
                    Return (\P171.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0AC, PCNT))
    {
        Scope (\P172)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0AC, PCNT))
                {
                    Return (\P172.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0AD, PCNT))
    {
        Scope (\P173)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0AD, PCNT))
                {
                    Return (\P173.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0AE, PCNT))
    {
        Scope (\P174)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0AE, PCNT))
                {
                    Return (\P174.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0AF, PCNT))
    {
        Scope (\P175)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0AF, PCNT))
                {
                    Return (\P175.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B0, PCNT))
    {
        Scope (\P176)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B0, PCNT))
                {
                    Return (\P176.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B1, PCNT))
    {
        Scope (\P177)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B1, PCNT))
                {
                    Return (\P177.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B2, PCNT))
    {
        Scope (\P178)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B2, PCNT))
                {
                    Return (\P178.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B3, PCNT))
    {
        Scope (\P179)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B3, PCNT))
                {
                    Return (\P179.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B4, PCNT))
    {
        Scope (\P180)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B4, PCNT))
                {
                    Return (\P180.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B5, PCNT))
    {
        Scope (\P181)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B5, PCNT))
                {
                    Return (\P181.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B6, PCNT))
    {
        Scope (\P182)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B6, PCNT))
                {
                    Return (\P182.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B7, PCNT))
    {
        Scope (\P183)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B7, PCNT))
                {
                    Return (\P183.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B8, PCNT))
    {
        Scope (\P184)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B8, PCNT))
                {
                    Return (\P184.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0B9, PCNT))
    {
        Scope (\P185)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0B9, PCNT))
                {
                    Return (\P185.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0BA, PCNT))
    {
        Scope (\P186)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0BA, PCNT))
                {
                    Return (\P186.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0BB, PCNT))
    {
        Scope (\P187)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0BB, PCNT))
                {
                    Return (\P187.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0BC, PCNT))
    {
        Scope (\P188)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0BC, PCNT))
                {
                    Return (\P188.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0BD, PCNT))
    {
        Scope (\P189)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0BD, PCNT))
                {
                    Return (\P189.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0BE, PCNT))
    {
        Scope (\P190)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0BE, PCNT))
                {
                    Return (\P190.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0BF, PCNT))
    {
        Scope (\P191)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0BF, PCNT))
                {
                    Return (\P191.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C0, PCNT))
    {
        Scope (\P192)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C0, PCNT))
                {
                    Return (\P192.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C1, PCNT))
    {
        Scope (\P193)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C1, PCNT))
                {
                    Return (\P193.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C2, PCNT))
    {
        Scope (\P194)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C2, PCNT))
                {
                    Return (\P194.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C3, PCNT))
    {
        Scope (\P195)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C3, PCNT))
                {
                    Return (\P195.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C4, PCNT))
    {
        Scope (\P196)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C4, PCNT))
                {
                    Return (\P196.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C5, PCNT))
    {
        Scope (\P197)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C5, PCNT))
                {
                    Return (\P197.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C6, PCNT))
    {
        Scope (\P198)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C6, PCNT))
                {
                    Return (\P198.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C7, PCNT))
    {
        Scope (\P199)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C7, PCNT))
                {
                    Return (\P199.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C8, PCNT))
    {
        Scope (\P200)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C8, PCNT))
                {
                    Return (\P200.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0C9, PCNT))
    {
        Scope (\P201)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0C9, PCNT))
                {
                    Return (\P201.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0CA, PCNT))
    {
        Scope (\P202)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0CA, PCNT))
                {
                    Return (\P202.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0CB, PCNT))
    {
        Scope (\P203)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0CB, PCNT))
                {
                    Return (\P203.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0CC, PCNT))
    {
        Scope (\P204)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0CC, PCNT))
                {
                    Return (\P204.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0CD, PCNT))
    {
        Scope (\P205)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0CD, PCNT))
                {
                    Return (\P205.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0CE, PCNT))
    {
        Scope (\P206)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0CE, PCNT))
                {
                    Return (\P206.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0CF, PCNT))
    {
        Scope (\P207)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0CF, PCNT))
                {
                    Return (\P207.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D0, PCNT))
    {
        Scope (\P208)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D0, PCNT))
                {
                    Return (\P208.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D1, PCNT))
    {
        Scope (\P209)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D1, PCNT))
                {
                    Return (\P209.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D2, PCNT))
    {
        Scope (\P210)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D2, PCNT))
                {
                    Return (\P210.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D3, PCNT))
    {
        Scope (\P211)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D3, PCNT))
                {
                    Return (\P211.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D4, PCNT))
    {
        Scope (\P212)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D4, PCNT))
                {
                    Return (\P212.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D5, PCNT))
    {
        Scope (\P213)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D5, PCNT))
                {
                    Return (\P213.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D6, PCNT))
    {
        Scope (\P214)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D6, PCNT))
                {
                    Return (\P214.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D7, PCNT))
    {
        Scope (\P215)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D7, PCNT))
                {
                    Return (\P215.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D8, PCNT))
    {
        Scope (\P216)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D8, PCNT))
                {
                    Return (\P216.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0D9, PCNT))
    {
        Scope (\P217)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0D9, PCNT))
                {
                    Return (\P217.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0DA, PCNT))
    {
        Scope (\P218)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0DA, PCNT))
                {
                    Return (\P218.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0DB, PCNT))
    {
        Scope (\P219)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0DB, PCNT))
                {
                    Return (\P219.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0DC, PCNT))
    {
        Scope (\P220)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0DC, PCNT))
                {
                    Return (\P220.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0DD, PCNT))
    {
        Scope (\P221)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0DD, PCNT))
                {
                    Return (\P221.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0DE, PCNT))
    {
        Scope (\P222)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0DE, PCNT))
                {
                    Return (\P222.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0DF, PCNT))
    {
        Scope (\P223)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0DF, PCNT))
                {
                    Return (\P223.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E0, PCNT))
    {
        Scope (\P224)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E0, PCNT))
                {
                    Return (\P224.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E1, PCNT))
    {
        Scope (\P225)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E1, PCNT))
                {
                    Return (\P225.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E2, PCNT))
    {
        Scope (\P226)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E2, PCNT))
                {
                    Return (\P226.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E3, PCNT))
    {
        Scope (\P227)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E3, PCNT))
                {
                    Return (\P227.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E4, PCNT))
    {
        Scope (\P228)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E4, PCNT))
                {
                    Return (\P228.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E5, PCNT))
    {
        Scope (\P229)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E5, PCNT))
                {
                    Return (\P229.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E6, PCNT))
    {
        Scope (\P230)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E6, PCNT))
                {
                    Return (\P230.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E7, PCNT))
    {
        Scope (\P231)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E7, PCNT))
                {
                    Return (\P231.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E8, PCNT))
    {
        Scope (\P232)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E8, PCNT))
                {
                    Return (\P232.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0E9, PCNT))
    {
        Scope (\P233)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0E9, PCNT))
                {
                    Return (\P233.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0EA, PCNT))
    {
        Scope (\P234)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0EA, PCNT))
                {
                    Return (\P234.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0EB, PCNT))
    {
        Scope (\P235)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0EB, PCNT))
                {
                    Return (\P235.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0EC, PCNT))
    {
        Scope (\P236)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0EC, PCNT))
                {
                    Return (\P236.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0ED, PCNT))
    {
        Scope (\P237)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0ED, PCNT))
                {
                    Return (\P237.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0EE, PCNT))
    {
        Scope (\P238)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0EE, PCNT))
                {
                    Return (\P238.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0EF, PCNT))
    {
        Scope (\P239)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0EF, PCNT))
                {
                    Return (\P239.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0F0, PCNT))
    {
        Scope (\P240)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F0, PCNT))
                {
                    Return (\P240.XSTA())
                }

                Return (Zero)
            }
        }
    }
}
