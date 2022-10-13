/*
 * Hyper-V device SSDT to disable unsupported devices under macOS when running under Windows Server 2022 / Windows 11, or newer.
 *
 * Windows 10 and newer define various objects conditionally, which are
 * unsupported in older versions of macOS. All objects are enabled regardless
 * of state in older versions, causing slow boot behavior and crashes during boot caused by AppleACPIPlatform.
 *
 * Windows 10 and newer also use ACPI0007 Device objects for CPUs, which macOS cannot detect.
 * 
 * This SSDT defines _STA methods for all affected objects and disables them entirely, or enables only if
 * the appropriate register is set (enabled in Hyper-V).
 *
 * On Windows Server 2019 and older this SSDT is unnecessary and can be removed.
 *
 * Requires the following ACPI patches:
 * (1) Base:            \P241
 *     Comment:         _STA to XSTA rename (additional Hyper-V processors)
 *     Count:           1808
 *     Find:            _STA
 *     Replace:         XSTA
 *     TableSignature:  44534454 (DSDT)
 */

DefinitionBlock ("", "SSDT", 2, "ACDT", "HVDEVW22", 0x00000000)
{
    //
    // EC fields in DSDT
    //
    External (PCNT, FieldUnitObj)
    External (PADE, FieldUnitObj)

    //
    // Device and method objects in DSDT
    //
    External (\_SB.VMOD.PAD1, DeviceObj)
    External (\P241, DeviceObj)
    External (\P241.XSTA, MethodObj)
    External (\P242, DeviceObj)
    External (\P242.XSTA, MethodObj)
    External (\P243, DeviceObj)
    External (\P243.XSTA, MethodObj)
    External (\P244, DeviceObj)
    External (\P244.XSTA, MethodObj)
    External (\P245, DeviceObj)
    External (\P245.XSTA, MethodObj)
    External (\P246, DeviceObj)
    External (\P246.XSTA, MethodObj)
    External (\P247, DeviceObj)
    External (\P247.XSTA, MethodObj)
    External (\P248, DeviceObj)
    External (\P248.XSTA, MethodObj)
    External (\P249, DeviceObj)
    External (\P249.XSTA, MethodObj)
    External (\P250, DeviceObj)
    External (\P250.XSTA, MethodObj)
    External (\P251, DeviceObj)
    External (\P251.XSTA, MethodObj)
    External (\P252, DeviceObj)
    External (\P252.XSTA, MethodObj)
    External (\P253, DeviceObj)
    External (\P253.XSTA, MethodObj)
    External (\P254, DeviceObj)
    External (\P254.XSTA, MethodObj)
    External (\P255, DeviceObj)
    External (\P255.XSTA, MethodObj)
    External (\P256, DeviceObj)
    External (\P256.XSTA, MethodObj)
    External (\P257, DeviceObj)
    External (\P257.XSTA, MethodObj)
    External (\P258, DeviceObj)
    External (\P258.XSTA, MethodObj)
    External (\P259, DeviceObj)
    External (\P259.XSTA, MethodObj)
    External (\P260, DeviceObj)
    External (\P260.XSTA, MethodObj)
    External (\P261, DeviceObj)
    External (\P261.XSTA, MethodObj)
    External (\P262, DeviceObj)
    External (\P262.XSTA, MethodObj)
    External (\P263, DeviceObj)
    External (\P263.XSTA, MethodObj)
    External (\P264, DeviceObj)
    External (\P264.XSTA, MethodObj)
    External (\P265, DeviceObj)
    External (\P265.XSTA, MethodObj)
    External (\P266, DeviceObj)
    External (\P266.XSTA, MethodObj)
    External (\P267, DeviceObj)
    External (\P267.XSTA, MethodObj)
    External (\P268, DeviceObj)
    External (\P268.XSTA, MethodObj)
    External (\P269, DeviceObj)
    External (\P269.XSTA, MethodObj)
    External (\P270, DeviceObj)
    External (\P270.XSTA, MethodObj)
    External (\P271, DeviceObj)
    External (\P271.XSTA, MethodObj)
    External (\P272, DeviceObj)
    External (\P272.XSTA, MethodObj)
    External (\P273, DeviceObj)
    External (\P273.XSTA, MethodObj)
    External (\P274, DeviceObj)
    External (\P274.XSTA, MethodObj)
    External (\P275, DeviceObj)
    External (\P275.XSTA, MethodObj)
    External (\P276, DeviceObj)
    External (\P276.XSTA, MethodObj)
    External (\P277, DeviceObj)
    External (\P277.XSTA, MethodObj)
    External (\P278, DeviceObj)
    External (\P278.XSTA, MethodObj)
    External (\P279, DeviceObj)
    External (\P279.XSTA, MethodObj)
    External (\P280, DeviceObj)
    External (\P280.XSTA, MethodObj)
    External (\P281, DeviceObj)
    External (\P281.XSTA, MethodObj)
    External (\P282, DeviceObj)
    External (\P282.XSTA, MethodObj)
    External (\P283, DeviceObj)
    External (\P283.XSTA, MethodObj)
    External (\P284, DeviceObj)
    External (\P284.XSTA, MethodObj)
    External (\P285, DeviceObj)
    External (\P285.XSTA, MethodObj)
    External (\P286, DeviceObj)
    External (\P286.XSTA, MethodObj)
    External (\P287, DeviceObj)
    External (\P287.XSTA, MethodObj)
    External (\P288, DeviceObj)
    External (\P288.XSTA, MethodObj)
    External (\P289, DeviceObj)
    External (\P289.XSTA, MethodObj)
    External (\P290, DeviceObj)
    External (\P290.XSTA, MethodObj)
    External (\P291, DeviceObj)
    External (\P291.XSTA, MethodObj)
    External (\P292, DeviceObj)
    External (\P292.XSTA, MethodObj)
    External (\P293, DeviceObj)
    External (\P293.XSTA, MethodObj)
    External (\P294, DeviceObj)
    External (\P294.XSTA, MethodObj)
    External (\P295, DeviceObj)
    External (\P295.XSTA, MethodObj)
    External (\P296, DeviceObj)
    External (\P296.XSTA, MethodObj)
    External (\P297, DeviceObj)
    External (\P297.XSTA, MethodObj)
    External (\P298, DeviceObj)
    External (\P298.XSTA, MethodObj)
    External (\P299, DeviceObj)
    External (\P299.XSTA, MethodObj)
    External (\P300, DeviceObj)
    External (\P300.XSTA, MethodObj)
    External (\P301, DeviceObj)
    External (\P301.XSTA, MethodObj)
    External (\P302, DeviceObj)
    External (\P302.XSTA, MethodObj)
    External (\P303, DeviceObj)
    External (\P303.XSTA, MethodObj)
    External (\P304, DeviceObj)
    External (\P304.XSTA, MethodObj)
    External (\P305, DeviceObj)
    External (\P305.XSTA, MethodObj)
    External (\P306, DeviceObj)
    External (\P306.XSTA, MethodObj)
    External (\P307, DeviceObj)
    External (\P307.XSTA, MethodObj)
    External (\P308, DeviceObj)
    External (\P308.XSTA, MethodObj)
    External (\P309, DeviceObj)
    External (\P309.XSTA, MethodObj)
    External (\P310, DeviceObj)
    External (\P310.XSTA, MethodObj)
    External (\P311, DeviceObj)
    External (\P311.XSTA, MethodObj)
    External (\P312, DeviceObj)
    External (\P312.XSTA, MethodObj)
    External (\P313, DeviceObj)
    External (\P313.XSTA, MethodObj)
    External (\P314, DeviceObj)
    External (\P314.XSTA, MethodObj)
    External (\P315, DeviceObj)
    External (\P315.XSTA, MethodObj)
    External (\P316, DeviceObj)
    External (\P316.XSTA, MethodObj)
    External (\P317, DeviceObj)
    External (\P317.XSTA, MethodObj)
    External (\P318, DeviceObj)
    External (\P318.XSTA, MethodObj)
    External (\P319, DeviceObj)
    External (\P319.XSTA, MethodObj)
    External (\P320, DeviceObj)
    External (\P320.XSTA, MethodObj)
    External (\P321, DeviceObj)
    External (\P321.XSTA, MethodObj)
    External (\P322, DeviceObj)
    External (\P322.XSTA, MethodObj)
    External (\P323, DeviceObj)
    External (\P323.XSTA, MethodObj)
    External (\P324, DeviceObj)
    External (\P324.XSTA, MethodObj)
    External (\P325, DeviceObj)
    External (\P325.XSTA, MethodObj)
    External (\P326, DeviceObj)
    External (\P326.XSTA, MethodObj)
    External (\P327, DeviceObj)
    External (\P327.XSTA, MethodObj)
    External (\P328, DeviceObj)
    External (\P328.XSTA, MethodObj)
    External (\P329, DeviceObj)
    External (\P329.XSTA, MethodObj)
    External (\P330, DeviceObj)
    External (\P330.XSTA, MethodObj)
    External (\P331, DeviceObj)
    External (\P331.XSTA, MethodObj)
    External (\P332, DeviceObj)
    External (\P332.XSTA, MethodObj)
    External (\P333, DeviceObj)
    External (\P333.XSTA, MethodObj)
    External (\P334, DeviceObj)
    External (\P334.XSTA, MethodObj)
    External (\P335, DeviceObj)
    External (\P335.XSTA, MethodObj)
    External (\P336, DeviceObj)
    External (\P336.XSTA, MethodObj)
    External (\P337, DeviceObj)
    External (\P337.XSTA, MethodObj)
    External (\P338, DeviceObj)
    External (\P338.XSTA, MethodObj)
    External (\P339, DeviceObj)
    External (\P339.XSTA, MethodObj)
    External (\P340, DeviceObj)
    External (\P340.XSTA, MethodObj)
    External (\P341, DeviceObj)
    External (\P341.XSTA, MethodObj)
    External (\P342, DeviceObj)
    External (\P342.XSTA, MethodObj)
    External (\P343, DeviceObj)
    External (\P343.XSTA, MethodObj)
    External (\P344, DeviceObj)
    External (\P344.XSTA, MethodObj)
    External (\P345, DeviceObj)
    External (\P345.XSTA, MethodObj)
    External (\P346, DeviceObj)
    External (\P346.XSTA, MethodObj)
    External (\P347, DeviceObj)
    External (\P347.XSTA, MethodObj)
    External (\P348, DeviceObj)
    External (\P348.XSTA, MethodObj)
    External (\P349, DeviceObj)
    External (\P349.XSTA, MethodObj)
    External (\P350, DeviceObj)
    External (\P350.XSTA, MethodObj)
    External (\P351, DeviceObj)
    External (\P351.XSTA, MethodObj)
    External (\P352, DeviceObj)
    External (\P352.XSTA, MethodObj)
    External (\P353, DeviceObj)
    External (\P353.XSTA, MethodObj)
    External (\P354, DeviceObj)
    External (\P354.XSTA, MethodObj)
    External (\P355, DeviceObj)
    External (\P355.XSTA, MethodObj)
    External (\P356, DeviceObj)
    External (\P356.XSTA, MethodObj)
    External (\P357, DeviceObj)
    External (\P357.XSTA, MethodObj)
    External (\P358, DeviceObj)
    External (\P358.XSTA, MethodObj)
    External (\P359, DeviceObj)
    External (\P359.XSTA, MethodObj)
    External (\P360, DeviceObj)
    External (\P360.XSTA, MethodObj)
    External (\P361, DeviceObj)
    External (\P361.XSTA, MethodObj)
    External (\P362, DeviceObj)
    External (\P362.XSTA, MethodObj)
    External (\P363, DeviceObj)
    External (\P363.XSTA, MethodObj)
    External (\P364, DeviceObj)
    External (\P364.XSTA, MethodObj)
    External (\P365, DeviceObj)
    External (\P365.XSTA, MethodObj)
    External (\P366, DeviceObj)
    External (\P366.XSTA, MethodObj)
    External (\P367, DeviceObj)
    External (\P367.XSTA, MethodObj)
    External (\P368, DeviceObj)
    External (\P368.XSTA, MethodObj)
    External (\P369, DeviceObj)
    External (\P369.XSTA, MethodObj)
    External (\P370, DeviceObj)
    External (\P370.XSTA, MethodObj)
    External (\P371, DeviceObj)
    External (\P371.XSTA, MethodObj)
    External (\P372, DeviceObj)
    External (\P372.XSTA, MethodObj)
    External (\P373, DeviceObj)
    External (\P373.XSTA, MethodObj)
    External (\P374, DeviceObj)
    External (\P374.XSTA, MethodObj)
    External (\P375, DeviceObj)
    External (\P375.XSTA, MethodObj)
    External (\P376, DeviceObj)
    External (\P376.XSTA, MethodObj)
    External (\P377, DeviceObj)
    External (\P377.XSTA, MethodObj)
    External (\P378, DeviceObj)
    External (\P378.XSTA, MethodObj)
    External (\P379, DeviceObj)
    External (\P379.XSTA, MethodObj)
    External (\P380, DeviceObj)
    External (\P380.XSTA, MethodObj)
    External (\P381, DeviceObj)
    External (\P381.XSTA, MethodObj)
    External (\P382, DeviceObj)
    External (\P382.XSTA, MethodObj)
    External (\P383, DeviceObj)
    External (\P383.XSTA, MethodObj)
    External (\P384, DeviceObj)
    External (\P384.XSTA, MethodObj)
    External (\P385, DeviceObj)
    External (\P385.XSTA, MethodObj)
    External (\P386, DeviceObj)
    External (\P386.XSTA, MethodObj)
    External (\P387, DeviceObj)
    External (\P387.XSTA, MethodObj)
    External (\P388, DeviceObj)
    External (\P388.XSTA, MethodObj)
    External (\P389, DeviceObj)
    External (\P389.XSTA, MethodObj)
    External (\P390, DeviceObj)
    External (\P390.XSTA, MethodObj)
    External (\P391, DeviceObj)
    External (\P391.XSTA, MethodObj)
    External (\P392, DeviceObj)
    External (\P392.XSTA, MethodObj)
    External (\P393, DeviceObj)
    External (\P393.XSTA, MethodObj)
    External (\P394, DeviceObj)
    External (\P394.XSTA, MethodObj)
    External (\P395, DeviceObj)
    External (\P395.XSTA, MethodObj)
    External (\P396, DeviceObj)
    External (\P396.XSTA, MethodObj)
    External (\P397, DeviceObj)
    External (\P397.XSTA, MethodObj)
    External (\P398, DeviceObj)
    External (\P398.XSTA, MethodObj)
    External (\P399, DeviceObj)
    External (\P399.XSTA, MethodObj)
    External (\P400, DeviceObj)
    External (\P400.XSTA, MethodObj)
    External (\P401, DeviceObj)
    External (\P401.XSTA, MethodObj)
    External (\P402, DeviceObj)
    External (\P402.XSTA, MethodObj)
    External (\P403, DeviceObj)
    External (\P403.XSTA, MethodObj)
    External (\P404, DeviceObj)
    External (\P404.XSTA, MethodObj)
    External (\P405, DeviceObj)
    External (\P405.XSTA, MethodObj)
    External (\P406, DeviceObj)
    External (\P406.XSTA, MethodObj)
    External (\P407, DeviceObj)
    External (\P407.XSTA, MethodObj)
    External (\P408, DeviceObj)
    External (\P408.XSTA, MethodObj)
    External (\P409, DeviceObj)
    External (\P409.XSTA, MethodObj)
    External (\P410, DeviceObj)
    External (\P410.XSTA, MethodObj)
    External (\P411, DeviceObj)
    External (\P411.XSTA, MethodObj)
    External (\P412, DeviceObj)
    External (\P412.XSTA, MethodObj)
    External (\P413, DeviceObj)
    External (\P413.XSTA, MethodObj)
    External (\P414, DeviceObj)
    External (\P414.XSTA, MethodObj)
    External (\P415, DeviceObj)
    External (\P415.XSTA, MethodObj)
    External (\P416, DeviceObj)
    External (\P416.XSTA, MethodObj)
    External (\P417, DeviceObj)
    External (\P417.XSTA, MethodObj)
    External (\P418, DeviceObj)
    External (\P418.XSTA, MethodObj)
    External (\P419, DeviceObj)
    External (\P419.XSTA, MethodObj)
    External (\P420, DeviceObj)
    External (\P420.XSTA, MethodObj)
    External (\P421, DeviceObj)
    External (\P421.XSTA, MethodObj)
    External (\P422, DeviceObj)
    External (\P422.XSTA, MethodObj)
    External (\P423, DeviceObj)
    External (\P423.XSTA, MethodObj)
    External (\P424, DeviceObj)
    External (\P424.XSTA, MethodObj)
    External (\P425, DeviceObj)
    External (\P425.XSTA, MethodObj)
    External (\P426, DeviceObj)
    External (\P426.XSTA, MethodObj)
    External (\P427, DeviceObj)
    External (\P427.XSTA, MethodObj)
    External (\P428, DeviceObj)
    External (\P428.XSTA, MethodObj)
    External (\P429, DeviceObj)
    External (\P429.XSTA, MethodObj)
    External (\P430, DeviceObj)
    External (\P430.XSTA, MethodObj)
    External (\P431, DeviceObj)
    External (\P431.XSTA, MethodObj)
    External (\P432, DeviceObj)
    External (\P432.XSTA, MethodObj)
    External (\P433, DeviceObj)
    External (\P433.XSTA, MethodObj)
    External (\P434, DeviceObj)
    External (\P434.XSTA, MethodObj)
    External (\P435, DeviceObj)
    External (\P435.XSTA, MethodObj)
    External (\P436, DeviceObj)
    External (\P436.XSTA, MethodObj)
    External (\P437, DeviceObj)
    External (\P437.XSTA, MethodObj)
    External (\P438, DeviceObj)
    External (\P438.XSTA, MethodObj)
    External (\P439, DeviceObj)
    External (\P439.XSTA, MethodObj)
    External (\P440, DeviceObj)
    External (\P440.XSTA, MethodObj)
    External (\P441, DeviceObj)
    External (\P441.XSTA, MethodObj)
    External (\P442, DeviceObj)
    External (\P442.XSTA, MethodObj)
    External (\P443, DeviceObj)
    External (\P443.XSTA, MethodObj)
    External (\P444, DeviceObj)
    External (\P444.XSTA, MethodObj)
    External (\P445, DeviceObj)
    External (\P445.XSTA, MethodObj)
    External (\P446, DeviceObj)
    External (\P446.XSTA, MethodObj)
    External (\P447, DeviceObj)
    External (\P447.XSTA, MethodObj)
    External (\P448, DeviceObj)
    External (\P448.XSTA, MethodObj)
    External (\P449, DeviceObj)
    External (\P449.XSTA, MethodObj)
    External (\P450, DeviceObj)
    External (\P450.XSTA, MethodObj)
    External (\P451, DeviceObj)
    External (\P451.XSTA, MethodObj)
    External (\P452, DeviceObj)
    External (\P452.XSTA, MethodObj)
    External (\P453, DeviceObj)
    External (\P453.XSTA, MethodObj)
    External (\P454, DeviceObj)
    External (\P454.XSTA, MethodObj)
    External (\P455, DeviceObj)
    External (\P455.XSTA, MethodObj)
    External (\P456, DeviceObj)
    External (\P456.XSTA, MethodObj)
    External (\P457, DeviceObj)
    External (\P457.XSTA, MethodObj)
    External (\P458, DeviceObj)
    External (\P458.XSTA, MethodObj)
    External (\P459, DeviceObj)
    External (\P459.XSTA, MethodObj)
    External (\P460, DeviceObj)
    External (\P460.XSTA, MethodObj)
    External (\P461, DeviceObj)
    External (\P461.XSTA, MethodObj)
    External (\P462, DeviceObj)
    External (\P462.XSTA, MethodObj)
    External (\P463, DeviceObj)
    External (\P463.XSTA, MethodObj)
    External (\P464, DeviceObj)
    External (\P464.XSTA, MethodObj)
    External (\P465, DeviceObj)
    External (\P465.XSTA, MethodObj)
    External (\P466, DeviceObj)
    External (\P466.XSTA, MethodObj)
    External (\P467, DeviceObj)
    External (\P467.XSTA, MethodObj)
    External (\P468, DeviceObj)
    External (\P468.XSTA, MethodObj)
    External (\P469, DeviceObj)
    External (\P469.XSTA, MethodObj)
    External (\P470, DeviceObj)
    External (\P470.XSTA, MethodObj)
    External (\P471, DeviceObj)
    External (\P471.XSTA, MethodObj)
    External (\P472, DeviceObj)
    External (\P472.XSTA, MethodObj)
    External (\P473, DeviceObj)
    External (\P473.XSTA, MethodObj)
    External (\P474, DeviceObj)
    External (\P474.XSTA, MethodObj)
    External (\P475, DeviceObj)
    External (\P475.XSTA, MethodObj)
    External (\P476, DeviceObj)
    External (\P476.XSTA, MethodObj)
    External (\P477, DeviceObj)
    External (\P477.XSTA, MethodObj)
    External (\P478, DeviceObj)
    External (\P478.XSTA, MethodObj)
    External (\P479, DeviceObj)
    External (\P479.XSTA, MethodObj)
    External (\P480, DeviceObj)
    External (\P480.XSTA, MethodObj)
    External (\P481, DeviceObj)
    External (\P481.XSTA, MethodObj)
    External (\P482, DeviceObj)
    External (\P482.XSTA, MethodObj)
    External (\P483, DeviceObj)
    External (\P483.XSTA, MethodObj)
    External (\P484, DeviceObj)
    External (\P484.XSTA, MethodObj)
    External (\P485, DeviceObj)
    External (\P485.XSTA, MethodObj)
    External (\P486, DeviceObj)
    External (\P486.XSTA, MethodObj)
    External (\P487, DeviceObj)
    External (\P487.XSTA, MethodObj)
    External (\P488, DeviceObj)
    External (\P488.XSTA, MethodObj)
    External (\P489, DeviceObj)
    External (\P489.XSTA, MethodObj)
    External (\P490, DeviceObj)
    External (\P490.XSTA, MethodObj)
    External (\P491, DeviceObj)
    External (\P491.XSTA, MethodObj)
    External (\P492, DeviceObj)
    External (\P492.XSTA, MethodObj)
    External (\P493, DeviceObj)
    External (\P493.XSTA, MethodObj)
    External (\P494, DeviceObj)
    External (\P494.XSTA, MethodObj)
    External (\P495, DeviceObj)
    External (\P495.XSTA, MethodObj)
    External (\P496, DeviceObj)
    External (\P496.XSTA, MethodObj)
    External (\P497, DeviceObj)
    External (\P497.XSTA, MethodObj)
    External (\P498, DeviceObj)
    External (\P498.XSTA, MethodObj)
    External (\P499, DeviceObj)
    External (\P499.XSTA, MethodObj)
    External (\P500, DeviceObj)
    External (\P500.XSTA, MethodObj)
    External (\P501, DeviceObj)
    External (\P501.XSTA, MethodObj)
    External (\P502, DeviceObj)
    External (\P502.XSTA, MethodObj)
    External (\P503, DeviceObj)
    External (\P503.XSTA, MethodObj)
    External (\P504, DeviceObj)
    External (\P504.XSTA, MethodObj)
    External (\P505, DeviceObj)
    External (\P505.XSTA, MethodObj)
    External (\P506, DeviceObj)
    External (\P506.XSTA, MethodObj)
    External (\P507, DeviceObj)
    External (\P507.XSTA, MethodObj)
    External (\P508, DeviceObj)
    External (\P508.XSTA, MethodObj)
    External (\P509, DeviceObj)
    External (\P509.XSTA, MethodObj)
    External (\P510, DeviceObj)
    External (\P510.XSTA, MethodObj)
    External (\P511, DeviceObj)
    External (\P511.XSTA, MethodObj)
    External (\P512, DeviceObj)
    External (\P512.XSTA, MethodObj)
    External (\P513, DeviceObj)
    External (\P513.XSTA, MethodObj)
    External (\P514, DeviceObj)
    External (\P514.XSTA, MethodObj)
    External (\P515, DeviceObj)
    External (\P515.XSTA, MethodObj)
    External (\P516, DeviceObj)
    External (\P516.XSTA, MethodObj)
    External (\P517, DeviceObj)
    External (\P517.XSTA, MethodObj)
    External (\P518, DeviceObj)
    External (\P518.XSTA, MethodObj)
    External (\P519, DeviceObj)
    External (\P519.XSTA, MethodObj)
    External (\P520, DeviceObj)
    External (\P520.XSTA, MethodObj)
    External (\P521, DeviceObj)
    External (\P521.XSTA, MethodObj)
    External (\P522, DeviceObj)
    External (\P522.XSTA, MethodObj)
    External (\P523, DeviceObj)
    External (\P523.XSTA, MethodObj)
    External (\P524, DeviceObj)
    External (\P524.XSTA, MethodObj)
    External (\P525, DeviceObj)
    External (\P525.XSTA, MethodObj)
    External (\P526, DeviceObj)
    External (\P526.XSTA, MethodObj)
    External (\P527, DeviceObj)
    External (\P527.XSTA, MethodObj)
    External (\P528, DeviceObj)
    External (\P528.XSTA, MethodObj)
    External (\P529, DeviceObj)
    External (\P529.XSTA, MethodObj)
    External (\P530, DeviceObj)
    External (\P530.XSTA, MethodObj)
    External (\P531, DeviceObj)
    External (\P531.XSTA, MethodObj)
    External (\P532, DeviceObj)
    External (\P532.XSTA, MethodObj)
    External (\P533, DeviceObj)
    External (\P533.XSTA, MethodObj)
    External (\P534, DeviceObj)
    External (\P534.XSTA, MethodObj)
    External (\P535, DeviceObj)
    External (\P535.XSTA, MethodObj)
    External (\P536, DeviceObj)
    External (\P536.XSTA, MethodObj)
    External (\P537, DeviceObj)
    External (\P537.XSTA, MethodObj)
    External (\P538, DeviceObj)
    External (\P538.XSTA, MethodObj)
    External (\P539, DeviceObj)
    External (\P539.XSTA, MethodObj)
    External (\P540, DeviceObj)
    External (\P540.XSTA, MethodObj)
    External (\P541, DeviceObj)
    External (\P541.XSTA, MethodObj)
    External (\P542, DeviceObj)
    External (\P542.XSTA, MethodObj)
    External (\P543, DeviceObj)
    External (\P543.XSTA, MethodObj)
    External (\P544, DeviceObj)
    External (\P544.XSTA, MethodObj)
    External (\P545, DeviceObj)
    External (\P545.XSTA, MethodObj)
    External (\P546, DeviceObj)
    External (\P546.XSTA, MethodObj)
    External (\P547, DeviceObj)
    External (\P547.XSTA, MethodObj)
    External (\P548, DeviceObj)
    External (\P548.XSTA, MethodObj)
    External (\P549, DeviceObj)
    External (\P549.XSTA, MethodObj)
    External (\P550, DeviceObj)
    External (\P550.XSTA, MethodObj)
    External (\P551, DeviceObj)
    External (\P551.XSTA, MethodObj)
    External (\P552, DeviceObj)
    External (\P552.XSTA, MethodObj)
    External (\P553, DeviceObj)
    External (\P553.XSTA, MethodObj)
    External (\P554, DeviceObj)
    External (\P554.XSTA, MethodObj)
    External (\P555, DeviceObj)
    External (\P555.XSTA, MethodObj)
    External (\P556, DeviceObj)
    External (\P556.XSTA, MethodObj)
    External (\P557, DeviceObj)
    External (\P557.XSTA, MethodObj)
    External (\P558, DeviceObj)
    External (\P558.XSTA, MethodObj)
    External (\P559, DeviceObj)
    External (\P559.XSTA, MethodObj)
    External (\P560, DeviceObj)
    External (\P560.XSTA, MethodObj)
    External (\P561, DeviceObj)
    External (\P561.XSTA, MethodObj)
    External (\P562, DeviceObj)
    External (\P562.XSTA, MethodObj)
    External (\P563, DeviceObj)
    External (\P563.XSTA, MethodObj)
    External (\P564, DeviceObj)
    External (\P564.XSTA, MethodObj)
    External (\P565, DeviceObj)
    External (\P565.XSTA, MethodObj)
    External (\P566, DeviceObj)
    External (\P566.XSTA, MethodObj)
    External (\P567, DeviceObj)
    External (\P567.XSTA, MethodObj)
    External (\P568, DeviceObj)
    External (\P568.XSTA, MethodObj)
    External (\P569, DeviceObj)
    External (\P569.XSTA, MethodObj)
    External (\P570, DeviceObj)
    External (\P570.XSTA, MethodObj)
    External (\P571, DeviceObj)
    External (\P571.XSTA, MethodObj)
    External (\P572, DeviceObj)
    External (\P572.XSTA, MethodObj)
    External (\P573, DeviceObj)
    External (\P573.XSTA, MethodObj)
    External (\P574, DeviceObj)
    External (\P574.XSTA, MethodObj)
    External (\P575, DeviceObj)
    External (\P575.XSTA, MethodObj)
    External (\P576, DeviceObj)
    External (\P576.XSTA, MethodObj)
    External (\P577, DeviceObj)
    External (\P577.XSTA, MethodObj)
    External (\P578, DeviceObj)
    External (\P578.XSTA, MethodObj)
    External (\P579, DeviceObj)
    External (\P579.XSTA, MethodObj)
    External (\P580, DeviceObj)
    External (\P580.XSTA, MethodObj)
    External (\P581, DeviceObj)
    External (\P581.XSTA, MethodObj)
    External (\P582, DeviceObj)
    External (\P582.XSTA, MethodObj)
    External (\P583, DeviceObj)
    External (\P583.XSTA, MethodObj)
    External (\P584, DeviceObj)
    External (\P584.XSTA, MethodObj)
    External (\P585, DeviceObj)
    External (\P585.XSTA, MethodObj)
    External (\P586, DeviceObj)
    External (\P586.XSTA, MethodObj)
    External (\P587, DeviceObj)
    External (\P587.XSTA, MethodObj)
    External (\P588, DeviceObj)
    External (\P588.XSTA, MethodObj)
    External (\P589, DeviceObj)
    External (\P589.XSTA, MethodObj)
    External (\P590, DeviceObj)
    External (\P590.XSTA, MethodObj)
    External (\P591, DeviceObj)
    External (\P591.XSTA, MethodObj)
    External (\P592, DeviceObj)
    External (\P592.XSTA, MethodObj)
    External (\P593, DeviceObj)
    External (\P593.XSTA, MethodObj)
    External (\P594, DeviceObj)
    External (\P594.XSTA, MethodObj)
    External (\P595, DeviceObj)
    External (\P595.XSTA, MethodObj)
    External (\P596, DeviceObj)
    External (\P596.XSTA, MethodObj)
    External (\P597, DeviceObj)
    External (\P597.XSTA, MethodObj)
    External (\P598, DeviceObj)
    External (\P598.XSTA, MethodObj)
    External (\P599, DeviceObj)
    External (\P599.XSTA, MethodObj)
    External (\P600, DeviceObj)
    External (\P600.XSTA, MethodObj)
    External (\P601, DeviceObj)
    External (\P601.XSTA, MethodObj)
    External (\P602, DeviceObj)
    External (\P602.XSTA, MethodObj)
    External (\P603, DeviceObj)
    External (\P603.XSTA, MethodObj)
    External (\P604, DeviceObj)
    External (\P604.XSTA, MethodObj)
    External (\P605, DeviceObj)
    External (\P605.XSTA, MethodObj)
    External (\P606, DeviceObj)
    External (\P606.XSTA, MethodObj)
    External (\P607, DeviceObj)
    External (\P607.XSTA, MethodObj)
    External (\P608, DeviceObj)
    External (\P608.XSTA, MethodObj)
    External (\P609, DeviceObj)
    External (\P609.XSTA, MethodObj)
    External (\P610, DeviceObj)
    External (\P610.XSTA, MethodObj)
    External (\P611, DeviceObj)
    External (\P611.XSTA, MethodObj)
    External (\P612, DeviceObj)
    External (\P612.XSTA, MethodObj)
    External (\P613, DeviceObj)
    External (\P613.XSTA, MethodObj)
    External (\P614, DeviceObj)
    External (\P614.XSTA, MethodObj)
    External (\P615, DeviceObj)
    External (\P615.XSTA, MethodObj)
    External (\P616, DeviceObj)
    External (\P616.XSTA, MethodObj)
    External (\P617, DeviceObj)
    External (\P617.XSTA, MethodObj)
    External (\P618, DeviceObj)
    External (\P618.XSTA, MethodObj)
    External (\P619, DeviceObj)
    External (\P619.XSTA, MethodObj)
    External (\P620, DeviceObj)
    External (\P620.XSTA, MethodObj)
    External (\P621, DeviceObj)
    External (\P621.XSTA, MethodObj)
    External (\P622, DeviceObj)
    External (\P622.XSTA, MethodObj)
    External (\P623, DeviceObj)
    External (\P623.XSTA, MethodObj)
    External (\P624, DeviceObj)
    External (\P624.XSTA, MethodObj)
    External (\P625, DeviceObj)
    External (\P625.XSTA, MethodObj)
    External (\P626, DeviceObj)
    External (\P626.XSTA, MethodObj)
    External (\P627, DeviceObj)
    External (\P627.XSTA, MethodObj)
    External (\P628, DeviceObj)
    External (\P628.XSTA, MethodObj)
    External (\P629, DeviceObj)
    External (\P629.XSTA, MethodObj)
    External (\P630, DeviceObj)
    External (\P630.XSTA, MethodObj)
    External (\P631, DeviceObj)
    External (\P631.XSTA, MethodObj)
    External (\P632, DeviceObj)
    External (\P632.XSTA, MethodObj)
    External (\P633, DeviceObj)
    External (\P633.XSTA, MethodObj)
    External (\P634, DeviceObj)
    External (\P634.XSTA, MethodObj)
    External (\P635, DeviceObj)
    External (\P635.XSTA, MethodObj)
    External (\P636, DeviceObj)
    External (\P636.XSTA, MethodObj)
    External (\P637, DeviceObj)
    External (\P637.XSTA, MethodObj)
    External (\P638, DeviceObj)
    External (\P638.XSTA, MethodObj)
    External (\P639, DeviceObj)
    External (\P639.XSTA, MethodObj)
    External (\P640, DeviceObj)
    External (\P640.XSTA, MethodObj)
    External (\P641, DeviceObj)
    External (\P641.XSTA, MethodObj)
    External (\P642, DeviceObj)
    External (\P642.XSTA, MethodObj)
    External (\P643, DeviceObj)
    External (\P643.XSTA, MethodObj)
    External (\P644, DeviceObj)
    External (\P644.XSTA, MethodObj)
    External (\P645, DeviceObj)
    External (\P645.XSTA, MethodObj)
    External (\P646, DeviceObj)
    External (\P646.XSTA, MethodObj)
    External (\P647, DeviceObj)
    External (\P647.XSTA, MethodObj)
    External (\P648, DeviceObj)
    External (\P648.XSTA, MethodObj)
    External (\P649, DeviceObj)
    External (\P649.XSTA, MethodObj)
    External (\P650, DeviceObj)
    External (\P650.XSTA, MethodObj)
    External (\P651, DeviceObj)
    External (\P651.XSTA, MethodObj)
    External (\P652, DeviceObj)
    External (\P652.XSTA, MethodObj)
    External (\P653, DeviceObj)
    External (\P653.XSTA, MethodObj)
    External (\P654, DeviceObj)
    External (\P654.XSTA, MethodObj)
    External (\P655, DeviceObj)
    External (\P655.XSTA, MethodObj)
    External (\P656, DeviceObj)
    External (\P656.XSTA, MethodObj)
    External (\P657, DeviceObj)
    External (\P657.XSTA, MethodObj)
    External (\P658, DeviceObj)
    External (\P658.XSTA, MethodObj)
    External (\P659, DeviceObj)
    External (\P659.XSTA, MethodObj)
    External (\P660, DeviceObj)
    External (\P660.XSTA, MethodObj)
    External (\P661, DeviceObj)
    External (\P661.XSTA, MethodObj)
    External (\P662, DeviceObj)
    External (\P662.XSTA, MethodObj)
    External (\P663, DeviceObj)
    External (\P663.XSTA, MethodObj)
    External (\P664, DeviceObj)
    External (\P664.XSTA, MethodObj)
    External (\P665, DeviceObj)
    External (\P665.XSTA, MethodObj)
    External (\P666, DeviceObj)
    External (\P666.XSTA, MethodObj)
    External (\P667, DeviceObj)
    External (\P667.XSTA, MethodObj)
    External (\P668, DeviceObj)
    External (\P668.XSTA, MethodObj)
    External (\P669, DeviceObj)
    External (\P669.XSTA, MethodObj)
    External (\P670, DeviceObj)
    External (\P670.XSTA, MethodObj)
    External (\P671, DeviceObj)
    External (\P671.XSTA, MethodObj)
    External (\P672, DeviceObj)
    External (\P672.XSTA, MethodObj)
    External (\P673, DeviceObj)
    External (\P673.XSTA, MethodObj)
    External (\P674, DeviceObj)
    External (\P674.XSTA, MethodObj)
    External (\P675, DeviceObj)
    External (\P675.XSTA, MethodObj)
    External (\P676, DeviceObj)
    External (\P676.XSTA, MethodObj)
    External (\P677, DeviceObj)
    External (\P677.XSTA, MethodObj)
    External (\P678, DeviceObj)
    External (\P678.XSTA, MethodObj)
    External (\P679, DeviceObj)
    External (\P679.XSTA, MethodObj)
    External (\P680, DeviceObj)
    External (\P680.XSTA, MethodObj)
    External (\P681, DeviceObj)
    External (\P681.XSTA, MethodObj)
    External (\P682, DeviceObj)
    External (\P682.XSTA, MethodObj)
    External (\P683, DeviceObj)
    External (\P683.XSTA, MethodObj)
    External (\P684, DeviceObj)
    External (\P684.XSTA, MethodObj)
    External (\P685, DeviceObj)
    External (\P685.XSTA, MethodObj)
    External (\P686, DeviceObj)
    External (\P686.XSTA, MethodObj)
    External (\P687, DeviceObj)
    External (\P687.XSTA, MethodObj)
    External (\P688, DeviceObj)
    External (\P688.XSTA, MethodObj)
    External (\P689, DeviceObj)
    External (\P689.XSTA, MethodObj)
    External (\P690, DeviceObj)
    External (\P690.XSTA, MethodObj)
    External (\P691, DeviceObj)
    External (\P691.XSTA, MethodObj)
    External (\P692, DeviceObj)
    External (\P692.XSTA, MethodObj)
    External (\P693, DeviceObj)
    External (\P693.XSTA, MethodObj)
    External (\P694, DeviceObj)
    External (\P694.XSTA, MethodObj)
    External (\P695, DeviceObj)
    External (\P695.XSTA, MethodObj)
    External (\P696, DeviceObj)
    External (\P696.XSTA, MethodObj)
    External (\P697, DeviceObj)
    External (\P697.XSTA, MethodObj)
    External (\P698, DeviceObj)
    External (\P698.XSTA, MethodObj)
    External (\P699, DeviceObj)
    External (\P699.XSTA, MethodObj)
    External (\P700, DeviceObj)
    External (\P700.XSTA, MethodObj)
    External (\P701, DeviceObj)
    External (\P701.XSTA, MethodObj)
    External (\P702, DeviceObj)
    External (\P702.XSTA, MethodObj)
    External (\P703, DeviceObj)
    External (\P703.XSTA, MethodObj)
    External (\P704, DeviceObj)
    External (\P704.XSTA, MethodObj)
    External (\P705, DeviceObj)
    External (\P705.XSTA, MethodObj)
    External (\P706, DeviceObj)
    External (\P706.XSTA, MethodObj)
    External (\P707, DeviceObj)
    External (\P707.XSTA, MethodObj)
    External (\P708, DeviceObj)
    External (\P708.XSTA, MethodObj)
    External (\P709, DeviceObj)
    External (\P709.XSTA, MethodObj)
    External (\P710, DeviceObj)
    External (\P710.XSTA, MethodObj)
    External (\P711, DeviceObj)
    External (\P711.XSTA, MethodObj)
    External (\P712, DeviceObj)
    External (\P712.XSTA, MethodObj)
    External (\P713, DeviceObj)
    External (\P713.XSTA, MethodObj)
    External (\P714, DeviceObj)
    External (\P714.XSTA, MethodObj)
    External (\P715, DeviceObj)
    External (\P715.XSTA, MethodObj)
    External (\P716, DeviceObj)
    External (\P716.XSTA, MethodObj)
    External (\P717, DeviceObj)
    External (\P717.XSTA, MethodObj)
    External (\P718, DeviceObj)
    External (\P718.XSTA, MethodObj)
    External (\P719, DeviceObj)
    External (\P719.XSTA, MethodObj)
    External (\P720, DeviceObj)
    External (\P720.XSTA, MethodObj)
    External (\P721, DeviceObj)
    External (\P721.XSTA, MethodObj)
    External (\P722, DeviceObj)
    External (\P722.XSTA, MethodObj)
    External (\P723, DeviceObj)
    External (\P723.XSTA, MethodObj)
    External (\P724, DeviceObj)
    External (\P724.XSTA, MethodObj)
    External (\P725, DeviceObj)
    External (\P725.XSTA, MethodObj)
    External (\P726, DeviceObj)
    External (\P726.XSTA, MethodObj)
    External (\P727, DeviceObj)
    External (\P727.XSTA, MethodObj)
    External (\P728, DeviceObj)
    External (\P728.XSTA, MethodObj)
    External (\P729, DeviceObj)
    External (\P729.XSTA, MethodObj)
    External (\P730, DeviceObj)
    External (\P730.XSTA, MethodObj)
    External (\P731, DeviceObj)
    External (\P731.XSTA, MethodObj)
    External (\P732, DeviceObj)
    External (\P732.XSTA, MethodObj)
    External (\P733, DeviceObj)
    External (\P733.XSTA, MethodObj)
    External (\P734, DeviceObj)
    External (\P734.XSTA, MethodObj)
    External (\P735, DeviceObj)
    External (\P735.XSTA, MethodObj)
    External (\P736, DeviceObj)
    External (\P736.XSTA, MethodObj)
    External (\P737, DeviceObj)
    External (\P737.XSTA, MethodObj)
    External (\P738, DeviceObj)
    External (\P738.XSTA, MethodObj)
    External (\P739, DeviceObj)
    External (\P739.XSTA, MethodObj)
    External (\P740, DeviceObj)
    External (\P740.XSTA, MethodObj)
    External (\P741, DeviceObj)
    External (\P741.XSTA, MethodObj)
    External (\P742, DeviceObj)
    External (\P742.XSTA, MethodObj)
    External (\P743, DeviceObj)
    External (\P743.XSTA, MethodObj)
    External (\P744, DeviceObj)
    External (\P744.XSTA, MethodObj)
    External (\P745, DeviceObj)
    External (\P745.XSTA, MethodObj)
    External (\P746, DeviceObj)
    External (\P746.XSTA, MethodObj)
    External (\P747, DeviceObj)
    External (\P747.XSTA, MethodObj)
    External (\P748, DeviceObj)
    External (\P748.XSTA, MethodObj)
    External (\P749, DeviceObj)
    External (\P749.XSTA, MethodObj)
    External (\P750, DeviceObj)
    External (\P750.XSTA, MethodObj)
    External (\P751, DeviceObj)
    External (\P751.XSTA, MethodObj)
    External (\P752, DeviceObj)
    External (\P752.XSTA, MethodObj)
    External (\P753, DeviceObj)
    External (\P753.XSTA, MethodObj)
    External (\P754, DeviceObj)
    External (\P754.XSTA, MethodObj)
    External (\P755, DeviceObj)
    External (\P755.XSTA, MethodObj)
    External (\P756, DeviceObj)
    External (\P756.XSTA, MethodObj)
    External (\P757, DeviceObj)
    External (\P757.XSTA, MethodObj)
    External (\P758, DeviceObj)
    External (\P758.XSTA, MethodObj)
    External (\P759, DeviceObj)
    External (\P759.XSTA, MethodObj)
    External (\P760, DeviceObj)
    External (\P760.XSTA, MethodObj)
    External (\P761, DeviceObj)
    External (\P761.XSTA, MethodObj)
    External (\P762, DeviceObj)
    External (\P762.XSTA, MethodObj)
    External (\P763, DeviceObj)
    External (\P763.XSTA, MethodObj)
    External (\P764, DeviceObj)
    External (\P764.XSTA, MethodObj)
    External (\P765, DeviceObj)
    External (\P765.XSTA, MethodObj)
    External (\P766, DeviceObj)
    External (\P766.XSTA, MethodObj)
    External (\P767, DeviceObj)
    External (\P767.XSTA, MethodObj)
    External (\P768, DeviceObj)
    External (\P768.XSTA, MethodObj)
    External (\P769, DeviceObj)
    External (\P769.XSTA, MethodObj)
    External (\P770, DeviceObj)
    External (\P770.XSTA, MethodObj)
    External (\P771, DeviceObj)
    External (\P771.XSTA, MethodObj)
    External (\P772, DeviceObj)
    External (\P772.XSTA, MethodObj)
    External (\P773, DeviceObj)
    External (\P773.XSTA, MethodObj)
    External (\P774, DeviceObj)
    External (\P774.XSTA, MethodObj)
    External (\P775, DeviceObj)
    External (\P775.XSTA, MethodObj)
    External (\P776, DeviceObj)
    External (\P776.XSTA, MethodObj)
    External (\P777, DeviceObj)
    External (\P777.XSTA, MethodObj)
    External (\P778, DeviceObj)
    External (\P778.XSTA, MethodObj)
    External (\P779, DeviceObj)
    External (\P779.XSTA, MethodObj)
    External (\P780, DeviceObj)
    External (\P780.XSTA, MethodObj)
    External (\P781, DeviceObj)
    External (\P781.XSTA, MethodObj)
    External (\P782, DeviceObj)
    External (\P782.XSTA, MethodObj)
    External (\P783, DeviceObj)
    External (\P783.XSTA, MethodObj)
    External (\P784, DeviceObj)
    External (\P784.XSTA, MethodObj)
    External (\P785, DeviceObj)
    External (\P785.XSTA, MethodObj)
    External (\P786, DeviceObj)
    External (\P786.XSTA, MethodObj)
    External (\P787, DeviceObj)
    External (\P787.XSTA, MethodObj)
    External (\P788, DeviceObj)
    External (\P788.XSTA, MethodObj)
    External (\P789, DeviceObj)
    External (\P789.XSTA, MethodObj)
    External (\P790, DeviceObj)
    External (\P790.XSTA, MethodObj)
    External (\P791, DeviceObj)
    External (\P791.XSTA, MethodObj)
    External (\P792, DeviceObj)
    External (\P792.XSTA, MethodObj)
    External (\P793, DeviceObj)
    External (\P793.XSTA, MethodObj)
    External (\P794, DeviceObj)
    External (\P794.XSTA, MethodObj)
    External (\P795, DeviceObj)
    External (\P795.XSTA, MethodObj)
    External (\P796, DeviceObj)
    External (\P796.XSTA, MethodObj)
    External (\P797, DeviceObj)
    External (\P797.XSTA, MethodObj)
    External (\P798, DeviceObj)
    External (\P798.XSTA, MethodObj)
    External (\P799, DeviceObj)
    External (\P799.XSTA, MethodObj)
    External (\P800, DeviceObj)
    External (\P800.XSTA, MethodObj)
    External (\P801, DeviceObj)
    External (\P801.XSTA, MethodObj)
    External (\P802, DeviceObj)
    External (\P802.XSTA, MethodObj)
    External (\P803, DeviceObj)
    External (\P803.XSTA, MethodObj)
    External (\P804, DeviceObj)
    External (\P804.XSTA, MethodObj)
    External (\P805, DeviceObj)
    External (\P805.XSTA, MethodObj)
    External (\P806, DeviceObj)
    External (\P806.XSTA, MethodObj)
    External (\P807, DeviceObj)
    External (\P807.XSTA, MethodObj)
    External (\P808, DeviceObj)
    External (\P808.XSTA, MethodObj)
    External (\P809, DeviceObj)
    External (\P809.XSTA, MethodObj)
    External (\P810, DeviceObj)
    External (\P810.XSTA, MethodObj)
    External (\P811, DeviceObj)
    External (\P811.XSTA, MethodObj)
    External (\P812, DeviceObj)
    External (\P812.XSTA, MethodObj)
    External (\P813, DeviceObj)
    External (\P813.XSTA, MethodObj)
    External (\P814, DeviceObj)
    External (\P814.XSTA, MethodObj)
    External (\P815, DeviceObj)
    External (\P815.XSTA, MethodObj)
    External (\P816, DeviceObj)
    External (\P816.XSTA, MethodObj)
    External (\P817, DeviceObj)
    External (\P817.XSTA, MethodObj)
    External (\P818, DeviceObj)
    External (\P818.XSTA, MethodObj)
    External (\P819, DeviceObj)
    External (\P819.XSTA, MethodObj)
    External (\P820, DeviceObj)
    External (\P820.XSTA, MethodObj)
    External (\P821, DeviceObj)
    External (\P821.XSTA, MethodObj)
    External (\P822, DeviceObj)
    External (\P822.XSTA, MethodObj)
    External (\P823, DeviceObj)
    External (\P823.XSTA, MethodObj)
    External (\P824, DeviceObj)
    External (\P824.XSTA, MethodObj)
    External (\P825, DeviceObj)
    External (\P825.XSTA, MethodObj)
    External (\P826, DeviceObj)
    External (\P826.XSTA, MethodObj)
    External (\P827, DeviceObj)
    External (\P827.XSTA, MethodObj)
    External (\P828, DeviceObj)
    External (\P828.XSTA, MethodObj)
    External (\P829, DeviceObj)
    External (\P829.XSTA, MethodObj)
    External (\P830, DeviceObj)
    External (\P830.XSTA, MethodObj)
    External (\P831, DeviceObj)
    External (\P831.XSTA, MethodObj)
    External (\P832, DeviceObj)
    External (\P832.XSTA, MethodObj)
    External (\P833, DeviceObj)
    External (\P833.XSTA, MethodObj)
    External (\P834, DeviceObj)
    External (\P834.XSTA, MethodObj)
    External (\P835, DeviceObj)
    External (\P835.XSTA, MethodObj)
    External (\P836, DeviceObj)
    External (\P836.XSTA, MethodObj)
    External (\P837, DeviceObj)
    External (\P837.XSTA, MethodObj)
    External (\P838, DeviceObj)
    External (\P838.XSTA, MethodObj)
    External (\P839, DeviceObj)
    External (\P839.XSTA, MethodObj)
    External (\P840, DeviceObj)
    External (\P840.XSTA, MethodObj)
    External (\P841, DeviceObj)
    External (\P841.XSTA, MethodObj)
    External (\P842, DeviceObj)
    External (\P842.XSTA, MethodObj)
    External (\P843, DeviceObj)
    External (\P843.XSTA, MethodObj)
    External (\P844, DeviceObj)
    External (\P844.XSTA, MethodObj)
    External (\P845, DeviceObj)
    External (\P845.XSTA, MethodObj)
    External (\P846, DeviceObj)
    External (\P846.XSTA, MethodObj)
    External (\P847, DeviceObj)
    External (\P847.XSTA, MethodObj)
    External (\P848, DeviceObj)
    External (\P848.XSTA, MethodObj)
    External (\P849, DeviceObj)
    External (\P849.XSTA, MethodObj)
    External (\P850, DeviceObj)
    External (\P850.XSTA, MethodObj)
    External (\P851, DeviceObj)
    External (\P851.XSTA, MethodObj)
    External (\P852, DeviceObj)
    External (\P852.XSTA, MethodObj)
    External (\P853, DeviceObj)
    External (\P853.XSTA, MethodObj)
    External (\P854, DeviceObj)
    External (\P854.XSTA, MethodObj)
    External (\P855, DeviceObj)
    External (\P855.XSTA, MethodObj)
    External (\P856, DeviceObj)
    External (\P856.XSTA, MethodObj)
    External (\P857, DeviceObj)
    External (\P857.XSTA, MethodObj)
    External (\P858, DeviceObj)
    External (\P858.XSTA, MethodObj)
    External (\P859, DeviceObj)
    External (\P859.XSTA, MethodObj)
    External (\P860, DeviceObj)
    External (\P860.XSTA, MethodObj)
    External (\P861, DeviceObj)
    External (\P861.XSTA, MethodObj)
    External (\P862, DeviceObj)
    External (\P862.XSTA, MethodObj)
    External (\P863, DeviceObj)
    External (\P863.XSTA, MethodObj)
    External (\P864, DeviceObj)
    External (\P864.XSTA, MethodObj)
    External (\P865, DeviceObj)
    External (\P865.XSTA, MethodObj)
    External (\P866, DeviceObj)
    External (\P866.XSTA, MethodObj)
    External (\P867, DeviceObj)
    External (\P867.XSTA, MethodObj)
    External (\P868, DeviceObj)
    External (\P868.XSTA, MethodObj)
    External (\P869, DeviceObj)
    External (\P869.XSTA, MethodObj)
    External (\P870, DeviceObj)
    External (\P870.XSTA, MethodObj)
    External (\P871, DeviceObj)
    External (\P871.XSTA, MethodObj)
    External (\P872, DeviceObj)
    External (\P872.XSTA, MethodObj)
    External (\P873, DeviceObj)
    External (\P873.XSTA, MethodObj)
    External (\P874, DeviceObj)
    External (\P874.XSTA, MethodObj)
    External (\P875, DeviceObj)
    External (\P875.XSTA, MethodObj)
    External (\P876, DeviceObj)
    External (\P876.XSTA, MethodObj)
    External (\P877, DeviceObj)
    External (\P877.XSTA, MethodObj)
    External (\P878, DeviceObj)
    External (\P878.XSTA, MethodObj)
    External (\P879, DeviceObj)
    External (\P879.XSTA, MethodObj)
    External (\P880, DeviceObj)
    External (\P880.XSTA, MethodObj)
    External (\P881, DeviceObj)
    External (\P881.XSTA, MethodObj)
    External (\P882, DeviceObj)
    External (\P882.XSTA, MethodObj)
    External (\P883, DeviceObj)
    External (\P883.XSTA, MethodObj)
    External (\P884, DeviceObj)
    External (\P884.XSTA, MethodObj)
    External (\P885, DeviceObj)
    External (\P885.XSTA, MethodObj)
    External (\P886, DeviceObj)
    External (\P886.XSTA, MethodObj)
    External (\P887, DeviceObj)
    External (\P887.XSTA, MethodObj)
    External (\P888, DeviceObj)
    External (\P888.XSTA, MethodObj)
    External (\P889, DeviceObj)
    External (\P889.XSTA, MethodObj)
    External (\P890, DeviceObj)
    External (\P890.XSTA, MethodObj)
    External (\P891, DeviceObj)
    External (\P891.XSTA, MethodObj)
    External (\P892, DeviceObj)
    External (\P892.XSTA, MethodObj)
    External (\P893, DeviceObj)
    External (\P893.XSTA, MethodObj)
    External (\P894, DeviceObj)
    External (\P894.XSTA, MethodObj)
    External (\P895, DeviceObj)
    External (\P895.XSTA, MethodObj)
    External (\P896, DeviceObj)
    External (\P896.XSTA, MethodObj)
    External (\P897, DeviceObj)
    External (\P897.XSTA, MethodObj)
    External (\P898, DeviceObj)
    External (\P898.XSTA, MethodObj)
    External (\P899, DeviceObj)
    External (\P899.XSTA, MethodObj)
    External (\P900, DeviceObj)
    External (\P900.XSTA, MethodObj)
    External (\P901, DeviceObj)
    External (\P901.XSTA, MethodObj)
    External (\P902, DeviceObj)
    External (\P902.XSTA, MethodObj)
    External (\P903, DeviceObj)
    External (\P903.XSTA, MethodObj)
    External (\P904, DeviceObj)
    External (\P904.XSTA, MethodObj)
    External (\P905, DeviceObj)
    External (\P905.XSTA, MethodObj)
    External (\P906, DeviceObj)
    External (\P906.XSTA, MethodObj)
    External (\P907, DeviceObj)
    External (\P907.XSTA, MethodObj)
    External (\P908, DeviceObj)
    External (\P908.XSTA, MethodObj)
    External (\P909, DeviceObj)
    External (\P909.XSTA, MethodObj)
    External (\P910, DeviceObj)
    External (\P910.XSTA, MethodObj)
    External (\P911, DeviceObj)
    External (\P911.XSTA, MethodObj)
    External (\P912, DeviceObj)
    External (\P912.XSTA, MethodObj)
    External (\P913, DeviceObj)
    External (\P913.XSTA, MethodObj)
    External (\P914, DeviceObj)
    External (\P914.XSTA, MethodObj)
    External (\P915, DeviceObj)
    External (\P915.XSTA, MethodObj)
    External (\P916, DeviceObj)
    External (\P916.XSTA, MethodObj)
    External (\P917, DeviceObj)
    External (\P917.XSTA, MethodObj)
    External (\P918, DeviceObj)
    External (\P918.XSTA, MethodObj)
    External (\P919, DeviceObj)
    External (\P919.XSTA, MethodObj)
    External (\P920, DeviceObj)
    External (\P920.XSTA, MethodObj)
    External (\P921, DeviceObj)
    External (\P921.XSTA, MethodObj)
    External (\P922, DeviceObj)
    External (\P922.XSTA, MethodObj)
    External (\P923, DeviceObj)
    External (\P923.XSTA, MethodObj)
    External (\P924, DeviceObj)
    External (\P924.XSTA, MethodObj)
    External (\P925, DeviceObj)
    External (\P925.XSTA, MethodObj)
    External (\P926, DeviceObj)
    External (\P926.XSTA, MethodObj)
    External (\P927, DeviceObj)
    External (\P927.XSTA, MethodObj)
    External (\P928, DeviceObj)
    External (\P928.XSTA, MethodObj)
    External (\P929, DeviceObj)
    External (\P929.XSTA, MethodObj)
    External (\P930, DeviceObj)
    External (\P930.XSTA, MethodObj)
    External (\P931, DeviceObj)
    External (\P931.XSTA, MethodObj)
    External (\P932, DeviceObj)
    External (\P932.XSTA, MethodObj)
    External (\P933, DeviceObj)
    External (\P933.XSTA, MethodObj)
    External (\P934, DeviceObj)
    External (\P934.XSTA, MethodObj)
    External (\P935, DeviceObj)
    External (\P935.XSTA, MethodObj)
    External (\P936, DeviceObj)
    External (\P936.XSTA, MethodObj)
    External (\P937, DeviceObj)
    External (\P937.XSTA, MethodObj)
    External (\P938, DeviceObj)
    External (\P938.XSTA, MethodObj)
    External (\P939, DeviceObj)
    External (\P939.XSTA, MethodObj)
    External (\P940, DeviceObj)
    External (\P940.XSTA, MethodObj)
    External (\P941, DeviceObj)
    External (\P941.XSTA, MethodObj)
    External (\P942, DeviceObj)
    External (\P942.XSTA, MethodObj)
    External (\P943, DeviceObj)
    External (\P943.XSTA, MethodObj)
    External (\P944, DeviceObj)
    External (\P944.XSTA, MethodObj)
    External (\P945, DeviceObj)
    External (\P945.XSTA, MethodObj)
    External (\P946, DeviceObj)
    External (\P946.XSTA, MethodObj)
    External (\P947, DeviceObj)
    External (\P947.XSTA, MethodObj)
    External (\P948, DeviceObj)
    External (\P948.XSTA, MethodObj)
    External (\P949, DeviceObj)
    External (\P949.XSTA, MethodObj)
    External (\P950, DeviceObj)
    External (\P950.XSTA, MethodObj)
    External (\P951, DeviceObj)
    External (\P951.XSTA, MethodObj)
    External (\P952, DeviceObj)
    External (\P952.XSTA, MethodObj)
    External (\P953, DeviceObj)
    External (\P953.XSTA, MethodObj)
    External (\P954, DeviceObj)
    External (\P954.XSTA, MethodObj)
    External (\P955, DeviceObj)
    External (\P955.XSTA, MethodObj)
    External (\P956, DeviceObj)
    External (\P956.XSTA, MethodObj)
    External (\P957, DeviceObj)
    External (\P957.XSTA, MethodObj)
    External (\P958, DeviceObj)
    External (\P958.XSTA, MethodObj)
    External (\P959, DeviceObj)
    External (\P959.XSTA, MethodObj)
    External (\P960, DeviceObj)
    External (\P960.XSTA, MethodObj)
    External (\P961, DeviceObj)
    External (\P961.XSTA, MethodObj)
    External (\P962, DeviceObj)
    External (\P962.XSTA, MethodObj)
    External (\P963, DeviceObj)
    External (\P963.XSTA, MethodObj)
    External (\P964, DeviceObj)
    External (\P964.XSTA, MethodObj)
    External (\P965, DeviceObj)
    External (\P965.XSTA, MethodObj)
    External (\P966, DeviceObj)
    External (\P966.XSTA, MethodObj)
    External (\P967, DeviceObj)
    External (\P967.XSTA, MethodObj)
    External (\P968, DeviceObj)
    External (\P968.XSTA, MethodObj)
    External (\P969, DeviceObj)
    External (\P969.XSTA, MethodObj)
    External (\P970, DeviceObj)
    External (\P970.XSTA, MethodObj)
    External (\P971, DeviceObj)
    External (\P971.XSTA, MethodObj)
    External (\P972, DeviceObj)
    External (\P972.XSTA, MethodObj)
    External (\P973, DeviceObj)
    External (\P973.XSTA, MethodObj)
    External (\P974, DeviceObj)
    External (\P974.XSTA, MethodObj)
    External (\P975, DeviceObj)
    External (\P975.XSTA, MethodObj)
    External (\P976, DeviceObj)
    External (\P976.XSTA, MethodObj)
    External (\P977, DeviceObj)
    External (\P977.XSTA, MethodObj)
    External (\P978, DeviceObj)
    External (\P978.XSTA, MethodObj)
    External (\P979, DeviceObj)
    External (\P979.XSTA, MethodObj)
    External (\P980, DeviceObj)
    External (\P980.XSTA, MethodObj)
    External (\P981, DeviceObj)
    External (\P981.XSTA, MethodObj)
    External (\P982, DeviceObj)
    External (\P982.XSTA, MethodObj)
    External (\P983, DeviceObj)
    External (\P983.XSTA, MethodObj)
    External (\P984, DeviceObj)
    External (\P984.XSTA, MethodObj)
    External (\P985, DeviceObj)
    External (\P985.XSTA, MethodObj)
    External (\P986, DeviceObj)
    External (\P986.XSTA, MethodObj)
    External (\P987, DeviceObj)
    External (\P987.XSTA, MethodObj)
    External (\P988, DeviceObj)
    External (\P988.XSTA, MethodObj)
    External (\P989, DeviceObj)
    External (\P989.XSTA, MethodObj)
    External (\P990, DeviceObj)
    External (\P990.XSTA, MethodObj)
    External (\P991, DeviceObj)
    External (\P991.XSTA, MethodObj)
    External (\P992, DeviceObj)
    External (\P992.XSTA, MethodObj)
    External (\P993, DeviceObj)
    External (\P993.XSTA, MethodObj)
    External (\P994, DeviceObj)
    External (\P994.XSTA, MethodObj)
    External (\P995, DeviceObj)
    External (\P995.XSTA, MethodObj)
    External (\P996, DeviceObj)
    External (\P996.XSTA, MethodObj)
    External (\P997, DeviceObj)
    External (\P997.XSTA, MethodObj)
    External (\P998, DeviceObj)
    External (\P998.XSTA, MethodObj)
    External (\P999, DeviceObj)
    External (\P999.XSTA, MethodObj)
    External (\Q000, DeviceObj)
    External (\Q000.XSTA, MethodObj)
    External (\Q001, DeviceObj)
    External (\Q001.XSTA, MethodObj)
    External (\Q002, DeviceObj)
    External (\Q002.XSTA, MethodObj)
    External (\Q003, DeviceObj)
    External (\Q003.XSTA, MethodObj)
    External (\Q004, DeviceObj)
    External (\Q004.XSTA, MethodObj)
    External (\Q005, DeviceObj)
    External (\Q005.XSTA, MethodObj)
    External (\Q006, DeviceObj)
    External (\Q006.XSTA, MethodObj)
    External (\Q007, DeviceObj)
    External (\Q007.XSTA, MethodObj)
    External (\Q008, DeviceObj)
    External (\Q008.XSTA, MethodObj)
    External (\Q009, DeviceObj)
    External (\Q009.XSTA, MethodObj)
    External (\Q010, DeviceObj)
    External (\Q010.XSTA, MethodObj)
    External (\Q011, DeviceObj)
    External (\Q011.XSTA, MethodObj)
    External (\Q012, DeviceObj)
    External (\Q012.XSTA, MethodObj)
    External (\Q013, DeviceObj)
    External (\Q013.XSTA, MethodObj)
    External (\Q014, DeviceObj)
    External (\Q014.XSTA, MethodObj)
    External (\Q015, DeviceObj)
    External (\Q015.XSTA, MethodObj)
    External (\Q016, DeviceObj)
    External (\Q016.XSTA, MethodObj)
    External (\Q017, DeviceObj)
    External (\Q017.XSTA, MethodObj)
    External (\Q018, DeviceObj)
    External (\Q018.XSTA, MethodObj)
    External (\Q019, DeviceObj)
    External (\Q019.XSTA, MethodObj)
    External (\Q020, DeviceObj)
    External (\Q020.XSTA, MethodObj)
    External (\Q021, DeviceObj)
    External (\Q021.XSTA, MethodObj)
    External (\Q022, DeviceObj)
    External (\Q022.XSTA, MethodObj)
    External (\Q023, DeviceObj)
    External (\Q023.XSTA, MethodObj)
    External (\Q024, DeviceObj)
    External (\Q024.XSTA, MethodObj)
    External (\Q025, DeviceObj)
    External (\Q025.XSTA, MethodObj)
    External (\Q026, DeviceObj)
    External (\Q026.XSTA, MethodObj)
    External (\Q027, DeviceObj)
    External (\Q027.XSTA, MethodObj)
    External (\Q028, DeviceObj)
    External (\Q028.XSTA, MethodObj)
    External (\Q029, DeviceObj)
    External (\Q029.XSTA, MethodObj)
    External (\Q030, DeviceObj)
    External (\Q030.XSTA, MethodObj)
    External (\Q031, DeviceObj)
    External (\Q031.XSTA, MethodObj)
    External (\Q032, DeviceObj)
    External (\Q032.XSTA, MethodObj)
    External (\Q033, DeviceObj)
    External (\Q033.XSTA, MethodObj)
    External (\Q034, DeviceObj)
    External (\Q034.XSTA, MethodObj)
    External (\Q035, DeviceObj)
    External (\Q035.XSTA, MethodObj)
    External (\Q036, DeviceObj)
    External (\Q036.XSTA, MethodObj)
    External (\Q037, DeviceObj)
    External (\Q037.XSTA, MethodObj)
    External (\Q038, DeviceObj)
    External (\Q038.XSTA, MethodObj)
    External (\Q039, DeviceObj)
    External (\Q039.XSTA, MethodObj)
    External (\Q040, DeviceObj)
    External (\Q040.XSTA, MethodObj)
    External (\Q041, DeviceObj)
    External (\Q041.XSTA, MethodObj)
    External (\Q042, DeviceObj)
    External (\Q042.XSTA, MethodObj)
    External (\Q043, DeviceObj)
    External (\Q043.XSTA, MethodObj)
    External (\Q044, DeviceObj)
    External (\Q044.XSTA, MethodObj)
    External (\Q045, DeviceObj)
    External (\Q045.XSTA, MethodObj)
    External (\Q046, DeviceObj)
    External (\Q046.XSTA, MethodObj)
    External (\Q047, DeviceObj)
    External (\Q047.XSTA, MethodObj)
    External (\Q048, DeviceObj)
    External (\Q048.XSTA, MethodObj)
    External (\Q049, DeviceObj)
    External (\Q049.XSTA, MethodObj)
    External (\Q050, DeviceObj)
    External (\Q050.XSTA, MethodObj)
    External (\Q051, DeviceObj)
    External (\Q051.XSTA, MethodObj)
    External (\Q052, DeviceObj)
    External (\Q052.XSTA, MethodObj)
    External (\Q053, DeviceObj)
    External (\Q053.XSTA, MethodObj)
    External (\Q054, DeviceObj)
    External (\Q054.XSTA, MethodObj)
    External (\Q055, DeviceObj)
    External (\Q055.XSTA, MethodObj)
    External (\Q056, DeviceObj)
    External (\Q056.XSTA, MethodObj)
    External (\Q057, DeviceObj)
    External (\Q057.XSTA, MethodObj)
    External (\Q058, DeviceObj)
    External (\Q058.XSTA, MethodObj)
    External (\Q059, DeviceObj)
    External (\Q059.XSTA, MethodObj)
    External (\Q060, DeviceObj)
    External (\Q060.XSTA, MethodObj)
    External (\Q061, DeviceObj)
    External (\Q061.XSTA, MethodObj)
    External (\Q062, DeviceObj)
    External (\Q062.XSTA, MethodObj)
    External (\Q063, DeviceObj)
    External (\Q063.XSTA, MethodObj)
    External (\Q064, DeviceObj)
    External (\Q064.XSTA, MethodObj)
    External (\Q065, DeviceObj)
    External (\Q065.XSTA, MethodObj)
    External (\Q066, DeviceObj)
    External (\Q066.XSTA, MethodObj)
    External (\Q067, DeviceObj)
    External (\Q067.XSTA, MethodObj)
    External (\Q068, DeviceObj)
    External (\Q068.XSTA, MethodObj)
    External (\Q069, DeviceObj)
    External (\Q069.XSTA, MethodObj)
    External (\Q070, DeviceObj)
    External (\Q070.XSTA, MethodObj)
    External (\Q071, DeviceObj)
    External (\Q071.XSTA, MethodObj)
    External (\Q072, DeviceObj)
    External (\Q072.XSTA, MethodObj)
    External (\Q073, DeviceObj)
    External (\Q073.XSTA, MethodObj)
    External (\Q074, DeviceObj)
    External (\Q074.XSTA, MethodObj)
    External (\Q075, DeviceObj)
    External (\Q075.XSTA, MethodObj)
    External (\Q076, DeviceObj)
    External (\Q076.XSTA, MethodObj)
    External (\Q077, DeviceObj)
    External (\Q077.XSTA, MethodObj)
    External (\Q078, DeviceObj)
    External (\Q078.XSTA, MethodObj)
    External (\Q079, DeviceObj)
    External (\Q079.XSTA, MethodObj)
    External (\Q080, DeviceObj)
    External (\Q080.XSTA, MethodObj)
    External (\Q081, DeviceObj)
    External (\Q081.XSTA, MethodObj)
    External (\Q082, DeviceObj)
    External (\Q082.XSTA, MethodObj)
    External (\Q083, DeviceObj)
    External (\Q083.XSTA, MethodObj)
    External (\Q084, DeviceObj)
    External (\Q084.XSTA, MethodObj)
    External (\Q085, DeviceObj)
    External (\Q085.XSTA, MethodObj)
    External (\Q086, DeviceObj)
    External (\Q086.XSTA, MethodObj)
    External (\Q087, DeviceObj)
    External (\Q087.XSTA, MethodObj)
    External (\Q088, DeviceObj)
    External (\Q088.XSTA, MethodObj)
    External (\Q089, DeviceObj)
    External (\Q089.XSTA, MethodObj)
    External (\Q090, DeviceObj)
    External (\Q090.XSTA, MethodObj)
    External (\Q091, DeviceObj)
    External (\Q091.XSTA, MethodObj)
    External (\Q092, DeviceObj)
    External (\Q092.XSTA, MethodObj)
    External (\Q093, DeviceObj)
    External (\Q093.XSTA, MethodObj)
    External (\Q094, DeviceObj)
    External (\Q094.XSTA, MethodObj)
    External (\Q095, DeviceObj)
    External (\Q095.XSTA, MethodObj)
    External (\Q096, DeviceObj)
    External (\Q096.XSTA, MethodObj)
    External (\Q097, DeviceObj)
    External (\Q097.XSTA, MethodObj)
    External (\Q098, DeviceObj)
    External (\Q098.XSTA, MethodObj)
    External (\Q099, DeviceObj)
    External (\Q099.XSTA, MethodObj)
    External (\Q100, DeviceObj)
    External (\Q100.XSTA, MethodObj)
    External (\Q101, DeviceObj)
    External (\Q101.XSTA, MethodObj)
    External (\Q102, DeviceObj)
    External (\Q102.XSTA, MethodObj)
    External (\Q103, DeviceObj)
    External (\Q103.XSTA, MethodObj)
    External (\Q104, DeviceObj)
    External (\Q104.XSTA, MethodObj)
    External (\Q105, DeviceObj)
    External (\Q105.XSTA, MethodObj)
    External (\Q106, DeviceObj)
    External (\Q106.XSTA, MethodObj)
    External (\Q107, DeviceObj)
    External (\Q107.XSTA, MethodObj)
    External (\Q108, DeviceObj)
    External (\Q108.XSTA, MethodObj)
    External (\Q109, DeviceObj)
    External (\Q109.XSTA, MethodObj)
    External (\Q110, DeviceObj)
    External (\Q110.XSTA, MethodObj)
    External (\Q111, DeviceObj)
    External (\Q111.XSTA, MethodObj)
    External (\Q112, DeviceObj)
    External (\Q112.XSTA, MethodObj)
    External (\Q113, DeviceObj)
    External (\Q113.XSTA, MethodObj)
    External (\Q114, DeviceObj)
    External (\Q114.XSTA, MethodObj)
    External (\Q115, DeviceObj)
    External (\Q115.XSTA, MethodObj)
    External (\Q116, DeviceObj)
    External (\Q116.XSTA, MethodObj)
    External (\Q117, DeviceObj)
    External (\Q117.XSTA, MethodObj)
    External (\Q118, DeviceObj)
    External (\Q118.XSTA, MethodObj)
    External (\Q119, DeviceObj)
    External (\Q119.XSTA, MethodObj)
    External (\Q120, DeviceObj)
    External (\Q120.XSTA, MethodObj)
    External (\Q121, DeviceObj)
    External (\Q121.XSTA, MethodObj)
    External (\Q122, DeviceObj)
    External (\Q122.XSTA, MethodObj)
    External (\Q123, DeviceObj)
    External (\Q123.XSTA, MethodObj)
    External (\Q124, DeviceObj)
    External (\Q124.XSTA, MethodObj)
    External (\Q125, DeviceObj)
    External (\Q125.XSTA, MethodObj)
    External (\Q126, DeviceObj)
    External (\Q126.XSTA, MethodObj)
    External (\Q127, DeviceObj)
    External (\Q127.XSTA, MethodObj)
    External (\Q128, DeviceObj)
    External (\Q128.XSTA, MethodObj)
    External (\Q129, DeviceObj)
    External (\Q129.XSTA, MethodObj)
    External (\Q130, DeviceObj)
    External (\Q130.XSTA, MethodObj)
    External (\Q131, DeviceObj)
    External (\Q131.XSTA, MethodObj)
    External (\Q132, DeviceObj)
    External (\Q132.XSTA, MethodObj)
    External (\Q133, DeviceObj)
    External (\Q133.XSTA, MethodObj)
    External (\Q134, DeviceObj)
    External (\Q134.XSTA, MethodObj)
    External (\Q135, DeviceObj)
    External (\Q135.XSTA, MethodObj)
    External (\Q136, DeviceObj)
    External (\Q136.XSTA, MethodObj)
    External (\Q137, DeviceObj)
    External (\Q137.XSTA, MethodObj)
    External (\Q138, DeviceObj)
    External (\Q138.XSTA, MethodObj)
    External (\Q139, DeviceObj)
    External (\Q139.XSTA, MethodObj)
    External (\Q140, DeviceObj)
    External (\Q140.XSTA, MethodObj)
    External (\Q141, DeviceObj)
    External (\Q141.XSTA, MethodObj)
    External (\Q142, DeviceObj)
    External (\Q142.XSTA, MethodObj)
    External (\Q143, DeviceObj)
    External (\Q143.XSTA, MethodObj)
    External (\Q144, DeviceObj)
    External (\Q144.XSTA, MethodObj)
    External (\Q145, DeviceObj)
    External (\Q145.XSTA, MethodObj)
    External (\Q146, DeviceObj)
    External (\Q146.XSTA, MethodObj)
    External (\Q147, DeviceObj)
    External (\Q147.XSTA, MethodObj)
    External (\Q148, DeviceObj)
    External (\Q148.XSTA, MethodObj)
    External (\Q149, DeviceObj)
    External (\Q149.XSTA, MethodObj)
    External (\Q150, DeviceObj)
    External (\Q150.XSTA, MethodObj)
    External (\Q151, DeviceObj)
    External (\Q151.XSTA, MethodObj)
    External (\Q152, DeviceObj)
    External (\Q152.XSTA, MethodObj)
    External (\Q153, DeviceObj)
    External (\Q153.XSTA, MethodObj)
    External (\Q154, DeviceObj)
    External (\Q154.XSTA, MethodObj)
    External (\Q155, DeviceObj)
    External (\Q155.XSTA, MethodObj)
    External (\Q156, DeviceObj)
    External (\Q156.XSTA, MethodObj)
    External (\Q157, DeviceObj)
    External (\Q157.XSTA, MethodObj)
    External (\Q158, DeviceObj)
    External (\Q158.XSTA, MethodObj)
    External (\Q159, DeviceObj)
    External (\Q159.XSTA, MethodObj)
    External (\Q160, DeviceObj)
    External (\Q160.XSTA, MethodObj)
    External (\Q161, DeviceObj)
    External (\Q161.XSTA, MethodObj)
    External (\Q162, DeviceObj)
    External (\Q162.XSTA, MethodObj)
    External (\Q163, DeviceObj)
    External (\Q163.XSTA, MethodObj)
    External (\Q164, DeviceObj)
    External (\Q164.XSTA, MethodObj)
    External (\Q165, DeviceObj)
    External (\Q165.XSTA, MethodObj)
    External (\Q166, DeviceObj)
    External (\Q166.XSTA, MethodObj)
    External (\Q167, DeviceObj)
    External (\Q167.XSTA, MethodObj)
    External (\Q168, DeviceObj)
    External (\Q168.XSTA, MethodObj)
    External (\Q169, DeviceObj)
    External (\Q169.XSTA, MethodObj)
    External (\Q170, DeviceObj)
    External (\Q170.XSTA, MethodObj)
    External (\Q171, DeviceObj)
    External (\Q171.XSTA, MethodObj)
    External (\Q172, DeviceObj)
    External (\Q172.XSTA, MethodObj)
    External (\Q173, DeviceObj)
    External (\Q173.XSTA, MethodObj)
    External (\Q174, DeviceObj)
    External (\Q174.XSTA, MethodObj)
    External (\Q175, DeviceObj)
    External (\Q175.XSTA, MethodObj)
    External (\Q176, DeviceObj)
    External (\Q176.XSTA, MethodObj)
    External (\Q177, DeviceObj)
    External (\Q177.XSTA, MethodObj)
    External (\Q178, DeviceObj)
    External (\Q178.XSTA, MethodObj)
    External (\Q179, DeviceObj)
    External (\Q179.XSTA, MethodObj)
    External (\Q180, DeviceObj)
    External (\Q180.XSTA, MethodObj)
    External (\Q181, DeviceObj)
    External (\Q181.XSTA, MethodObj)
    External (\Q182, DeviceObj)
    External (\Q182.XSTA, MethodObj)
    External (\Q183, DeviceObj)
    External (\Q183.XSTA, MethodObj)
    External (\Q184, DeviceObj)
    External (\Q184.XSTA, MethodObj)
    External (\Q185, DeviceObj)
    External (\Q185.XSTA, MethodObj)
    External (\Q186, DeviceObj)
    External (\Q186.XSTA, MethodObj)
    External (\Q187, DeviceObj)
    External (\Q187.XSTA, MethodObj)
    External (\Q188, DeviceObj)
    External (\Q188.XSTA, MethodObj)
    External (\Q189, DeviceObj)
    External (\Q189.XSTA, MethodObj)
    External (\Q190, DeviceObj)
    External (\Q190.XSTA, MethodObj)
    External (\Q191, DeviceObj)
    External (\Q191.XSTA, MethodObj)
    External (\Q192, DeviceObj)
    External (\Q192.XSTA, MethodObj)
    External (\Q193, DeviceObj)
    External (\Q193.XSTA, MethodObj)
    External (\Q194, DeviceObj)
    External (\Q194.XSTA, MethodObj)
    External (\Q195, DeviceObj)
    External (\Q195.XSTA, MethodObj)
    External (\Q196, DeviceObj)
    External (\Q196.XSTA, MethodObj)
    External (\Q197, DeviceObj)
    External (\Q197.XSTA, MethodObj)
    External (\Q198, DeviceObj)
    External (\Q198.XSTA, MethodObj)
    External (\Q199, DeviceObj)
    External (\Q199.XSTA, MethodObj)
    External (\Q200, DeviceObj)
    External (\Q200.XSTA, MethodObj)
    External (\Q201, DeviceObj)
    External (\Q201.XSTA, MethodObj)
    External (\Q202, DeviceObj)
    External (\Q202.XSTA, MethodObj)
    External (\Q203, DeviceObj)
    External (\Q203.XSTA, MethodObj)
    External (\Q204, DeviceObj)
    External (\Q204.XSTA, MethodObj)
    External (\Q205, DeviceObj)
    External (\Q205.XSTA, MethodObj)
    External (\Q206, DeviceObj)
    External (\Q206.XSTA, MethodObj)
    External (\Q207, DeviceObj)
    External (\Q207.XSTA, MethodObj)
    External (\Q208, DeviceObj)
    External (\Q208.XSTA, MethodObj)
    External (\Q209, DeviceObj)
    External (\Q209.XSTA, MethodObj)
    External (\Q210, DeviceObj)
    External (\Q210.XSTA, MethodObj)
    External (\Q211, DeviceObj)
    External (\Q211.XSTA, MethodObj)
    External (\Q212, DeviceObj)
    External (\Q212.XSTA, MethodObj)
    External (\Q213, DeviceObj)
    External (\Q213.XSTA, MethodObj)
    External (\Q214, DeviceObj)
    External (\Q214.XSTA, MethodObj)
    External (\Q215, DeviceObj)
    External (\Q215.XSTA, MethodObj)
    External (\Q216, DeviceObj)
    External (\Q216.XSTA, MethodObj)
    External (\Q217, DeviceObj)
    External (\Q217.XSTA, MethodObj)
    External (\Q218, DeviceObj)
    External (\Q218.XSTA, MethodObj)
    External (\Q219, DeviceObj)
    External (\Q219.XSTA, MethodObj)
    External (\Q220, DeviceObj)
    External (\Q220.XSTA, MethodObj)
    External (\Q221, DeviceObj)
    External (\Q221.XSTA, MethodObj)
    External (\Q222, DeviceObj)
    External (\Q222.XSTA, MethodObj)
    External (\Q223, DeviceObj)
    External (\Q223.XSTA, MethodObj)
    External (\Q224, DeviceObj)
    External (\Q224.XSTA, MethodObj)
    External (\Q225, DeviceObj)
    External (\Q225.XSTA, MethodObj)
    External (\Q226, DeviceObj)
    External (\Q226.XSTA, MethodObj)
    External (\Q227, DeviceObj)
    External (\Q227.XSTA, MethodObj)
    External (\Q228, DeviceObj)
    External (\Q228.XSTA, MethodObj)
    External (\Q229, DeviceObj)
    External (\Q229.XSTA, MethodObj)
    External (\Q230, DeviceObj)
    External (\Q230.XSTA, MethodObj)
    External (\Q231, DeviceObj)
    External (\Q231.XSTA, MethodObj)
    External (\Q232, DeviceObj)
    External (\Q232.XSTA, MethodObj)
    External (\Q233, DeviceObj)
    External (\Q233.XSTA, MethodObj)
    External (\Q234, DeviceObj)
    External (\Q234.XSTA, MethodObj)
    External (\Q235, DeviceObj)
    External (\Q235.XSTA, MethodObj)
    External (\Q236, DeviceObj)
    External (\Q236.XSTA, MethodObj)
    External (\Q237, DeviceObj)
    External (\Q237.XSTA, MethodObj)
    External (\Q238, DeviceObj)
    External (\Q238.XSTA, MethodObj)
    External (\Q239, DeviceObj)
    External (\Q239.XSTA, MethodObj)
    External (\Q240, DeviceObj)
    External (\Q240.XSTA, MethodObj)
    External (\Q241, DeviceObj)
    External (\Q241.XSTA, MethodObj)
    External (\Q242, DeviceObj)
    External (\Q242.XSTA, MethodObj)
    External (\Q243, DeviceObj)
    External (\Q243.XSTA, MethodObj)
    External (\Q244, DeviceObj)
    External (\Q244.XSTA, MethodObj)
    External (\Q245, DeviceObj)
    External (\Q245.XSTA, MethodObj)
    External (\Q246, DeviceObj)
    External (\Q246.XSTA, MethodObj)
    External (\Q247, DeviceObj)
    External (\Q247.XSTA, MethodObj)
    External (\Q248, DeviceObj)
    External (\Q248.XSTA, MethodObj)
    External (\Q249, DeviceObj)
    External (\Q249.XSTA, MethodObj)
    External (\Q250, DeviceObj)
    External (\Q250.XSTA, MethodObj)
    External (\Q251, DeviceObj)
    External (\Q251.XSTA, MethodObj)
    External (\Q252, DeviceObj)
    External (\Q252.XSTA, MethodObj)
    External (\Q253, DeviceObj)
    External (\Q253.XSTA, MethodObj)
    External (\Q254, DeviceObj)
    External (\Q254.XSTA, MethodObj)
    External (\Q255, DeviceObj)
    External (\Q255.XSTA, MethodObj)
    External (\Q256, DeviceObj)
    External (\Q256.XSTA, MethodObj)
    External (\Q257, DeviceObj)
    External (\Q257.XSTA, MethodObj)
    External (\Q258, DeviceObj)
    External (\Q258.XSTA, MethodObj)
    External (\Q259, DeviceObj)
    External (\Q259.XSTA, MethodObj)
    External (\Q260, DeviceObj)
    External (\Q260.XSTA, MethodObj)
    External (\Q261, DeviceObj)
    External (\Q261.XSTA, MethodObj)
    External (\Q262, DeviceObj)
    External (\Q262.XSTA, MethodObj)
    External (\Q263, DeviceObj)
    External (\Q263.XSTA, MethodObj)
    External (\Q264, DeviceObj)
    External (\Q264.XSTA, MethodObj)
    External (\Q265, DeviceObj)
    External (\Q265.XSTA, MethodObj)
    External (\Q266, DeviceObj)
    External (\Q266.XSTA, MethodObj)
    External (\Q267, DeviceObj)
    External (\Q267.XSTA, MethodObj)
    External (\Q268, DeviceObj)
    External (\Q268.XSTA, MethodObj)
    External (\Q269, DeviceObj)
    External (\Q269.XSTA, MethodObj)
    External (\Q270, DeviceObj)
    External (\Q270.XSTA, MethodObj)
    External (\Q271, DeviceObj)
    External (\Q271.XSTA, MethodObj)
    External (\Q272, DeviceObj)
    External (\Q272.XSTA, MethodObj)
    External (\Q273, DeviceObj)
    External (\Q273.XSTA, MethodObj)
    External (\Q274, DeviceObj)
    External (\Q274.XSTA, MethodObj)
    External (\Q275, DeviceObj)
    External (\Q275.XSTA, MethodObj)
    External (\Q276, DeviceObj)
    External (\Q276.XSTA, MethodObj)
    External (\Q277, DeviceObj)
    External (\Q277.XSTA, MethodObj)
    External (\Q278, DeviceObj)
    External (\Q278.XSTA, MethodObj)
    External (\Q279, DeviceObj)
    External (\Q279.XSTA, MethodObj)
    External (\Q280, DeviceObj)
    External (\Q280.XSTA, MethodObj)
    External (\Q281, DeviceObj)
    External (\Q281.XSTA, MethodObj)
    External (\Q282, DeviceObj)
    External (\Q282.XSTA, MethodObj)
    External (\Q283, DeviceObj)
    External (\Q283.XSTA, MethodObj)
    External (\Q284, DeviceObj)
    External (\Q284.XSTA, MethodObj)
    External (\Q285, DeviceObj)
    External (\Q285.XSTA, MethodObj)
    External (\Q286, DeviceObj)
    External (\Q286.XSTA, MethodObj)
    External (\Q287, DeviceObj)
    External (\Q287.XSTA, MethodObj)
    External (\Q288, DeviceObj)
    External (\Q288.XSTA, MethodObj)
    External (\Q289, DeviceObj)
    External (\Q289.XSTA, MethodObj)
    External (\Q290, DeviceObj)
    External (\Q290.XSTA, MethodObj)
    External (\Q291, DeviceObj)
    External (\Q291.XSTA, MethodObj)
    External (\Q292, DeviceObj)
    External (\Q292.XSTA, MethodObj)
    External (\Q293, DeviceObj)
    External (\Q293.XSTA, MethodObj)
    External (\Q294, DeviceObj)
    External (\Q294.XSTA, MethodObj)
    External (\Q295, DeviceObj)
    External (\Q295.XSTA, MethodObj)
    External (\Q296, DeviceObj)
    External (\Q296.XSTA, MethodObj)
    External (\Q297, DeviceObj)
    External (\Q297.XSTA, MethodObj)
    External (\Q298, DeviceObj)
    External (\Q298.XSTA, MethodObj)
    External (\Q299, DeviceObj)
    External (\Q299.XSTA, MethodObj)
    External (\Q300, DeviceObj)
    External (\Q300.XSTA, MethodObj)
    External (\Q301, DeviceObj)
    External (\Q301.XSTA, MethodObj)
    External (\Q302, DeviceObj)
    External (\Q302.XSTA, MethodObj)
    External (\Q303, DeviceObj)
    External (\Q303.XSTA, MethodObj)
    External (\Q304, DeviceObj)
    External (\Q304.XSTA, MethodObj)
    External (\Q305, DeviceObj)
    External (\Q305.XSTA, MethodObj)
    External (\Q306, DeviceObj)
    External (\Q306.XSTA, MethodObj)
    External (\Q307, DeviceObj)
    External (\Q307.XSTA, MethodObj)
    External (\Q308, DeviceObj)
    External (\Q308.XSTA, MethodObj)
    External (\Q309, DeviceObj)
    External (\Q309.XSTA, MethodObj)
    External (\Q310, DeviceObj)
    External (\Q310.XSTA, MethodObj)
    External (\Q311, DeviceObj)
    External (\Q311.XSTA, MethodObj)
    External (\Q312, DeviceObj)
    External (\Q312.XSTA, MethodObj)
    External (\Q313, DeviceObj)
    External (\Q313.XSTA, MethodObj)
    External (\Q314, DeviceObj)
    External (\Q314.XSTA, MethodObj)
    External (\Q315, DeviceObj)
    External (\Q315.XSTA, MethodObj)
    External (\Q316, DeviceObj)
    External (\Q316.XSTA, MethodObj)
    External (\Q317, DeviceObj)
    External (\Q317.XSTA, MethodObj)
    External (\Q318, DeviceObj)
    External (\Q318.XSTA, MethodObj)
    External (\Q319, DeviceObj)
    External (\Q319.XSTA, MethodObj)
    External (\Q320, DeviceObj)
    External (\Q320.XSTA, MethodObj)
    External (\Q321, DeviceObj)
    External (\Q321.XSTA, MethodObj)
    External (\Q322, DeviceObj)
    External (\Q322.XSTA, MethodObj)
    External (\Q323, DeviceObj)
    External (\Q323.XSTA, MethodObj)
    External (\Q324, DeviceObj)
    External (\Q324.XSTA, MethodObj)
    External (\Q325, DeviceObj)
    External (\Q325.XSTA, MethodObj)
    External (\Q326, DeviceObj)
    External (\Q326.XSTA, MethodObj)
    External (\Q327, DeviceObj)
    External (\Q327.XSTA, MethodObj)
    External (\Q328, DeviceObj)
    External (\Q328.XSTA, MethodObj)
    External (\Q329, DeviceObj)
    External (\Q329.XSTA, MethodObj)
    External (\Q330, DeviceObj)
    External (\Q330.XSTA, MethodObj)
    External (\Q331, DeviceObj)
    External (\Q331.XSTA, MethodObj)
    External (\Q332, DeviceObj)
    External (\Q332.XSTA, MethodObj)
    External (\Q333, DeviceObj)
    External (\Q333.XSTA, MethodObj)
    External (\Q334, DeviceObj)
    External (\Q334.XSTA, MethodObj)
    External (\Q335, DeviceObj)
    External (\Q335.XSTA, MethodObj)
    External (\Q336, DeviceObj)
    External (\Q336.XSTA, MethodObj)
    External (\Q337, DeviceObj)
    External (\Q337.XSTA, MethodObj)
    External (\Q338, DeviceObj)
    External (\Q338.XSTA, MethodObj)
    External (\Q339, DeviceObj)
    External (\Q339.XSTA, MethodObj)
    External (\Q340, DeviceObj)
    External (\Q340.XSTA, MethodObj)
    External (\Q341, DeviceObj)
    External (\Q341.XSTA, MethodObj)
    External (\Q342, DeviceObj)
    External (\Q342.XSTA, MethodObj)
    External (\Q343, DeviceObj)
    External (\Q343.XSTA, MethodObj)
    External (\Q344, DeviceObj)
    External (\Q344.XSTA, MethodObj)
    External (\Q345, DeviceObj)
    External (\Q345.XSTA, MethodObj)
    External (\Q346, DeviceObj)
    External (\Q346.XSTA, MethodObj)
    External (\Q347, DeviceObj)
    External (\Q347.XSTA, MethodObj)
    External (\Q348, DeviceObj)
    External (\Q348.XSTA, MethodObj)
    External (\Q349, DeviceObj)
    External (\Q349.XSTA, MethodObj)
    External (\Q350, DeviceObj)
    External (\Q350.XSTA, MethodObj)
    External (\Q351, DeviceObj)
    External (\Q351.XSTA, MethodObj)
    External (\Q352, DeviceObj)
    External (\Q352.XSTA, MethodObj)
    External (\Q353, DeviceObj)
    External (\Q353.XSTA, MethodObj)
    External (\Q354, DeviceObj)
    External (\Q354.XSTA, MethodObj)
    External (\Q355, DeviceObj)
    External (\Q355.XSTA, MethodObj)
    External (\Q356, DeviceObj)
    External (\Q356.XSTA, MethodObj)
    External (\Q357, DeviceObj)
    External (\Q357.XSTA, MethodObj)
    External (\Q358, DeviceObj)
    External (\Q358.XSTA, MethodObj)
    External (\Q359, DeviceObj)
    External (\Q359.XSTA, MethodObj)
    External (\Q360, DeviceObj)
    External (\Q360.XSTA, MethodObj)
    External (\Q361, DeviceObj)
    External (\Q361.XSTA, MethodObj)
    External (\Q362, DeviceObj)
    External (\Q362.XSTA, MethodObj)
    External (\Q363, DeviceObj)
    External (\Q363.XSTA, MethodObj)
    External (\Q364, DeviceObj)
    External (\Q364.XSTA, MethodObj)
    External (\Q365, DeviceObj)
    External (\Q365.XSTA, MethodObj)
    External (\Q366, DeviceObj)
    External (\Q366.XSTA, MethodObj)
    External (\Q367, DeviceObj)
    External (\Q367.XSTA, MethodObj)
    External (\Q368, DeviceObj)
    External (\Q368.XSTA, MethodObj)
    External (\Q369, DeviceObj)
    External (\Q369.XSTA, MethodObj)
    External (\Q370, DeviceObj)
    External (\Q370.XSTA, MethodObj)
    External (\Q371, DeviceObj)
    External (\Q371.XSTA, MethodObj)
    External (\Q372, DeviceObj)
    External (\Q372.XSTA, MethodObj)
    External (\Q373, DeviceObj)
    External (\Q373.XSTA, MethodObj)
    External (\Q374, DeviceObj)
    External (\Q374.XSTA, MethodObj)
    External (\Q375, DeviceObj)
    External (\Q375.XSTA, MethodObj)
    External (\Q376, DeviceObj)
    External (\Q376.XSTA, MethodObj)
    External (\Q377, DeviceObj)
    External (\Q377.XSTA, MethodObj)
    External (\Q378, DeviceObj)
    External (\Q378.XSTA, MethodObj)
    External (\Q379, DeviceObj)
    External (\Q379.XSTA, MethodObj)
    External (\Q380, DeviceObj)
    External (\Q380.XSTA, MethodObj)
    External (\Q381, DeviceObj)
    External (\Q381.XSTA, MethodObj)
    External (\Q382, DeviceObj)
    External (\Q382.XSTA, MethodObj)
    External (\Q383, DeviceObj)
    External (\Q383.XSTA, MethodObj)
    External (\Q384, DeviceObj)
    External (\Q384.XSTA, MethodObj)
    External (\Q385, DeviceObj)
    External (\Q385.XSTA, MethodObj)
    External (\Q386, DeviceObj)
    External (\Q386.XSTA, MethodObj)
    External (\Q387, DeviceObj)
    External (\Q387.XSTA, MethodObj)
    External (\Q388, DeviceObj)
    External (\Q388.XSTA, MethodObj)
    External (\Q389, DeviceObj)
    External (\Q389.XSTA, MethodObj)
    External (\Q390, DeviceObj)
    External (\Q390.XSTA, MethodObj)
    External (\Q391, DeviceObj)
    External (\Q391.XSTA, MethodObj)
    External (\Q392, DeviceObj)
    External (\Q392.XSTA, MethodObj)
    External (\Q393, DeviceObj)
    External (\Q393.XSTA, MethodObj)
    External (\Q394, DeviceObj)
    External (\Q394.XSTA, MethodObj)
    External (\Q395, DeviceObj)
    External (\Q395.XSTA, MethodObj)
    External (\Q396, DeviceObj)
    External (\Q396.XSTA, MethodObj)
    External (\Q397, DeviceObj)
    External (\Q397.XSTA, MethodObj)
    External (\Q398, DeviceObj)
    External (\Q398.XSTA, MethodObj)
    External (\Q399, DeviceObj)
    External (\Q399.XSTA, MethodObj)
    External (\Q400, DeviceObj)
    External (\Q400.XSTA, MethodObj)
    External (\Q401, DeviceObj)
    External (\Q401.XSTA, MethodObj)
    External (\Q402, DeviceObj)
    External (\Q402.XSTA, MethodObj)
    External (\Q403, DeviceObj)
    External (\Q403.XSTA, MethodObj)
    External (\Q404, DeviceObj)
    External (\Q404.XSTA, MethodObj)
    External (\Q405, DeviceObj)
    External (\Q405.XSTA, MethodObj)
    External (\Q406, DeviceObj)
    External (\Q406.XSTA, MethodObj)
    External (\Q407, DeviceObj)
    External (\Q407.XSTA, MethodObj)
    External (\Q408, DeviceObj)
    External (\Q408.XSTA, MethodObj)
    External (\Q409, DeviceObj)
    External (\Q409.XSTA, MethodObj)
    External (\Q410, DeviceObj)
    External (\Q410.XSTA, MethodObj)
    External (\Q411, DeviceObj)
    External (\Q411.XSTA, MethodObj)
    External (\Q412, DeviceObj)
    External (\Q412.XSTA, MethodObj)
    External (\Q413, DeviceObj)
    External (\Q413.XSTA, MethodObj)
    External (\Q414, DeviceObj)
    External (\Q414.XSTA, MethodObj)
    External (\Q415, DeviceObj)
    External (\Q415.XSTA, MethodObj)
    External (\Q416, DeviceObj)
    External (\Q416.XSTA, MethodObj)
    External (\Q417, DeviceObj)
    External (\Q417.XSTA, MethodObj)
    External (\Q418, DeviceObj)
    External (\Q418.XSTA, MethodObj)
    External (\Q419, DeviceObj)
    External (\Q419.XSTA, MethodObj)
    External (\Q420, DeviceObj)
    External (\Q420.XSTA, MethodObj)
    External (\Q421, DeviceObj)
    External (\Q421.XSTA, MethodObj)
    External (\Q422, DeviceObj)
    External (\Q422.XSTA, MethodObj)
    External (\Q423, DeviceObj)
    External (\Q423.XSTA, MethodObj)
    External (\Q424, DeviceObj)
    External (\Q424.XSTA, MethodObj)
    External (\Q425, DeviceObj)
    External (\Q425.XSTA, MethodObj)
    External (\Q426, DeviceObj)
    External (\Q426.XSTA, MethodObj)
    External (\Q427, DeviceObj)
    External (\Q427.XSTA, MethodObj)
    External (\Q428, DeviceObj)
    External (\Q428.XSTA, MethodObj)
    External (\Q429, DeviceObj)
    External (\Q429.XSTA, MethodObj)
    External (\Q430, DeviceObj)
    External (\Q430.XSTA, MethodObj)
    External (\Q431, DeviceObj)
    External (\Q431.XSTA, MethodObj)
    External (\Q432, DeviceObj)
    External (\Q432.XSTA, MethodObj)
    External (\Q433, DeviceObj)
    External (\Q433.XSTA, MethodObj)
    External (\Q434, DeviceObj)
    External (\Q434.XSTA, MethodObj)
    External (\Q435, DeviceObj)
    External (\Q435.XSTA, MethodObj)
    External (\Q436, DeviceObj)
    External (\Q436.XSTA, MethodObj)
    External (\Q437, DeviceObj)
    External (\Q437.XSTA, MethodObj)
    External (\Q438, DeviceObj)
    External (\Q438.XSTA, MethodObj)
    External (\Q439, DeviceObj)
    External (\Q439.XSTA, MethodObj)
    External (\Q440, DeviceObj)
    External (\Q440.XSTA, MethodObj)
    External (\Q441, DeviceObj)
    External (\Q441.XSTA, MethodObj)
    External (\Q442, DeviceObj)
    External (\Q442.XSTA, MethodObj)
    External (\Q443, DeviceObj)
    External (\Q443.XSTA, MethodObj)
    External (\Q444, DeviceObj)
    External (\Q444.XSTA, MethodObj)
    External (\Q445, DeviceObj)
    External (\Q445.XSTA, MethodObj)
    External (\Q446, DeviceObj)
    External (\Q446.XSTA, MethodObj)
    External (\Q447, DeviceObj)
    External (\Q447.XSTA, MethodObj)
    External (\Q448, DeviceObj)
    External (\Q448.XSTA, MethodObj)
    External (\Q449, DeviceObj)
    External (\Q449.XSTA, MethodObj)
    External (\Q450, DeviceObj)
    External (\Q450.XSTA, MethodObj)
    External (\Q451, DeviceObj)
    External (\Q451.XSTA, MethodObj)
    External (\Q452, DeviceObj)
    External (\Q452.XSTA, MethodObj)
    External (\Q453, DeviceObj)
    External (\Q453.XSTA, MethodObj)
    External (\Q454, DeviceObj)
    External (\Q454.XSTA, MethodObj)
    External (\Q455, DeviceObj)
    External (\Q455.XSTA, MethodObj)
    External (\Q456, DeviceObj)
    External (\Q456.XSTA, MethodObj)
    External (\Q457, DeviceObj)
    External (\Q457.XSTA, MethodObj)
    External (\Q458, DeviceObj)
    External (\Q458.XSTA, MethodObj)
    External (\Q459, DeviceObj)
    External (\Q459.XSTA, MethodObj)
    External (\Q460, DeviceObj)
    External (\Q460.XSTA, MethodObj)
    External (\Q461, DeviceObj)
    External (\Q461.XSTA, MethodObj)
    External (\Q462, DeviceObj)
    External (\Q462.XSTA, MethodObj)
    External (\Q463, DeviceObj)
    External (\Q463.XSTA, MethodObj)
    External (\Q464, DeviceObj)
    External (\Q464.XSTA, MethodObj)
    External (\Q465, DeviceObj)
    External (\Q465.XSTA, MethodObj)
    External (\Q466, DeviceObj)
    External (\Q466.XSTA, MethodObj)
    External (\Q467, DeviceObj)
    External (\Q467.XSTA, MethodObj)
    External (\Q468, DeviceObj)
    External (\Q468.XSTA, MethodObj)
    External (\Q469, DeviceObj)
    External (\Q469.XSTA, MethodObj)
    External (\Q470, DeviceObj)
    External (\Q470.XSTA, MethodObj)
    External (\Q471, DeviceObj)
    External (\Q471.XSTA, MethodObj)
    External (\Q472, DeviceObj)
    External (\Q472.XSTA, MethodObj)
    External (\Q473, DeviceObj)
    External (\Q473.XSTA, MethodObj)
    External (\Q474, DeviceObj)
    External (\Q474.XSTA, MethodObj)
    External (\Q475, DeviceObj)
    External (\Q475.XSTA, MethodObj)
    External (\Q476, DeviceObj)
    External (\Q476.XSTA, MethodObj)
    External (\Q477, DeviceObj)
    External (\Q477.XSTA, MethodObj)
    External (\Q478, DeviceObj)
    External (\Q478.XSTA, MethodObj)
    External (\Q479, DeviceObj)
    External (\Q479.XSTA, MethodObj)
    External (\Q480, DeviceObj)
    External (\Q480.XSTA, MethodObj)
    External (\Q481, DeviceObj)
    External (\Q481.XSTA, MethodObj)
    External (\Q482, DeviceObj)
    External (\Q482.XSTA, MethodObj)
    External (\Q483, DeviceObj)
    External (\Q483.XSTA, MethodObj)
    External (\Q484, DeviceObj)
    External (\Q484.XSTA, MethodObj)
    External (\Q485, DeviceObj)
    External (\Q485.XSTA, MethodObj)
    External (\Q486, DeviceObj)
    External (\Q486.XSTA, MethodObj)
    External (\Q487, DeviceObj)
    External (\Q487.XSTA, MethodObj)
    External (\Q488, DeviceObj)
    External (\Q488.XSTA, MethodObj)
    External (\Q489, DeviceObj)
    External (\Q489.XSTA, MethodObj)
    External (\Q490, DeviceObj)
    External (\Q490.XSTA, MethodObj)
    External (\Q491, DeviceObj)
    External (\Q491.XSTA, MethodObj)
    External (\Q492, DeviceObj)
    External (\Q492.XSTA, MethodObj)
    External (\Q493, DeviceObj)
    External (\Q493.XSTA, MethodObj)
    External (\Q494, DeviceObj)
    External (\Q494.XSTA, MethodObj)
    External (\Q495, DeviceObj)
    External (\Q495.XSTA, MethodObj)
    External (\Q496, DeviceObj)
    External (\Q496.XSTA, MethodObj)
    External (\Q497, DeviceObj)
    External (\Q497.XSTA, MethodObj)
    External (\Q498, DeviceObj)
    External (\Q498.XSTA, MethodObj)
    External (\Q499, DeviceObj)
    External (\Q499.XSTA, MethodObj)
    External (\Q500, DeviceObj)
    External (\Q500.XSTA, MethodObj)
    External (\Q501, DeviceObj)
    External (\Q501.XSTA, MethodObj)
    External (\Q502, DeviceObj)
    External (\Q502.XSTA, MethodObj)
    External (\Q503, DeviceObj)
    External (\Q503.XSTA, MethodObj)
    External (\Q504, DeviceObj)
    External (\Q504.XSTA, MethodObj)
    External (\Q505, DeviceObj)
    External (\Q505.XSTA, MethodObj)
    External (\Q506, DeviceObj)
    External (\Q506.XSTA, MethodObj)
    External (\Q507, DeviceObj)
    External (\Q507.XSTA, MethodObj)
    External (\Q508, DeviceObj)
    External (\Q508.XSTA, MethodObj)
    External (\Q509, DeviceObj)
    External (\Q509.XSTA, MethodObj)
    External (\Q510, DeviceObj)
    External (\Q510.XSTA, MethodObj)
    External (\Q511, DeviceObj)
    External (\Q511.XSTA, MethodObj)
    External (\Q512, DeviceObj)
    External (\Q512.XSTA, MethodObj)
    External (\Q513, DeviceObj)
    External (\Q513.XSTA, MethodObj)
    External (\Q514, DeviceObj)
    External (\Q514.XSTA, MethodObj)
    External (\Q515, DeviceObj)
    External (\Q515.XSTA, MethodObj)
    External (\Q516, DeviceObj)
    External (\Q516.XSTA, MethodObj)
    External (\Q517, DeviceObj)
    External (\Q517.XSTA, MethodObj)
    External (\Q518, DeviceObj)
    External (\Q518.XSTA, MethodObj)
    External (\Q519, DeviceObj)
    External (\Q519.XSTA, MethodObj)
    External (\Q520, DeviceObj)
    External (\Q520.XSTA, MethodObj)
    External (\Q521, DeviceObj)
    External (\Q521.XSTA, MethodObj)
    External (\Q522, DeviceObj)
    External (\Q522.XSTA, MethodObj)
    External (\Q523, DeviceObj)
    External (\Q523.XSTA, MethodObj)
    External (\Q524, DeviceObj)
    External (\Q524.XSTA, MethodObj)
    External (\Q525, DeviceObj)
    External (\Q525.XSTA, MethodObj)
    External (\Q526, DeviceObj)
    External (\Q526.XSTA, MethodObj)
    External (\Q527, DeviceObj)
    External (\Q527.XSTA, MethodObj)
    External (\Q528, DeviceObj)
    External (\Q528.XSTA, MethodObj)
    External (\Q529, DeviceObj)
    External (\Q529.XSTA, MethodObj)
    External (\Q530, DeviceObj)
    External (\Q530.XSTA, MethodObj)
    External (\Q531, DeviceObj)
    External (\Q531.XSTA, MethodObj)
    External (\Q532, DeviceObj)
    External (\Q532.XSTA, MethodObj)
    External (\Q533, DeviceObj)
    External (\Q533.XSTA, MethodObj)
    External (\Q534, DeviceObj)
    External (\Q534.XSTA, MethodObj)
    External (\Q535, DeviceObj)
    External (\Q535.XSTA, MethodObj)
    External (\Q536, DeviceObj)
    External (\Q536.XSTA, MethodObj)
    External (\Q537, DeviceObj)
    External (\Q537.XSTA, MethodObj)
    External (\Q538, DeviceObj)
    External (\Q538.XSTA, MethodObj)
    External (\Q539, DeviceObj)
    External (\Q539.XSTA, MethodObj)
    External (\Q540, DeviceObj)
    External (\Q540.XSTA, MethodObj)
    External (\Q541, DeviceObj)
    External (\Q541.XSTA, MethodObj)
    External (\Q542, DeviceObj)
    External (\Q542.XSTA, MethodObj)
    External (\Q543, DeviceObj)
    External (\Q543.XSTA, MethodObj)
    External (\Q544, DeviceObj)
    External (\Q544.XSTA, MethodObj)
    External (\Q545, DeviceObj)
    External (\Q545.XSTA, MethodObj)
    External (\Q546, DeviceObj)
    External (\Q546.XSTA, MethodObj)
    External (\Q547, DeviceObj)
    External (\Q547.XSTA, MethodObj)
    External (\Q548, DeviceObj)
    External (\Q548.XSTA, MethodObj)
    External (\Q549, DeviceObj)
    External (\Q549.XSTA, MethodObj)
    External (\Q550, DeviceObj)
    External (\Q550.XSTA, MethodObj)
    External (\Q551, DeviceObj)
    External (\Q551.XSTA, MethodObj)
    External (\Q552, DeviceObj)
    External (\Q552.XSTA, MethodObj)
    External (\Q553, DeviceObj)
    External (\Q553.XSTA, MethodObj)
    External (\Q554, DeviceObj)
    External (\Q554.XSTA, MethodObj)
    External (\Q555, DeviceObj)
    External (\Q555.XSTA, MethodObj)
    External (\Q556, DeviceObj)
    External (\Q556.XSTA, MethodObj)
    External (\Q557, DeviceObj)
    External (\Q557.XSTA, MethodObj)
    External (\Q558, DeviceObj)
    External (\Q558.XSTA, MethodObj)
    External (\Q559, DeviceObj)
    External (\Q559.XSTA, MethodObj)
    External (\Q560, DeviceObj)
    External (\Q560.XSTA, MethodObj)
    External (\Q561, DeviceObj)
    External (\Q561.XSTA, MethodObj)
    External (\Q562, DeviceObj)
    External (\Q562.XSTA, MethodObj)
    External (\Q563, DeviceObj)
    External (\Q563.XSTA, MethodObj)
    External (\Q564, DeviceObj)
    External (\Q564.XSTA, MethodObj)
    External (\Q565, DeviceObj)
    External (\Q565.XSTA, MethodObj)
    External (\Q566, DeviceObj)
    External (\Q566.XSTA, MethodObj)
    External (\Q567, DeviceObj)
    External (\Q567.XSTA, MethodObj)
    External (\Q568, DeviceObj)
    External (\Q568.XSTA, MethodObj)
    External (\Q569, DeviceObj)
    External (\Q569.XSTA, MethodObj)
    External (\Q570, DeviceObj)
    External (\Q570.XSTA, MethodObj)
    External (\Q571, DeviceObj)
    External (\Q571.XSTA, MethodObj)
    External (\Q572, DeviceObj)
    External (\Q572.XSTA, MethodObj)
    External (\Q573, DeviceObj)
    External (\Q573.XSTA, MethodObj)
    External (\Q574, DeviceObj)
    External (\Q574.XSTA, MethodObj)
    External (\Q575, DeviceObj)
    External (\Q575.XSTA, MethodObj)
    External (\Q576, DeviceObj)
    External (\Q576.XSTA, MethodObj)
    External (\Q577, DeviceObj)
    External (\Q577.XSTA, MethodObj)
    External (\Q578, DeviceObj)
    External (\Q578.XSTA, MethodObj)
    External (\Q579, DeviceObj)
    External (\Q579.XSTA, MethodObj)
    External (\Q580, DeviceObj)
    External (\Q580.XSTA, MethodObj)
    External (\Q581, DeviceObj)
    External (\Q581.XSTA, MethodObj)
    External (\Q582, DeviceObj)
    External (\Q582.XSTA, MethodObj)
    External (\Q583, DeviceObj)
    External (\Q583.XSTA, MethodObj)
    External (\Q584, DeviceObj)
    External (\Q584.XSTA, MethodObj)
    External (\Q585, DeviceObj)
    External (\Q585.XSTA, MethodObj)
    External (\Q586, DeviceObj)
    External (\Q586.XSTA, MethodObj)
    External (\Q587, DeviceObj)
    External (\Q587.XSTA, MethodObj)
    External (\Q588, DeviceObj)
    External (\Q588.XSTA, MethodObj)
    External (\Q589, DeviceObj)
    External (\Q589.XSTA, MethodObj)
    External (\Q590, DeviceObj)
    External (\Q590.XSTA, MethodObj)
    External (\Q591, DeviceObj)
    External (\Q591.XSTA, MethodObj)
    External (\Q592, DeviceObj)
    External (\Q592.XSTA, MethodObj)
    External (\Q593, DeviceObj)
    External (\Q593.XSTA, MethodObj)
    External (\Q594, DeviceObj)
    External (\Q594.XSTA, MethodObj)
    External (\Q595, DeviceObj)
    External (\Q595.XSTA, MethodObj)
    External (\Q596, DeviceObj)
    External (\Q596.XSTA, MethodObj)
    External (\Q597, DeviceObj)
    External (\Q597.XSTA, MethodObj)
    External (\Q598, DeviceObj)
    External (\Q598.XSTA, MethodObj)
    External (\Q599, DeviceObj)
    External (\Q599.XSTA, MethodObj)
    External (\Q600, DeviceObj)
    External (\Q600.XSTA, MethodObj)
    External (\Q601, DeviceObj)
    External (\Q601.XSTA, MethodObj)
    External (\Q602, DeviceObj)
    External (\Q602.XSTA, MethodObj)
    External (\Q603, DeviceObj)
    External (\Q603.XSTA, MethodObj)
    External (\Q604, DeviceObj)
    External (\Q604.XSTA, MethodObj)
    External (\Q605, DeviceObj)
    External (\Q605.XSTA, MethodObj)
    External (\Q606, DeviceObj)
    External (\Q606.XSTA, MethodObj)
    External (\Q607, DeviceObj)
    External (\Q607.XSTA, MethodObj)
    External (\Q608, DeviceObj)
    External (\Q608.XSTA, MethodObj)
    External (\Q609, DeviceObj)
    External (\Q609.XSTA, MethodObj)
    External (\Q610, DeviceObj)
    External (\Q610.XSTA, MethodObj)
    External (\Q611, DeviceObj)
    External (\Q611.XSTA, MethodObj)
    External (\Q612, DeviceObj)
    External (\Q612.XSTA, MethodObj)
    External (\Q613, DeviceObj)
    External (\Q613.XSTA, MethodObj)
    External (\Q614, DeviceObj)
    External (\Q614.XSTA, MethodObj)
    External (\Q615, DeviceObj)
    External (\Q615.XSTA, MethodObj)
    External (\Q616, DeviceObj)
    External (\Q616.XSTA, MethodObj)
    External (\Q617, DeviceObj)
    External (\Q617.XSTA, MethodObj)
    External (\Q618, DeviceObj)
    External (\Q618.XSTA, MethodObj)
    External (\Q619, DeviceObj)
    External (\Q619.XSTA, MethodObj)
    External (\Q620, DeviceObj)
    External (\Q620.XSTA, MethodObj)
    External (\Q621, DeviceObj)
    External (\Q621.XSTA, MethodObj)
    External (\Q622, DeviceObj)
    External (\Q622.XSTA, MethodObj)
    External (\Q623, DeviceObj)
    External (\Q623.XSTA, MethodObj)
    External (\Q624, DeviceObj)
    External (\Q624.XSTA, MethodObj)
    External (\Q625, DeviceObj)
    External (\Q625.XSTA, MethodObj)
    External (\Q626, DeviceObj)
    External (\Q626.XSTA, MethodObj)
    External (\Q627, DeviceObj)
    External (\Q627.XSTA, MethodObj)
    External (\Q628, DeviceObj)
    External (\Q628.XSTA, MethodObj)
    External (\Q629, DeviceObj)
    External (\Q629.XSTA, MethodObj)
    External (\Q630, DeviceObj)
    External (\Q630.XSTA, MethodObj)
    External (\Q631, DeviceObj)
    External (\Q631.XSTA, MethodObj)
    External (\Q632, DeviceObj)
    External (\Q632.XSTA, MethodObj)
    External (\Q633, DeviceObj)
    External (\Q633.XSTA, MethodObj)
    External (\Q634, DeviceObj)
    External (\Q634.XSTA, MethodObj)
    External (\Q635, DeviceObj)
    External (\Q635.XSTA, MethodObj)
    External (\Q636, DeviceObj)
    External (\Q636.XSTA, MethodObj)
    External (\Q637, DeviceObj)
    External (\Q637.XSTA, MethodObj)
    External (\Q638, DeviceObj)
    External (\Q638.XSTA, MethodObj)
    External (\Q639, DeviceObj)
    External (\Q639.XSTA, MethodObj)
    External (\Q640, DeviceObj)
    External (\Q640.XSTA, MethodObj)
    External (\Q641, DeviceObj)
    External (\Q641.XSTA, MethodObj)
    External (\Q642, DeviceObj)
    External (\Q642.XSTA, MethodObj)
    External (\Q643, DeviceObj)
    External (\Q643.XSTA, MethodObj)
    External (\Q644, DeviceObj)
    External (\Q644.XSTA, MethodObj)
    External (\Q645, DeviceObj)
    External (\Q645.XSTA, MethodObj)
    External (\Q646, DeviceObj)
    External (\Q646.XSTA, MethodObj)
    External (\Q647, DeviceObj)
    External (\Q647.XSTA, MethodObj)
    External (\Q648, DeviceObj)
    External (\Q648.XSTA, MethodObj)
    External (\Q649, DeviceObj)
    External (\Q649.XSTA, MethodObj)
    External (\Q650, DeviceObj)
    External (\Q650.XSTA, MethodObj)
    External (\Q651, DeviceObj)
    External (\Q651.XSTA, MethodObj)
    External (\Q652, DeviceObj)
    External (\Q652.XSTA, MethodObj)
    External (\Q653, DeviceObj)
    External (\Q653.XSTA, MethodObj)
    External (\Q654, DeviceObj)
    External (\Q654.XSTA, MethodObj)
    External (\Q655, DeviceObj)
    External (\Q655.XSTA, MethodObj)
    External (\Q656, DeviceObj)
    External (\Q656.XSTA, MethodObj)
    External (\Q657, DeviceObj)
    External (\Q657.XSTA, MethodObj)
    External (\Q658, DeviceObj)
    External (\Q658.XSTA, MethodObj)
    External (\Q659, DeviceObj)
    External (\Q659.XSTA, MethodObj)
    External (\Q660, DeviceObj)
    External (\Q660.XSTA, MethodObj)
    External (\Q661, DeviceObj)
    External (\Q661.XSTA, MethodObj)
    External (\Q662, DeviceObj)
    External (\Q662.XSTA, MethodObj)
    External (\Q663, DeviceObj)
    External (\Q663.XSTA, MethodObj)
    External (\Q664, DeviceObj)
    External (\Q664.XSTA, MethodObj)
    External (\Q665, DeviceObj)
    External (\Q665.XSTA, MethodObj)
    External (\Q666, DeviceObj)
    External (\Q666.XSTA, MethodObj)
    External (\Q667, DeviceObj)
    External (\Q667.XSTA, MethodObj)
    External (\Q668, DeviceObj)
    External (\Q668.XSTA, MethodObj)
    External (\Q669, DeviceObj)
    External (\Q669.XSTA, MethodObj)
    External (\Q670, DeviceObj)
    External (\Q670.XSTA, MethodObj)
    External (\Q671, DeviceObj)
    External (\Q671.XSTA, MethodObj)
    External (\Q672, DeviceObj)
    External (\Q672.XSTA, MethodObj)
    External (\Q673, DeviceObj)
    External (\Q673.XSTA, MethodObj)
    External (\Q674, DeviceObj)
    External (\Q674.XSTA, MethodObj)
    External (\Q675, DeviceObj)
    External (\Q675.XSTA, MethodObj)
    External (\Q676, DeviceObj)
    External (\Q676.XSTA, MethodObj)
    External (\Q677, DeviceObj)
    External (\Q677.XSTA, MethodObj)
    External (\Q678, DeviceObj)
    External (\Q678.XSTA, MethodObj)
    External (\Q679, DeviceObj)
    External (\Q679.XSTA, MethodObj)
    External (\Q680, DeviceObj)
    External (\Q680.XSTA, MethodObj)
    External (\Q681, DeviceObj)
    External (\Q681.XSTA, MethodObj)
    External (\Q682, DeviceObj)
    External (\Q682.XSTA, MethodObj)
    External (\Q683, DeviceObj)
    External (\Q683.XSTA, MethodObj)
    External (\Q684, DeviceObj)
    External (\Q684.XSTA, MethodObj)
    External (\Q685, DeviceObj)
    External (\Q685.XSTA, MethodObj)
    External (\Q686, DeviceObj)
    External (\Q686.XSTA, MethodObj)
    External (\Q687, DeviceObj)
    External (\Q687.XSTA, MethodObj)
    External (\Q688, DeviceObj)
    External (\Q688.XSTA, MethodObj)
    External (\Q689, DeviceObj)
    External (\Q689.XSTA, MethodObj)
    External (\Q690, DeviceObj)
    External (\Q690.XSTA, MethodObj)
    External (\Q691, DeviceObj)
    External (\Q691.XSTA, MethodObj)
    External (\Q692, DeviceObj)
    External (\Q692.XSTA, MethodObj)
    External (\Q693, DeviceObj)
    External (\Q693.XSTA, MethodObj)
    External (\Q694, DeviceObj)
    External (\Q694.XSTA, MethodObj)
    External (\Q695, DeviceObj)
    External (\Q695.XSTA, MethodObj)
    External (\Q696, DeviceObj)
    External (\Q696.XSTA, MethodObj)
    External (\Q697, DeviceObj)
    External (\Q697.XSTA, MethodObj)
    External (\Q698, DeviceObj)
    External (\Q698.XSTA, MethodObj)
    External (\Q699, DeviceObj)
    External (\Q699.XSTA, MethodObj)
    External (\Q700, DeviceObj)
    External (\Q700.XSTA, MethodObj)
    External (\Q701, DeviceObj)
    External (\Q701.XSTA, MethodObj)
    External (\Q702, DeviceObj)
    External (\Q702.XSTA, MethodObj)
    External (\Q703, DeviceObj)
    External (\Q703.XSTA, MethodObj)
    External (\Q704, DeviceObj)
    External (\Q704.XSTA, MethodObj)
    External (\Q705, DeviceObj)
    External (\Q705.XSTA, MethodObj)
    External (\Q706, DeviceObj)
    External (\Q706.XSTA, MethodObj)
    External (\Q707, DeviceObj)
    External (\Q707.XSTA, MethodObj)
    External (\Q708, DeviceObj)
    External (\Q708.XSTA, MethodObj)
    External (\Q709, DeviceObj)
    External (\Q709.XSTA, MethodObj)
    External (\Q710, DeviceObj)
    External (\Q710.XSTA, MethodObj)
    External (\Q711, DeviceObj)
    External (\Q711.XSTA, MethodObj)
    External (\Q712, DeviceObj)
    External (\Q712.XSTA, MethodObj)
    External (\Q713, DeviceObj)
    External (\Q713.XSTA, MethodObj)
    External (\Q714, DeviceObj)
    External (\Q714.XSTA, MethodObj)
    External (\Q715, DeviceObj)
    External (\Q715.XSTA, MethodObj)
    External (\Q716, DeviceObj)
    External (\Q716.XSTA, MethodObj)
    External (\Q717, DeviceObj)
    External (\Q717.XSTA, MethodObj)
    External (\Q718, DeviceObj)
    External (\Q718.XSTA, MethodObj)
    External (\Q719, DeviceObj)
    External (\Q719.XSTA, MethodObj)
    External (\Q720, DeviceObj)
    External (\Q720.XSTA, MethodObj)
    External (\Q721, DeviceObj)
    External (\Q721.XSTA, MethodObj)
    External (\Q722, DeviceObj)
    External (\Q722.XSTA, MethodObj)
    External (\Q723, DeviceObj)
    External (\Q723.XSTA, MethodObj)
    External (\Q724, DeviceObj)
    External (\Q724.XSTA, MethodObj)
    External (\Q725, DeviceObj)
    External (\Q725.XSTA, MethodObj)
    External (\Q726, DeviceObj)
    External (\Q726.XSTA, MethodObj)
    External (\Q727, DeviceObj)
    External (\Q727.XSTA, MethodObj)
    External (\Q728, DeviceObj)
    External (\Q728.XSTA, MethodObj)
    External (\Q729, DeviceObj)
    External (\Q729.XSTA, MethodObj)
    External (\Q730, DeviceObj)
    External (\Q730.XSTA, MethodObj)
    External (\Q731, DeviceObj)
    External (\Q731.XSTA, MethodObj)
    External (\Q732, DeviceObj)
    External (\Q732.XSTA, MethodObj)
    External (\Q733, DeviceObj)
    External (\Q733.XSTA, MethodObj)
    External (\Q734, DeviceObj)
    External (\Q734.XSTA, MethodObj)
    External (\Q735, DeviceObj)
    External (\Q735.XSTA, MethodObj)
    External (\Q736, DeviceObj)
    External (\Q736.XSTA, MethodObj)
    External (\Q737, DeviceObj)
    External (\Q737.XSTA, MethodObj)
    External (\Q738, DeviceObj)
    External (\Q738.XSTA, MethodObj)
    External (\Q739, DeviceObj)
    External (\Q739.XSTA, MethodObj)
    External (\Q740, DeviceObj)
    External (\Q740.XSTA, MethodObj)
    External (\Q741, DeviceObj)
    External (\Q741.XSTA, MethodObj)
    External (\Q742, DeviceObj)
    External (\Q742.XSTA, MethodObj)
    External (\Q743, DeviceObj)
    External (\Q743.XSTA, MethodObj)
    External (\Q744, DeviceObj)
    External (\Q744.XSTA, MethodObj)
    External (\Q745, DeviceObj)
    External (\Q745.XSTA, MethodObj)
    External (\Q746, DeviceObj)
    External (\Q746.XSTA, MethodObj)
    External (\Q747, DeviceObj)
    External (\Q747.XSTA, MethodObj)
    External (\Q748, DeviceObj)
    External (\Q748.XSTA, MethodObj)
    External (\Q749, DeviceObj)
    External (\Q749.XSTA, MethodObj)
    External (\Q750, DeviceObj)
    External (\Q750.XSTA, MethodObj)
    External (\Q751, DeviceObj)
    External (\Q751.XSTA, MethodObj)
    External (\Q752, DeviceObj)
    External (\Q752.XSTA, MethodObj)
    External (\Q753, DeviceObj)
    External (\Q753.XSTA, MethodObj)
    External (\Q754, DeviceObj)
    External (\Q754.XSTA, MethodObj)
    External (\Q755, DeviceObj)
    External (\Q755.XSTA, MethodObj)
    External (\Q756, DeviceObj)
    External (\Q756.XSTA, MethodObj)
    External (\Q757, DeviceObj)
    External (\Q757.XSTA, MethodObj)
    External (\Q758, DeviceObj)
    External (\Q758.XSTA, MethodObj)
    External (\Q759, DeviceObj)
    External (\Q759.XSTA, MethodObj)
    External (\Q760, DeviceObj)
    External (\Q760.XSTA, MethodObj)
    External (\Q761, DeviceObj)
    External (\Q761.XSTA, MethodObj)
    External (\Q762, DeviceObj)
    External (\Q762.XSTA, MethodObj)
    External (\Q763, DeviceObj)
    External (\Q763.XSTA, MethodObj)
    External (\Q764, DeviceObj)
    External (\Q764.XSTA, MethodObj)
    External (\Q765, DeviceObj)
    External (\Q765.XSTA, MethodObj)
    External (\Q766, DeviceObj)
    External (\Q766.XSTA, MethodObj)
    External (\Q767, DeviceObj)
    External (\Q767.XSTA, MethodObj)
    External (\Q768, DeviceObj)
    External (\Q768.XSTA, MethodObj)
    External (\Q769, DeviceObj)
    External (\Q769.XSTA, MethodObj)
    External (\Q770, DeviceObj)
    External (\Q770.XSTA, MethodObj)
    External (\Q771, DeviceObj)
    External (\Q771.XSTA, MethodObj)
    External (\Q772, DeviceObj)
    External (\Q772.XSTA, MethodObj)
    External (\Q773, DeviceObj)
    External (\Q773.XSTA, MethodObj)
    External (\Q774, DeviceObj)
    External (\Q774.XSTA, MethodObj)
    External (\Q775, DeviceObj)
    External (\Q775.XSTA, MethodObj)
    External (\Q776, DeviceObj)
    External (\Q776.XSTA, MethodObj)
    External (\Q777, DeviceObj)
    External (\Q777.XSTA, MethodObj)
    External (\Q778, DeviceObj)
    External (\Q778.XSTA, MethodObj)
    External (\Q779, DeviceObj)
    External (\Q779.XSTA, MethodObj)
    External (\Q780, DeviceObj)
    External (\Q780.XSTA, MethodObj)
    External (\Q781, DeviceObj)
    External (\Q781.XSTA, MethodObj)
    External (\Q782, DeviceObj)
    External (\Q782.XSTA, MethodObj)
    External (\Q783, DeviceObj)
    External (\Q783.XSTA, MethodObj)
    External (\Q784, DeviceObj)
    External (\Q784.XSTA, MethodObj)
    External (\Q785, DeviceObj)
    External (\Q785.XSTA, MethodObj)
    External (\Q786, DeviceObj)
    External (\Q786.XSTA, MethodObj)
    External (\Q787, DeviceObj)
    External (\Q787.XSTA, MethodObj)
    External (\Q788, DeviceObj)
    External (\Q788.XSTA, MethodObj)
    External (\Q789, DeviceObj)
    External (\Q789.XSTA, MethodObj)
    External (\Q790, DeviceObj)
    External (\Q790.XSTA, MethodObj)
    External (\Q791, DeviceObj)
    External (\Q791.XSTA, MethodObj)
    External (\Q792, DeviceObj)
    External (\Q792.XSTA, MethodObj)
    External (\Q793, DeviceObj)
    External (\Q793.XSTA, MethodObj)
    External (\Q794, DeviceObj)
    External (\Q794.XSTA, MethodObj)
    External (\Q795, DeviceObj)
    External (\Q795.XSTA, MethodObj)
    External (\Q796, DeviceObj)
    External (\Q796.XSTA, MethodObj)
    External (\Q797, DeviceObj)
    External (\Q797.XSTA, MethodObj)
    External (\Q798, DeviceObj)
    External (\Q798.XSTA, MethodObj)
    External (\Q799, DeviceObj)
    External (\Q799.XSTA, MethodObj)
    External (\Q800, DeviceObj)
    External (\Q800.XSTA, MethodObj)
    External (\Q801, DeviceObj)
    External (\Q801.XSTA, MethodObj)
    External (\Q802, DeviceObj)
    External (\Q802.XSTA, MethodObj)
    External (\Q803, DeviceObj)
    External (\Q803.XSTA, MethodObj)
    External (\Q804, DeviceObj)
    External (\Q804.XSTA, MethodObj)
    External (\Q805, DeviceObj)
    External (\Q805.XSTA, MethodObj)
    External (\Q806, DeviceObj)
    External (\Q806.XSTA, MethodObj)
    External (\Q807, DeviceObj)
    External (\Q807.XSTA, MethodObj)
    External (\Q808, DeviceObj)
    External (\Q808.XSTA, MethodObj)
    External (\Q809, DeviceObj)
    External (\Q809.XSTA, MethodObj)
    External (\Q810, DeviceObj)
    External (\Q810.XSTA, MethodObj)
    External (\Q811, DeviceObj)
    External (\Q811.XSTA, MethodObj)
    External (\Q812, DeviceObj)
    External (\Q812.XSTA, MethodObj)
    External (\Q813, DeviceObj)
    External (\Q813.XSTA, MethodObj)
    External (\Q814, DeviceObj)
    External (\Q814.XSTA, MethodObj)
    External (\Q815, DeviceObj)
    External (\Q815.XSTA, MethodObj)
    External (\Q816, DeviceObj)
    External (\Q816.XSTA, MethodObj)
    External (\Q817, DeviceObj)
    External (\Q817.XSTA, MethodObj)
    External (\Q818, DeviceObj)
    External (\Q818.XSTA, MethodObj)
    External (\Q819, DeviceObj)
    External (\Q819.XSTA, MethodObj)
    External (\Q820, DeviceObj)
    External (\Q820.XSTA, MethodObj)
    External (\Q821, DeviceObj)
    External (\Q821.XSTA, MethodObj)
    External (\Q822, DeviceObj)
    External (\Q822.XSTA, MethodObj)
    External (\Q823, DeviceObj)
    External (\Q823.XSTA, MethodObj)
    External (\Q824, DeviceObj)
    External (\Q824.XSTA, MethodObj)
    External (\Q825, DeviceObj)
    External (\Q825.XSTA, MethodObj)
    External (\Q826, DeviceObj)
    External (\Q826.XSTA, MethodObj)
    External (\Q827, DeviceObj)
    External (\Q827.XSTA, MethodObj)
    External (\Q828, DeviceObj)
    External (\Q828.XSTA, MethodObj)
    External (\Q829, DeviceObj)
    External (\Q829.XSTA, MethodObj)
    External (\Q830, DeviceObj)
    External (\Q830.XSTA, MethodObj)
    External (\Q831, DeviceObj)
    External (\Q831.XSTA, MethodObj)
    External (\Q832, DeviceObj)
    External (\Q832.XSTA, MethodObj)
    External (\Q833, DeviceObj)
    External (\Q833.XSTA, MethodObj)
    External (\Q834, DeviceObj)
    External (\Q834.XSTA, MethodObj)
    External (\Q835, DeviceObj)
    External (\Q835.XSTA, MethodObj)
    External (\Q836, DeviceObj)
    External (\Q836.XSTA, MethodObj)
    External (\Q837, DeviceObj)
    External (\Q837.XSTA, MethodObj)
    External (\Q838, DeviceObj)
    External (\Q838.XSTA, MethodObj)
    External (\Q839, DeviceObj)
    External (\Q839.XSTA, MethodObj)
    External (\Q840, DeviceObj)
    External (\Q840.XSTA, MethodObj)
    External (\Q841, DeviceObj)
    External (\Q841.XSTA, MethodObj)
    External (\Q842, DeviceObj)
    External (\Q842.XSTA, MethodObj)
    External (\Q843, DeviceObj)
    External (\Q843.XSTA, MethodObj)
    External (\Q844, DeviceObj)
    External (\Q844.XSTA, MethodObj)
    External (\Q845, DeviceObj)
    External (\Q845.XSTA, MethodObj)
    External (\Q846, DeviceObj)
    External (\Q846.XSTA, MethodObj)
    External (\Q847, DeviceObj)
    External (\Q847.XSTA, MethodObj)
    External (\Q848, DeviceObj)
    External (\Q848.XSTA, MethodObj)
    External (\Q849, DeviceObj)
    External (\Q849.XSTA, MethodObj)
    External (\Q850, DeviceObj)
    External (\Q850.XSTA, MethodObj)
    External (\Q851, DeviceObj)
    External (\Q851.XSTA, MethodObj)
    External (\Q852, DeviceObj)
    External (\Q852.XSTA, MethodObj)
    External (\Q853, DeviceObj)
    External (\Q853.XSTA, MethodObj)
    External (\Q854, DeviceObj)
    External (\Q854.XSTA, MethodObj)
    External (\Q855, DeviceObj)
    External (\Q855.XSTA, MethodObj)
    External (\Q856, DeviceObj)
    External (\Q856.XSTA, MethodObj)
    External (\Q857, DeviceObj)
    External (\Q857.XSTA, MethodObj)
    External (\Q858, DeviceObj)
    External (\Q858.XSTA, MethodObj)
    External (\Q859, DeviceObj)
    External (\Q859.XSTA, MethodObj)
    External (\Q860, DeviceObj)
    External (\Q860.XSTA, MethodObj)
    External (\Q861, DeviceObj)
    External (\Q861.XSTA, MethodObj)
    External (\Q862, DeviceObj)
    External (\Q862.XSTA, MethodObj)
    External (\Q863, DeviceObj)
    External (\Q863.XSTA, MethodObj)
    External (\Q864, DeviceObj)
    External (\Q864.XSTA, MethodObj)
    External (\Q865, DeviceObj)
    External (\Q865.XSTA, MethodObj)
    External (\Q866, DeviceObj)
    External (\Q866.XSTA, MethodObj)
    External (\Q867, DeviceObj)
    External (\Q867.XSTA, MethodObj)
    External (\Q868, DeviceObj)
    External (\Q868.XSTA, MethodObj)
    External (\Q869, DeviceObj)
    External (\Q869.XSTA, MethodObj)
    External (\Q870, DeviceObj)
    External (\Q870.XSTA, MethodObj)
    External (\Q871, DeviceObj)
    External (\Q871.XSTA, MethodObj)
    External (\Q872, DeviceObj)
    External (\Q872.XSTA, MethodObj)
    External (\Q873, DeviceObj)
    External (\Q873.XSTA, MethodObj)
    External (\Q874, DeviceObj)
    External (\Q874.XSTA, MethodObj)
    External (\Q875, DeviceObj)
    External (\Q875.XSTA, MethodObj)
    External (\Q876, DeviceObj)
    External (\Q876.XSTA, MethodObj)
    External (\Q877, DeviceObj)
    External (\Q877.XSTA, MethodObj)
    External (\Q878, DeviceObj)
    External (\Q878.XSTA, MethodObj)
    External (\Q879, DeviceObj)
    External (\Q879.XSTA, MethodObj)
    External (\Q880, DeviceObj)
    External (\Q880.XSTA, MethodObj)
    External (\Q881, DeviceObj)
    External (\Q881.XSTA, MethodObj)
    External (\Q882, DeviceObj)
    External (\Q882.XSTA, MethodObj)
    External (\Q883, DeviceObj)
    External (\Q883.XSTA, MethodObj)
    External (\Q884, DeviceObj)
    External (\Q884.XSTA, MethodObj)
    External (\Q885, DeviceObj)
    External (\Q885.XSTA, MethodObj)
    External (\Q886, DeviceObj)
    External (\Q886.XSTA, MethodObj)
    External (\Q887, DeviceObj)
    External (\Q887.XSTA, MethodObj)
    External (\Q888, DeviceObj)
    External (\Q888.XSTA, MethodObj)
    External (\Q889, DeviceObj)
    External (\Q889.XSTA, MethodObj)
    External (\Q890, DeviceObj)
    External (\Q890.XSTA, MethodObj)
    External (\Q891, DeviceObj)
    External (\Q891.XSTA, MethodObj)
    External (\Q892, DeviceObj)
    External (\Q892.XSTA, MethodObj)
    External (\Q893, DeviceObj)
    External (\Q893.XSTA, MethodObj)
    External (\Q894, DeviceObj)
    External (\Q894.XSTA, MethodObj)
    External (\Q895, DeviceObj)
    External (\Q895.XSTA, MethodObj)
    External (\Q896, DeviceObj)
    External (\Q896.XSTA, MethodObj)
    External (\Q897, DeviceObj)
    External (\Q897.XSTA, MethodObj)
    External (\Q898, DeviceObj)
    External (\Q898.XSTA, MethodObj)
    External (\Q899, DeviceObj)
    External (\Q899.XSTA, MethodObj)
    External (\Q900, DeviceObj)
    External (\Q900.XSTA, MethodObj)
    External (\Q901, DeviceObj)
    External (\Q901.XSTA, MethodObj)
    External (\Q902, DeviceObj)
    External (\Q902.XSTA, MethodObj)
    External (\Q903, DeviceObj)
    External (\Q903.XSTA, MethodObj)
    External (\Q904, DeviceObj)
    External (\Q904.XSTA, MethodObj)
    External (\Q905, DeviceObj)
    External (\Q905.XSTA, MethodObj)
    External (\Q906, DeviceObj)
    External (\Q906.XSTA, MethodObj)
    External (\Q907, DeviceObj)
    External (\Q907.XSTA, MethodObj)
    External (\Q908, DeviceObj)
    External (\Q908.XSTA, MethodObj)
    External (\Q909, DeviceObj)
    External (\Q909.XSTA, MethodObj)
    External (\Q910, DeviceObj)
    External (\Q910.XSTA, MethodObj)
    External (\Q911, DeviceObj)
    External (\Q911.XSTA, MethodObj)
    External (\Q912, DeviceObj)
    External (\Q912.XSTA, MethodObj)
    External (\Q913, DeviceObj)
    External (\Q913.XSTA, MethodObj)
    External (\Q914, DeviceObj)
    External (\Q914.XSTA, MethodObj)
    External (\Q915, DeviceObj)
    External (\Q915.XSTA, MethodObj)
    External (\Q916, DeviceObj)
    External (\Q916.XSTA, MethodObj)
    External (\Q917, DeviceObj)
    External (\Q917.XSTA, MethodObj)
    External (\Q918, DeviceObj)
    External (\Q918.XSTA, MethodObj)
    External (\Q919, DeviceObj)
    External (\Q919.XSTA, MethodObj)
    External (\Q920, DeviceObj)
    External (\Q920.XSTA, MethodObj)
    External (\Q921, DeviceObj)
    External (\Q921.XSTA, MethodObj)
    External (\Q922, DeviceObj)
    External (\Q922.XSTA, MethodObj)
    External (\Q923, DeviceObj)
    External (\Q923.XSTA, MethodObj)
    External (\Q924, DeviceObj)
    External (\Q924.XSTA, MethodObj)
    External (\Q925, DeviceObj)
    External (\Q925.XSTA, MethodObj)
    External (\Q926, DeviceObj)
    External (\Q926.XSTA, MethodObj)
    External (\Q927, DeviceObj)
    External (\Q927.XSTA, MethodObj)
    External (\Q928, DeviceObj)
    External (\Q928.XSTA, MethodObj)
    External (\Q929, DeviceObj)
    External (\Q929.XSTA, MethodObj)
    External (\Q930, DeviceObj)
    External (\Q930.XSTA, MethodObj)
    External (\Q931, DeviceObj)
    External (\Q931.XSTA, MethodObj)
    External (\Q932, DeviceObj)
    External (\Q932.XSTA, MethodObj)
    External (\Q933, DeviceObj)
    External (\Q933.XSTA, MethodObj)
    External (\Q934, DeviceObj)
    External (\Q934.XSTA, MethodObj)
    External (\Q935, DeviceObj)
    External (\Q935.XSTA, MethodObj)
    External (\Q936, DeviceObj)
    External (\Q936.XSTA, MethodObj)
    External (\Q937, DeviceObj)
    External (\Q937.XSTA, MethodObj)
    External (\Q938, DeviceObj)
    External (\Q938.XSTA, MethodObj)
    External (\Q939, DeviceObj)
    External (\Q939.XSTA, MethodObj)
    External (\Q940, DeviceObj)
    External (\Q940.XSTA, MethodObj)
    External (\Q941, DeviceObj)
    External (\Q941.XSTA, MethodObj)
    External (\Q942, DeviceObj)
    External (\Q942.XSTA, MethodObj)
    External (\Q943, DeviceObj)
    External (\Q943.XSTA, MethodObj)
    External (\Q944, DeviceObj)
    External (\Q944.XSTA, MethodObj)
    External (\Q945, DeviceObj)
    External (\Q945.XSTA, MethodObj)
    External (\Q946, DeviceObj)
    External (\Q946.XSTA, MethodObj)
    External (\Q947, DeviceObj)
    External (\Q947.XSTA, MethodObj)
    External (\Q948, DeviceObj)
    External (\Q948.XSTA, MethodObj)
    External (\Q949, DeviceObj)
    External (\Q949.XSTA, MethodObj)
    External (\Q950, DeviceObj)
    External (\Q950.XSTA, MethodObj)
    External (\Q951, DeviceObj)
    External (\Q951.XSTA, MethodObj)
    External (\Q952, DeviceObj)
    External (\Q952.XSTA, MethodObj)
    External (\Q953, DeviceObj)
    External (\Q953.XSTA, MethodObj)
    External (\Q954, DeviceObj)
    External (\Q954.XSTA, MethodObj)
    External (\Q955, DeviceObj)
    External (\Q955.XSTA, MethodObj)
    External (\Q956, DeviceObj)
    External (\Q956.XSTA, MethodObj)
    External (\Q957, DeviceObj)
    External (\Q957.XSTA, MethodObj)
    External (\Q958, DeviceObj)
    External (\Q958.XSTA, MethodObj)
    External (\Q959, DeviceObj)
    External (\Q959.XSTA, MethodObj)
    External (\Q960, DeviceObj)
    External (\Q960.XSTA, MethodObj)
    External (\Q961, DeviceObj)
    External (\Q961.XSTA, MethodObj)
    External (\Q962, DeviceObj)
    External (\Q962.XSTA, MethodObj)
    External (\Q963, DeviceObj)
    External (\Q963.XSTA, MethodObj)
    External (\Q964, DeviceObj)
    External (\Q964.XSTA, MethodObj)
    External (\Q965, DeviceObj)
    External (\Q965.XSTA, MethodObj)
    External (\Q966, DeviceObj)
    External (\Q966.XSTA, MethodObj)
    External (\Q967, DeviceObj)
    External (\Q967.XSTA, MethodObj)
    External (\Q968, DeviceObj)
    External (\Q968.XSTA, MethodObj)
    External (\Q969, DeviceObj)
    External (\Q969.XSTA, MethodObj)
    External (\Q970, DeviceObj)
    External (\Q970.XSTA, MethodObj)
    External (\Q971, DeviceObj)
    External (\Q971.XSTA, MethodObj)
    External (\Q972, DeviceObj)
    External (\Q972.XSTA, MethodObj)
    External (\Q973, DeviceObj)
    External (\Q973.XSTA, MethodObj)
    External (\Q974, DeviceObj)
    External (\Q974.XSTA, MethodObj)
    External (\Q975, DeviceObj)
    External (\Q975.XSTA, MethodObj)
    External (\Q976, DeviceObj)
    External (\Q976.XSTA, MethodObj)
    External (\Q977, DeviceObj)
    External (\Q977.XSTA, MethodObj)
    External (\Q978, DeviceObj)
    External (\Q978.XSTA, MethodObj)
    External (\Q979, DeviceObj)
    External (\Q979.XSTA, MethodObj)
    External (\Q980, DeviceObj)
    External (\Q980.XSTA, MethodObj)
    External (\Q981, DeviceObj)
    External (\Q981.XSTA, MethodObj)
    External (\Q982, DeviceObj)
    External (\Q982.XSTA, MethodObj)
    External (\Q983, DeviceObj)
    External (\Q983.XSTA, MethodObj)
    External (\Q984, DeviceObj)
    External (\Q984.XSTA, MethodObj)
    External (\Q985, DeviceObj)
    External (\Q985.XSTA, MethodObj)
    External (\Q986, DeviceObj)
    External (\Q986.XSTA, MethodObj)
    External (\Q987, DeviceObj)
    External (\Q987.XSTA, MethodObj)
    External (\Q988, DeviceObj)
    External (\Q988.XSTA, MethodObj)
    External (\Q989, DeviceObj)
    External (\Q989.XSTA, MethodObj)
    External (\Q990, DeviceObj)
    External (\Q990.XSTA, MethodObj)
    External (\Q991, DeviceObj)
    External (\Q991.XSTA, MethodObj)
    External (\Q992, DeviceObj)
    External (\Q992.XSTA, MethodObj)
    External (\Q993, DeviceObj)
    External (\Q993.XSTA, MethodObj)
    External (\Q994, DeviceObj)
    External (\Q994.XSTA, MethodObj)
    External (\Q995, DeviceObj)
    External (\Q995.XSTA, MethodObj)
    External (\Q996, DeviceObj)
    External (\Q996.XSTA, MethodObj)
    External (\Q997, DeviceObj)
    External (\Q997.XSTA, MethodObj)
    External (\Q998, DeviceObj)
    External (\Q998.XSTA, MethodObj)
    External (\Q999, DeviceObj)
    External (\Q999.XSTA, MethodObj)
    External (\R000, DeviceObj)
    External (\R000.XSTA, MethodObj)
    External (\R001, DeviceObj)
    External (\R001.XSTA, MethodObj)
    External (\R002, DeviceObj)
    External (\R002.XSTA, MethodObj)
    External (\R003, DeviceObj)
    External (\R003.XSTA, MethodObj)
    External (\R004, DeviceObj)
    External (\R004.XSTA, MethodObj)
    External (\R005, DeviceObj)
    External (\R005.XSTA, MethodObj)
    External (\R006, DeviceObj)
    External (\R006.XSTA, MethodObj)
    External (\R007, DeviceObj)
    External (\R007.XSTA, MethodObj)
    External (\R008, DeviceObj)
    External (\R008.XSTA, MethodObj)
    External (\R009, DeviceObj)
    External (\R009.XSTA, MethodObj)
    External (\R010, DeviceObj)
    External (\R010.XSTA, MethodObj)
    External (\R011, DeviceObj)
    External (\R011.XSTA, MethodObj)
    External (\R012, DeviceObj)
    External (\R012.XSTA, MethodObj)
    External (\R013, DeviceObj)
    External (\R013.XSTA, MethodObj)
    External (\R014, DeviceObj)
    External (\R014.XSTA, MethodObj)
    External (\R015, DeviceObj)
    External (\R015.XSTA, MethodObj)
    External (\R016, DeviceObj)
    External (\R016.XSTA, MethodObj)
    External (\R017, DeviceObj)
    External (\R017.XSTA, MethodObj)
    External (\R018, DeviceObj)
    External (\R018.XSTA, MethodObj)
    External (\R019, DeviceObj)
    External (\R019.XSTA, MethodObj)
    External (\R020, DeviceObj)
    External (\R020.XSTA, MethodObj)
    External (\R021, DeviceObj)
    External (\R021.XSTA, MethodObj)
    External (\R022, DeviceObj)
    External (\R022.XSTA, MethodObj)
    External (\R023, DeviceObj)
    External (\R023.XSTA, MethodObj)
    External (\R024, DeviceObj)
    External (\R024.XSTA, MethodObj)
    External (\R025, DeviceObj)
    External (\R025.XSTA, MethodObj)
    External (\R026, DeviceObj)
    External (\R026.XSTA, MethodObj)
    External (\R027, DeviceObj)
    External (\R027.XSTA, MethodObj)
    External (\R028, DeviceObj)
    External (\R028.XSTA, MethodObj)
    External (\R029, DeviceObj)
    External (\R029.XSTA, MethodObj)
    External (\R030, DeviceObj)
    External (\R030.XSTA, MethodObj)
    External (\R031, DeviceObj)
    External (\R031.XSTA, MethodObj)
    External (\R032, DeviceObj)
    External (\R032.XSTA, MethodObj)
    External (\R033, DeviceObj)
    External (\R033.XSTA, MethodObj)
    External (\R034, DeviceObj)
    External (\R034.XSTA, MethodObj)
    External (\R035, DeviceObj)
    External (\R035.XSTA, MethodObj)
    External (\R036, DeviceObj)
    External (\R036.XSTA, MethodObj)
    External (\R037, DeviceObj)
    External (\R037.XSTA, MethodObj)
    External (\R038, DeviceObj)
    External (\R038.XSTA, MethodObj)
    External (\R039, DeviceObj)
    External (\R039.XSTA, MethodObj)
    External (\R040, DeviceObj)
    External (\R040.XSTA, MethodObj)
    External (\R041, DeviceObj)
    External (\R041.XSTA, MethodObj)
    External (\R042, DeviceObj)
    External (\R042.XSTA, MethodObj)
    External (\R043, DeviceObj)
    External (\R043.XSTA, MethodObj)
    External (\R044, DeviceObj)
    External (\R044.XSTA, MethodObj)
    External (\R045, DeviceObj)
    External (\R045.XSTA, MethodObj)
    External (\R046, DeviceObj)
    External (\R046.XSTA, MethodObj)
    External (\R047, DeviceObj)
    External (\R047.XSTA, MethodObj)
    External (\R048, DeviceObj)
    External (\R048.XSTA, MethodObj)

    //
    // Processor aggregator device is not supported under macOS.
    //
    If (LGreater (PADE, Zero))
    {
        Scope (\_SB.VMOD.PAD1)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (_OSI ("Darwin"))    
                {
                    Return (Zero)
                }

                Return (0x0F)
            }
        }
    }

    //
    // Additional new-style ACPI0007 processor devices are not supported under macOS.
    //
    If (LLessEqual (0x0F1, PCNT))
    {
        Scope (\P241)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F1, PCNT))
                {
                    Return (\P241.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0F2, PCNT))
    {
        Scope (\P242)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F2, PCNT))
                {
                    Return (\P242.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0F3, PCNT))
    {
        Scope (\P243)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F3, PCNT))
                {
                    Return (\P243.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0F4, PCNT))
    {
        Scope (\P244)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F4, PCNT))
                {
                    Return (\P244.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0F5, PCNT))
    {
        Scope (\P245)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F5, PCNT))
                {
                    Return (\P245.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0F6, PCNT))
    {
        Scope (\P246)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F6, PCNT))
                {
                    Return (\P246.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0F7, PCNT))
    {
        Scope (\P247)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F7, PCNT))
                {
                    Return (\P247.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0F8, PCNT))
    {
        Scope (\P248)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F8, PCNT))
                {
                    Return (\P248.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0F9, PCNT))
    {
        Scope (\P249)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0F9, PCNT))
                {
                    Return (\P249.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0FA, PCNT))
    {
        Scope (\P250)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0FA, PCNT))
                {
                    Return (\P250.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0FB, PCNT))
    {
        Scope (\P251)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0FB, PCNT))
                {
                    Return (\P251.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0FC, PCNT))
    {
        Scope (\P252)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0FC, PCNT))
                {
                    Return (\P252.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0FD, PCNT))
    {
        Scope (\P253)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0FD, PCNT))
                {
                    Return (\P253.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0FE, PCNT))
    {
        Scope (\P254)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0FE, PCNT))
                {
                    Return (\P254.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0FF, PCNT))
    {
        Scope (\P255)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0FF, PCNT))
                {
                    Return (\P255.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0100, PCNT))
    {
        Scope (\P256)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0100, PCNT))
                {
                    Return (\P256.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0101, PCNT))
    {
        Scope (\P257)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0101, PCNT))
                {
                    Return (\P257.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0102, PCNT))
    {
        Scope (\P258)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0102, PCNT))
                {
                    Return (\P258.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0103, PCNT))
    {
        Scope (\P259)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0103, PCNT))
                {
                    Return (\P259.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0104, PCNT))
    {
        Scope (\P260)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0104, PCNT))
                {
                    Return (\P260.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0105, PCNT))
    {
        Scope (\P261)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0105, PCNT))
                {
                    Return (\P261.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0106, PCNT))
    {
        Scope (\P262)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0106, PCNT))
                {
                    Return (\P262.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0107, PCNT))
    {
        Scope (\P263)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0107, PCNT))
                {
                    Return (\P263.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0108, PCNT))
    {
        Scope (\P264)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0108, PCNT))
                {
                    Return (\P264.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0109, PCNT))
    {
        Scope (\P265)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0109, PCNT))
                {
                    Return (\P265.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x010A, PCNT))
    {
        Scope (\P266)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x010A, PCNT))
                {
                    Return (\P266.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x010B, PCNT))
    {
        Scope (\P267)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x010B, PCNT))
                {
                    Return (\P267.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x010C, PCNT))
    {
        Scope (\P268)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x010C, PCNT))
                {
                    Return (\P268.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x010D, PCNT))
    {
        Scope (\P269)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x010D, PCNT))
                {
                    Return (\P269.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x010E, PCNT))
    {
        Scope (\P270)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x010E, PCNT))
                {
                    Return (\P270.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x010F, PCNT))
    {
        Scope (\P271)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x010F, PCNT))
                {
                    Return (\P271.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0110, PCNT))
    {
        Scope (\P272)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0110, PCNT))
                {
                    Return (\P272.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0111, PCNT))
    {
        Scope (\P273)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0111, PCNT))
                {
                    Return (\P273.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0112, PCNT))
    {
        Scope (\P274)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0112, PCNT))
                {
                    Return (\P274.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0113, PCNT))
    {
        Scope (\P275)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0113, PCNT))
                {
                    Return (\P275.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0114, PCNT))
    {
        Scope (\P276)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0114, PCNT))
                {
                    Return (\P276.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0115, PCNT))
    {
        Scope (\P277)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0115, PCNT))
                {
                    Return (\P277.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0116, PCNT))
    {
        Scope (\P278)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0116, PCNT))
                {
                    Return (\P278.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0117, PCNT))
    {
        Scope (\P279)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0117, PCNT))
                {
                    Return (\P279.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0118, PCNT))
    {
        Scope (\P280)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0118, PCNT))
                {
                    Return (\P280.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0119, PCNT))
    {
        Scope (\P281)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0119, PCNT))
                {
                    Return (\P281.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x011A, PCNT))
    {
        Scope (\P282)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x011A, PCNT))
                {
                    Return (\P282.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x011B, PCNT))
    {
        Scope (\P283)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x011B, PCNT))
                {
                    Return (\P283.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x011C, PCNT))
    {
        Scope (\P284)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x011C, PCNT))
                {
                    Return (\P284.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x011D, PCNT))
    {
        Scope (\P285)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x011D, PCNT))
                {
                    Return (\P285.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x011E, PCNT))
    {
        Scope (\P286)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x011E, PCNT))
                {
                    Return (\P286.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x011F, PCNT))
    {
        Scope (\P287)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x011F, PCNT))
                {
                    Return (\P287.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0120, PCNT))
    {
        Scope (\P288)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0120, PCNT))
                {
                    Return (\P288.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0121, PCNT))
    {
        Scope (\P289)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0121, PCNT))
                {
                    Return (\P289.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0122, PCNT))
    {
        Scope (\P290)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0122, PCNT))
                {
                    Return (\P290.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0123, PCNT))
    {
        Scope (\P291)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0123, PCNT))
                {
                    Return (\P291.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0124, PCNT))
    {
        Scope (\P292)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0124, PCNT))
                {
                    Return (\P292.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0125, PCNT))
    {
        Scope (\P293)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0125, PCNT))
                {
                    Return (\P293.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0126, PCNT))
    {
        Scope (\P294)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0126, PCNT))
                {
                    Return (\P294.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0127, PCNT))
    {
        Scope (\P295)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0127, PCNT))
                {
                    Return (\P295.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0128, PCNT))
    {
        Scope (\P296)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0128, PCNT))
                {
                    Return (\P296.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0129, PCNT))
    {
        Scope (\P297)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0129, PCNT))
                {
                    Return (\P297.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x012A, PCNT))
    {
        Scope (\P298)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x012A, PCNT))
                {
                    Return (\P298.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x012B, PCNT))
    {
        Scope (\P299)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x012B, PCNT))
                {
                    Return (\P299.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x012C, PCNT))
    {
        Scope (\P300)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x012C, PCNT))
                {
                    Return (\P300.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x012D, PCNT))
    {
        Scope (\P301)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x012D, PCNT))
                {
                    Return (\P301.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x012E, PCNT))
    {
        Scope (\P302)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x012E, PCNT))
                {
                    Return (\P302.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x012F, PCNT))
    {
        Scope (\P303)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x012F, PCNT))
                {
                    Return (\P303.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0130, PCNT))
    {
        Scope (\P304)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0130, PCNT))
                {
                    Return (\P304.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0131, PCNT))
    {
        Scope (\P305)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0131, PCNT))
                {
                    Return (\P305.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0132, PCNT))
    {
        Scope (\P306)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0132, PCNT))
                {
                    Return (\P306.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0133, PCNT))
    {
        Scope (\P307)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0133, PCNT))
                {
                    Return (\P307.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0134, PCNT))
    {
        Scope (\P308)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0134, PCNT))
                {
                    Return (\P308.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0135, PCNT))
    {
        Scope (\P309)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0135, PCNT))
                {
                    Return (\P309.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0136, PCNT))
    {
        Scope (\P310)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0136, PCNT))
                {
                    Return (\P310.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0137, PCNT))
    {
        Scope (\P311)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0137, PCNT))
                {
                    Return (\P311.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0138, PCNT))
    {
        Scope (\P312)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0138, PCNT))
                {
                    Return (\P312.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0139, PCNT))
    {
        Scope (\P313)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0139, PCNT))
                {
                    Return (\P313.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x013A, PCNT))
    {
        Scope (\P314)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x013A, PCNT))
                {
                    Return (\P314.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x013B, PCNT))
    {
        Scope (\P315)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x013B, PCNT))
                {
                    Return (\P315.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x013C, PCNT))
    {
        Scope (\P316)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x013C, PCNT))
                {
                    Return (\P316.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x013D, PCNT))
    {
        Scope (\P317)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x013D, PCNT))
                {
                    Return (\P317.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x013E, PCNT))
    {
        Scope (\P318)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x013E, PCNT))
                {
                    Return (\P318.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x013F, PCNT))
    {
        Scope (\P319)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x013F, PCNT))
                {
                    Return (\P319.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0140, PCNT))
    {
        Scope (\P320)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0140, PCNT))
                {
                    Return (\P320.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0141, PCNT))
    {
        Scope (\P321)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0141, PCNT))
                {
                    Return (\P321.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0142, PCNT))
    {
        Scope (\P322)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0142, PCNT))
                {
                    Return (\P322.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0143, PCNT))
    {
        Scope (\P323)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0143, PCNT))
                {
                    Return (\P323.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0144, PCNT))
    {
        Scope (\P324)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0144, PCNT))
                {
                    Return (\P324.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0145, PCNT))
    {
        Scope (\P325)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0145, PCNT))
                {
                    Return (\P325.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0146, PCNT))
    {
        Scope (\P326)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0146, PCNT))
                {
                    Return (\P326.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0147, PCNT))
    {
        Scope (\P327)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0147, PCNT))
                {
                    Return (\P327.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0148, PCNT))
    {
        Scope (\P328)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0148, PCNT))
                {
                    Return (\P328.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0149, PCNT))
    {
        Scope (\P329)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0149, PCNT))
                {
                    Return (\P329.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x014A, PCNT))
    {
        Scope (\P330)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x014A, PCNT))
                {
                    Return (\P330.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x014B, PCNT))
    {
        Scope (\P331)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x014B, PCNT))
                {
                    Return (\P331.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x014C, PCNT))
    {
        Scope (\P332)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x014C, PCNT))
                {
                    Return (\P332.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x014D, PCNT))
    {
        Scope (\P333)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x014D, PCNT))
                {
                    Return (\P333.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x014E, PCNT))
    {
        Scope (\P334)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x014E, PCNT))
                {
                    Return (\P334.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x014F, PCNT))
    {
        Scope (\P335)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x014F, PCNT))
                {
                    Return (\P335.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0150, PCNT))
    {
        Scope (\P336)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0150, PCNT))
                {
                    Return (\P336.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0151, PCNT))
    {
        Scope (\P337)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0151, PCNT))
                {
                    Return (\P337.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0152, PCNT))
    {
        Scope (\P338)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0152, PCNT))
                {
                    Return (\P338.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0153, PCNT))
    {
        Scope (\P339)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0153, PCNT))
                {
                    Return (\P339.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0154, PCNT))
    {
        Scope (\P340)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0154, PCNT))
                {
                    Return (\P340.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0155, PCNT))
    {
        Scope (\P341)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0155, PCNT))
                {
                    Return (\P341.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0156, PCNT))
    {
        Scope (\P342)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0156, PCNT))
                {
                    Return (\P342.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0157, PCNT))
    {
        Scope (\P343)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0157, PCNT))
                {
                    Return (\P343.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0158, PCNT))
    {
        Scope (\P344)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0158, PCNT))
                {
                    Return (\P344.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0159, PCNT))
    {
        Scope (\P345)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0159, PCNT))
                {
                    Return (\P345.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x015A, PCNT))
    {
        Scope (\P346)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x015A, PCNT))
                {
                    Return (\P346.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x015B, PCNT))
    {
        Scope (\P347)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x015B, PCNT))
                {
                    Return (\P347.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x015C, PCNT))
    {
        Scope (\P348)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x015C, PCNT))
                {
                    Return (\P348.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x015D, PCNT))
    {
        Scope (\P349)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x015D, PCNT))
                {
                    Return (\P349.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x015E, PCNT))
    {
        Scope (\P350)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x015E, PCNT))
                {
                    Return (\P350.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x015F, PCNT))
    {
        Scope (\P351)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x015F, PCNT))
                {
                    Return (\P351.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0160, PCNT))
    {
        Scope (\P352)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0160, PCNT))
                {
                    Return (\P352.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0161, PCNT))
    {
        Scope (\P353)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0161, PCNT))
                {
                    Return (\P353.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0162, PCNT))
    {
        Scope (\P354)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0162, PCNT))
                {
                    Return (\P354.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0163, PCNT))
    {
        Scope (\P355)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0163, PCNT))
                {
                    Return (\P355.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0164, PCNT))
    {
        Scope (\P356)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0164, PCNT))
                {
                    Return (\P356.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0165, PCNT))
    {
        Scope (\P357)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0165, PCNT))
                {
                    Return (\P357.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0166, PCNT))
    {
        Scope (\P358)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0166, PCNT))
                {
                    Return (\P358.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0167, PCNT))
    {
        Scope (\P359)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0167, PCNT))
                {
                    Return (\P359.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0168, PCNT))
    {
        Scope (\P360)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0168, PCNT))
                {
                    Return (\P360.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0169, PCNT))
    {
        Scope (\P361)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0169, PCNT))
                {
                    Return (\P361.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x016A, PCNT))
    {
        Scope (\P362)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x016A, PCNT))
                {
                    Return (\P362.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x016B, PCNT))
    {
        Scope (\P363)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x016B, PCNT))
                {
                    Return (\P363.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x016C, PCNT))
    {
        Scope (\P364)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x016C, PCNT))
                {
                    Return (\P364.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x016D, PCNT))
    {
        Scope (\P365)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x016D, PCNT))
                {
                    Return (\P365.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x016E, PCNT))
    {
        Scope (\P366)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x016E, PCNT))
                {
                    Return (\P366.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x016F, PCNT))
    {
        Scope (\P367)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x016F, PCNT))
                {
                    Return (\P367.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0170, PCNT))
    {
        Scope (\P368)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0170, PCNT))
                {
                    Return (\P368.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0171, PCNT))
    {
        Scope (\P369)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0171, PCNT))
                {
                    Return (\P369.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0172, PCNT))
    {
        Scope (\P370)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0172, PCNT))
                {
                    Return (\P370.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0173, PCNT))
    {
        Scope (\P371)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0173, PCNT))
                {
                    Return (\P371.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0174, PCNT))
    {
        Scope (\P372)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0174, PCNT))
                {
                    Return (\P372.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0175, PCNT))
    {
        Scope (\P373)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0175, PCNT))
                {
                    Return (\P373.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0176, PCNT))
    {
        Scope (\P374)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0176, PCNT))
                {
                    Return (\P374.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0177, PCNT))
    {
        Scope (\P375)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0177, PCNT))
                {
                    Return (\P375.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0178, PCNT))
    {
        Scope (\P376)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0178, PCNT))
                {
                    Return (\P376.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0179, PCNT))
    {
        Scope (\P377)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0179, PCNT))
                {
                    Return (\P377.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x017A, PCNT))
    {
        Scope (\P378)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x017A, PCNT))
                {
                    Return (\P378.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x017B, PCNT))
    {
        Scope (\P379)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x017B, PCNT))
                {
                    Return (\P379.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x017C, PCNT))
    {
        Scope (\P380)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x017C, PCNT))
                {
                    Return (\P380.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x017D, PCNT))
    {
        Scope (\P381)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x017D, PCNT))
                {
                    Return (\P381.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x017E, PCNT))
    {
        Scope (\P382)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x017E, PCNT))
                {
                    Return (\P382.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x017F, PCNT))
    {
        Scope (\P383)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x017F, PCNT))
                {
                    Return (\P383.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0180, PCNT))
    {
        Scope (\P384)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0180, PCNT))
                {
                    Return (\P384.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0181, PCNT))
    {
        Scope (\P385)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0181, PCNT))
                {
                    Return (\P385.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0182, PCNT))
    {
        Scope (\P386)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0182, PCNT))
                {
                    Return (\P386.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0183, PCNT))
    {
        Scope (\P387)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0183, PCNT))
                {
                    Return (\P387.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0184, PCNT))
    {
        Scope (\P388)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0184, PCNT))
                {
                    Return (\P388.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0185, PCNT))
    {
        Scope (\P389)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0185, PCNT))
                {
                    Return (\P389.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0186, PCNT))
    {
        Scope (\P390)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0186, PCNT))
                {
                    Return (\P390.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0187, PCNT))
    {
        Scope (\P391)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0187, PCNT))
                {
                    Return (\P391.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0188, PCNT))
    {
        Scope (\P392)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0188, PCNT))
                {
                    Return (\P392.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0189, PCNT))
    {
        Scope (\P393)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0189, PCNT))
                {
                    Return (\P393.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x018A, PCNT))
    {
        Scope (\P394)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x018A, PCNT))
                {
                    Return (\P394.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x018B, PCNT))
    {
        Scope (\P395)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x018B, PCNT))
                {
                    Return (\P395.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x018C, PCNT))
    {
        Scope (\P396)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x018C, PCNT))
                {
                    Return (\P396.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x018D, PCNT))
    {
        Scope (\P397)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x018D, PCNT))
                {
                    Return (\P397.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x018E, PCNT))
    {
        Scope (\P398)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x018E, PCNT))
                {
                    Return (\P398.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x018F, PCNT))
    {
        Scope (\P399)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x018F, PCNT))
                {
                    Return (\P399.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0190, PCNT))
    {
        Scope (\P400)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0190, PCNT))
                {
                    Return (\P400.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0191, PCNT))
    {
        Scope (\P401)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0191, PCNT))
                {
                    Return (\P401.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0192, PCNT))
    {
        Scope (\P402)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0192, PCNT))
                {
                    Return (\P402.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0193, PCNT))
    {
        Scope (\P403)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0193, PCNT))
                {
                    Return (\P403.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0194, PCNT))
    {
        Scope (\P404)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0194, PCNT))
                {
                    Return (\P404.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0195, PCNT))
    {
        Scope (\P405)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0195, PCNT))
                {
                    Return (\P405.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0196, PCNT))
    {
        Scope (\P406)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0196, PCNT))
                {
                    Return (\P406.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0197, PCNT))
    {
        Scope (\P407)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0197, PCNT))
                {
                    Return (\P407.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0198, PCNT))
    {
        Scope (\P408)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0198, PCNT))
                {
                    Return (\P408.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0199, PCNT))
    {
        Scope (\P409)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0199, PCNT))
                {
                    Return (\P409.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x019A, PCNT))
    {
        Scope (\P410)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x019A, PCNT))
                {
                    Return (\P410.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x019B, PCNT))
    {
        Scope (\P411)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x019B, PCNT))
                {
                    Return (\P411.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x019C, PCNT))
    {
        Scope (\P412)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x019C, PCNT))
                {
                    Return (\P412.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x019D, PCNT))
    {
        Scope (\P413)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x019D, PCNT))
                {
                    Return (\P413.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x019E, PCNT))
    {
        Scope (\P414)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x019E, PCNT))
                {
                    Return (\P414.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x019F, PCNT))
    {
        Scope (\P415)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x019F, PCNT))
                {
                    Return (\P415.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A0, PCNT))
    {
        Scope (\P416)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A0, PCNT))
                {
                    Return (\P416.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A1, PCNT))
    {
        Scope (\P417)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A1, PCNT))
                {
                    Return (\P417.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A2, PCNT))
    {
        Scope (\P418)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A2, PCNT))
                {
                    Return (\P418.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A3, PCNT))
    {
        Scope (\P419)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A3, PCNT))
                {
                    Return (\P419.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A4, PCNT))
    {
        Scope (\P420)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A4, PCNT))
                {
                    Return (\P420.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A5, PCNT))
    {
        Scope (\P421)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A5, PCNT))
                {
                    Return (\P421.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A6, PCNT))
    {
        Scope (\P422)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A6, PCNT))
                {
                    Return (\P422.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A7, PCNT))
    {
        Scope (\P423)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A7, PCNT))
                {
                    Return (\P423.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A8, PCNT))
    {
        Scope (\P424)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A8, PCNT))
                {
                    Return (\P424.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01A9, PCNT))
    {
        Scope (\P425)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01A9, PCNT))
                {
                    Return (\P425.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01AA, PCNT))
    {
        Scope (\P426)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01AA, PCNT))
                {
                    Return (\P426.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01AB, PCNT))
    {
        Scope (\P427)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01AB, PCNT))
                {
                    Return (\P427.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01AC, PCNT))
    {
        Scope (\P428)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01AC, PCNT))
                {
                    Return (\P428.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01AD, PCNT))
    {
        Scope (\P429)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01AD, PCNT))
                {
                    Return (\P429.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01AE, PCNT))
    {
        Scope (\P430)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01AE, PCNT))
                {
                    Return (\P430.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01AF, PCNT))
    {
        Scope (\P431)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01AF, PCNT))
                {
                    Return (\P431.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B0, PCNT))
    {
        Scope (\P432)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B0, PCNT))
                {
                    Return (\P432.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B1, PCNT))
    {
        Scope (\P433)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B1, PCNT))
                {
                    Return (\P433.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B2, PCNT))
    {
        Scope (\P434)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B2, PCNT))
                {
                    Return (\P434.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B3, PCNT))
    {
        Scope (\P435)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B3, PCNT))
                {
                    Return (\P435.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B4, PCNT))
    {
        Scope (\P436)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B4, PCNT))
                {
                    Return (\P436.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B5, PCNT))
    {
        Scope (\P437)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B5, PCNT))
                {
                    Return (\P437.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B6, PCNT))
    {
        Scope (\P438)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B6, PCNT))
                {
                    Return (\P438.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B7, PCNT))
    {
        Scope (\P439)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B7, PCNT))
                {
                    Return (\P439.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B8, PCNT))
    {
        Scope (\P440)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B8, PCNT))
                {
                    Return (\P440.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01B9, PCNT))
    {
        Scope (\P441)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01B9, PCNT))
                {
                    Return (\P441.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01BA, PCNT))
    {
        Scope (\P442)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01BA, PCNT))
                {
                    Return (\P442.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01BB, PCNT))
    {
        Scope (\P443)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01BB, PCNT))
                {
                    Return (\P443.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01BC, PCNT))
    {
        Scope (\P444)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01BC, PCNT))
                {
                    Return (\P444.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01BD, PCNT))
    {
        Scope (\P445)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01BD, PCNT))
                {
                    Return (\P445.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01BE, PCNT))
    {
        Scope (\P446)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01BE, PCNT))
                {
                    Return (\P446.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01BF, PCNT))
    {
        Scope (\P447)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01BF, PCNT))
                {
                    Return (\P447.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C0, PCNT))
    {
        Scope (\P448)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C0, PCNT))
                {
                    Return (\P448.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C1, PCNT))
    {
        Scope (\P449)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C1, PCNT))
                {
                    Return (\P449.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C2, PCNT))
    {
        Scope (\P450)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C2, PCNT))
                {
                    Return (\P450.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C3, PCNT))
    {
        Scope (\P451)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C3, PCNT))
                {
                    Return (\P451.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C4, PCNT))
    {
        Scope (\P452)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C4, PCNT))
                {
                    Return (\P452.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C5, PCNT))
    {
        Scope (\P453)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C5, PCNT))
                {
                    Return (\P453.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C6, PCNT))
    {
        Scope (\P454)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C6, PCNT))
                {
                    Return (\P454.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C7, PCNT))
    {
        Scope (\P455)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C7, PCNT))
                {
                    Return (\P455.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C8, PCNT))
    {
        Scope (\P456)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C8, PCNT))
                {
                    Return (\P456.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01C9, PCNT))
    {
        Scope (\P457)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01C9, PCNT))
                {
                    Return (\P457.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01CA, PCNT))
    {
        Scope (\P458)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01CA, PCNT))
                {
                    Return (\P458.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01CB, PCNT))
    {
        Scope (\P459)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01CB, PCNT))
                {
                    Return (\P459.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01CC, PCNT))
    {
        Scope (\P460)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01CC, PCNT))
                {
                    Return (\P460.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01CD, PCNT))
    {
        Scope (\P461)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01CD, PCNT))
                {
                    Return (\P461.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01CE, PCNT))
    {
        Scope (\P462)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01CE, PCNT))
                {
                    Return (\P462.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01CF, PCNT))
    {
        Scope (\P463)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01CF, PCNT))
                {
                    Return (\P463.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D0, PCNT))
    {
        Scope (\P464)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D0, PCNT))
                {
                    Return (\P464.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D1, PCNT))
    {
        Scope (\P465)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D1, PCNT))
                {
                    Return (\P465.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D2, PCNT))
    {
        Scope (\P466)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D2, PCNT))
                {
                    Return (\P466.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D3, PCNT))
    {
        Scope (\P467)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D3, PCNT))
                {
                    Return (\P467.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D4, PCNT))
    {
        Scope (\P468)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D4, PCNT))
                {
                    Return (\P468.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D5, PCNT))
    {
        Scope (\P469)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D5, PCNT))
                {
                    Return (\P469.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D6, PCNT))
    {
        Scope (\P470)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D6, PCNT))
                {
                    Return (\P470.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D7, PCNT))
    {
        Scope (\P471)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D7, PCNT))
                {
                    Return (\P471.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D8, PCNT))
    {
        Scope (\P472)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D8, PCNT))
                {
                    Return (\P472.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01D9, PCNT))
    {
        Scope (\P473)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01D9, PCNT))
                {
                    Return (\P473.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01DA, PCNT))
    {
        Scope (\P474)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01DA, PCNT))
                {
                    Return (\P474.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01DB, PCNT))
    {
        Scope (\P475)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01DB, PCNT))
                {
                    Return (\P475.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01DC, PCNT))
    {
        Scope (\P476)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01DC, PCNT))
                {
                    Return (\P476.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01DD, PCNT))
    {
        Scope (\P477)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01DD, PCNT))
                {
                    Return (\P477.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01DE, PCNT))
    {
        Scope (\P478)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01DE, PCNT))
                {
                    Return (\P478.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01DF, PCNT))
    {
        Scope (\P479)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01DF, PCNT))
                {
                    Return (\P479.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E0, PCNT))
    {
        Scope (\P480)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E0, PCNT))
                {
                    Return (\P480.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E1, PCNT))
    {
        Scope (\P481)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E1, PCNT))
                {
                    Return (\P481.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E2, PCNT))
    {
        Scope (\P482)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E2, PCNT))
                {
                    Return (\P482.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E3, PCNT))
    {
        Scope (\P483)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E3, PCNT))
                {
                    Return (\P483.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E4, PCNT))
    {
        Scope (\P484)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E4, PCNT))
                {
                    Return (\P484.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E5, PCNT))
    {
        Scope (\P485)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E5, PCNT))
                {
                    Return (\P485.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E6, PCNT))
    {
        Scope (\P486)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E6, PCNT))
                {
                    Return (\P486.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E7, PCNT))
    {
        Scope (\P487)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E7, PCNT))
                {
                    Return (\P487.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E8, PCNT))
    {
        Scope (\P488)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E8, PCNT))
                {
                    Return (\P488.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01E9, PCNT))
    {
        Scope (\P489)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01E9, PCNT))
                {
                    Return (\P489.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01EA, PCNT))
    {
        Scope (\P490)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01EA, PCNT))
                {
                    Return (\P490.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01EB, PCNT))
    {
        Scope (\P491)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01EB, PCNT))
                {
                    Return (\P491.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01EC, PCNT))
    {
        Scope (\P492)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01EC, PCNT))
                {
                    Return (\P492.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01ED, PCNT))
    {
        Scope (\P493)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01ED, PCNT))
                {
                    Return (\P493.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01EE, PCNT))
    {
        Scope (\P494)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01EE, PCNT))
                {
                    Return (\P494.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01EF, PCNT))
    {
        Scope (\P495)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01EF, PCNT))
                {
                    Return (\P495.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F0, PCNT))
    {
        Scope (\P496)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F0, PCNT))
                {
                    Return (\P496.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F1, PCNT))
    {
        Scope (\P497)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F1, PCNT))
                {
                    Return (\P497.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F2, PCNT))
    {
        Scope (\P498)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F2, PCNT))
                {
                    Return (\P498.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F3, PCNT))
    {
        Scope (\P499)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F3, PCNT))
                {
                    Return (\P499.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F4, PCNT))
    {
        Scope (\P500)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F4, PCNT))
                {
                    Return (\P500.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F5, PCNT))
    {
        Scope (\P501)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F5, PCNT))
                {
                    Return (\P501.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F6, PCNT))
    {
        Scope (\P502)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F6, PCNT))
                {
                    Return (\P502.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F7, PCNT))
    {
        Scope (\P503)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F7, PCNT))
                {
                    Return (\P503.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F8, PCNT))
    {
        Scope (\P504)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F8, PCNT))
                {
                    Return (\P504.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01F9, PCNT))
    {
        Scope (\P505)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01F9, PCNT))
                {
                    Return (\P505.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01FA, PCNT))
    {
        Scope (\P506)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01FA, PCNT))
                {
                    Return (\P506.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01FB, PCNT))
    {
        Scope (\P507)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01FB, PCNT))
                {
                    Return (\P507.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01FC, PCNT))
    {
        Scope (\P508)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01FC, PCNT))
                {
                    Return (\P508.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01FD, PCNT))
    {
        Scope (\P509)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01FD, PCNT))
                {
                    Return (\P509.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01FE, PCNT))
    {
        Scope (\P510)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01FE, PCNT))
                {
                    Return (\P510.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x01FF, PCNT))
    {
        Scope (\P511)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x01FF, PCNT))
                {
                    Return (\P511.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0200, PCNT))
    {
        Scope (\P512)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0200, PCNT))
                {
                    Return (\P512.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0201, PCNT))
    {
        Scope (\P513)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0201, PCNT))
                {
                    Return (\P513.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0202, PCNT))
    {
        Scope (\P514)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0202, PCNT))
                {
                    Return (\P514.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0203, PCNT))
    {
        Scope (\P515)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0203, PCNT))
                {
                    Return (\P515.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0204, PCNT))
    {
        Scope (\P516)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0204, PCNT))
                {
                    Return (\P516.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0205, PCNT))
    {
        Scope (\P517)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0205, PCNT))
                {
                    Return (\P517.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0206, PCNT))
    {
        Scope (\P518)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0206, PCNT))
                {
                    Return (\P518.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0207, PCNT))
    {
        Scope (\P519)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0207, PCNT))
                {
                    Return (\P519.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0208, PCNT))
    {
        Scope (\P520)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0208, PCNT))
                {
                    Return (\P520.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0209, PCNT))
    {
        Scope (\P521)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0209, PCNT))
                {
                    Return (\P521.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x020A, PCNT))
    {
        Scope (\P522)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x020A, PCNT))
                {
                    Return (\P522.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x020B, PCNT))
    {
        Scope (\P523)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x020B, PCNT))
                {
                    Return (\P523.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x020C, PCNT))
    {
        Scope (\P524)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x020C, PCNT))
                {
                    Return (\P524.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x020D, PCNT))
    {
        Scope (\P525)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x020D, PCNT))
                {
                    Return (\P525.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x020E, PCNT))
    {
        Scope (\P526)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x020E, PCNT))
                {
                    Return (\P526.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x020F, PCNT))
    {
        Scope (\P527)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x020F, PCNT))
                {
                    Return (\P527.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0210, PCNT))
    {
        Scope (\P528)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0210, PCNT))
                {
                    Return (\P528.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0211, PCNT))
    {
        Scope (\P529)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0211, PCNT))
                {
                    Return (\P529.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0212, PCNT))
    {
        Scope (\P530)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0212, PCNT))
                {
                    Return (\P530.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0213, PCNT))
    {
        Scope (\P531)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0213, PCNT))
                {
                    Return (\P531.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0214, PCNT))
    {
        Scope (\P532)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0214, PCNT))
                {
                    Return (\P532.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0215, PCNT))
    {
        Scope (\P533)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0215, PCNT))
                {
                    Return (\P533.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0216, PCNT))
    {
        Scope (\P534)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0216, PCNT))
                {
                    Return (\P534.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0217, PCNT))
    {
        Scope (\P535)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0217, PCNT))
                {
                    Return (\P535.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0218, PCNT))
    {
        Scope (\P536)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0218, PCNT))
                {
                    Return (\P536.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0219, PCNT))
    {
        Scope (\P537)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0219, PCNT))
                {
                    Return (\P537.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x021A, PCNT))
    {
        Scope (\P538)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x021A, PCNT))
                {
                    Return (\P538.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x021B, PCNT))
    {
        Scope (\P539)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x021B, PCNT))
                {
                    Return (\P539.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x021C, PCNT))
    {
        Scope (\P540)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x021C, PCNT))
                {
                    Return (\P540.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x021D, PCNT))
    {
        Scope (\P541)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x021D, PCNT))
                {
                    Return (\P541.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x021E, PCNT))
    {
        Scope (\P542)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x021E, PCNT))
                {
                    Return (\P542.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x021F, PCNT))
    {
        Scope (\P543)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x021F, PCNT))
                {
                    Return (\P543.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0220, PCNT))
    {
        Scope (\P544)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0220, PCNT))
                {
                    Return (\P544.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0221, PCNT))
    {
        Scope (\P545)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0221, PCNT))
                {
                    Return (\P545.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0222, PCNT))
    {
        Scope (\P546)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0222, PCNT))
                {
                    Return (\P546.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0223, PCNT))
    {
        Scope (\P547)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0223, PCNT))
                {
                    Return (\P547.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0224, PCNT))
    {
        Scope (\P548)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0224, PCNT))
                {
                    Return (\P548.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0225, PCNT))
    {
        Scope (\P549)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0225, PCNT))
                {
                    Return (\P549.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0226, PCNT))
    {
        Scope (\P550)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0226, PCNT))
                {
                    Return (\P550.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0227, PCNT))
    {
        Scope (\P551)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0227, PCNT))
                {
                    Return (\P551.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0228, PCNT))
    {
        Scope (\P552)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0228, PCNT))
                {
                    Return (\P552.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0229, PCNT))
    {
        Scope (\P553)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0229, PCNT))
                {
                    Return (\P553.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x022A, PCNT))
    {
        Scope (\P554)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x022A, PCNT))
                {
                    Return (\P554.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x022B, PCNT))
    {
        Scope (\P555)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x022B, PCNT))
                {
                    Return (\P555.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x022C, PCNT))
    {
        Scope (\P556)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x022C, PCNT))
                {
                    Return (\P556.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x022D, PCNT))
    {
        Scope (\P557)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x022D, PCNT))
                {
                    Return (\P557.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x022E, PCNT))
    {
        Scope (\P558)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x022E, PCNT))
                {
                    Return (\P558.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x022F, PCNT))
    {
        Scope (\P559)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x022F, PCNT))
                {
                    Return (\P559.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0230, PCNT))
    {
        Scope (\P560)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0230, PCNT))
                {
                    Return (\P560.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0231, PCNT))
    {
        Scope (\P561)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0231, PCNT))
                {
                    Return (\P561.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0232, PCNT))
    {
        Scope (\P562)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0232, PCNT))
                {
                    Return (\P562.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0233, PCNT))
    {
        Scope (\P563)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0233, PCNT))
                {
                    Return (\P563.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0234, PCNT))
    {
        Scope (\P564)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0234, PCNT))
                {
                    Return (\P564.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0235, PCNT))
    {
        Scope (\P565)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0235, PCNT))
                {
                    Return (\P565.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0236, PCNT))
    {
        Scope (\P566)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0236, PCNT))
                {
                    Return (\P566.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0237, PCNT))
    {
        Scope (\P567)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0237, PCNT))
                {
                    Return (\P567.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0238, PCNT))
    {
        Scope (\P568)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0238, PCNT))
                {
                    Return (\P568.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0239, PCNT))
    {
        Scope (\P569)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0239, PCNT))
                {
                    Return (\P569.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x023A, PCNT))
    {
        Scope (\P570)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x023A, PCNT))
                {
                    Return (\P570.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x023B, PCNT))
    {
        Scope (\P571)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x023B, PCNT))
                {
                    Return (\P571.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x023C, PCNT))
    {
        Scope (\P572)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x023C, PCNT))
                {
                    Return (\P572.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x023D, PCNT))
    {
        Scope (\P573)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x023D, PCNT))
                {
                    Return (\P573.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x023E, PCNT))
    {
        Scope (\P574)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x023E, PCNT))
                {
                    Return (\P574.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x023F, PCNT))
    {
        Scope (\P575)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x023F, PCNT))
                {
                    Return (\P575.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0240, PCNT))
    {
        Scope (\P576)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0240, PCNT))
                {
                    Return (\P576.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0241, PCNT))
    {
        Scope (\P577)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0241, PCNT))
                {
                    Return (\P577.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0242, PCNT))
    {
        Scope (\P578)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0242, PCNT))
                {
                    Return (\P578.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0243, PCNT))
    {
        Scope (\P579)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0243, PCNT))
                {
                    Return (\P579.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0244, PCNT))
    {
        Scope (\P580)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0244, PCNT))
                {
                    Return (\P580.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0245, PCNT))
    {
        Scope (\P581)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0245, PCNT))
                {
                    Return (\P581.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0246, PCNT))
    {
        Scope (\P582)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0246, PCNT))
                {
                    Return (\P582.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0247, PCNT))
    {
        Scope (\P583)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0247, PCNT))
                {
                    Return (\P583.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0248, PCNT))
    {
        Scope (\P584)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0248, PCNT))
                {
                    Return (\P584.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0249, PCNT))
    {
        Scope (\P585)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0249, PCNT))
                {
                    Return (\P585.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x024A, PCNT))
    {
        Scope (\P586)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x024A, PCNT))
                {
                    Return (\P586.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x024B, PCNT))
    {
        Scope (\P587)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x024B, PCNT))
                {
                    Return (\P587.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x024C, PCNT))
    {
        Scope (\P588)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x024C, PCNT))
                {
                    Return (\P588.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x024D, PCNT))
    {
        Scope (\P589)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x024D, PCNT))
                {
                    Return (\P589.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x024E, PCNT))
    {
        Scope (\P590)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x024E, PCNT))
                {
                    Return (\P590.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x024F, PCNT))
    {
        Scope (\P591)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x024F, PCNT))
                {
                    Return (\P591.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0250, PCNT))
    {
        Scope (\P592)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0250, PCNT))
                {
                    Return (\P592.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0251, PCNT))
    {
        Scope (\P593)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0251, PCNT))
                {
                    Return (\P593.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0252, PCNT))
    {
        Scope (\P594)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0252, PCNT))
                {
                    Return (\P594.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0253, PCNT))
    {
        Scope (\P595)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0253, PCNT))
                {
                    Return (\P595.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0254, PCNT))
    {
        Scope (\P596)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0254, PCNT))
                {
                    Return (\P596.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0255, PCNT))
    {
        Scope (\P597)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0255, PCNT))
                {
                    Return (\P597.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0256, PCNT))
    {
        Scope (\P598)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0256, PCNT))
                {
                    Return (\P598.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0257, PCNT))
    {
        Scope (\P599)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0257, PCNT))
                {
                    Return (\P599.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0258, PCNT))
    {
        Scope (\P600)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0258, PCNT))
                {
                    Return (\P600.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0259, PCNT))
    {
        Scope (\P601)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0259, PCNT))
                {
                    Return (\P601.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x025A, PCNT))
    {
        Scope (\P602)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x025A, PCNT))
                {
                    Return (\P602.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x025B, PCNT))
    {
        Scope (\P603)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x025B, PCNT))
                {
                    Return (\P603.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x025C, PCNT))
    {
        Scope (\P604)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x025C, PCNT))
                {
                    Return (\P604.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x025D, PCNT))
    {
        Scope (\P605)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x025D, PCNT))
                {
                    Return (\P605.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x025E, PCNT))
    {
        Scope (\P606)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x025E, PCNT))
                {
                    Return (\P606.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x025F, PCNT))
    {
        Scope (\P607)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x025F, PCNT))
                {
                    Return (\P607.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0260, PCNT))
    {
        Scope (\P608)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0260, PCNT))
                {
                    Return (\P608.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0261, PCNT))
    {
        Scope (\P609)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0261, PCNT))
                {
                    Return (\P609.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0262, PCNT))
    {
        Scope (\P610)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0262, PCNT))
                {
                    Return (\P610.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0263, PCNT))
    {
        Scope (\P611)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0263, PCNT))
                {
                    Return (\P611.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0264, PCNT))
    {
        Scope (\P612)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0264, PCNT))
                {
                    Return (\P612.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0265, PCNT))
    {
        Scope (\P613)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0265, PCNT))
                {
                    Return (\P613.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0266, PCNT))
    {
        Scope (\P614)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0266, PCNT))
                {
                    Return (\P614.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0267, PCNT))
    {
        Scope (\P615)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0267, PCNT))
                {
                    Return (\P615.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0268, PCNT))
    {
        Scope (\P616)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0268, PCNT))
                {
                    Return (\P616.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0269, PCNT))
    {
        Scope (\P617)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0269, PCNT))
                {
                    Return (\P617.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x026A, PCNT))
    {
        Scope (\P618)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x026A, PCNT))
                {
                    Return (\P618.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x026B, PCNT))
    {
        Scope (\P619)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x026B, PCNT))
                {
                    Return (\P619.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x026C, PCNT))
    {
        Scope (\P620)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x026C, PCNT))
                {
                    Return (\P620.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x026D, PCNT))
    {
        Scope (\P621)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x026D, PCNT))
                {
                    Return (\P621.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x026E, PCNT))
    {
        Scope (\P622)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x026E, PCNT))
                {
                    Return (\P622.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x026F, PCNT))
    {
        Scope (\P623)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x026F, PCNT))
                {
                    Return (\P623.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0270, PCNT))
    {
        Scope (\P624)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0270, PCNT))
                {
                    Return (\P624.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0271, PCNT))
    {
        Scope (\P625)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0271, PCNT))
                {
                    Return (\P625.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0272, PCNT))
    {
        Scope (\P626)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0272, PCNT))
                {
                    Return (\P626.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0273, PCNT))
    {
        Scope (\P627)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0273, PCNT))
                {
                    Return (\P627.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0274, PCNT))
    {
        Scope (\P628)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0274, PCNT))
                {
                    Return (\P628.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0275, PCNT))
    {
        Scope (\P629)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0275, PCNT))
                {
                    Return (\P629.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0276, PCNT))
    {
        Scope (\P630)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0276, PCNT))
                {
                    Return (\P630.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0277, PCNT))
    {
        Scope (\P631)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0277, PCNT))
                {
                    Return (\P631.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0278, PCNT))
    {
        Scope (\P632)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0278, PCNT))
                {
                    Return (\P632.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0279, PCNT))
    {
        Scope (\P633)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0279, PCNT))
                {
                    Return (\P633.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x027A, PCNT))
    {
        Scope (\P634)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x027A, PCNT))
                {
                    Return (\P634.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x027B, PCNT))
    {
        Scope (\P635)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x027B, PCNT))
                {
                    Return (\P635.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x027C, PCNT))
    {
        Scope (\P636)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x027C, PCNT))
                {
                    Return (\P636.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x027D, PCNT))
    {
        Scope (\P637)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x027D, PCNT))
                {
                    Return (\P637.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x027E, PCNT))
    {
        Scope (\P638)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x027E, PCNT))
                {
                    Return (\P638.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x027F, PCNT))
    {
        Scope (\P639)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x027F, PCNT))
                {
                    Return (\P639.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0280, PCNT))
    {
        Scope (\P640)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0280, PCNT))
                {
                    Return (\P640.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0281, PCNT))
    {
        Scope (\P641)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0281, PCNT))
                {
                    Return (\P641.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0282, PCNT))
    {
        Scope (\P642)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0282, PCNT))
                {
                    Return (\P642.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0283, PCNT))
    {
        Scope (\P643)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0283, PCNT))
                {
                    Return (\P643.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0284, PCNT))
    {
        Scope (\P644)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0284, PCNT))
                {
                    Return (\P644.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0285, PCNT))
    {
        Scope (\P645)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0285, PCNT))
                {
                    Return (\P645.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0286, PCNT))
    {
        Scope (\P646)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0286, PCNT))
                {
                    Return (\P646.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0287, PCNT))
    {
        Scope (\P647)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0287, PCNT))
                {
                    Return (\P647.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0288, PCNT))
    {
        Scope (\P648)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0288, PCNT))
                {
                    Return (\P648.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0289, PCNT))
    {
        Scope (\P649)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0289, PCNT))
                {
                    Return (\P649.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x028A, PCNT))
    {
        Scope (\P650)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x028A, PCNT))
                {
                    Return (\P650.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x028B, PCNT))
    {
        Scope (\P651)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x028B, PCNT))
                {
                    Return (\P651.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x028C, PCNT))
    {
        Scope (\P652)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x028C, PCNT))
                {
                    Return (\P652.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x028D, PCNT))
    {
        Scope (\P653)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x028D, PCNT))
                {
                    Return (\P653.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x028E, PCNT))
    {
        Scope (\P654)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x028E, PCNT))
                {
                    Return (\P654.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x028F, PCNT))
    {
        Scope (\P655)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x028F, PCNT))
                {
                    Return (\P655.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0290, PCNT))
    {
        Scope (\P656)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0290, PCNT))
                {
                    Return (\P656.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0291, PCNT))
    {
        Scope (\P657)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0291, PCNT))
                {
                    Return (\P657.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0292, PCNT))
    {
        Scope (\P658)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0292, PCNT))
                {
                    Return (\P658.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0293, PCNT))
    {
        Scope (\P659)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0293, PCNT))
                {
                    Return (\P659.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0294, PCNT))
    {
        Scope (\P660)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0294, PCNT))
                {
                    Return (\P660.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0295, PCNT))
    {
        Scope (\P661)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0295, PCNT))
                {
                    Return (\P661.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0296, PCNT))
    {
        Scope (\P662)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0296, PCNT))
                {
                    Return (\P662.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0297, PCNT))
    {
        Scope (\P663)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0297, PCNT))
                {
                    Return (\P663.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0298, PCNT))
    {
        Scope (\P664)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0298, PCNT))
                {
                    Return (\P664.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0299, PCNT))
    {
        Scope (\P665)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0299, PCNT))
                {
                    Return (\P665.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x029A, PCNT))
    {
        Scope (\P666)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x029A, PCNT))
                {
                    Return (\P666.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x029B, PCNT))
    {
        Scope (\P667)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x029B, PCNT))
                {
                    Return (\P667.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x029C, PCNT))
    {
        Scope (\P668)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x029C, PCNT))
                {
                    Return (\P668.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x029D, PCNT))
    {
        Scope (\P669)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x029D, PCNT))
                {
                    Return (\P669.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x029E, PCNT))
    {
        Scope (\P670)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x029E, PCNT))
                {
                    Return (\P670.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x029F, PCNT))
    {
        Scope (\P671)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x029F, PCNT))
                {
                    Return (\P671.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A0, PCNT))
    {
        Scope (\P672)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A0, PCNT))
                {
                    Return (\P672.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A1, PCNT))
    {
        Scope (\P673)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A1, PCNT))
                {
                    Return (\P673.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A2, PCNT))
    {
        Scope (\P674)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A2, PCNT))
                {
                    Return (\P674.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A3, PCNT))
    {
        Scope (\P675)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A3, PCNT))
                {
                    Return (\P675.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A4, PCNT))
    {
        Scope (\P676)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A4, PCNT))
                {
                    Return (\P676.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A5, PCNT))
    {
        Scope (\P677)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A5, PCNT))
                {
                    Return (\P677.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A6, PCNT))
    {
        Scope (\P678)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A6, PCNT))
                {
                    Return (\P678.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A7, PCNT))
    {
        Scope (\P679)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A7, PCNT))
                {
                    Return (\P679.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A8, PCNT))
    {
        Scope (\P680)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A8, PCNT))
                {
                    Return (\P680.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02A9, PCNT))
    {
        Scope (\P681)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02A9, PCNT))
                {
                    Return (\P681.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02AA, PCNT))
    {
        Scope (\P682)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02AA, PCNT))
                {
                    Return (\P682.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02AB, PCNT))
    {
        Scope (\P683)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02AB, PCNT))
                {
                    Return (\P683.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02AC, PCNT))
    {
        Scope (\P684)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02AC, PCNT))
                {
                    Return (\P684.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02AD, PCNT))
    {
        Scope (\P685)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02AD, PCNT))
                {
                    Return (\P685.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02AE, PCNT))
    {
        Scope (\P686)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02AE, PCNT))
                {
                    Return (\P686.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02AF, PCNT))
    {
        Scope (\P687)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02AF, PCNT))
                {
                    Return (\P687.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B0, PCNT))
    {
        Scope (\P688)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B0, PCNT))
                {
                    Return (\P688.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B1, PCNT))
    {
        Scope (\P689)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B1, PCNT))
                {
                    Return (\P689.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B2, PCNT))
    {
        Scope (\P690)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B2, PCNT))
                {
                    Return (\P690.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B3, PCNT))
    {
        Scope (\P691)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B3, PCNT))
                {
                    Return (\P691.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B4, PCNT))
    {
        Scope (\P692)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B4, PCNT))
                {
                    Return (\P692.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B5, PCNT))
    {
        Scope (\P693)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B5, PCNT))
                {
                    Return (\P693.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B6, PCNT))
    {
        Scope (\P694)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B6, PCNT))
                {
                    Return (\P694.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B7, PCNT))
    {
        Scope (\P695)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B7, PCNT))
                {
                    Return (\P695.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B8, PCNT))
    {
        Scope (\P696)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B8, PCNT))
                {
                    Return (\P696.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02B9, PCNT))
    {
        Scope (\P697)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02B9, PCNT))
                {
                    Return (\P697.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02BA, PCNT))
    {
        Scope (\P698)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02BA, PCNT))
                {
                    Return (\P698.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02BB, PCNT))
    {
        Scope (\P699)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02BB, PCNT))
                {
                    Return (\P699.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02BC, PCNT))
    {
        Scope (\P700)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02BC, PCNT))
                {
                    Return (\P700.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02BD, PCNT))
    {
        Scope (\P701)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02BD, PCNT))
                {
                    Return (\P701.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02BE, PCNT))
    {
        Scope (\P702)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02BE, PCNT))
                {
                    Return (\P702.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02BF, PCNT))
    {
        Scope (\P703)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02BF, PCNT))
                {
                    Return (\P703.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C0, PCNT))
    {
        Scope (\P704)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C0, PCNT))
                {
                    Return (\P704.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C1, PCNT))
    {
        Scope (\P705)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C1, PCNT))
                {
                    Return (\P705.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C2, PCNT))
    {
        Scope (\P706)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C2, PCNT))
                {
                    Return (\P706.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C3, PCNT))
    {
        Scope (\P707)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C3, PCNT))
                {
                    Return (\P707.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C4, PCNT))
    {
        Scope (\P708)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C4, PCNT))
                {
                    Return (\P708.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C5, PCNT))
    {
        Scope (\P709)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C5, PCNT))
                {
                    Return (\P709.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C6, PCNT))
    {
        Scope (\P710)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C6, PCNT))
                {
                    Return (\P710.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C7, PCNT))
    {
        Scope (\P711)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C7, PCNT))
                {
                    Return (\P711.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C8, PCNT))
    {
        Scope (\P712)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C8, PCNT))
                {
                    Return (\P712.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02C9, PCNT))
    {
        Scope (\P713)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02C9, PCNT))
                {
                    Return (\P713.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02CA, PCNT))
    {
        Scope (\P714)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02CA, PCNT))
                {
                    Return (\P714.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02CB, PCNT))
    {
        Scope (\P715)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02CB, PCNT))
                {
                    Return (\P715.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02CC, PCNT))
    {
        Scope (\P716)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02CC, PCNT))
                {
                    Return (\P716.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02CD, PCNT))
    {
        Scope (\P717)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02CD, PCNT))
                {
                    Return (\P717.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02CE, PCNT))
    {
        Scope (\P718)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02CE, PCNT))
                {
                    Return (\P718.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02CF, PCNT))
    {
        Scope (\P719)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02CF, PCNT))
                {
                    Return (\P719.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D0, PCNT))
    {
        Scope (\P720)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D0, PCNT))
                {
                    Return (\P720.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D1, PCNT))
    {
        Scope (\P721)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D1, PCNT))
                {
                    Return (\P721.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D2, PCNT))
    {
        Scope (\P722)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D2, PCNT))
                {
                    Return (\P722.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D3, PCNT))
    {
        Scope (\P723)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D3, PCNT))
                {
                    Return (\P723.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D4, PCNT))
    {
        Scope (\P724)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D4, PCNT))
                {
                    Return (\P724.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D5, PCNT))
    {
        Scope (\P725)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D5, PCNT))
                {
                    Return (\P725.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D6, PCNT))
    {
        Scope (\P726)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D6, PCNT))
                {
                    Return (\P726.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D7, PCNT))
    {
        Scope (\P727)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D7, PCNT))
                {
                    Return (\P727.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D8, PCNT))
    {
        Scope (\P728)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D8, PCNT))
                {
                    Return (\P728.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02D9, PCNT))
    {
        Scope (\P729)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02D9, PCNT))
                {
                    Return (\P729.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02DA, PCNT))
    {
        Scope (\P730)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02DA, PCNT))
                {
                    Return (\P730.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02DB, PCNT))
    {
        Scope (\P731)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02DB, PCNT))
                {
                    Return (\P731.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02DC, PCNT))
    {
        Scope (\P732)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02DC, PCNT))
                {
                    Return (\P732.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02DD, PCNT))
    {
        Scope (\P733)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02DD, PCNT))
                {
                    Return (\P733.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02DE, PCNT))
    {
        Scope (\P734)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02DE, PCNT))
                {
                    Return (\P734.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02DF, PCNT))
    {
        Scope (\P735)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02DF, PCNT))
                {
                    Return (\P735.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E0, PCNT))
    {
        Scope (\P736)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E0, PCNT))
                {
                    Return (\P736.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E1, PCNT))
    {
        Scope (\P737)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E1, PCNT))
                {
                    Return (\P737.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E2, PCNT))
    {
        Scope (\P738)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E2, PCNT))
                {
                    Return (\P738.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E3, PCNT))
    {
        Scope (\P739)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E3, PCNT))
                {
                    Return (\P739.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E4, PCNT))
    {
        Scope (\P740)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E4, PCNT))
                {
                    Return (\P740.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E5, PCNT))
    {
        Scope (\P741)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E5, PCNT))
                {
                    Return (\P741.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E6, PCNT))
    {
        Scope (\P742)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E6, PCNT))
                {
                    Return (\P742.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E7, PCNT))
    {
        Scope (\P743)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E7, PCNT))
                {
                    Return (\P743.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E8, PCNT))
    {
        Scope (\P744)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E8, PCNT))
                {
                    Return (\P744.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02E9, PCNT))
    {
        Scope (\P745)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02E9, PCNT))
                {
                    Return (\P745.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02EA, PCNT))
    {
        Scope (\P746)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02EA, PCNT))
                {
                    Return (\P746.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02EB, PCNT))
    {
        Scope (\P747)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02EB, PCNT))
                {
                    Return (\P747.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02EC, PCNT))
    {
        Scope (\P748)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02EC, PCNT))
                {
                    Return (\P748.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02ED, PCNT))
    {
        Scope (\P749)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02ED, PCNT))
                {
                    Return (\P749.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02EE, PCNT))
    {
        Scope (\P750)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02EE, PCNT))
                {
                    Return (\P750.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02EF, PCNT))
    {
        Scope (\P751)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02EF, PCNT))
                {
                    Return (\P751.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F0, PCNT))
    {
        Scope (\P752)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F0, PCNT))
                {
                    Return (\P752.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F1, PCNT))
    {
        Scope (\P753)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F1, PCNT))
                {
                    Return (\P753.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F2, PCNT))
    {
        Scope (\P754)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F2, PCNT))
                {
                    Return (\P754.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F3, PCNT))
    {
        Scope (\P755)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F3, PCNT))
                {
                    Return (\P755.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F4, PCNT))
    {
        Scope (\P756)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F4, PCNT))
                {
                    Return (\P756.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F5, PCNT))
    {
        Scope (\P757)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F5, PCNT))
                {
                    Return (\P757.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F6, PCNT))
    {
        Scope (\P758)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F6, PCNT))
                {
                    Return (\P758.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F7, PCNT))
    {
        Scope (\P759)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F7, PCNT))
                {
                    Return (\P759.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F8, PCNT))
    {
        Scope (\P760)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F8, PCNT))
                {
                    Return (\P760.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02F9, PCNT))
    {
        Scope (\P761)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02F9, PCNT))
                {
                    Return (\P761.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02FA, PCNT))
    {
        Scope (\P762)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02FA, PCNT))
                {
                    Return (\P762.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02FB, PCNT))
    {
        Scope (\P763)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02FB, PCNT))
                {
                    Return (\P763.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02FC, PCNT))
    {
        Scope (\P764)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02FC, PCNT))
                {
                    Return (\P764.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02FD, PCNT))
    {
        Scope (\P765)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02FD, PCNT))
                {
                    Return (\P765.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02FE, PCNT))
    {
        Scope (\P766)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02FE, PCNT))
                {
                    Return (\P766.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x02FF, PCNT))
    {
        Scope (\P767)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x02FF, PCNT))
                {
                    Return (\P767.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0300, PCNT))
    {
        Scope (\P768)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0300, PCNT))
                {
                    Return (\P768.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0301, PCNT))
    {
        Scope (\P769)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0301, PCNT))
                {
                    Return (\P769.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0302, PCNT))
    {
        Scope (\P770)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0302, PCNT))
                {
                    Return (\P770.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0303, PCNT))
    {
        Scope (\P771)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0303, PCNT))
                {
                    Return (\P771.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0304, PCNT))
    {
        Scope (\P772)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0304, PCNT))
                {
                    Return (\P772.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0305, PCNT))
    {
        Scope (\P773)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0305, PCNT))
                {
                    Return (\P773.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0306, PCNT))
    {
        Scope (\P774)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0306, PCNT))
                {
                    Return (\P774.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0307, PCNT))
    {
        Scope (\P775)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0307, PCNT))
                {
                    Return (\P775.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0308, PCNT))
    {
        Scope (\P776)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0308, PCNT))
                {
                    Return (\P776.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0309, PCNT))
    {
        Scope (\P777)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0309, PCNT))
                {
                    Return (\P777.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x030A, PCNT))
    {
        Scope (\P778)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x030A, PCNT))
                {
                    Return (\P778.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x030B, PCNT))
    {
        Scope (\P779)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x030B, PCNT))
                {
                    Return (\P779.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x030C, PCNT))
    {
        Scope (\P780)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x030C, PCNT))
                {
                    Return (\P780.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x030D, PCNT))
    {
        Scope (\P781)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x030D, PCNT))
                {
                    Return (\P781.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x030E, PCNT))
    {
        Scope (\P782)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x030E, PCNT))
                {
                    Return (\P782.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x030F, PCNT))
    {
        Scope (\P783)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x030F, PCNT))
                {
                    Return (\P783.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0310, PCNT))
    {
        Scope (\P784)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0310, PCNT))
                {
                    Return (\P784.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0311, PCNT))
    {
        Scope (\P785)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0311, PCNT))
                {
                    Return (\P785.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0312, PCNT))
    {
        Scope (\P786)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0312, PCNT))
                {
                    Return (\P786.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0313, PCNT))
    {
        Scope (\P787)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0313, PCNT))
                {
                    Return (\P787.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0314, PCNT))
    {
        Scope (\P788)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0314, PCNT))
                {
                    Return (\P788.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0315, PCNT))
    {
        Scope (\P789)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0315, PCNT))
                {
                    Return (\P789.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0316, PCNT))
    {
        Scope (\P790)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0316, PCNT))
                {
                    Return (\P790.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0317, PCNT))
    {
        Scope (\P791)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0317, PCNT))
                {
                    Return (\P791.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0318, PCNT))
    {
        Scope (\P792)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0318, PCNT))
                {
                    Return (\P792.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0319, PCNT))
    {
        Scope (\P793)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0319, PCNT))
                {
                    Return (\P793.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x031A, PCNT))
    {
        Scope (\P794)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x031A, PCNT))
                {
                    Return (\P794.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x031B, PCNT))
    {
        Scope (\P795)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x031B, PCNT))
                {
                    Return (\P795.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x031C, PCNT))
    {
        Scope (\P796)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x031C, PCNT))
                {
                    Return (\P796.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x031D, PCNT))
    {
        Scope (\P797)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x031D, PCNT))
                {
                    Return (\P797.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x031E, PCNT))
    {
        Scope (\P798)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x031E, PCNT))
                {
                    Return (\P798.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x031F, PCNT))
    {
        Scope (\P799)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x031F, PCNT))
                {
                    Return (\P799.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0320, PCNT))
    {
        Scope (\P800)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0320, PCNT))
                {
                    Return (\P800.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0321, PCNT))
    {
        Scope (\P801)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0321, PCNT))
                {
                    Return (\P801.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0322, PCNT))
    {
        Scope (\P802)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0322, PCNT))
                {
                    Return (\P802.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0323, PCNT))
    {
        Scope (\P803)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0323, PCNT))
                {
                    Return (\P803.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0324, PCNT))
    {
        Scope (\P804)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0324, PCNT))
                {
                    Return (\P804.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0325, PCNT))
    {
        Scope (\P805)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0325, PCNT))
                {
                    Return (\P805.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0326, PCNT))
    {
        Scope (\P806)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0326, PCNT))
                {
                    Return (\P806.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0327, PCNT))
    {
        Scope (\P807)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0327, PCNT))
                {
                    Return (\P807.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0328, PCNT))
    {
        Scope (\P808)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0328, PCNT))
                {
                    Return (\P808.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0329, PCNT))
    {
        Scope (\P809)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0329, PCNT))
                {
                    Return (\P809.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x032A, PCNT))
    {
        Scope (\P810)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x032A, PCNT))
                {
                    Return (\P810.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x032B, PCNT))
    {
        Scope (\P811)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x032B, PCNT))
                {
                    Return (\P811.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x032C, PCNT))
    {
        Scope (\P812)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x032C, PCNT))
                {
                    Return (\P812.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x032D, PCNT))
    {
        Scope (\P813)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x032D, PCNT))
                {
                    Return (\P813.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x032E, PCNT))
    {
        Scope (\P814)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x032E, PCNT))
                {
                    Return (\P814.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x032F, PCNT))
    {
        Scope (\P815)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x032F, PCNT))
                {
                    Return (\P815.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0330, PCNT))
    {
        Scope (\P816)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0330, PCNT))
                {
                    Return (\P816.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0331, PCNT))
    {
        Scope (\P817)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0331, PCNT))
                {
                    Return (\P817.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0332, PCNT))
    {
        Scope (\P818)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0332, PCNT))
                {
                    Return (\P818.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0333, PCNT))
    {
        Scope (\P819)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0333, PCNT))
                {
                    Return (\P819.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0334, PCNT))
    {
        Scope (\P820)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0334, PCNT))
                {
                    Return (\P820.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0335, PCNT))
    {
        Scope (\P821)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0335, PCNT))
                {
                    Return (\P821.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0336, PCNT))
    {
        Scope (\P822)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0336, PCNT))
                {
                    Return (\P822.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0337, PCNT))
    {
        Scope (\P823)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0337, PCNT))
                {
                    Return (\P823.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0338, PCNT))
    {
        Scope (\P824)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0338, PCNT))
                {
                    Return (\P824.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0339, PCNT))
    {
        Scope (\P825)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0339, PCNT))
                {
                    Return (\P825.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x033A, PCNT))
    {
        Scope (\P826)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x033A, PCNT))
                {
                    Return (\P826.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x033B, PCNT))
    {
        Scope (\P827)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x033B, PCNT))
                {
                    Return (\P827.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x033C, PCNT))
    {
        Scope (\P828)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x033C, PCNT))
                {
                    Return (\P828.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x033D, PCNT))
    {
        Scope (\P829)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x033D, PCNT))
                {
                    Return (\P829.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x033E, PCNT))
    {
        Scope (\P830)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x033E, PCNT))
                {
                    Return (\P830.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x033F, PCNT))
    {
        Scope (\P831)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x033F, PCNT))
                {
                    Return (\P831.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0340, PCNT))
    {
        Scope (\P832)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0340, PCNT))
                {
                    Return (\P832.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0341, PCNT))
    {
        Scope (\P833)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0341, PCNT))
                {
                    Return (\P833.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0342, PCNT))
    {
        Scope (\P834)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0342, PCNT))
                {
                    Return (\P834.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0343, PCNT))
    {
        Scope (\P835)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0343, PCNT))
                {
                    Return (\P835.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0344, PCNT))
    {
        Scope (\P836)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0344, PCNT))
                {
                    Return (\P836.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0345, PCNT))
    {
        Scope (\P837)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0345, PCNT))
                {
                    Return (\P837.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0346, PCNT))
    {
        Scope (\P838)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0346, PCNT))
                {
                    Return (\P838.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0347, PCNT))
    {
        Scope (\P839)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0347, PCNT))
                {
                    Return (\P839.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0348, PCNT))
    {
        Scope (\P840)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0348, PCNT))
                {
                    Return (\P840.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0349, PCNT))
    {
        Scope (\P841)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0349, PCNT))
                {
                    Return (\P841.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x034A, PCNT))
    {
        Scope (\P842)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x034A, PCNT))
                {
                    Return (\P842.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x034B, PCNT))
    {
        Scope (\P843)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x034B, PCNT))
                {
                    Return (\P843.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x034C, PCNT))
    {
        Scope (\P844)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x034C, PCNT))
                {
                    Return (\P844.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x034D, PCNT))
    {
        Scope (\P845)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x034D, PCNT))
                {
                    Return (\P845.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x034E, PCNT))
    {
        Scope (\P846)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x034E, PCNT))
                {
                    Return (\P846.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x034F, PCNT))
    {
        Scope (\P847)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x034F, PCNT))
                {
                    Return (\P847.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0350, PCNT))
    {
        Scope (\P848)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0350, PCNT))
                {
                    Return (\P848.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0351, PCNT))
    {
        Scope (\P849)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0351, PCNT))
                {
                    Return (\P849.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0352, PCNT))
    {
        Scope (\P850)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0352, PCNT))
                {
                    Return (\P850.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0353, PCNT))
    {
        Scope (\P851)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0353, PCNT))
                {
                    Return (\P851.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0354, PCNT))
    {
        Scope (\P852)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0354, PCNT))
                {
                    Return (\P852.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0355, PCNT))
    {
        Scope (\P853)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0355, PCNT))
                {
                    Return (\P853.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0356, PCNT))
    {
        Scope (\P854)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0356, PCNT))
                {
                    Return (\P854.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0357, PCNT))
    {
        Scope (\P855)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0357, PCNT))
                {
                    Return (\P855.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0358, PCNT))
    {
        Scope (\P856)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0358, PCNT))
                {
                    Return (\P856.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0359, PCNT))
    {
        Scope (\P857)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0359, PCNT))
                {
                    Return (\P857.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x035A, PCNT))
    {
        Scope (\P858)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x035A, PCNT))
                {
                    Return (\P858.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x035B, PCNT))
    {
        Scope (\P859)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x035B, PCNT))
                {
                    Return (\P859.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x035C, PCNT))
    {
        Scope (\P860)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x035C, PCNT))
                {
                    Return (\P860.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x035D, PCNT))
    {
        Scope (\P861)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x035D, PCNT))
                {
                    Return (\P861.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x035E, PCNT))
    {
        Scope (\P862)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x035E, PCNT))
                {
                    Return (\P862.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x035F, PCNT))
    {
        Scope (\P863)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x035F, PCNT))
                {
                    Return (\P863.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0360, PCNT))
    {
        Scope (\P864)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0360, PCNT))
                {
                    Return (\P864.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0361, PCNT))
    {
        Scope (\P865)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0361, PCNT))
                {
                    Return (\P865.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0362, PCNT))
    {
        Scope (\P866)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0362, PCNT))
                {
                    Return (\P866.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0363, PCNT))
    {
        Scope (\P867)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0363, PCNT))
                {
                    Return (\P867.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0364, PCNT))
    {
        Scope (\P868)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0364, PCNT))
                {
                    Return (\P868.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0365, PCNT))
    {
        Scope (\P869)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0365, PCNT))
                {
                    Return (\P869.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0366, PCNT))
    {
        Scope (\P870)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0366, PCNT))
                {
                    Return (\P870.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0367, PCNT))
    {
        Scope (\P871)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0367, PCNT))
                {
                    Return (\P871.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0368, PCNT))
    {
        Scope (\P872)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0368, PCNT))
                {
                    Return (\P872.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0369, PCNT))
    {
        Scope (\P873)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0369, PCNT))
                {
                    Return (\P873.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x036A, PCNT))
    {
        Scope (\P874)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x036A, PCNT))
                {
                    Return (\P874.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x036B, PCNT))
    {
        Scope (\P875)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x036B, PCNT))
                {
                    Return (\P875.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x036C, PCNT))
    {
        Scope (\P876)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x036C, PCNT))
                {
                    Return (\P876.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x036D, PCNT))
    {
        Scope (\P877)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x036D, PCNT))
                {
                    Return (\P877.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x036E, PCNT))
    {
        Scope (\P878)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x036E, PCNT))
                {
                    Return (\P878.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x036F, PCNT))
    {
        Scope (\P879)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x036F, PCNT))
                {
                    Return (\P879.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0370, PCNT))
    {
        Scope (\P880)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0370, PCNT))
                {
                    Return (\P880.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0371, PCNT))
    {
        Scope (\P881)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0371, PCNT))
                {
                    Return (\P881.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0372, PCNT))
    {
        Scope (\P882)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0372, PCNT))
                {
                    Return (\P882.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0373, PCNT))
    {
        Scope (\P883)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0373, PCNT))
                {
                    Return (\P883.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0374, PCNT))
    {
        Scope (\P884)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0374, PCNT))
                {
                    Return (\P884.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0375, PCNT))
    {
        Scope (\P885)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0375, PCNT))
                {
                    Return (\P885.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0376, PCNT))
    {
        Scope (\P886)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0376, PCNT))
                {
                    Return (\P886.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0377, PCNT))
    {
        Scope (\P887)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0377, PCNT))
                {
                    Return (\P887.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0378, PCNT))
    {
        Scope (\P888)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0378, PCNT))
                {
                    Return (\P888.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0379, PCNT))
    {
        Scope (\P889)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0379, PCNT))
                {
                    Return (\P889.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x037A, PCNT))
    {
        Scope (\P890)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x037A, PCNT))
                {
                    Return (\P890.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x037B, PCNT))
    {
        Scope (\P891)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x037B, PCNT))
                {
                    Return (\P891.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x037C, PCNT))
    {
        Scope (\P892)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x037C, PCNT))
                {
                    Return (\P892.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x037D, PCNT))
    {
        Scope (\P893)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x037D, PCNT))
                {
                    Return (\P893.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x037E, PCNT))
    {
        Scope (\P894)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x037E, PCNT))
                {
                    Return (\P894.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x037F, PCNT))
    {
        Scope (\P895)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x037F, PCNT))
                {
                    Return (\P895.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0380, PCNT))
    {
        Scope (\P896)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0380, PCNT))
                {
                    Return (\P896.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0381, PCNT))
    {
        Scope (\P897)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0381, PCNT))
                {
                    Return (\P897.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0382, PCNT))
    {
        Scope (\P898)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0382, PCNT))
                {
                    Return (\P898.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0383, PCNT))
    {
        Scope (\P899)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0383, PCNT))
                {
                    Return (\P899.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0384, PCNT))
    {
        Scope (\P900)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0384, PCNT))
                {
                    Return (\P900.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0385, PCNT))
    {
        Scope (\P901)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0385, PCNT))
                {
                    Return (\P901.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0386, PCNT))
    {
        Scope (\P902)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0386, PCNT))
                {
                    Return (\P902.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0387, PCNT))
    {
        Scope (\P903)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0387, PCNT))
                {
                    Return (\P903.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0388, PCNT))
    {
        Scope (\P904)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0388, PCNT))
                {
                    Return (\P904.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0389, PCNT))
    {
        Scope (\P905)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0389, PCNT))
                {
                    Return (\P905.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x038A, PCNT))
    {
        Scope (\P906)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x038A, PCNT))
                {
                    Return (\P906.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x038B, PCNT))
    {
        Scope (\P907)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x038B, PCNT))
                {
                    Return (\P907.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x038C, PCNT))
    {
        Scope (\P908)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x038C, PCNT))
                {
                    Return (\P908.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x038D, PCNT))
    {
        Scope (\P909)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x038D, PCNT))
                {
                    Return (\P909.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x038E, PCNT))
    {
        Scope (\P910)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x038E, PCNT))
                {
                    Return (\P910.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x038F, PCNT))
    {
        Scope (\P911)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x038F, PCNT))
                {
                    Return (\P911.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0390, PCNT))
    {
        Scope (\P912)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0390, PCNT))
                {
                    Return (\P912.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0391, PCNT))
    {
        Scope (\P913)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0391, PCNT))
                {
                    Return (\P913.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0392, PCNT))
    {
        Scope (\P914)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0392, PCNT))
                {
                    Return (\P914.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0393, PCNT))
    {
        Scope (\P915)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0393, PCNT))
                {
                    Return (\P915.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0394, PCNT))
    {
        Scope (\P916)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0394, PCNT))
                {
                    Return (\P916.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0395, PCNT))
    {
        Scope (\P917)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0395, PCNT))
                {
                    Return (\P917.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0396, PCNT))
    {
        Scope (\P918)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0396, PCNT))
                {
                    Return (\P918.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0397, PCNT))
    {
        Scope (\P919)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0397, PCNT))
                {
                    Return (\P919.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0398, PCNT))
    {
        Scope (\P920)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0398, PCNT))
                {
                    Return (\P920.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0399, PCNT))
    {
        Scope (\P921)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0399, PCNT))
                {
                    Return (\P921.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x039A, PCNT))
    {
        Scope (\P922)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x039A, PCNT))
                {
                    Return (\P922.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x039B, PCNT))
    {
        Scope (\P923)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x039B, PCNT))
                {
                    Return (\P923.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x039C, PCNT))
    {
        Scope (\P924)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x039C, PCNT))
                {
                    Return (\P924.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x039D, PCNT))
    {
        Scope (\P925)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x039D, PCNT))
                {
                    Return (\P925.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x039E, PCNT))
    {
        Scope (\P926)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x039E, PCNT))
                {
                    Return (\P926.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x039F, PCNT))
    {
        Scope (\P927)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x039F, PCNT))
                {
                    Return (\P927.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A0, PCNT))
    {
        Scope (\P928)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A0, PCNT))
                {
                    Return (\P928.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A1, PCNT))
    {
        Scope (\P929)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A1, PCNT))
                {
                    Return (\P929.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A2, PCNT))
    {
        Scope (\P930)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A2, PCNT))
                {
                    Return (\P930.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A3, PCNT))
    {
        Scope (\P931)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A3, PCNT))
                {
                    Return (\P931.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A4, PCNT))
    {
        Scope (\P932)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A4, PCNT))
                {
                    Return (\P932.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A5, PCNT))
    {
        Scope (\P933)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A5, PCNT))
                {
                    Return (\P933.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A6, PCNT))
    {
        Scope (\P934)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A6, PCNT))
                {
                    Return (\P934.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A7, PCNT))
    {
        Scope (\P935)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A7, PCNT))
                {
                    Return (\P935.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A8, PCNT))
    {
        Scope (\P936)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A8, PCNT))
                {
                    Return (\P936.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03A9, PCNT))
    {
        Scope (\P937)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03A9, PCNT))
                {
                    Return (\P937.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03AA, PCNT))
    {
        Scope (\P938)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03AA, PCNT))
                {
                    Return (\P938.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03AB, PCNT))
    {
        Scope (\P939)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03AB, PCNT))
                {
                    Return (\P939.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03AC, PCNT))
    {
        Scope (\P940)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03AC, PCNT))
                {
                    Return (\P940.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03AD, PCNT))
    {
        Scope (\P941)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03AD, PCNT))
                {
                    Return (\P941.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03AE, PCNT))
    {
        Scope (\P942)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03AE, PCNT))
                {
                    Return (\P942.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03AF, PCNT))
    {
        Scope (\P943)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03AF, PCNT))
                {
                    Return (\P943.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B0, PCNT))
    {
        Scope (\P944)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B0, PCNT))
                {
                    Return (\P944.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B1, PCNT))
    {
        Scope (\P945)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B1, PCNT))
                {
                    Return (\P945.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B2, PCNT))
    {
        Scope (\P946)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B2, PCNT))
                {
                    Return (\P946.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B3, PCNT))
    {
        Scope (\P947)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B3, PCNT))
                {
                    Return (\P947.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B4, PCNT))
    {
        Scope (\P948)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B4, PCNT))
                {
                    Return (\P948.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B5, PCNT))
    {
        Scope (\P949)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B5, PCNT))
                {
                    Return (\P949.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B6, PCNT))
    {
        Scope (\P950)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B6, PCNT))
                {
                    Return (\P950.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B7, PCNT))
    {
        Scope (\P951)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B7, PCNT))
                {
                    Return (\P951.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B8, PCNT))
    {
        Scope (\P952)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B8, PCNT))
                {
                    Return (\P952.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03B9, PCNT))
    {
        Scope (\P953)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03B9, PCNT))
                {
                    Return (\P953.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03BA, PCNT))
    {
        Scope (\P954)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03BA, PCNT))
                {
                    Return (\P954.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03BB, PCNT))
    {
        Scope (\P955)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03BB, PCNT))
                {
                    Return (\P955.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03BC, PCNT))
    {
        Scope (\P956)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03BC, PCNT))
                {
                    Return (\P956.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03BD, PCNT))
    {
        Scope (\P957)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03BD, PCNT))
                {
                    Return (\P957.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03BE, PCNT))
    {
        Scope (\P958)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03BE, PCNT))
                {
                    Return (\P958.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03BF, PCNT))
    {
        Scope (\P959)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03BF, PCNT))
                {
                    Return (\P959.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C0, PCNT))
    {
        Scope (\P960)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C0, PCNT))
                {
                    Return (\P960.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C1, PCNT))
    {
        Scope (\P961)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C1, PCNT))
                {
                    Return (\P961.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C2, PCNT))
    {
        Scope (\P962)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C2, PCNT))
                {
                    Return (\P962.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C3, PCNT))
    {
        Scope (\P963)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C3, PCNT))
                {
                    Return (\P963.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C4, PCNT))
    {
        Scope (\P964)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C4, PCNT))
                {
                    Return (\P964.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C5, PCNT))
    {
        Scope (\P965)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C5, PCNT))
                {
                    Return (\P965.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C6, PCNT))
    {
        Scope (\P966)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C6, PCNT))
                {
                    Return (\P966.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C7, PCNT))
    {
        Scope (\P967)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C7, PCNT))
                {
                    Return (\P967.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C8, PCNT))
    {
        Scope (\P968)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C8, PCNT))
                {
                    Return (\P968.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03C9, PCNT))
    {
        Scope (\P969)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03C9, PCNT))
                {
                    Return (\P969.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03CA, PCNT))
    {
        Scope (\P970)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03CA, PCNT))
                {
                    Return (\P970.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03CB, PCNT))
    {
        Scope (\P971)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03CB, PCNT))
                {
                    Return (\P971.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03CC, PCNT))
    {
        Scope (\P972)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03CC, PCNT))
                {
                    Return (\P972.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03CD, PCNT))
    {
        Scope (\P973)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03CD, PCNT))
                {
                    Return (\P973.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03CE, PCNT))
    {
        Scope (\P974)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03CE, PCNT))
                {
                    Return (\P974.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03CF, PCNT))
    {
        Scope (\P975)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03CF, PCNT))
                {
                    Return (\P975.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D0, PCNT))
    {
        Scope (\P976)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D0, PCNT))
                {
                    Return (\P976.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D1, PCNT))
    {
        Scope (\P977)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D1, PCNT))
                {
                    Return (\P977.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D2, PCNT))
    {
        Scope (\P978)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D2, PCNT))
                {
                    Return (\P978.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D3, PCNT))
    {
        Scope (\P979)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D3, PCNT))
                {
                    Return (\P979.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D4, PCNT))
    {
        Scope (\P980)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D4, PCNT))
                {
                    Return (\P980.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D5, PCNT))
    {
        Scope (\P981)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D5, PCNT))
                {
                    Return (\P981.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D6, PCNT))
    {
        Scope (\P982)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D6, PCNT))
                {
                    Return (\P982.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D7, PCNT))
    {
        Scope (\P983)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D7, PCNT))
                {
                    Return (\P983.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D8, PCNT))
    {
        Scope (\P984)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D8, PCNT))
                {
                    Return (\P984.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03D9, PCNT))
    {
        Scope (\P985)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03D9, PCNT))
                {
                    Return (\P985.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03DA, PCNT))
    {
        Scope (\P986)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03DA, PCNT))
                {
                    Return (\P986.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03DB, PCNT))
    {
        Scope (\P987)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03DB, PCNT))
                {
                    Return (\P987.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03DC, PCNT))
    {
        Scope (\P988)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03DC, PCNT))
                {
                    Return (\P988.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03DD, PCNT))
    {
        Scope (\P989)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03DD, PCNT))
                {
                    Return (\P989.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03DE, PCNT))
    {
        Scope (\P990)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03DE, PCNT))
                {
                    Return (\P990.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03DF, PCNT))
    {
        Scope (\P991)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03DF, PCNT))
                {
                    Return (\P991.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E0, PCNT))
    {
        Scope (\P992)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E0, PCNT))
                {
                    Return (\P992.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E1, PCNT))
    {
        Scope (\P993)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E1, PCNT))
                {
                    Return (\P993.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E2, PCNT))
    {
        Scope (\P994)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E2, PCNT))
                {
                    Return (\P994.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E3, PCNT))
    {
        Scope (\P995)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E3, PCNT))
                {
                    Return (\P995.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E4, PCNT))
    {
        Scope (\P996)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E4, PCNT))
                {
                    Return (\P996.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E5, PCNT))
    {
        Scope (\P997)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E5, PCNT))
                {
                    Return (\P997.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E6, PCNT))
    {
        Scope (\P998)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E6, PCNT))
                {
                    Return (\P998.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E7, PCNT))
    {
        Scope (\P999)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E7, PCNT))
                {
                    Return (\P999.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E8, PCNT))
    {
        Scope (\Q000)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E8, PCNT))
                {
                    Return (\Q000.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03E9, PCNT))
    {
        Scope (\Q001)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03E9, PCNT))
                {
                    Return (\Q001.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03EA, PCNT))
    {
        Scope (\Q002)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03EA, PCNT))
                {
                    Return (\Q002.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03EB, PCNT))
    {
        Scope (\Q003)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03EB, PCNT))
                {
                    Return (\Q003.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03EC, PCNT))
    {
        Scope (\Q004)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03EC, PCNT))
                {
                    Return (\Q004.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03ED, PCNT))
    {
        Scope (\Q005)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03ED, PCNT))
                {
                    Return (\Q005.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03EE, PCNT))
    {
        Scope (\Q006)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03EE, PCNT))
                {
                    Return (\Q006.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03EF, PCNT))
    {
        Scope (\Q007)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03EF, PCNT))
                {
                    Return (\Q007.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F0, PCNT))
    {
        Scope (\Q008)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F0, PCNT))
                {
                    Return (\Q008.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F1, PCNT))
    {
        Scope (\Q009)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F1, PCNT))
                {
                    Return (\Q009.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F2, PCNT))
    {
        Scope (\Q010)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F2, PCNT))
                {
                    Return (\Q010.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F3, PCNT))
    {
        Scope (\Q011)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F3, PCNT))
                {
                    Return (\Q011.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F4, PCNT))
    {
        Scope (\Q012)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F4, PCNT))
                {
                    Return (\Q012.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F5, PCNT))
    {
        Scope (\Q013)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F5, PCNT))
                {
                    Return (\Q013.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F6, PCNT))
    {
        Scope (\Q014)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F6, PCNT))
                {
                    Return (\Q014.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F7, PCNT))
    {
        Scope (\Q015)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F7, PCNT))
                {
                    Return (\Q015.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F8, PCNT))
    {
        Scope (\Q016)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F8, PCNT))
                {
                    Return (\Q016.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03F9, PCNT))
    {
        Scope (\Q017)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03F9, PCNT))
                {
                    Return (\Q017.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03FA, PCNT))
    {
        Scope (\Q018)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03FA, PCNT))
                {
                    Return (\Q018.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03FB, PCNT))
    {
        Scope (\Q019)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03FB, PCNT))
                {
                    Return (\Q019.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03FC, PCNT))
    {
        Scope (\Q020)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03FC, PCNT))
                {
                    Return (\Q020.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03FD, PCNT))
    {
        Scope (\Q021)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03FD, PCNT))
                {
                    Return (\Q021.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03FE, PCNT))
    {
        Scope (\Q022)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03FE, PCNT))
                {
                    Return (\Q022.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x03FF, PCNT))
    {
        Scope (\Q023)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x03FF, PCNT))
                {
                    Return (\Q023.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0400, PCNT))
    {
        Scope (\Q024)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0400, PCNT))
                {
                    Return (\Q024.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0401, PCNT))
    {
        Scope (\Q025)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0401, PCNT))
                {
                    Return (\Q025.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0402, PCNT))
    {
        Scope (\Q026)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0402, PCNT))
                {
                    Return (\Q026.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0403, PCNT))
    {
        Scope (\Q027)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0403, PCNT))
                {
                    Return (\Q027.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0404, PCNT))
    {
        Scope (\Q028)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0404, PCNT))
                {
                    Return (\Q028.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0405, PCNT))
    {
        Scope (\Q029)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0405, PCNT))
                {
                    Return (\Q029.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0406, PCNT))
    {
        Scope (\Q030)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0406, PCNT))
                {
                    Return (\Q030.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0407, PCNT))
    {
        Scope (\Q031)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0407, PCNT))
                {
                    Return (\Q031.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0408, PCNT))
    {
        Scope (\Q032)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0408, PCNT))
                {
                    Return (\Q032.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0409, PCNT))
    {
        Scope (\Q033)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0409, PCNT))
                {
                    Return (\Q033.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x040A, PCNT))
    {
        Scope (\Q034)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x040A, PCNT))
                {
                    Return (\Q034.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x040B, PCNT))
    {
        Scope (\Q035)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x040B, PCNT))
                {
                    Return (\Q035.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x040C, PCNT))
    {
        Scope (\Q036)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x040C, PCNT))
                {
                    Return (\Q036.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x040D, PCNT))
    {
        Scope (\Q037)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x040D, PCNT))
                {
                    Return (\Q037.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x040E, PCNT))
    {
        Scope (\Q038)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x040E, PCNT))
                {
                    Return (\Q038.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x040F, PCNT))
    {
        Scope (\Q039)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x040F, PCNT))
                {
                    Return (\Q039.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0410, PCNT))
    {
        Scope (\Q040)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0410, PCNT))
                {
                    Return (\Q040.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0411, PCNT))
    {
        Scope (\Q041)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0411, PCNT))
                {
                    Return (\Q041.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0412, PCNT))
    {
        Scope (\Q042)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0412, PCNT))
                {
                    Return (\Q042.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0413, PCNT))
    {
        Scope (\Q043)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0413, PCNT))
                {
                    Return (\Q043.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0414, PCNT))
    {
        Scope (\Q044)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0414, PCNT))
                {
                    Return (\Q044.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0415, PCNT))
    {
        Scope (\Q045)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0415, PCNT))
                {
                    Return (\Q045.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0416, PCNT))
    {
        Scope (\Q046)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0416, PCNT))
                {
                    Return (\Q046.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0417, PCNT))
    {
        Scope (\Q047)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0417, PCNT))
                {
                    Return (\Q047.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0418, PCNT))
    {
        Scope (\Q048)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0418, PCNT))
                {
                    Return (\Q048.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0419, PCNT))
    {
        Scope (\Q049)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0419, PCNT))
                {
                    Return (\Q049.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x041A, PCNT))
    {
        Scope (\Q050)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x041A, PCNT))
                {
                    Return (\Q050.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x041B, PCNT))
    {
        Scope (\Q051)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x041B, PCNT))
                {
                    Return (\Q051.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x041C, PCNT))
    {
        Scope (\Q052)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x041C, PCNT))
                {
                    Return (\Q052.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x041D, PCNT))
    {
        Scope (\Q053)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x041D, PCNT))
                {
                    Return (\Q053.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x041E, PCNT))
    {
        Scope (\Q054)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x041E, PCNT))
                {
                    Return (\Q054.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x041F, PCNT))
    {
        Scope (\Q055)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x041F, PCNT))
                {
                    Return (\Q055.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0420, PCNT))
    {
        Scope (\Q056)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0420, PCNT))
                {
                    Return (\Q056.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0421, PCNT))
    {
        Scope (\Q057)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0421, PCNT))
                {
                    Return (\Q057.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0422, PCNT))
    {
        Scope (\Q058)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0422, PCNT))
                {
                    Return (\Q058.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0423, PCNT))
    {
        Scope (\Q059)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0423, PCNT))
                {
                    Return (\Q059.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0424, PCNT))
    {
        Scope (\Q060)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0424, PCNT))
                {
                    Return (\Q060.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0425, PCNT))
    {
        Scope (\Q061)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0425, PCNT))
                {
                    Return (\Q061.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0426, PCNT))
    {
        Scope (\Q062)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0426, PCNT))
                {
                    Return (\Q062.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0427, PCNT))
    {
        Scope (\Q063)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0427, PCNT))
                {
                    Return (\Q063.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0428, PCNT))
    {
        Scope (\Q064)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0428, PCNT))
                {
                    Return (\Q064.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0429, PCNT))
    {
        Scope (\Q065)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0429, PCNT))
                {
                    Return (\Q065.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x042A, PCNT))
    {
        Scope (\Q066)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x042A, PCNT))
                {
                    Return (\Q066.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x042B, PCNT))
    {
        Scope (\Q067)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x042B, PCNT))
                {
                    Return (\Q067.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x042C, PCNT))
    {
        Scope (\Q068)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x042C, PCNT))
                {
                    Return (\Q068.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x042D, PCNT))
    {
        Scope (\Q069)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x042D, PCNT))
                {
                    Return (\Q069.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x042E, PCNT))
    {
        Scope (\Q070)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x042E, PCNT))
                {
                    Return (\Q070.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x042F, PCNT))
    {
        Scope (\Q071)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x042F, PCNT))
                {
                    Return (\Q071.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0430, PCNT))
    {
        Scope (\Q072)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0430, PCNT))
                {
                    Return (\Q072.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0431, PCNT))
    {
        Scope (\Q073)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0431, PCNT))
                {
                    Return (\Q073.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0432, PCNT))
    {
        Scope (\Q074)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0432, PCNT))
                {
                    Return (\Q074.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0433, PCNT))
    {
        Scope (\Q075)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0433, PCNT))
                {
                    Return (\Q075.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0434, PCNT))
    {
        Scope (\Q076)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0434, PCNT))
                {
                    Return (\Q076.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0435, PCNT))
    {
        Scope (\Q077)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0435, PCNT))
                {
                    Return (\Q077.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0436, PCNT))
    {
        Scope (\Q078)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0436, PCNT))
                {
                    Return (\Q078.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0437, PCNT))
    {
        Scope (\Q079)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0437, PCNT))
                {
                    Return (\Q079.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0438, PCNT))
    {
        Scope (\Q080)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0438, PCNT))
                {
                    Return (\Q080.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0439, PCNT))
    {
        Scope (\Q081)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0439, PCNT))
                {
                    Return (\Q081.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x043A, PCNT))
    {
        Scope (\Q082)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x043A, PCNT))
                {
                    Return (\Q082.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x043B, PCNT))
    {
        Scope (\Q083)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x043B, PCNT))
                {
                    Return (\Q083.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x043C, PCNT))
    {
        Scope (\Q084)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x043C, PCNT))
                {
                    Return (\Q084.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x043D, PCNT))
    {
        Scope (\Q085)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x043D, PCNT))
                {
                    Return (\Q085.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x043E, PCNT))
    {
        Scope (\Q086)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x043E, PCNT))
                {
                    Return (\Q086.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x043F, PCNT))
    {
        Scope (\Q087)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x043F, PCNT))
                {
                    Return (\Q087.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0440, PCNT))
    {
        Scope (\Q088)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0440, PCNT))
                {
                    Return (\Q088.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0441, PCNT))
    {
        Scope (\Q089)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0441, PCNT))
                {
                    Return (\Q089.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0442, PCNT))
    {
        Scope (\Q090)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0442, PCNT))
                {
                    Return (\Q090.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0443, PCNT))
    {
        Scope (\Q091)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0443, PCNT))
                {
                    Return (\Q091.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0444, PCNT))
    {
        Scope (\Q092)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0444, PCNT))
                {
                    Return (\Q092.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0445, PCNT))
    {
        Scope (\Q093)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0445, PCNT))
                {
                    Return (\Q093.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0446, PCNT))
    {
        Scope (\Q094)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0446, PCNT))
                {
                    Return (\Q094.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0447, PCNT))
    {
        Scope (\Q095)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0447, PCNT))
                {
                    Return (\Q095.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0448, PCNT))
    {
        Scope (\Q096)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0448, PCNT))
                {
                    Return (\Q096.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0449, PCNT))
    {
        Scope (\Q097)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0449, PCNT))
                {
                    Return (\Q097.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x044A, PCNT))
    {
        Scope (\Q098)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x044A, PCNT))
                {
                    Return (\Q098.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x044B, PCNT))
    {
        Scope (\Q099)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x044B, PCNT))
                {
                    Return (\Q099.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x044C, PCNT))
    {
        Scope (\Q100)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x044C, PCNT))
                {
                    Return (\Q100.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x044D, PCNT))
    {
        Scope (\Q101)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x044D, PCNT))
                {
                    Return (\Q101.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x044E, PCNT))
    {
        Scope (\Q102)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x044E, PCNT))
                {
                    Return (\Q102.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x044F, PCNT))
    {
        Scope (\Q103)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x044F, PCNT))
                {
                    Return (\Q103.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0450, PCNT))
    {
        Scope (\Q104)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0450, PCNT))
                {
                    Return (\Q104.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0451, PCNT))
    {
        Scope (\Q105)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0451, PCNT))
                {
                    Return (\Q105.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0452, PCNT))
    {
        Scope (\Q106)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0452, PCNT))
                {
                    Return (\Q106.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0453, PCNT))
    {
        Scope (\Q107)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0453, PCNT))
                {
                    Return (\Q107.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0454, PCNT))
    {
        Scope (\Q108)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0454, PCNT))
                {
                    Return (\Q108.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0455, PCNT))
    {
        Scope (\Q109)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0455, PCNT))
                {
                    Return (\Q109.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0456, PCNT))
    {
        Scope (\Q110)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0456, PCNT))
                {
                    Return (\Q110.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0457, PCNT))
    {
        Scope (\Q111)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0457, PCNT))
                {
                    Return (\Q111.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0458, PCNT))
    {
        Scope (\Q112)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0458, PCNT))
                {
                    Return (\Q112.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0459, PCNT))
    {
        Scope (\Q113)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0459, PCNT))
                {
                    Return (\Q113.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x045A, PCNT))
    {
        Scope (\Q114)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x045A, PCNT))
                {
                    Return (\Q114.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x045B, PCNT))
    {
        Scope (\Q115)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x045B, PCNT))
                {
                    Return (\Q115.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x045C, PCNT))
    {
        Scope (\Q116)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x045C, PCNT))
                {
                    Return (\Q116.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x045D, PCNT))
    {
        Scope (\Q117)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x045D, PCNT))
                {
                    Return (\Q117.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x045E, PCNT))
    {
        Scope (\Q118)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x045E, PCNT))
                {
                    Return (\Q118.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x045F, PCNT))
    {
        Scope (\Q119)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x045F, PCNT))
                {
                    Return (\Q119.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0460, PCNT))
    {
        Scope (\Q120)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0460, PCNT))
                {
                    Return (\Q120.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0461, PCNT))
    {
        Scope (\Q121)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0461, PCNT))
                {
                    Return (\Q121.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0462, PCNT))
    {
        Scope (\Q122)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0462, PCNT))
                {
                    Return (\Q122.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0463, PCNT))
    {
        Scope (\Q123)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0463, PCNT))
                {
                    Return (\Q123.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0464, PCNT))
    {
        Scope (\Q124)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0464, PCNT))
                {
                    Return (\Q124.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0465, PCNT))
    {
        Scope (\Q125)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0465, PCNT))
                {
                    Return (\Q125.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0466, PCNT))
    {
        Scope (\Q126)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0466, PCNT))
                {
                    Return (\Q126.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0467, PCNT))
    {
        Scope (\Q127)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0467, PCNT))
                {
                    Return (\Q127.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0468, PCNT))
    {
        Scope (\Q128)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0468, PCNT))
                {
                    Return (\Q128.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0469, PCNT))
    {
        Scope (\Q129)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0469, PCNT))
                {
                    Return (\Q129.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x046A, PCNT))
    {
        Scope (\Q130)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x046A, PCNT))
                {
                    Return (\Q130.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x046B, PCNT))
    {
        Scope (\Q131)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x046B, PCNT))
                {
                    Return (\Q131.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x046C, PCNT))
    {
        Scope (\Q132)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x046C, PCNT))
                {
                    Return (\Q132.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x046D, PCNT))
    {
        Scope (\Q133)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x046D, PCNT))
                {
                    Return (\Q133.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x046E, PCNT))
    {
        Scope (\Q134)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x046E, PCNT))
                {
                    Return (\Q134.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x046F, PCNT))
    {
        Scope (\Q135)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x046F, PCNT))
                {
                    Return (\Q135.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0470, PCNT))
    {
        Scope (\Q136)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0470, PCNT))
                {
                    Return (\Q136.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0471, PCNT))
    {
        Scope (\Q137)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0471, PCNT))
                {
                    Return (\Q137.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0472, PCNT))
    {
        Scope (\Q138)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0472, PCNT))
                {
                    Return (\Q138.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0473, PCNT))
    {
        Scope (\Q139)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0473, PCNT))
                {
                    Return (\Q139.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0474, PCNT))
    {
        Scope (\Q140)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0474, PCNT))
                {
                    Return (\Q140.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0475, PCNT))
    {
        Scope (\Q141)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0475, PCNT))
                {
                    Return (\Q141.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0476, PCNT))
    {
        Scope (\Q142)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0476, PCNT))
                {
                    Return (\Q142.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0477, PCNT))
    {
        Scope (\Q143)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0477, PCNT))
                {
                    Return (\Q143.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0478, PCNT))
    {
        Scope (\Q144)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0478, PCNT))
                {
                    Return (\Q144.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0479, PCNT))
    {
        Scope (\Q145)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0479, PCNT))
                {
                    Return (\Q145.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x047A, PCNT))
    {
        Scope (\Q146)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x047A, PCNT))
                {
                    Return (\Q146.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x047B, PCNT))
    {
        Scope (\Q147)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x047B, PCNT))
                {
                    Return (\Q147.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x047C, PCNT))
    {
        Scope (\Q148)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x047C, PCNT))
                {
                    Return (\Q148.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x047D, PCNT))
    {
        Scope (\Q149)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x047D, PCNT))
                {
                    Return (\Q149.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x047E, PCNT))
    {
        Scope (\Q150)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x047E, PCNT))
                {
                    Return (\Q150.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x047F, PCNT))
    {
        Scope (\Q151)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x047F, PCNT))
                {
                    Return (\Q151.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0480, PCNT))
    {
        Scope (\Q152)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0480, PCNT))
                {
                    Return (\Q152.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0481, PCNT))
    {
        Scope (\Q153)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0481, PCNT))
                {
                    Return (\Q153.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0482, PCNT))
    {
        Scope (\Q154)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0482, PCNT))
                {
                    Return (\Q154.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0483, PCNT))
    {
        Scope (\Q155)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0483, PCNT))
                {
                    Return (\Q155.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0484, PCNT))
    {
        Scope (\Q156)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0484, PCNT))
                {
                    Return (\Q156.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0485, PCNT))
    {
        Scope (\Q157)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0485, PCNT))
                {
                    Return (\Q157.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0486, PCNT))
    {
        Scope (\Q158)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0486, PCNT))
                {
                    Return (\Q158.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0487, PCNT))
    {
        Scope (\Q159)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0487, PCNT))
                {
                    Return (\Q159.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0488, PCNT))
    {
        Scope (\Q160)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0488, PCNT))
                {
                    Return (\Q160.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0489, PCNT))
    {
        Scope (\Q161)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0489, PCNT))
                {
                    Return (\Q161.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x048A, PCNT))
    {
        Scope (\Q162)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x048A, PCNT))
                {
                    Return (\Q162.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x048B, PCNT))
    {
        Scope (\Q163)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x048B, PCNT))
                {
                    Return (\Q163.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x048C, PCNT))
    {
        Scope (\Q164)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x048C, PCNT))
                {
                    Return (\Q164.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x048D, PCNT))
    {
        Scope (\Q165)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x048D, PCNT))
                {
                    Return (\Q165.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x048E, PCNT))
    {
        Scope (\Q166)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x048E, PCNT))
                {
                    Return (\Q166.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x048F, PCNT))
    {
        Scope (\Q167)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x048F, PCNT))
                {
                    Return (\Q167.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0490, PCNT))
    {
        Scope (\Q168)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0490, PCNT))
                {
                    Return (\Q168.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0491, PCNT))
    {
        Scope (\Q169)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0491, PCNT))
                {
                    Return (\Q169.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0492, PCNT))
    {
        Scope (\Q170)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0492, PCNT))
                {
                    Return (\Q170.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0493, PCNT))
    {
        Scope (\Q171)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0493, PCNT))
                {
                    Return (\Q171.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0494, PCNT))
    {
        Scope (\Q172)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0494, PCNT))
                {
                    Return (\Q172.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0495, PCNT))
    {
        Scope (\Q173)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0495, PCNT))
                {
                    Return (\Q173.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0496, PCNT))
    {
        Scope (\Q174)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0496, PCNT))
                {
                    Return (\Q174.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0497, PCNT))
    {
        Scope (\Q175)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0497, PCNT))
                {
                    Return (\Q175.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0498, PCNT))
    {
        Scope (\Q176)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0498, PCNT))
                {
                    Return (\Q176.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0499, PCNT))
    {
        Scope (\Q177)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0499, PCNT))
                {
                    Return (\Q177.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x049A, PCNT))
    {
        Scope (\Q178)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x049A, PCNT))
                {
                    Return (\Q178.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x049B, PCNT))
    {
        Scope (\Q179)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x049B, PCNT))
                {
                    Return (\Q179.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x049C, PCNT))
    {
        Scope (\Q180)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x049C, PCNT))
                {
                    Return (\Q180.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x049D, PCNT))
    {
        Scope (\Q181)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x049D, PCNT))
                {
                    Return (\Q181.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x049E, PCNT))
    {
        Scope (\Q182)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x049E, PCNT))
                {
                    Return (\Q182.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x049F, PCNT))
    {
        Scope (\Q183)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x049F, PCNT))
                {
                    Return (\Q183.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A0, PCNT))
    {
        Scope (\Q184)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A0, PCNT))
                {
                    Return (\Q184.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A1, PCNT))
    {
        Scope (\Q185)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A1, PCNT))
                {
                    Return (\Q185.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A2, PCNT))
    {
        Scope (\Q186)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A2, PCNT))
                {
                    Return (\Q186.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A3, PCNT))
    {
        Scope (\Q187)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A3, PCNT))
                {
                    Return (\Q187.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A4, PCNT))
    {
        Scope (\Q188)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A4, PCNT))
                {
                    Return (\Q188.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A5, PCNT))
    {
        Scope (\Q189)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A5, PCNT))
                {
                    Return (\Q189.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A6, PCNT))
    {
        Scope (\Q190)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A6, PCNT))
                {
                    Return (\Q190.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A7, PCNT))
    {
        Scope (\Q191)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A7, PCNT))
                {
                    Return (\Q191.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A8, PCNT))
    {
        Scope (\Q192)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A8, PCNT))
                {
                    Return (\Q192.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04A9, PCNT))
    {
        Scope (\Q193)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04A9, PCNT))
                {
                    Return (\Q193.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04AA, PCNT))
    {
        Scope (\Q194)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04AA, PCNT))
                {
                    Return (\Q194.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04AB, PCNT))
    {
        Scope (\Q195)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04AB, PCNT))
                {
                    Return (\Q195.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04AC, PCNT))
    {
        Scope (\Q196)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04AC, PCNT))
                {
                    Return (\Q196.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04AD, PCNT))
    {
        Scope (\Q197)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04AD, PCNT))
                {
                    Return (\Q197.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04AE, PCNT))
    {
        Scope (\Q198)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04AE, PCNT))
                {
                    Return (\Q198.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04AF, PCNT))
    {
        Scope (\Q199)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04AF, PCNT))
                {
                    Return (\Q199.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B0, PCNT))
    {
        Scope (\Q200)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B0, PCNT))
                {
                    Return (\Q200.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B1, PCNT))
    {
        Scope (\Q201)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B1, PCNT))
                {
                    Return (\Q201.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B2, PCNT))
    {
        Scope (\Q202)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B2, PCNT))
                {
                    Return (\Q202.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B3, PCNT))
    {
        Scope (\Q203)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B3, PCNT))
                {
                    Return (\Q203.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B4, PCNT))
    {
        Scope (\Q204)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B4, PCNT))
                {
                    Return (\Q204.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B5, PCNT))
    {
        Scope (\Q205)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B5, PCNT))
                {
                    Return (\Q205.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B6, PCNT))
    {
        Scope (\Q206)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B6, PCNT))
                {
                    Return (\Q206.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B7, PCNT))
    {
        Scope (\Q207)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B7, PCNT))
                {
                    Return (\Q207.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B8, PCNT))
    {
        Scope (\Q208)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B8, PCNT))
                {
                    Return (\Q208.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04B9, PCNT))
    {
        Scope (\Q209)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04B9, PCNT))
                {
                    Return (\Q209.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04BA, PCNT))
    {
        Scope (\Q210)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04BA, PCNT))
                {
                    Return (\Q210.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04BB, PCNT))
    {
        Scope (\Q211)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04BB, PCNT))
                {
                    Return (\Q211.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04BC, PCNT))
    {
        Scope (\Q212)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04BC, PCNT))
                {
                    Return (\Q212.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04BD, PCNT))
    {
        Scope (\Q213)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04BD, PCNT))
                {
                    Return (\Q213.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04BE, PCNT))
    {
        Scope (\Q214)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04BE, PCNT))
                {
                    Return (\Q214.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04BF, PCNT))
    {
        Scope (\Q215)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04BF, PCNT))
                {
                    Return (\Q215.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C0, PCNT))
    {
        Scope (\Q216)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C0, PCNT))
                {
                    Return (\Q216.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C1, PCNT))
    {
        Scope (\Q217)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C1, PCNT))
                {
                    Return (\Q217.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C2, PCNT))
    {
        Scope (\Q218)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C2, PCNT))
                {
                    Return (\Q218.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C3, PCNT))
    {
        Scope (\Q219)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C3, PCNT))
                {
                    Return (\Q219.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C4, PCNT))
    {
        Scope (\Q220)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C4, PCNT))
                {
                    Return (\Q220.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C5, PCNT))
    {
        Scope (\Q221)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C5, PCNT))
                {
                    Return (\Q221.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C6, PCNT))
    {
        Scope (\Q222)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C6, PCNT))
                {
                    Return (\Q222.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C7, PCNT))
    {
        Scope (\Q223)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C7, PCNT))
                {
                    Return (\Q223.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C8, PCNT))
    {
        Scope (\Q224)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C8, PCNT))
                {
                    Return (\Q224.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04C9, PCNT))
    {
        Scope (\Q225)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04C9, PCNT))
                {
                    Return (\Q225.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04CA, PCNT))
    {
        Scope (\Q226)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04CA, PCNT))
                {
                    Return (\Q226.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04CB, PCNT))
    {
        Scope (\Q227)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04CB, PCNT))
                {
                    Return (\Q227.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04CC, PCNT))
    {
        Scope (\Q228)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04CC, PCNT))
                {
                    Return (\Q228.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04CD, PCNT))
    {
        Scope (\Q229)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04CD, PCNT))
                {
                    Return (\Q229.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04CE, PCNT))
    {
        Scope (\Q230)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04CE, PCNT))
                {
                    Return (\Q230.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04CF, PCNT))
    {
        Scope (\Q231)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04CF, PCNT))
                {
                    Return (\Q231.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D0, PCNT))
    {
        Scope (\Q232)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D0, PCNT))
                {
                    Return (\Q232.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D1, PCNT))
    {
        Scope (\Q233)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D1, PCNT))
                {
                    Return (\Q233.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D2, PCNT))
    {
        Scope (\Q234)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D2, PCNT))
                {
                    Return (\Q234.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D3, PCNT))
    {
        Scope (\Q235)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D3, PCNT))
                {
                    Return (\Q235.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D4, PCNT))
    {
        Scope (\Q236)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D4, PCNT))
                {
                    Return (\Q236.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D5, PCNT))
    {
        Scope (\Q237)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D5, PCNT))
                {
                    Return (\Q237.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D6, PCNT))
    {
        Scope (\Q238)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D6, PCNT))
                {
                    Return (\Q238.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D7, PCNT))
    {
        Scope (\Q239)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D7, PCNT))
                {
                    Return (\Q239.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D8, PCNT))
    {
        Scope (\Q240)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D8, PCNT))
                {
                    Return (\Q240.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04D9, PCNT))
    {
        Scope (\Q241)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04D9, PCNT))
                {
                    Return (\Q241.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04DA, PCNT))
    {
        Scope (\Q242)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04DA, PCNT))
                {
                    Return (\Q242.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04DB, PCNT))
    {
        Scope (\Q243)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04DB, PCNT))
                {
                    Return (\Q243.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04DC, PCNT))
    {
        Scope (\Q244)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04DC, PCNT))
                {
                    Return (\Q244.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04DD, PCNT))
    {
        Scope (\Q245)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04DD, PCNT))
                {
                    Return (\Q245.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04DE, PCNT))
    {
        Scope (\Q246)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04DE, PCNT))
                {
                    Return (\Q246.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04DF, PCNT))
    {
        Scope (\Q247)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04DF, PCNT))
                {
                    Return (\Q247.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E0, PCNT))
    {
        Scope (\Q248)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E0, PCNT))
                {
                    Return (\Q248.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E1, PCNT))
    {
        Scope (\Q249)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E1, PCNT))
                {
                    Return (\Q249.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E2, PCNT))
    {
        Scope (\Q250)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E2, PCNT))
                {
                    Return (\Q250.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E3, PCNT))
    {
        Scope (\Q251)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E3, PCNT))
                {
                    Return (\Q251.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E4, PCNT))
    {
        Scope (\Q252)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E4, PCNT))
                {
                    Return (\Q252.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E5, PCNT))
    {
        Scope (\Q253)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E5, PCNT))
                {
                    Return (\Q253.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E6, PCNT))
    {
        Scope (\Q254)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E6, PCNT))
                {
                    Return (\Q254.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E7, PCNT))
    {
        Scope (\Q255)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E7, PCNT))
                {
                    Return (\Q255.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E8, PCNT))
    {
        Scope (\Q256)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E8, PCNT))
                {
                    Return (\Q256.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04E9, PCNT))
    {
        Scope (\Q257)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04E9, PCNT))
                {
                    Return (\Q257.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04EA, PCNT))
    {
        Scope (\Q258)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04EA, PCNT))
                {
                    Return (\Q258.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04EB, PCNT))
    {
        Scope (\Q259)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04EB, PCNT))
                {
                    Return (\Q259.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04EC, PCNT))
    {
        Scope (\Q260)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04EC, PCNT))
                {
                    Return (\Q260.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04ED, PCNT))
    {
        Scope (\Q261)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04ED, PCNT))
                {
                    Return (\Q261.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04EE, PCNT))
    {
        Scope (\Q262)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04EE, PCNT))
                {
                    Return (\Q262.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04EF, PCNT))
    {
        Scope (\Q263)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04EF, PCNT))
                {
                    Return (\Q263.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F0, PCNT))
    {
        Scope (\Q264)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F0, PCNT))
                {
                    Return (\Q264.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F1, PCNT))
    {
        Scope (\Q265)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F1, PCNT))
                {
                    Return (\Q265.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F2, PCNT))
    {
        Scope (\Q266)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F2, PCNT))
                {
                    Return (\Q266.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F3, PCNT))
    {
        Scope (\Q267)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F3, PCNT))
                {
                    Return (\Q267.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F4, PCNT))
    {
        Scope (\Q268)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F4, PCNT))
                {
                    Return (\Q268.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F5, PCNT))
    {
        Scope (\Q269)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F5, PCNT))
                {
                    Return (\Q269.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F6, PCNT))
    {
        Scope (\Q270)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F6, PCNT))
                {
                    Return (\Q270.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F7, PCNT))
    {
        Scope (\Q271)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F7, PCNT))
                {
                    Return (\Q271.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F8, PCNT))
    {
        Scope (\Q272)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F8, PCNT))
                {
                    Return (\Q272.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04F9, PCNT))
    {
        Scope (\Q273)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04F9, PCNT))
                {
                    Return (\Q273.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04FA, PCNT))
    {
        Scope (\Q274)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04FA, PCNT))
                {
                    Return (\Q274.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04FB, PCNT))
    {
        Scope (\Q275)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04FB, PCNT))
                {
                    Return (\Q275.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04FC, PCNT))
    {
        Scope (\Q276)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04FC, PCNT))
                {
                    Return (\Q276.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04FD, PCNT))
    {
        Scope (\Q277)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04FD, PCNT))
                {
                    Return (\Q277.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04FE, PCNT))
    {
        Scope (\Q278)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04FE, PCNT))
                {
                    Return (\Q278.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x04FF, PCNT))
    {
        Scope (\Q279)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x04FF, PCNT))
                {
                    Return (\Q279.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0500, PCNT))
    {
        Scope (\Q280)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0500, PCNT))
                {
                    Return (\Q280.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0501, PCNT))
    {
        Scope (\Q281)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0501, PCNT))
                {
                    Return (\Q281.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0502, PCNT))
    {
        Scope (\Q282)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0502, PCNT))
                {
                    Return (\Q282.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0503, PCNT))
    {
        Scope (\Q283)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0503, PCNT))
                {
                    Return (\Q283.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0504, PCNT))
    {
        Scope (\Q284)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0504, PCNT))
                {
                    Return (\Q284.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0505, PCNT))
    {
        Scope (\Q285)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0505, PCNT))
                {
                    Return (\Q285.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0506, PCNT))
    {
        Scope (\Q286)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0506, PCNT))
                {
                    Return (\Q286.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0507, PCNT))
    {
        Scope (\Q287)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0507, PCNT))
                {
                    Return (\Q287.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0508, PCNT))
    {
        Scope (\Q288)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0508, PCNT))
                {
                    Return (\Q288.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0509, PCNT))
    {
        Scope (\Q289)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0509, PCNT))
                {
                    Return (\Q289.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x050A, PCNT))
    {
        Scope (\Q290)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x050A, PCNT))
                {
                    Return (\Q290.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x050B, PCNT))
    {
        Scope (\Q291)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x050B, PCNT))
                {
                    Return (\Q291.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x050C, PCNT))
    {
        Scope (\Q292)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x050C, PCNT))
                {
                    Return (\Q292.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x050D, PCNT))
    {
        Scope (\Q293)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x050D, PCNT))
                {
                    Return (\Q293.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x050E, PCNT))
    {
        Scope (\Q294)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x050E, PCNT))
                {
                    Return (\Q294.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x050F, PCNT))
    {
        Scope (\Q295)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x050F, PCNT))
                {
                    Return (\Q295.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0510, PCNT))
    {
        Scope (\Q296)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0510, PCNT))
                {
                    Return (\Q296.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0511, PCNT))
    {
        Scope (\Q297)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0511, PCNT))
                {
                    Return (\Q297.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0512, PCNT))
    {
        Scope (\Q298)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0512, PCNT))
                {
                    Return (\Q298.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0513, PCNT))
    {
        Scope (\Q299)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0513, PCNT))
                {
                    Return (\Q299.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0514, PCNT))
    {
        Scope (\Q300)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0514, PCNT))
                {
                    Return (\Q300.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0515, PCNT))
    {
        Scope (\Q301)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0515, PCNT))
                {
                    Return (\Q301.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0516, PCNT))
    {
        Scope (\Q302)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0516, PCNT))
                {
                    Return (\Q302.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0517, PCNT))
    {
        Scope (\Q303)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0517, PCNT))
                {
                    Return (\Q303.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0518, PCNT))
    {
        Scope (\Q304)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0518, PCNT))
                {
                    Return (\Q304.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0519, PCNT))
    {
        Scope (\Q305)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0519, PCNT))
                {
                    Return (\Q305.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x051A, PCNT))
    {
        Scope (\Q306)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x051A, PCNT))
                {
                    Return (\Q306.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x051B, PCNT))
    {
        Scope (\Q307)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x051B, PCNT))
                {
                    Return (\Q307.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x051C, PCNT))
    {
        Scope (\Q308)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x051C, PCNT))
                {
                    Return (\Q308.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x051D, PCNT))
    {
        Scope (\Q309)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x051D, PCNT))
                {
                    Return (\Q309.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x051E, PCNT))
    {
        Scope (\Q310)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x051E, PCNT))
                {
                    Return (\Q310.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x051F, PCNT))
    {
        Scope (\Q311)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x051F, PCNT))
                {
                    Return (\Q311.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0520, PCNT))
    {
        Scope (\Q312)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0520, PCNT))
                {
                    Return (\Q312.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0521, PCNT))
    {
        Scope (\Q313)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0521, PCNT))
                {
                    Return (\Q313.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0522, PCNT))
    {
        Scope (\Q314)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0522, PCNT))
                {
                    Return (\Q314.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0523, PCNT))
    {
        Scope (\Q315)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0523, PCNT))
                {
                    Return (\Q315.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0524, PCNT))
    {
        Scope (\Q316)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0524, PCNT))
                {
                    Return (\Q316.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0525, PCNT))
    {
        Scope (\Q317)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0525, PCNT))
                {
                    Return (\Q317.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0526, PCNT))
    {
        Scope (\Q318)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0526, PCNT))
                {
                    Return (\Q318.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0527, PCNT))
    {
        Scope (\Q319)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0527, PCNT))
                {
                    Return (\Q319.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0528, PCNT))
    {
        Scope (\Q320)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0528, PCNT))
                {
                    Return (\Q320.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0529, PCNT))
    {
        Scope (\Q321)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0529, PCNT))
                {
                    Return (\Q321.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x052A, PCNT))
    {
        Scope (\Q322)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x052A, PCNT))
                {
                    Return (\Q322.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x052B, PCNT))
    {
        Scope (\Q323)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x052B, PCNT))
                {
                    Return (\Q323.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x052C, PCNT))
    {
        Scope (\Q324)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x052C, PCNT))
                {
                    Return (\Q324.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x052D, PCNT))
    {
        Scope (\Q325)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x052D, PCNT))
                {
                    Return (\Q325.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x052E, PCNT))
    {
        Scope (\Q326)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x052E, PCNT))
                {
                    Return (\Q326.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x052F, PCNT))
    {
        Scope (\Q327)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x052F, PCNT))
                {
                    Return (\Q327.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0530, PCNT))
    {
        Scope (\Q328)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0530, PCNT))
                {
                    Return (\Q328.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0531, PCNT))
    {
        Scope (\Q329)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0531, PCNT))
                {
                    Return (\Q329.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0532, PCNT))
    {
        Scope (\Q330)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0532, PCNT))
                {
                    Return (\Q330.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0533, PCNT))
    {
        Scope (\Q331)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0533, PCNT))
                {
                    Return (\Q331.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0534, PCNT))
    {
        Scope (\Q332)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0534, PCNT))
                {
                    Return (\Q332.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0535, PCNT))
    {
        Scope (\Q333)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0535, PCNT))
                {
                    Return (\Q333.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0536, PCNT))
    {
        Scope (\Q334)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0536, PCNT))
                {
                    Return (\Q334.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0537, PCNT))
    {
        Scope (\Q335)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0537, PCNT))
                {
                    Return (\Q335.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0538, PCNT))
    {
        Scope (\Q336)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0538, PCNT))
                {
                    Return (\Q336.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0539, PCNT))
    {
        Scope (\Q337)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0539, PCNT))
                {
                    Return (\Q337.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x053A, PCNT))
    {
        Scope (\Q338)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x053A, PCNT))
                {
                    Return (\Q338.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x053B, PCNT))
    {
        Scope (\Q339)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x053B, PCNT))
                {
                    Return (\Q339.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x053C, PCNT))
    {
        Scope (\Q340)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x053C, PCNT))
                {
                    Return (\Q340.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x053D, PCNT))
    {
        Scope (\Q341)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x053D, PCNT))
                {
                    Return (\Q341.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x053E, PCNT))
    {
        Scope (\Q342)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x053E, PCNT))
                {
                    Return (\Q342.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x053F, PCNT))
    {
        Scope (\Q343)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x053F, PCNT))
                {
                    Return (\Q343.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0540, PCNT))
    {
        Scope (\Q344)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0540, PCNT))
                {
                    Return (\Q344.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0541, PCNT))
    {
        Scope (\Q345)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0541, PCNT))
                {
                    Return (\Q345.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0542, PCNT))
    {
        Scope (\Q346)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0542, PCNT))
                {
                    Return (\Q346.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0543, PCNT))
    {
        Scope (\Q347)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0543, PCNT))
                {
                    Return (\Q347.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0544, PCNT))
    {
        Scope (\Q348)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0544, PCNT))
                {
                    Return (\Q348.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0545, PCNT))
    {
        Scope (\Q349)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0545, PCNT))
                {
                    Return (\Q349.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0546, PCNT))
    {
        Scope (\Q350)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0546, PCNT))
                {
                    Return (\Q350.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0547, PCNT))
    {
        Scope (\Q351)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0547, PCNT))
                {
                    Return (\Q351.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0548, PCNT))
    {
        Scope (\Q352)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0548, PCNT))
                {
                    Return (\Q352.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0549, PCNT))
    {
        Scope (\Q353)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0549, PCNT))
                {
                    Return (\Q353.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x054A, PCNT))
    {
        Scope (\Q354)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x054A, PCNT))
                {
                    Return (\Q354.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x054B, PCNT))
    {
        Scope (\Q355)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x054B, PCNT))
                {
                    Return (\Q355.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x054C, PCNT))
    {
        Scope (\Q356)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x054C, PCNT))
                {
                    Return (\Q356.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x054D, PCNT))
    {
        Scope (\Q357)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x054D, PCNT))
                {
                    Return (\Q357.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x054E, PCNT))
    {
        Scope (\Q358)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x054E, PCNT))
                {
                    Return (\Q358.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x054F, PCNT))
    {
        Scope (\Q359)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x054F, PCNT))
                {
                    Return (\Q359.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0550, PCNT))
    {
        Scope (\Q360)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0550, PCNT))
                {
                    Return (\Q360.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0551, PCNT))
    {
        Scope (\Q361)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0551, PCNT))
                {
                    Return (\Q361.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0552, PCNT))
    {
        Scope (\Q362)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0552, PCNT))
                {
                    Return (\Q362.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0553, PCNT))
    {
        Scope (\Q363)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0553, PCNT))
                {
                    Return (\Q363.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0554, PCNT))
    {
        Scope (\Q364)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0554, PCNT))
                {
                    Return (\Q364.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0555, PCNT))
    {
        Scope (\Q365)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0555, PCNT))
                {
                    Return (\Q365.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0556, PCNT))
    {
        Scope (\Q366)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0556, PCNT))
                {
                    Return (\Q366.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0557, PCNT))
    {
        Scope (\Q367)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0557, PCNT))
                {
                    Return (\Q367.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0558, PCNT))
    {
        Scope (\Q368)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0558, PCNT))
                {
                    Return (\Q368.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0559, PCNT))
    {
        Scope (\Q369)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0559, PCNT))
                {
                    Return (\Q369.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x055A, PCNT))
    {
        Scope (\Q370)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x055A, PCNT))
                {
                    Return (\Q370.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x055B, PCNT))
    {
        Scope (\Q371)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x055B, PCNT))
                {
                    Return (\Q371.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x055C, PCNT))
    {
        Scope (\Q372)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x055C, PCNT))
                {
                    Return (\Q372.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x055D, PCNT))
    {
        Scope (\Q373)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x055D, PCNT))
                {
                    Return (\Q373.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x055E, PCNT))
    {
        Scope (\Q374)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x055E, PCNT))
                {
                    Return (\Q374.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x055F, PCNT))
    {
        Scope (\Q375)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x055F, PCNT))
                {
                    Return (\Q375.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0560, PCNT))
    {
        Scope (\Q376)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0560, PCNT))
                {
                    Return (\Q376.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0561, PCNT))
    {
        Scope (\Q377)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0561, PCNT))
                {
                    Return (\Q377.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0562, PCNT))
    {
        Scope (\Q378)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0562, PCNT))
                {
                    Return (\Q378.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0563, PCNT))
    {
        Scope (\Q379)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0563, PCNT))
                {
                    Return (\Q379.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0564, PCNT))
    {
        Scope (\Q380)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0564, PCNT))
                {
                    Return (\Q380.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0565, PCNT))
    {
        Scope (\Q381)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0565, PCNT))
                {
                    Return (\Q381.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0566, PCNT))
    {
        Scope (\Q382)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0566, PCNT))
                {
                    Return (\Q382.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0567, PCNT))
    {
        Scope (\Q383)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0567, PCNT))
                {
                    Return (\Q383.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0568, PCNT))
    {
        Scope (\Q384)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0568, PCNT))
                {
                    Return (\Q384.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0569, PCNT))
    {
        Scope (\Q385)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0569, PCNT))
                {
                    Return (\Q385.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x056A, PCNT))
    {
        Scope (\Q386)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x056A, PCNT))
                {
                    Return (\Q386.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x056B, PCNT))
    {
        Scope (\Q387)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x056B, PCNT))
                {
                    Return (\Q387.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x056C, PCNT))
    {
        Scope (\Q388)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x056C, PCNT))
                {
                    Return (\Q388.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x056D, PCNT))
    {
        Scope (\Q389)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x056D, PCNT))
                {
                    Return (\Q389.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x056E, PCNT))
    {
        Scope (\Q390)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x056E, PCNT))
                {
                    Return (\Q390.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x056F, PCNT))
    {
        Scope (\Q391)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x056F, PCNT))
                {
                    Return (\Q391.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0570, PCNT))
    {
        Scope (\Q392)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0570, PCNT))
                {
                    Return (\Q392.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0571, PCNT))
    {
        Scope (\Q393)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0571, PCNT))
                {
                    Return (\Q393.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0572, PCNT))
    {
        Scope (\Q394)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0572, PCNT))
                {
                    Return (\Q394.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0573, PCNT))
    {
        Scope (\Q395)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0573, PCNT))
                {
                    Return (\Q395.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0574, PCNT))
    {
        Scope (\Q396)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0574, PCNT))
                {
                    Return (\Q396.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0575, PCNT))
    {
        Scope (\Q397)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0575, PCNT))
                {
                    Return (\Q397.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0576, PCNT))
    {
        Scope (\Q398)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0576, PCNT))
                {
                    Return (\Q398.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0577, PCNT))
    {
        Scope (\Q399)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0577, PCNT))
                {
                    Return (\Q399.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0578, PCNT))
    {
        Scope (\Q400)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0578, PCNT))
                {
                    Return (\Q400.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0579, PCNT))
    {
        Scope (\Q401)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0579, PCNT))
                {
                    Return (\Q401.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x057A, PCNT))
    {
        Scope (\Q402)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x057A, PCNT))
                {
                    Return (\Q402.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x057B, PCNT))
    {
        Scope (\Q403)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x057B, PCNT))
                {
                    Return (\Q403.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x057C, PCNT))
    {
        Scope (\Q404)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x057C, PCNT))
                {
                    Return (\Q404.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x057D, PCNT))
    {
        Scope (\Q405)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x057D, PCNT))
                {
                    Return (\Q405.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x057E, PCNT))
    {
        Scope (\Q406)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x057E, PCNT))
                {
                    Return (\Q406.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x057F, PCNT))
    {
        Scope (\Q407)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x057F, PCNT))
                {
                    Return (\Q407.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0580, PCNT))
    {
        Scope (\Q408)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0580, PCNT))
                {
                    Return (\Q408.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0581, PCNT))
    {
        Scope (\Q409)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0581, PCNT))
                {
                    Return (\Q409.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0582, PCNT))
    {
        Scope (\Q410)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0582, PCNT))
                {
                    Return (\Q410.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0583, PCNT))
    {
        Scope (\Q411)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0583, PCNT))
                {
                    Return (\Q411.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0584, PCNT))
    {
        Scope (\Q412)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0584, PCNT))
                {
                    Return (\Q412.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0585, PCNT))
    {
        Scope (\Q413)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0585, PCNT))
                {
                    Return (\Q413.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0586, PCNT))
    {
        Scope (\Q414)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0586, PCNT))
                {
                    Return (\Q414.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0587, PCNT))
    {
        Scope (\Q415)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0587, PCNT))
                {
                    Return (\Q415.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0588, PCNT))
    {
        Scope (\Q416)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0588, PCNT))
                {
                    Return (\Q416.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0589, PCNT))
    {
        Scope (\Q417)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0589, PCNT))
                {
                    Return (\Q417.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x058A, PCNT))
    {
        Scope (\Q418)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x058A, PCNT))
                {
                    Return (\Q418.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x058B, PCNT))
    {
        Scope (\Q419)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x058B, PCNT))
                {
                    Return (\Q419.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x058C, PCNT))
    {
        Scope (\Q420)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x058C, PCNT))
                {
                    Return (\Q420.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x058D, PCNT))
    {
        Scope (\Q421)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x058D, PCNT))
                {
                    Return (\Q421.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x058E, PCNT))
    {
        Scope (\Q422)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x058E, PCNT))
                {
                    Return (\Q422.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x058F, PCNT))
    {
        Scope (\Q423)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x058F, PCNT))
                {
                    Return (\Q423.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0590, PCNT))
    {
        Scope (\Q424)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0590, PCNT))
                {
                    Return (\Q424.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0591, PCNT))
    {
        Scope (\Q425)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0591, PCNT))
                {
                    Return (\Q425.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0592, PCNT))
    {
        Scope (\Q426)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0592, PCNT))
                {
                    Return (\Q426.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0593, PCNT))
    {
        Scope (\Q427)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0593, PCNT))
                {
                    Return (\Q427.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0594, PCNT))
    {
        Scope (\Q428)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0594, PCNT))
                {
                    Return (\Q428.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0595, PCNT))
    {
        Scope (\Q429)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0595, PCNT))
                {
                    Return (\Q429.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0596, PCNT))
    {
        Scope (\Q430)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0596, PCNT))
                {
                    Return (\Q430.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0597, PCNT))
    {
        Scope (\Q431)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0597, PCNT))
                {
                    Return (\Q431.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0598, PCNT))
    {
        Scope (\Q432)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0598, PCNT))
                {
                    Return (\Q432.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0599, PCNT))
    {
        Scope (\Q433)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0599, PCNT))
                {
                    Return (\Q433.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x059A, PCNT))
    {
        Scope (\Q434)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x059A, PCNT))
                {
                    Return (\Q434.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x059B, PCNT))
    {
        Scope (\Q435)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x059B, PCNT))
                {
                    Return (\Q435.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x059C, PCNT))
    {
        Scope (\Q436)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x059C, PCNT))
                {
                    Return (\Q436.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x059D, PCNT))
    {
        Scope (\Q437)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x059D, PCNT))
                {
                    Return (\Q437.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x059E, PCNT))
    {
        Scope (\Q438)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x059E, PCNT))
                {
                    Return (\Q438.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x059F, PCNT))
    {
        Scope (\Q439)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x059F, PCNT))
                {
                    Return (\Q439.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A0, PCNT))
    {
        Scope (\Q440)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A0, PCNT))
                {
                    Return (\Q440.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A1, PCNT))
    {
        Scope (\Q441)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A1, PCNT))
                {
                    Return (\Q441.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A2, PCNT))
    {
        Scope (\Q442)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A2, PCNT))
                {
                    Return (\Q442.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A3, PCNT))
    {
        Scope (\Q443)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A3, PCNT))
                {
                    Return (\Q443.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A4, PCNT))
    {
        Scope (\Q444)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A4, PCNT))
                {
                    Return (\Q444.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A5, PCNT))
    {
        Scope (\Q445)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A5, PCNT))
                {
                    Return (\Q445.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A6, PCNT))
    {
        Scope (\Q446)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A6, PCNT))
                {
                    Return (\Q446.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A7, PCNT))
    {
        Scope (\Q447)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A7, PCNT))
                {
                    Return (\Q447.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A8, PCNT))
    {
        Scope (\Q448)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A8, PCNT))
                {
                    Return (\Q448.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05A9, PCNT))
    {
        Scope (\Q449)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05A9, PCNT))
                {
                    Return (\Q449.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05AA, PCNT))
    {
        Scope (\Q450)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05AA, PCNT))
                {
                    Return (\Q450.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05AB, PCNT))
    {
        Scope (\Q451)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05AB, PCNT))
                {
                    Return (\Q451.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05AC, PCNT))
    {
        Scope (\Q452)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05AC, PCNT))
                {
                    Return (\Q452.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05AD, PCNT))
    {
        Scope (\Q453)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05AD, PCNT))
                {
                    Return (\Q453.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05AE, PCNT))
    {
        Scope (\Q454)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05AE, PCNT))
                {
                    Return (\Q454.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05AF, PCNT))
    {
        Scope (\Q455)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05AF, PCNT))
                {
                    Return (\Q455.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B0, PCNT))
    {
        Scope (\Q456)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B0, PCNT))
                {
                    Return (\Q456.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B1, PCNT))
    {
        Scope (\Q457)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B1, PCNT))
                {
                    Return (\Q457.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B2, PCNT))
    {
        Scope (\Q458)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B2, PCNT))
                {
                    Return (\Q458.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B3, PCNT))
    {
        Scope (\Q459)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B3, PCNT))
                {
                    Return (\Q459.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B4, PCNT))
    {
        Scope (\Q460)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B4, PCNT))
                {
                    Return (\Q460.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B5, PCNT))
    {
        Scope (\Q461)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B5, PCNT))
                {
                    Return (\Q461.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B6, PCNT))
    {
        Scope (\Q462)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B6, PCNT))
                {
                    Return (\Q462.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B7, PCNT))
    {
        Scope (\Q463)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B7, PCNT))
                {
                    Return (\Q463.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B8, PCNT))
    {
        Scope (\Q464)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B8, PCNT))
                {
                    Return (\Q464.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05B9, PCNT))
    {
        Scope (\Q465)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05B9, PCNT))
                {
                    Return (\Q465.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05BA, PCNT))
    {
        Scope (\Q466)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05BA, PCNT))
                {
                    Return (\Q466.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05BB, PCNT))
    {
        Scope (\Q467)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05BB, PCNT))
                {
                    Return (\Q467.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05BC, PCNT))
    {
        Scope (\Q468)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05BC, PCNT))
                {
                    Return (\Q468.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05BD, PCNT))
    {
        Scope (\Q469)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05BD, PCNT))
                {
                    Return (\Q469.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05BE, PCNT))
    {
        Scope (\Q470)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05BE, PCNT))
                {
                    Return (\Q470.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05BF, PCNT))
    {
        Scope (\Q471)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05BF, PCNT))
                {
                    Return (\Q471.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C0, PCNT))
    {
        Scope (\Q472)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C0, PCNT))
                {
                    Return (\Q472.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C1, PCNT))
    {
        Scope (\Q473)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C1, PCNT))
                {
                    Return (\Q473.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C2, PCNT))
    {
        Scope (\Q474)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C2, PCNT))
                {
                    Return (\Q474.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C3, PCNT))
    {
        Scope (\Q475)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C3, PCNT))
                {
                    Return (\Q475.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C4, PCNT))
    {
        Scope (\Q476)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C4, PCNT))
                {
                    Return (\Q476.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C5, PCNT))
    {
        Scope (\Q477)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C5, PCNT))
                {
                    Return (\Q477.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C6, PCNT))
    {
        Scope (\Q478)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C6, PCNT))
                {
                    Return (\Q478.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C7, PCNT))
    {
        Scope (\Q479)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C7, PCNT))
                {
                    Return (\Q479.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C8, PCNT))
    {
        Scope (\Q480)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C8, PCNT))
                {
                    Return (\Q480.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05C9, PCNT))
    {
        Scope (\Q481)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05C9, PCNT))
                {
                    Return (\Q481.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05CA, PCNT))
    {
        Scope (\Q482)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05CA, PCNT))
                {
                    Return (\Q482.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05CB, PCNT))
    {
        Scope (\Q483)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05CB, PCNT))
                {
                    Return (\Q483.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05CC, PCNT))
    {
        Scope (\Q484)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05CC, PCNT))
                {
                    Return (\Q484.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05CD, PCNT))
    {
        Scope (\Q485)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05CD, PCNT))
                {
                    Return (\Q485.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05CE, PCNT))
    {
        Scope (\Q486)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05CE, PCNT))
                {
                    Return (\Q486.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05CF, PCNT))
    {
        Scope (\Q487)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05CF, PCNT))
                {
                    Return (\Q487.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D0, PCNT))
    {
        Scope (\Q488)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D0, PCNT))
                {
                    Return (\Q488.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D1, PCNT))
    {
        Scope (\Q489)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D1, PCNT))
                {
                    Return (\Q489.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D2, PCNT))
    {
        Scope (\Q490)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D2, PCNT))
                {
                    Return (\Q490.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D3, PCNT))
    {
        Scope (\Q491)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D3, PCNT))
                {
                    Return (\Q491.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D4, PCNT))
    {
        Scope (\Q492)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D4, PCNT))
                {
                    Return (\Q492.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D5, PCNT))
    {
        Scope (\Q493)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D5, PCNT))
                {
                    Return (\Q493.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D6, PCNT))
    {
        Scope (\Q494)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D6, PCNT))
                {
                    Return (\Q494.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D7, PCNT))
    {
        Scope (\Q495)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D7, PCNT))
                {
                    Return (\Q495.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D8, PCNT))
    {
        Scope (\Q496)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D8, PCNT))
                {
                    Return (\Q496.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05D9, PCNT))
    {
        Scope (\Q497)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05D9, PCNT))
                {
                    Return (\Q497.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05DA, PCNT))
    {
        Scope (\Q498)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05DA, PCNT))
                {
                    Return (\Q498.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05DB, PCNT))
    {
        Scope (\Q499)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05DB, PCNT))
                {
                    Return (\Q499.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05DC, PCNT))
    {
        Scope (\Q500)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05DC, PCNT))
                {
                    Return (\Q500.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05DD, PCNT))
    {
        Scope (\Q501)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05DD, PCNT))
                {
                    Return (\Q501.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05DE, PCNT))
    {
        Scope (\Q502)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05DE, PCNT))
                {
                    Return (\Q502.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05DF, PCNT))
    {
        Scope (\Q503)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05DF, PCNT))
                {
                    Return (\Q503.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E0, PCNT))
    {
        Scope (\Q504)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E0, PCNT))
                {
                    Return (\Q504.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E1, PCNT))
    {
        Scope (\Q505)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E1, PCNT))
                {
                    Return (\Q505.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E2, PCNT))
    {
        Scope (\Q506)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E2, PCNT))
                {
                    Return (\Q506.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E3, PCNT))
    {
        Scope (\Q507)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E3, PCNT))
                {
                    Return (\Q507.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E4, PCNT))
    {
        Scope (\Q508)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E4, PCNT))
                {
                    Return (\Q508.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E5, PCNT))
    {
        Scope (\Q509)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E5, PCNT))
                {
                    Return (\Q509.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E6, PCNT))
    {
        Scope (\Q510)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E6, PCNT))
                {
                    Return (\Q510.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E7, PCNT))
    {
        Scope (\Q511)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E7, PCNT))
                {
                    Return (\Q511.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E8, PCNT))
    {
        Scope (\Q512)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E8, PCNT))
                {
                    Return (\Q512.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05E9, PCNT))
    {
        Scope (\Q513)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05E9, PCNT))
                {
                    Return (\Q513.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05EA, PCNT))
    {
        Scope (\Q514)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05EA, PCNT))
                {
                    Return (\Q514.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05EB, PCNT))
    {
        Scope (\Q515)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05EB, PCNT))
                {
                    Return (\Q515.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05EC, PCNT))
    {
        Scope (\Q516)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05EC, PCNT))
                {
                    Return (\Q516.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05ED, PCNT))
    {
        Scope (\Q517)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05ED, PCNT))
                {
                    Return (\Q517.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05EE, PCNT))
    {
        Scope (\Q518)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05EE, PCNT))
                {
                    Return (\Q518.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05EF, PCNT))
    {
        Scope (\Q519)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05EF, PCNT))
                {
                    Return (\Q519.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F0, PCNT))
    {
        Scope (\Q520)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F0, PCNT))
                {
                    Return (\Q520.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F1, PCNT))
    {
        Scope (\Q521)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F1, PCNT))
                {
                    Return (\Q521.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F2, PCNT))
    {
        Scope (\Q522)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F2, PCNT))
                {
                    Return (\Q522.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F3, PCNT))
    {
        Scope (\Q523)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F3, PCNT))
                {
                    Return (\Q523.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F4, PCNT))
    {
        Scope (\Q524)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F4, PCNT))
                {
                    Return (\Q524.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F5, PCNT))
    {
        Scope (\Q525)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F5, PCNT))
                {
                    Return (\Q525.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F6, PCNT))
    {
        Scope (\Q526)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F6, PCNT))
                {
                    Return (\Q526.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F7, PCNT))
    {
        Scope (\Q527)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F7, PCNT))
                {
                    Return (\Q527.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F8, PCNT))
    {
        Scope (\Q528)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F8, PCNT))
                {
                    Return (\Q528.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05F9, PCNT))
    {
        Scope (\Q529)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05F9, PCNT))
                {
                    Return (\Q529.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05FA, PCNT))
    {
        Scope (\Q530)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05FA, PCNT))
                {
                    Return (\Q530.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05FB, PCNT))
    {
        Scope (\Q531)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05FB, PCNT))
                {
                    Return (\Q531.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05FC, PCNT))
    {
        Scope (\Q532)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05FC, PCNT))
                {
                    Return (\Q532.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05FD, PCNT))
    {
        Scope (\Q533)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05FD, PCNT))
                {
                    Return (\Q533.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05FE, PCNT))
    {
        Scope (\Q534)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05FE, PCNT))
                {
                    Return (\Q534.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x05FF, PCNT))
    {
        Scope (\Q535)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x05FF, PCNT))
                {
                    Return (\Q535.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0600, PCNT))
    {
        Scope (\Q536)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0600, PCNT))
                {
                    Return (\Q536.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0601, PCNT))
    {
        Scope (\Q537)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0601, PCNT))
                {
                    Return (\Q537.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0602, PCNT))
    {
        Scope (\Q538)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0602, PCNT))
                {
                    Return (\Q538.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0603, PCNT))
    {
        Scope (\Q539)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0603, PCNT))
                {
                    Return (\Q539.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0604, PCNT))
    {
        Scope (\Q540)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0604, PCNT))
                {
                    Return (\Q540.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0605, PCNT))
    {
        Scope (\Q541)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0605, PCNT))
                {
                    Return (\Q541.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0606, PCNT))
    {
        Scope (\Q542)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0606, PCNT))
                {
                    Return (\Q542.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0607, PCNT))
    {
        Scope (\Q543)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0607, PCNT))
                {
                    Return (\Q543.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0608, PCNT))
    {
        Scope (\Q544)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0608, PCNT))
                {
                    Return (\Q544.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0609, PCNT))
    {
        Scope (\Q545)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0609, PCNT))
                {
                    Return (\Q545.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x060A, PCNT))
    {
        Scope (\Q546)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x060A, PCNT))
                {
                    Return (\Q546.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x060B, PCNT))
    {
        Scope (\Q547)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x060B, PCNT))
                {
                    Return (\Q547.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x060C, PCNT))
    {
        Scope (\Q548)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x060C, PCNT))
                {
                    Return (\Q548.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x060D, PCNT))
    {
        Scope (\Q549)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x060D, PCNT))
                {
                    Return (\Q549.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x060E, PCNT))
    {
        Scope (\Q550)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x060E, PCNT))
                {
                    Return (\Q550.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x060F, PCNT))
    {
        Scope (\Q551)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x060F, PCNT))
                {
                    Return (\Q551.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0610, PCNT))
    {
        Scope (\Q552)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0610, PCNT))
                {
                    Return (\Q552.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0611, PCNT))
    {
        Scope (\Q553)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0611, PCNT))
                {
                    Return (\Q553.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0612, PCNT))
    {
        Scope (\Q554)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0612, PCNT))
                {
                    Return (\Q554.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0613, PCNT))
    {
        Scope (\Q555)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0613, PCNT))
                {
                    Return (\Q555.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0614, PCNT))
    {
        Scope (\Q556)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0614, PCNT))
                {
                    Return (\Q556.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0615, PCNT))
    {
        Scope (\Q557)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0615, PCNT))
                {
                    Return (\Q557.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0616, PCNT))
    {
        Scope (\Q558)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0616, PCNT))
                {
                    Return (\Q558.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0617, PCNT))
    {
        Scope (\Q559)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0617, PCNT))
                {
                    Return (\Q559.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0618, PCNT))
    {
        Scope (\Q560)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0618, PCNT))
                {
                    Return (\Q560.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0619, PCNT))
    {
        Scope (\Q561)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0619, PCNT))
                {
                    Return (\Q561.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x061A, PCNT))
    {
        Scope (\Q562)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x061A, PCNT))
                {
                    Return (\Q562.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x061B, PCNT))
    {
        Scope (\Q563)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x061B, PCNT))
                {
                    Return (\Q563.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x061C, PCNT))
    {
        Scope (\Q564)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x061C, PCNT))
                {
                    Return (\Q564.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x061D, PCNT))
    {
        Scope (\Q565)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x061D, PCNT))
                {
                    Return (\Q565.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x061E, PCNT))
    {
        Scope (\Q566)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x061E, PCNT))
                {
                    Return (\Q566.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x061F, PCNT))
    {
        Scope (\Q567)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x061F, PCNT))
                {
                    Return (\Q567.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0620, PCNT))
    {
        Scope (\Q568)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0620, PCNT))
                {
                    Return (\Q568.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0621, PCNT))
    {
        Scope (\Q569)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0621, PCNT))
                {
                    Return (\Q569.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0622, PCNT))
    {
        Scope (\Q570)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0622, PCNT))
                {
                    Return (\Q570.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0623, PCNT))
    {
        Scope (\Q571)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0623, PCNT))
                {
                    Return (\Q571.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0624, PCNT))
    {
        Scope (\Q572)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0624, PCNT))
                {
                    Return (\Q572.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0625, PCNT))
    {
        Scope (\Q573)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0625, PCNT))
                {
                    Return (\Q573.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0626, PCNT))
    {
        Scope (\Q574)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0626, PCNT))
                {
                    Return (\Q574.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0627, PCNT))
    {
        Scope (\Q575)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0627, PCNT))
                {
                    Return (\Q575.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0628, PCNT))
    {
        Scope (\Q576)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0628, PCNT))
                {
                    Return (\Q576.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0629, PCNT))
    {
        Scope (\Q577)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0629, PCNT))
                {
                    Return (\Q577.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x062A, PCNT))
    {
        Scope (\Q578)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x062A, PCNT))
                {
                    Return (\Q578.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x062B, PCNT))
    {
        Scope (\Q579)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x062B, PCNT))
                {
                    Return (\Q579.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x062C, PCNT))
    {
        Scope (\Q580)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x062C, PCNT))
                {
                    Return (\Q580.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x062D, PCNT))
    {
        Scope (\Q581)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x062D, PCNT))
                {
                    Return (\Q581.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x062E, PCNT))
    {
        Scope (\Q582)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x062E, PCNT))
                {
                    Return (\Q582.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x062F, PCNT))
    {
        Scope (\Q583)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x062F, PCNT))
                {
                    Return (\Q583.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0630, PCNT))
    {
        Scope (\Q584)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0630, PCNT))
                {
                    Return (\Q584.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0631, PCNT))
    {
        Scope (\Q585)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0631, PCNT))
                {
                    Return (\Q585.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0632, PCNT))
    {
        Scope (\Q586)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0632, PCNT))
                {
                    Return (\Q586.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0633, PCNT))
    {
        Scope (\Q587)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0633, PCNT))
                {
                    Return (\Q587.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0634, PCNT))
    {
        Scope (\Q588)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0634, PCNT))
                {
                    Return (\Q588.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0635, PCNT))
    {
        Scope (\Q589)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0635, PCNT))
                {
                    Return (\Q589.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0636, PCNT))
    {
        Scope (\Q590)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0636, PCNT))
                {
                    Return (\Q590.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0637, PCNT))
    {
        Scope (\Q591)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0637, PCNT))
                {
                    Return (\Q591.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0638, PCNT))
    {
        Scope (\Q592)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0638, PCNT))
                {
                    Return (\Q592.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0639, PCNT))
    {
        Scope (\Q593)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0639, PCNT))
                {
                    Return (\Q593.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x063A, PCNT))
    {
        Scope (\Q594)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x063A, PCNT))
                {
                    Return (\Q594.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x063B, PCNT))
    {
        Scope (\Q595)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x063B, PCNT))
                {
                    Return (\Q595.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x063C, PCNT))
    {
        Scope (\Q596)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x063C, PCNT))
                {
                    Return (\Q596.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x063D, PCNT))
    {
        Scope (\Q597)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x063D, PCNT))
                {
                    Return (\Q597.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x063E, PCNT))
    {
        Scope (\Q598)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x063E, PCNT))
                {
                    Return (\Q598.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x063F, PCNT))
    {
        Scope (\Q599)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x063F, PCNT))
                {
                    Return (\Q599.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0640, PCNT))
    {
        Scope (\Q600)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0640, PCNT))
                {
                    Return (\Q600.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0641, PCNT))
    {
        Scope (\Q601)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0641, PCNT))
                {
                    Return (\Q601.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0642, PCNT))
    {
        Scope (\Q602)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0642, PCNT))
                {
                    Return (\Q602.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0643, PCNT))
    {
        Scope (\Q603)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0643, PCNT))
                {
                    Return (\Q603.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0644, PCNT))
    {
        Scope (\Q604)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0644, PCNT))
                {
                    Return (\Q604.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0645, PCNT))
    {
        Scope (\Q605)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0645, PCNT))
                {
                    Return (\Q605.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0646, PCNT))
    {
        Scope (\Q606)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0646, PCNT))
                {
                    Return (\Q606.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0647, PCNT))
    {
        Scope (\Q607)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0647, PCNT))
                {
                    Return (\Q607.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0648, PCNT))
    {
        Scope (\Q608)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0648, PCNT))
                {
                    Return (\Q608.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0649, PCNT))
    {
        Scope (\Q609)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0649, PCNT))
                {
                    Return (\Q609.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x064A, PCNT))
    {
        Scope (\Q610)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x064A, PCNT))
                {
                    Return (\Q610.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x064B, PCNT))
    {
        Scope (\Q611)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x064B, PCNT))
                {
                    Return (\Q611.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x064C, PCNT))
    {
        Scope (\Q612)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x064C, PCNT))
                {
                    Return (\Q612.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x064D, PCNT))
    {
        Scope (\Q613)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x064D, PCNT))
                {
                    Return (\Q613.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x064E, PCNT))
    {
        Scope (\Q614)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x064E, PCNT))
                {
                    Return (\Q614.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x064F, PCNT))
    {
        Scope (\Q615)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x064F, PCNT))
                {
                    Return (\Q615.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0650, PCNT))
    {
        Scope (\Q616)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0650, PCNT))
                {
                    Return (\Q616.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0651, PCNT))
    {
        Scope (\Q617)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0651, PCNT))
                {
                    Return (\Q617.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0652, PCNT))
    {
        Scope (\Q618)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0652, PCNT))
                {
                    Return (\Q618.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0653, PCNT))
    {
        Scope (\Q619)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0653, PCNT))
                {
                    Return (\Q619.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0654, PCNT))
    {
        Scope (\Q620)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0654, PCNT))
                {
                    Return (\Q620.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0655, PCNT))
    {
        Scope (\Q621)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0655, PCNT))
                {
                    Return (\Q621.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0656, PCNT))
    {
        Scope (\Q622)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0656, PCNT))
                {
                    Return (\Q622.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0657, PCNT))
    {
        Scope (\Q623)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0657, PCNT))
                {
                    Return (\Q623.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0658, PCNT))
    {
        Scope (\Q624)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0658, PCNT))
                {
                    Return (\Q624.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0659, PCNT))
    {
        Scope (\Q625)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0659, PCNT))
                {
                    Return (\Q625.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x065A, PCNT))
    {
        Scope (\Q626)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x065A, PCNT))
                {
                    Return (\Q626.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x065B, PCNT))
    {
        Scope (\Q627)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x065B, PCNT))
                {
                    Return (\Q627.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x065C, PCNT))
    {
        Scope (\Q628)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x065C, PCNT))
                {
                    Return (\Q628.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x065D, PCNT))
    {
        Scope (\Q629)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x065D, PCNT))
                {
                    Return (\Q629.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x065E, PCNT))
    {
        Scope (\Q630)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x065E, PCNT))
                {
                    Return (\Q630.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x065F, PCNT))
    {
        Scope (\Q631)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x065F, PCNT))
                {
                    Return (\Q631.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0660, PCNT))
    {
        Scope (\Q632)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0660, PCNT))
                {
                    Return (\Q632.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0661, PCNT))
    {
        Scope (\Q633)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0661, PCNT))
                {
                    Return (\Q633.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0662, PCNT))
    {
        Scope (\Q634)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0662, PCNT))
                {
                    Return (\Q634.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0663, PCNT))
    {
        Scope (\Q635)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0663, PCNT))
                {
                    Return (\Q635.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0664, PCNT))
    {
        Scope (\Q636)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0664, PCNT))
                {
                    Return (\Q636.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0665, PCNT))
    {
        Scope (\Q637)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0665, PCNT))
                {
                    Return (\Q637.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0666, PCNT))
    {
        Scope (\Q638)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0666, PCNT))
                {
                    Return (\Q638.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0667, PCNT))
    {
        Scope (\Q639)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0667, PCNT))
                {
                    Return (\Q639.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0668, PCNT))
    {
        Scope (\Q640)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0668, PCNT))
                {
                    Return (\Q640.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0669, PCNT))
    {
        Scope (\Q641)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0669, PCNT))
                {
                    Return (\Q641.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x066A, PCNT))
    {
        Scope (\Q642)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x066A, PCNT))
                {
                    Return (\Q642.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x066B, PCNT))
    {
        Scope (\Q643)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x066B, PCNT))
                {
                    Return (\Q643.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x066C, PCNT))
    {
        Scope (\Q644)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x066C, PCNT))
                {
                    Return (\Q644.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x066D, PCNT))
    {
        Scope (\Q645)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x066D, PCNT))
                {
                    Return (\Q645.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x066E, PCNT))
    {
        Scope (\Q646)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x066E, PCNT))
                {
                    Return (\Q646.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x066F, PCNT))
    {
        Scope (\Q647)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x066F, PCNT))
                {
                    Return (\Q647.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0670, PCNT))
    {
        Scope (\Q648)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0670, PCNT))
                {
                    Return (\Q648.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0671, PCNT))
    {
        Scope (\Q649)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0671, PCNT))
                {
                    Return (\Q649.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0672, PCNT))
    {
        Scope (\Q650)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0672, PCNT))
                {
                    Return (\Q650.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0673, PCNT))
    {
        Scope (\Q651)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0673, PCNT))
                {
                    Return (\Q651.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0674, PCNT))
    {
        Scope (\Q652)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0674, PCNT))
                {
                    Return (\Q652.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0675, PCNT))
    {
        Scope (\Q653)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0675, PCNT))
                {
                    Return (\Q653.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0676, PCNT))
    {
        Scope (\Q654)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0676, PCNT))
                {
                    Return (\Q654.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0677, PCNT))
    {
        Scope (\Q655)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0677, PCNT))
                {
                    Return (\Q655.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0678, PCNT))
    {
        Scope (\Q656)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0678, PCNT))
                {
                    Return (\Q656.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0679, PCNT))
    {
        Scope (\Q657)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0679, PCNT))
                {
                    Return (\Q657.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x067A, PCNT))
    {
        Scope (\Q658)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x067A, PCNT))
                {
                    Return (\Q658.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x067B, PCNT))
    {
        Scope (\Q659)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x067B, PCNT))
                {
                    Return (\Q659.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x067C, PCNT))
    {
        Scope (\Q660)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x067C, PCNT))
                {
                    Return (\Q660.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x067D, PCNT))
    {
        Scope (\Q661)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x067D, PCNT))
                {
                    Return (\Q661.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x067E, PCNT))
    {
        Scope (\Q662)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x067E, PCNT))
                {
                    Return (\Q662.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x067F, PCNT))
    {
        Scope (\Q663)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x067F, PCNT))
                {
                    Return (\Q663.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0680, PCNT))
    {
        Scope (\Q664)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0680, PCNT))
                {
                    Return (\Q664.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0681, PCNT))
    {
        Scope (\Q665)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0681, PCNT))
                {
                    Return (\Q665.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0682, PCNT))
    {
        Scope (\Q666)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0682, PCNT))
                {
                    Return (\Q666.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0683, PCNT))
    {
        Scope (\Q667)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0683, PCNT))
                {
                    Return (\Q667.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0684, PCNT))
    {
        Scope (\Q668)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0684, PCNT))
                {
                    Return (\Q668.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0685, PCNT))
    {
        Scope (\Q669)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0685, PCNT))
                {
                    Return (\Q669.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0686, PCNT))
    {
        Scope (\Q670)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0686, PCNT))
                {
                    Return (\Q670.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0687, PCNT))
    {
        Scope (\Q671)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0687, PCNT))
                {
                    Return (\Q671.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0688, PCNT))
    {
        Scope (\Q672)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0688, PCNT))
                {
                    Return (\Q672.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0689, PCNT))
    {
        Scope (\Q673)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0689, PCNT))
                {
                    Return (\Q673.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x068A, PCNT))
    {
        Scope (\Q674)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x068A, PCNT))
                {
                    Return (\Q674.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x068B, PCNT))
    {
        Scope (\Q675)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x068B, PCNT))
                {
                    Return (\Q675.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x068C, PCNT))
    {
        Scope (\Q676)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x068C, PCNT))
                {
                    Return (\Q676.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x068D, PCNT))
    {
        Scope (\Q677)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x068D, PCNT))
                {
                    Return (\Q677.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x068E, PCNT))
    {
        Scope (\Q678)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x068E, PCNT))
                {
                    Return (\Q678.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x068F, PCNT))
    {
        Scope (\Q679)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x068F, PCNT))
                {
                    Return (\Q679.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0690, PCNT))
    {
        Scope (\Q680)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0690, PCNT))
                {
                    Return (\Q680.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0691, PCNT))
    {
        Scope (\Q681)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0691, PCNT))
                {
                    Return (\Q681.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0692, PCNT))
    {
        Scope (\Q682)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0692, PCNT))
                {
                    Return (\Q682.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0693, PCNT))
    {
        Scope (\Q683)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0693, PCNT))
                {
                    Return (\Q683.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0694, PCNT))
    {
        Scope (\Q684)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0694, PCNT))
                {
                    Return (\Q684.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0695, PCNT))
    {
        Scope (\Q685)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0695, PCNT))
                {
                    Return (\Q685.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0696, PCNT))
    {
        Scope (\Q686)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0696, PCNT))
                {
                    Return (\Q686.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0697, PCNT))
    {
        Scope (\Q687)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0697, PCNT))
                {
                    Return (\Q687.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0698, PCNT))
    {
        Scope (\Q688)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0698, PCNT))
                {
                    Return (\Q688.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0699, PCNT))
    {
        Scope (\Q689)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0699, PCNT))
                {
                    Return (\Q689.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x069A, PCNT))
    {
        Scope (\Q690)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x069A, PCNT))
                {
                    Return (\Q690.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x069B, PCNT))
    {
        Scope (\Q691)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x069B, PCNT))
                {
                    Return (\Q691.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x069C, PCNT))
    {
        Scope (\Q692)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x069C, PCNT))
                {
                    Return (\Q692.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x069D, PCNT))
    {
        Scope (\Q693)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x069D, PCNT))
                {
                    Return (\Q693.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x069E, PCNT))
    {
        Scope (\Q694)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x069E, PCNT))
                {
                    Return (\Q694.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x069F, PCNT))
    {
        Scope (\Q695)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x069F, PCNT))
                {
                    Return (\Q695.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A0, PCNT))
    {
        Scope (\Q696)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A0, PCNT))
                {
                    Return (\Q696.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A1, PCNT))
    {
        Scope (\Q697)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A1, PCNT))
                {
                    Return (\Q697.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A2, PCNT))
    {
        Scope (\Q698)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A2, PCNT))
                {
                    Return (\Q698.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A3, PCNT))
    {
        Scope (\Q699)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A3, PCNT))
                {
                    Return (\Q699.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A4, PCNT))
    {
        Scope (\Q700)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A4, PCNT))
                {
                    Return (\Q700.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A5, PCNT))
    {
        Scope (\Q701)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A5, PCNT))
                {
                    Return (\Q701.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A6, PCNT))
    {
        Scope (\Q702)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A6, PCNT))
                {
                    Return (\Q702.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A7, PCNT))
    {
        Scope (\Q703)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A7, PCNT))
                {
                    Return (\Q703.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A8, PCNT))
    {
        Scope (\Q704)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A8, PCNT))
                {
                    Return (\Q704.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06A9, PCNT))
    {
        Scope (\Q705)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06A9, PCNT))
                {
                    Return (\Q705.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06AA, PCNT))
    {
        Scope (\Q706)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06AA, PCNT))
                {
                    Return (\Q706.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06AB, PCNT))
    {
        Scope (\Q707)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06AB, PCNT))
                {
                    Return (\Q707.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06AC, PCNT))
    {
        Scope (\Q708)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06AC, PCNT))
                {
                    Return (\Q708.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06AD, PCNT))
    {
        Scope (\Q709)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06AD, PCNT))
                {
                    Return (\Q709.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06AE, PCNT))
    {
        Scope (\Q710)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06AE, PCNT))
                {
                    Return (\Q710.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06AF, PCNT))
    {
        Scope (\Q711)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06AF, PCNT))
                {
                    Return (\Q711.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B0, PCNT))
    {
        Scope (\Q712)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B0, PCNT))
                {
                    Return (\Q712.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B1, PCNT))
    {
        Scope (\Q713)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B1, PCNT))
                {
                    Return (\Q713.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B2, PCNT))
    {
        Scope (\Q714)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B2, PCNT))
                {
                    Return (\Q714.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B3, PCNT))
    {
        Scope (\Q715)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B3, PCNT))
                {
                    Return (\Q715.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B4, PCNT))
    {
        Scope (\Q716)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B4, PCNT))
                {
                    Return (\Q716.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B5, PCNT))
    {
        Scope (\Q717)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B5, PCNT))
                {
                    Return (\Q717.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B6, PCNT))
    {
        Scope (\Q718)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B6, PCNT))
                {
                    Return (\Q718.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B7, PCNT))
    {
        Scope (\Q719)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B7, PCNT))
                {
                    Return (\Q719.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B8, PCNT))
    {
        Scope (\Q720)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B8, PCNT))
                {
                    Return (\Q720.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06B9, PCNT))
    {
        Scope (\Q721)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06B9, PCNT))
                {
                    Return (\Q721.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06BA, PCNT))
    {
        Scope (\Q722)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06BA, PCNT))
                {
                    Return (\Q722.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06BB, PCNT))
    {
        Scope (\Q723)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06BB, PCNT))
                {
                    Return (\Q723.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06BC, PCNT))
    {
        Scope (\Q724)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06BC, PCNT))
                {
                    Return (\Q724.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06BD, PCNT))
    {
        Scope (\Q725)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06BD, PCNT))
                {
                    Return (\Q725.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06BE, PCNT))
    {
        Scope (\Q726)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06BE, PCNT))
                {
                    Return (\Q726.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06BF, PCNT))
    {
        Scope (\Q727)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06BF, PCNT))
                {
                    Return (\Q727.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C0, PCNT))
    {
        Scope (\Q728)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C0, PCNT))
                {
                    Return (\Q728.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C1, PCNT))
    {
        Scope (\Q729)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C1, PCNT))
                {
                    Return (\Q729.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C2, PCNT))
    {
        Scope (\Q730)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C2, PCNT))
                {
                    Return (\Q730.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C3, PCNT))
    {
        Scope (\Q731)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C3, PCNT))
                {
                    Return (\Q731.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C4, PCNT))
    {
        Scope (\Q732)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C4, PCNT))
                {
                    Return (\Q732.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C5, PCNT))
    {
        Scope (\Q733)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C5, PCNT))
                {
                    Return (\Q733.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C6, PCNT))
    {
        Scope (\Q734)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C6, PCNT))
                {
                    Return (\Q734.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C7, PCNT))
    {
        Scope (\Q735)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C7, PCNT))
                {
                    Return (\Q735.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C8, PCNT))
    {
        Scope (\Q736)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C8, PCNT))
                {
                    Return (\Q736.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06C9, PCNT))
    {
        Scope (\Q737)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06C9, PCNT))
                {
                    Return (\Q737.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06CA, PCNT))
    {
        Scope (\Q738)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06CA, PCNT))
                {
                    Return (\Q738.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06CB, PCNT))
    {
        Scope (\Q739)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06CB, PCNT))
                {
                    Return (\Q739.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06CC, PCNT))
    {
        Scope (\Q740)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06CC, PCNT))
                {
                    Return (\Q740.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06CD, PCNT))
    {
        Scope (\Q741)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06CD, PCNT))
                {
                    Return (\Q741.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06CE, PCNT))
    {
        Scope (\Q742)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06CE, PCNT))
                {
                    Return (\Q742.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06CF, PCNT))
    {
        Scope (\Q743)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06CF, PCNT))
                {
                    Return (\Q743.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D0, PCNT))
    {
        Scope (\Q744)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D0, PCNT))
                {
                    Return (\Q744.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D1, PCNT))
    {
        Scope (\Q745)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D1, PCNT))
                {
                    Return (\Q745.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D2, PCNT))
    {
        Scope (\Q746)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D2, PCNT))
                {
                    Return (\Q746.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D3, PCNT))
    {
        Scope (\Q747)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D3, PCNT))
                {
                    Return (\Q747.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D4, PCNT))
    {
        Scope (\Q748)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D4, PCNT))
                {
                    Return (\Q748.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D5, PCNT))
    {
        Scope (\Q749)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D5, PCNT))
                {
                    Return (\Q749.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D6, PCNT))
    {
        Scope (\Q750)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D6, PCNT))
                {
                    Return (\Q750.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D7, PCNT))
    {
        Scope (\Q751)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D7, PCNT))
                {
                    Return (\Q751.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D8, PCNT))
    {
        Scope (\Q752)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D8, PCNT))
                {
                    Return (\Q752.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06D9, PCNT))
    {
        Scope (\Q753)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06D9, PCNT))
                {
                    Return (\Q753.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06DA, PCNT))
    {
        Scope (\Q754)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06DA, PCNT))
                {
                    Return (\Q754.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06DB, PCNT))
    {
        Scope (\Q755)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06DB, PCNT))
                {
                    Return (\Q755.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06DC, PCNT))
    {
        Scope (\Q756)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06DC, PCNT))
                {
                    Return (\Q756.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06DD, PCNT))
    {
        Scope (\Q757)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06DD, PCNT))
                {
                    Return (\Q757.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06DE, PCNT))
    {
        Scope (\Q758)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06DE, PCNT))
                {
                    Return (\Q758.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06DF, PCNT))
    {
        Scope (\Q759)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06DF, PCNT))
                {
                    Return (\Q759.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E0, PCNT))
    {
        Scope (\Q760)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E0, PCNT))
                {
                    Return (\Q760.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E1, PCNT))
    {
        Scope (\Q761)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E1, PCNT))
                {
                    Return (\Q761.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E2, PCNT))
    {
        Scope (\Q762)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E2, PCNT))
                {
                    Return (\Q762.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E3, PCNT))
    {
        Scope (\Q763)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E3, PCNT))
                {
                    Return (\Q763.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E4, PCNT))
    {
        Scope (\Q764)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E4, PCNT))
                {
                    Return (\Q764.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E5, PCNT))
    {
        Scope (\Q765)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E5, PCNT))
                {
                    Return (\Q765.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E6, PCNT))
    {
        Scope (\Q766)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E6, PCNT))
                {
                    Return (\Q766.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E7, PCNT))
    {
        Scope (\Q767)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E7, PCNT))
                {
                    Return (\Q767.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E8, PCNT))
    {
        Scope (\Q768)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E8, PCNT))
                {
                    Return (\Q768.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06E9, PCNT))
    {
        Scope (\Q769)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06E9, PCNT))
                {
                    Return (\Q769.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06EA, PCNT))
    {
        Scope (\Q770)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06EA, PCNT))
                {
                    Return (\Q770.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06EB, PCNT))
    {
        Scope (\Q771)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06EB, PCNT))
                {
                    Return (\Q771.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06EC, PCNT))
    {
        Scope (\Q772)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06EC, PCNT))
                {
                    Return (\Q772.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06ED, PCNT))
    {
        Scope (\Q773)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06ED, PCNT))
                {
                    Return (\Q773.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06EE, PCNT))
    {
        Scope (\Q774)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06EE, PCNT))
                {
                    Return (\Q774.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06EF, PCNT))
    {
        Scope (\Q775)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06EF, PCNT))
                {
                    Return (\Q775.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F0, PCNT))
    {
        Scope (\Q776)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F0, PCNT))
                {
                    Return (\Q776.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F1, PCNT))
    {
        Scope (\Q777)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F1, PCNT))
                {
                    Return (\Q777.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F2, PCNT))
    {
        Scope (\Q778)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F2, PCNT))
                {
                    Return (\Q778.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F3, PCNT))
    {
        Scope (\Q779)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F3, PCNT))
                {
                    Return (\Q779.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F4, PCNT))
    {
        Scope (\Q780)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F4, PCNT))
                {
                    Return (\Q780.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F5, PCNT))
    {
        Scope (\Q781)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F5, PCNT))
                {
                    Return (\Q781.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F6, PCNT))
    {
        Scope (\Q782)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F6, PCNT))
                {
                    Return (\Q782.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F7, PCNT))
    {
        Scope (\Q783)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F7, PCNT))
                {
                    Return (\Q783.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F8, PCNT))
    {
        Scope (\Q784)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F8, PCNT))
                {
                    Return (\Q784.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06F9, PCNT))
    {
        Scope (\Q785)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06F9, PCNT))
                {
                    Return (\Q785.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06FA, PCNT))
    {
        Scope (\Q786)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06FA, PCNT))
                {
                    Return (\Q786.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06FB, PCNT))
    {
        Scope (\Q787)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06FB, PCNT))
                {
                    Return (\Q787.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06FC, PCNT))
    {
        Scope (\Q788)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06FC, PCNT))
                {
                    Return (\Q788.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06FD, PCNT))
    {
        Scope (\Q789)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06FD, PCNT))
                {
                    Return (\Q789.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06FE, PCNT))
    {
        Scope (\Q790)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06FE, PCNT))
                {
                    Return (\Q790.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x06FF, PCNT))
    {
        Scope (\Q791)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x06FF, PCNT))
                {
                    Return (\Q791.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0700, PCNT))
    {
        Scope (\Q792)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0700, PCNT))
                {
                    Return (\Q792.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0701, PCNT))
    {
        Scope (\Q793)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0701, PCNT))
                {
                    Return (\Q793.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0702, PCNT))
    {
        Scope (\Q794)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0702, PCNT))
                {
                    Return (\Q794.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0703, PCNT))
    {
        Scope (\Q795)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0703, PCNT))
                {
                    Return (\Q795.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0704, PCNT))
    {
        Scope (\Q796)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0704, PCNT))
                {
                    Return (\Q796.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0705, PCNT))
    {
        Scope (\Q797)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0705, PCNT))
                {
                    Return (\Q797.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0706, PCNT))
    {
        Scope (\Q798)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0706, PCNT))
                {
                    Return (\Q798.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0707, PCNT))
    {
        Scope (\Q799)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0707, PCNT))
                {
                    Return (\Q799.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0708, PCNT))
    {
        Scope (\Q800)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0708, PCNT))
                {
                    Return (\Q800.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0709, PCNT))
    {
        Scope (\Q801)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0709, PCNT))
                {
                    Return (\Q801.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x070A, PCNT))
    {
        Scope (\Q802)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x070A, PCNT))
                {
                    Return (\Q802.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x070B, PCNT))
    {
        Scope (\Q803)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x070B, PCNT))
                {
                    Return (\Q803.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x070C, PCNT))
    {
        Scope (\Q804)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x070C, PCNT))
                {
                    Return (\Q804.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x070D, PCNT))
    {
        Scope (\Q805)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x070D, PCNT))
                {
                    Return (\Q805.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x070E, PCNT))
    {
        Scope (\Q806)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x070E, PCNT))
                {
                    Return (\Q806.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x070F, PCNT))
    {
        Scope (\Q807)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x070F, PCNT))
                {
                    Return (\Q807.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0710, PCNT))
    {
        Scope (\Q808)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0710, PCNT))
                {
                    Return (\Q808.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0711, PCNT))
    {
        Scope (\Q809)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0711, PCNT))
                {
                    Return (\Q809.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0712, PCNT))
    {
        Scope (\Q810)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0712, PCNT))
                {
                    Return (\Q810.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0713, PCNT))
    {
        Scope (\Q811)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0713, PCNT))
                {
                    Return (\Q811.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0714, PCNT))
    {
        Scope (\Q812)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0714, PCNT))
                {
                    Return (\Q812.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0715, PCNT))
    {
        Scope (\Q813)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0715, PCNT))
                {
                    Return (\Q813.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0716, PCNT))
    {
        Scope (\Q814)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0716, PCNT))
                {
                    Return (\Q814.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0717, PCNT))
    {
        Scope (\Q815)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0717, PCNT))
                {
                    Return (\Q815.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0718, PCNT))
    {
        Scope (\Q816)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0718, PCNT))
                {
                    Return (\Q816.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0719, PCNT))
    {
        Scope (\Q817)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0719, PCNT))
                {
                    Return (\Q817.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x071A, PCNT))
    {
        Scope (\Q818)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x071A, PCNT))
                {
                    Return (\Q818.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x071B, PCNT))
    {
        Scope (\Q819)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x071B, PCNT))
                {
                    Return (\Q819.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x071C, PCNT))
    {
        Scope (\Q820)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x071C, PCNT))
                {
                    Return (\Q820.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x071D, PCNT))
    {
        Scope (\Q821)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x071D, PCNT))
                {
                    Return (\Q821.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x071E, PCNT))
    {
        Scope (\Q822)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x071E, PCNT))
                {
                    Return (\Q822.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x071F, PCNT))
    {
        Scope (\Q823)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x071F, PCNT))
                {
                    Return (\Q823.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0720, PCNT))
    {
        Scope (\Q824)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0720, PCNT))
                {
                    Return (\Q824.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0721, PCNT))
    {
        Scope (\Q825)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0721, PCNT))
                {
                    Return (\Q825.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0722, PCNT))
    {
        Scope (\Q826)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0722, PCNT))
                {
                    Return (\Q826.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0723, PCNT))
    {
        Scope (\Q827)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0723, PCNT))
                {
                    Return (\Q827.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0724, PCNT))
    {
        Scope (\Q828)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0724, PCNT))
                {
                    Return (\Q828.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0725, PCNT))
    {
        Scope (\Q829)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0725, PCNT))
                {
                    Return (\Q829.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0726, PCNT))
    {
        Scope (\Q830)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0726, PCNT))
                {
                    Return (\Q830.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0727, PCNT))
    {
        Scope (\Q831)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0727, PCNT))
                {
                    Return (\Q831.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0728, PCNT))
    {
        Scope (\Q832)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0728, PCNT))
                {
                    Return (\Q832.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0729, PCNT))
    {
        Scope (\Q833)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0729, PCNT))
                {
                    Return (\Q833.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x072A, PCNT))
    {
        Scope (\Q834)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x072A, PCNT))
                {
                    Return (\Q834.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x072B, PCNT))
    {
        Scope (\Q835)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x072B, PCNT))
                {
                    Return (\Q835.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x072C, PCNT))
    {
        Scope (\Q836)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x072C, PCNT))
                {
                    Return (\Q836.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x072D, PCNT))
    {
        Scope (\Q837)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x072D, PCNT))
                {
                    Return (\Q837.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x072E, PCNT))
    {
        Scope (\Q838)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x072E, PCNT))
                {
                    Return (\Q838.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x072F, PCNT))
    {
        Scope (\Q839)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x072F, PCNT))
                {
                    Return (\Q839.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0730, PCNT))
    {
        Scope (\Q840)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0730, PCNT))
                {
                    Return (\Q840.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0731, PCNT))
    {
        Scope (\Q841)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0731, PCNT))
                {
                    Return (\Q841.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0732, PCNT))
    {
        Scope (\Q842)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0732, PCNT))
                {
                    Return (\Q842.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0733, PCNT))
    {
        Scope (\Q843)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0733, PCNT))
                {
                    Return (\Q843.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0734, PCNT))
    {
        Scope (\Q844)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0734, PCNT))
                {
                    Return (\Q844.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0735, PCNT))
    {
        Scope (\Q845)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0735, PCNT))
                {
                    Return (\Q845.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0736, PCNT))
    {
        Scope (\Q846)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0736, PCNT))
                {
                    Return (\Q846.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0737, PCNT))
    {
        Scope (\Q847)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0737, PCNT))
                {
                    Return (\Q847.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0738, PCNT))
    {
        Scope (\Q848)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0738, PCNT))
                {
                    Return (\Q848.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0739, PCNT))
    {
        Scope (\Q849)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0739, PCNT))
                {
                    Return (\Q849.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x073A, PCNT))
    {
        Scope (\Q850)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x073A, PCNT))
                {
                    Return (\Q850.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x073B, PCNT))
    {
        Scope (\Q851)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x073B, PCNT))
                {
                    Return (\Q851.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x073C, PCNT))
    {
        Scope (\Q852)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x073C, PCNT))
                {
                    Return (\Q852.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x073D, PCNT))
    {
        Scope (\Q853)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x073D, PCNT))
                {
                    Return (\Q853.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x073E, PCNT))
    {
        Scope (\Q854)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x073E, PCNT))
                {
                    Return (\Q854.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x073F, PCNT))
    {
        Scope (\Q855)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x073F, PCNT))
                {
                    Return (\Q855.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0740, PCNT))
    {
        Scope (\Q856)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0740, PCNT))
                {
                    Return (\Q856.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0741, PCNT))
    {
        Scope (\Q857)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0741, PCNT))
                {
                    Return (\Q857.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0742, PCNT))
    {
        Scope (\Q858)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0742, PCNT))
                {
                    Return (\Q858.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0743, PCNT))
    {
        Scope (\Q859)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0743, PCNT))
                {
                    Return (\Q859.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0744, PCNT))
    {
        Scope (\Q860)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0744, PCNT))
                {
                    Return (\Q860.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0745, PCNT))
    {
        Scope (\Q861)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0745, PCNT))
                {
                    Return (\Q861.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0746, PCNT))
    {
        Scope (\Q862)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0746, PCNT))
                {
                    Return (\Q862.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0747, PCNT))
    {
        Scope (\Q863)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0747, PCNT))
                {
                    Return (\Q863.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0748, PCNT))
    {
        Scope (\Q864)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0748, PCNT))
                {
                    Return (\Q864.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0749, PCNT))
    {
        Scope (\Q865)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0749, PCNT))
                {
                    Return (\Q865.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x074A, PCNT))
    {
        Scope (\Q866)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x074A, PCNT))
                {
                    Return (\Q866.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x074B, PCNT))
    {
        Scope (\Q867)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x074B, PCNT))
                {
                    Return (\Q867.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x074C, PCNT))
    {
        Scope (\Q868)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x074C, PCNT))
                {
                    Return (\Q868.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x074D, PCNT))
    {
        Scope (\Q869)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x074D, PCNT))
                {
                    Return (\Q869.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x074E, PCNT))
    {
        Scope (\Q870)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x074E, PCNT))
                {
                    Return (\Q870.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x074F, PCNT))
    {
        Scope (\Q871)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x074F, PCNT))
                {
                    Return (\Q871.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0750, PCNT))
    {
        Scope (\Q872)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0750, PCNT))
                {
                    Return (\Q872.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0751, PCNT))
    {
        Scope (\Q873)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0751, PCNT))
                {
                    Return (\Q873.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0752, PCNT))
    {
        Scope (\Q874)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0752, PCNT))
                {
                    Return (\Q874.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0753, PCNT))
    {
        Scope (\Q875)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0753, PCNT))
                {
                    Return (\Q875.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0754, PCNT))
    {
        Scope (\Q876)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0754, PCNT))
                {
                    Return (\Q876.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0755, PCNT))
    {
        Scope (\Q877)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0755, PCNT))
                {
                    Return (\Q877.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0756, PCNT))
    {
        Scope (\Q878)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0756, PCNT))
                {
                    Return (\Q878.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0757, PCNT))
    {
        Scope (\Q879)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0757, PCNT))
                {
                    Return (\Q879.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0758, PCNT))
    {
        Scope (\Q880)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0758, PCNT))
                {
                    Return (\Q880.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0759, PCNT))
    {
        Scope (\Q881)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0759, PCNT))
                {
                    Return (\Q881.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x075A, PCNT))
    {
        Scope (\Q882)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x075A, PCNT))
                {
                    Return (\Q882.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x075B, PCNT))
    {
        Scope (\Q883)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x075B, PCNT))
                {
                    Return (\Q883.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x075C, PCNT))
    {
        Scope (\Q884)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x075C, PCNT))
                {
                    Return (\Q884.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x075D, PCNT))
    {
        Scope (\Q885)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x075D, PCNT))
                {
                    Return (\Q885.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x075E, PCNT))
    {
        Scope (\Q886)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x075E, PCNT))
                {
                    Return (\Q886.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x075F, PCNT))
    {
        Scope (\Q887)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x075F, PCNT))
                {
                    Return (\Q887.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0760, PCNT))
    {
        Scope (\Q888)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0760, PCNT))
                {
                    Return (\Q888.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0761, PCNT))
    {
        Scope (\Q889)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0761, PCNT))
                {
                    Return (\Q889.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0762, PCNT))
    {
        Scope (\Q890)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0762, PCNT))
                {
                    Return (\Q890.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0763, PCNT))
    {
        Scope (\Q891)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0763, PCNT))
                {
                    Return (\Q891.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0764, PCNT))
    {
        Scope (\Q892)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0764, PCNT))
                {
                    Return (\Q892.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0765, PCNT))
    {
        Scope (\Q893)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0765, PCNT))
                {
                    Return (\Q893.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0766, PCNT))
    {
        Scope (\Q894)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0766, PCNT))
                {
                    Return (\Q894.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0767, PCNT))
    {
        Scope (\Q895)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0767, PCNT))
                {
                    Return (\Q895.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0768, PCNT))
    {
        Scope (\Q896)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0768, PCNT))
                {
                    Return (\Q896.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0769, PCNT))
    {
        Scope (\Q897)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0769, PCNT))
                {
                    Return (\Q897.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x076A, PCNT))
    {
        Scope (\Q898)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x076A, PCNT))
                {
                    Return (\Q898.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x076B, PCNT))
    {
        Scope (\Q899)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x076B, PCNT))
                {
                    Return (\Q899.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x076C, PCNT))
    {
        Scope (\Q900)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x076C, PCNT))
                {
                    Return (\Q900.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x076D, PCNT))
    {
        Scope (\Q901)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x076D, PCNT))
                {
                    Return (\Q901.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x076E, PCNT))
    {
        Scope (\Q902)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x076E, PCNT))
                {
                    Return (\Q902.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x076F, PCNT))
    {
        Scope (\Q903)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x076F, PCNT))
                {
                    Return (\Q903.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0770, PCNT))
    {
        Scope (\Q904)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0770, PCNT))
                {
                    Return (\Q904.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0771, PCNT))
    {
        Scope (\Q905)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0771, PCNT))
                {
                    Return (\Q905.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0772, PCNT))
    {
        Scope (\Q906)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0772, PCNT))
                {
                    Return (\Q906.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0773, PCNT))
    {
        Scope (\Q907)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0773, PCNT))
                {
                    Return (\Q907.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0774, PCNT))
    {
        Scope (\Q908)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0774, PCNT))
                {
                    Return (\Q908.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0775, PCNT))
    {
        Scope (\Q909)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0775, PCNT))
                {
                    Return (\Q909.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0776, PCNT))
    {
        Scope (\Q910)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0776, PCNT))
                {
                    Return (\Q910.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0777, PCNT))
    {
        Scope (\Q911)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0777, PCNT))
                {
                    Return (\Q911.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0778, PCNT))
    {
        Scope (\Q912)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0778, PCNT))
                {
                    Return (\Q912.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0779, PCNT))
    {
        Scope (\Q913)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0779, PCNT))
                {
                    Return (\Q913.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x077A, PCNT))
    {
        Scope (\Q914)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x077A, PCNT))
                {
                    Return (\Q914.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x077B, PCNT))
    {
        Scope (\Q915)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x077B, PCNT))
                {
                    Return (\Q915.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x077C, PCNT))
    {
        Scope (\Q916)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x077C, PCNT))
                {
                    Return (\Q916.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x077D, PCNT))
    {
        Scope (\Q917)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x077D, PCNT))
                {
                    Return (\Q917.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x077E, PCNT))
    {
        Scope (\Q918)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x077E, PCNT))
                {
                    Return (\Q918.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x077F, PCNT))
    {
        Scope (\Q919)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x077F, PCNT))
                {
                    Return (\Q919.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0780, PCNT))
    {
        Scope (\Q920)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0780, PCNT))
                {
                    Return (\Q920.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0781, PCNT))
    {
        Scope (\Q921)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0781, PCNT))
                {
                    Return (\Q921.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0782, PCNT))
    {
        Scope (\Q922)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0782, PCNT))
                {
                    Return (\Q922.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0783, PCNT))
    {
        Scope (\Q923)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0783, PCNT))
                {
                    Return (\Q923.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0784, PCNT))
    {
        Scope (\Q924)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0784, PCNT))
                {
                    Return (\Q924.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0785, PCNT))
    {
        Scope (\Q925)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0785, PCNT))
                {
                    Return (\Q925.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0786, PCNT))
    {
        Scope (\Q926)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0786, PCNT))
                {
                    Return (\Q926.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0787, PCNT))
    {
        Scope (\Q927)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0787, PCNT))
                {
                    Return (\Q927.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0788, PCNT))
    {
        Scope (\Q928)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0788, PCNT))
                {
                    Return (\Q928.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0789, PCNT))
    {
        Scope (\Q929)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0789, PCNT))
                {
                    Return (\Q929.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x078A, PCNT))
    {
        Scope (\Q930)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x078A, PCNT))
                {
                    Return (\Q930.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x078B, PCNT))
    {
        Scope (\Q931)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x078B, PCNT))
                {
                    Return (\Q931.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x078C, PCNT))
    {
        Scope (\Q932)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x078C, PCNT))
                {
                    Return (\Q932.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x078D, PCNT))
    {
        Scope (\Q933)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x078D, PCNT))
                {
                    Return (\Q933.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x078E, PCNT))
    {
        Scope (\Q934)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x078E, PCNT))
                {
                    Return (\Q934.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x078F, PCNT))
    {
        Scope (\Q935)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x078F, PCNT))
                {
                    Return (\Q935.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0790, PCNT))
    {
        Scope (\Q936)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0790, PCNT))
                {
                    Return (\Q936.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0791, PCNT))
    {
        Scope (\Q937)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0791, PCNT))
                {
                    Return (\Q937.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0792, PCNT))
    {
        Scope (\Q938)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0792, PCNT))
                {
                    Return (\Q938.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0793, PCNT))
    {
        Scope (\Q939)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0793, PCNT))
                {
                    Return (\Q939.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0794, PCNT))
    {
        Scope (\Q940)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0794, PCNT))
                {
                    Return (\Q940.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0795, PCNT))
    {
        Scope (\Q941)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0795, PCNT))
                {
                    Return (\Q941.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0796, PCNT))
    {
        Scope (\Q942)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0796, PCNT))
                {
                    Return (\Q942.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0797, PCNT))
    {
        Scope (\Q943)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0797, PCNT))
                {
                    Return (\Q943.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0798, PCNT))
    {
        Scope (\Q944)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0798, PCNT))
                {
                    Return (\Q944.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0799, PCNT))
    {
        Scope (\Q945)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0799, PCNT))
                {
                    Return (\Q945.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x079A, PCNT))
    {
        Scope (\Q946)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x079A, PCNT))
                {
                    Return (\Q946.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x079B, PCNT))
    {
        Scope (\Q947)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x079B, PCNT))
                {
                    Return (\Q947.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x079C, PCNT))
    {
        Scope (\Q948)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x079C, PCNT))
                {
                    Return (\Q948.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x079D, PCNT))
    {
        Scope (\Q949)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x079D, PCNT))
                {
                    Return (\Q949.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x079E, PCNT))
    {
        Scope (\Q950)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x079E, PCNT))
                {
                    Return (\Q950.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x079F, PCNT))
    {
        Scope (\Q951)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x079F, PCNT))
                {
                    Return (\Q951.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A0, PCNT))
    {
        Scope (\Q952)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A0, PCNT))
                {
                    Return (\Q952.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A1, PCNT))
    {
        Scope (\Q953)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A1, PCNT))
                {
                    Return (\Q953.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A2, PCNT))
    {
        Scope (\Q954)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A2, PCNT))
                {
                    Return (\Q954.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A3, PCNT))
    {
        Scope (\Q955)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A3, PCNT))
                {
                    Return (\Q955.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A4, PCNT))
    {
        Scope (\Q956)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A4, PCNT))
                {
                    Return (\Q956.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A5, PCNT))
    {
        Scope (\Q957)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A5, PCNT))
                {
                    Return (\Q957.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A6, PCNT))
    {
        Scope (\Q958)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A6, PCNT))
                {
                    Return (\Q958.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A7, PCNT))
    {
        Scope (\Q959)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A7, PCNT))
                {
                    Return (\Q959.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A8, PCNT))
    {
        Scope (\Q960)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A8, PCNT))
                {
                    Return (\Q960.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07A9, PCNT))
    {
        Scope (\Q961)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07A9, PCNT))
                {
                    Return (\Q961.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07AA, PCNT))
    {
        Scope (\Q962)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07AA, PCNT))
                {
                    Return (\Q962.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07AB, PCNT))
    {
        Scope (\Q963)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07AB, PCNT))
                {
                    Return (\Q963.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07AC, PCNT))
    {
        Scope (\Q964)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07AC, PCNT))
                {
                    Return (\Q964.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07AD, PCNT))
    {
        Scope (\Q965)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07AD, PCNT))
                {
                    Return (\Q965.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07AE, PCNT))
    {
        Scope (\Q966)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07AE, PCNT))
                {
                    Return (\Q966.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07AF, PCNT))
    {
        Scope (\Q967)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07AF, PCNT))
                {
                    Return (\Q967.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B0, PCNT))
    {
        Scope (\Q968)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B0, PCNT))
                {
                    Return (\Q968.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B1, PCNT))
    {
        Scope (\Q969)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B1, PCNT))
                {
                    Return (\Q969.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B2, PCNT))
    {
        Scope (\Q970)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B2, PCNT))
                {
                    Return (\Q970.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B3, PCNT))
    {
        Scope (\Q971)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B3, PCNT))
                {
                    Return (\Q971.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B4, PCNT))
    {
        Scope (\Q972)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B4, PCNT))
                {
                    Return (\Q972.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B5, PCNT))
    {
        Scope (\Q973)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B5, PCNT))
                {
                    Return (\Q973.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B6, PCNT))
    {
        Scope (\Q974)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B6, PCNT))
                {
                    Return (\Q974.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B7, PCNT))
    {
        Scope (\Q975)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B7, PCNT))
                {
                    Return (\Q975.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B8, PCNT))
    {
        Scope (\Q976)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B8, PCNT))
                {
                    Return (\Q976.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07B9, PCNT))
    {
        Scope (\Q977)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07B9, PCNT))
                {
                    Return (\Q977.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07BA, PCNT))
    {
        Scope (\Q978)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07BA, PCNT))
                {
                    Return (\Q978.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07BB, PCNT))
    {
        Scope (\Q979)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07BB, PCNT))
                {
                    Return (\Q979.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07BC, PCNT))
    {
        Scope (\Q980)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07BC, PCNT))
                {
                    Return (\Q980.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07BD, PCNT))
    {
        Scope (\Q981)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07BD, PCNT))
                {
                    Return (\Q981.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07BE, PCNT))
    {
        Scope (\Q982)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07BE, PCNT))
                {
                    Return (\Q982.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07BF, PCNT))
    {
        Scope (\Q983)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07BF, PCNT))
                {
                    Return (\Q983.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C0, PCNT))
    {
        Scope (\Q984)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C0, PCNT))
                {
                    Return (\Q984.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C1, PCNT))
    {
        Scope (\Q985)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C1, PCNT))
                {
                    Return (\Q985.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C2, PCNT))
    {
        Scope (\Q986)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C2, PCNT))
                {
                    Return (\Q986.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C3, PCNT))
    {
        Scope (\Q987)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C3, PCNT))
                {
                    Return (\Q987.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C4, PCNT))
    {
        Scope (\Q988)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C4, PCNT))
                {
                    Return (\Q988.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C5, PCNT))
    {
        Scope (\Q989)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C5, PCNT))
                {
                    Return (\Q989.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C6, PCNT))
    {
        Scope (\Q990)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C6, PCNT))
                {
                    Return (\Q990.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C7, PCNT))
    {
        Scope (\Q991)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C7, PCNT))
                {
                    Return (\Q991.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C8, PCNT))
    {
        Scope (\Q992)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C8, PCNT))
                {
                    Return (\Q992.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07C9, PCNT))
    {
        Scope (\Q993)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07C9, PCNT))
                {
                    Return (\Q993.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07CA, PCNT))
    {
        Scope (\Q994)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07CA, PCNT))
                {
                    Return (\Q994.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07CB, PCNT))
    {
        Scope (\Q995)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07CB, PCNT))
                {
                    Return (\Q995.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07CC, PCNT))
    {
        Scope (\Q996)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07CC, PCNT))
                {
                    Return (\Q996.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07CD, PCNT))
    {
        Scope (\Q997)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07CD, PCNT))
                {
                    Return (\Q997.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07CE, PCNT))
    {
        Scope (\Q998)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07CE, PCNT))
                {
                    Return (\Q998.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07CF, PCNT))
    {
        Scope (\Q999)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07CF, PCNT))
                {
                    Return (\Q999.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D0, PCNT))
    {
        Scope (\R000)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D0, PCNT))
                {
                    Return (\R000.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D1, PCNT))
    {
        Scope (\R001)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D1, PCNT))
                {
                    Return (\R001.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D2, PCNT))
    {
        Scope (\R002)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D2, PCNT))
                {
                    Return (\R002.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D3, PCNT))
    {
        Scope (\R003)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D3, PCNT))
                {
                    Return (\R003.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D4, PCNT))
    {
        Scope (\R004)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D4, PCNT))
                {
                    Return (\R004.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D5, PCNT))
    {
        Scope (\R005)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D5, PCNT))
                {
                    Return (\R005.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D6, PCNT))
    {
        Scope (\R006)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D6, PCNT))
                {
                    Return (\R006.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D7, PCNT))
    {
        Scope (\R007)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D7, PCNT))
                {
                    Return (\R007.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D8, PCNT))
    {
        Scope (\R008)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D8, PCNT))
                {
                    Return (\R008.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07D9, PCNT))
    {
        Scope (\R009)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07D9, PCNT))
                {
                    Return (\R009.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07DA, PCNT))
    {
        Scope (\R010)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07DA, PCNT))
                {
                    Return (\R010.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07DB, PCNT))
    {
        Scope (\R011)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07DB, PCNT))
                {
                    Return (\R011.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07DC, PCNT))
    {
        Scope (\R012)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07DC, PCNT))
                {
                    Return (\R012.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07DD, PCNT))
    {
        Scope (\R013)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07DD, PCNT))
                {
                    Return (\R013.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07DE, PCNT))
    {
        Scope (\R014)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07DE, PCNT))
                {
                    Return (\R014.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07DF, PCNT))
    {
        Scope (\R015)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07DF, PCNT))
                {
                    Return (\R015.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E0, PCNT))
    {
        Scope (\R016)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E0, PCNT))
                {
                    Return (\R016.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E1, PCNT))
    {
        Scope (\R017)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E1, PCNT))
                {
                    Return (\R017.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E2, PCNT))
    {
        Scope (\R018)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E2, PCNT))
                {
                    Return (\R018.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E3, PCNT))
    {
        Scope (\R019)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E3, PCNT))
                {
                    Return (\R019.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E4, PCNT))
    {
        Scope (\R020)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E4, PCNT))
                {
                    Return (\R020.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E5, PCNT))
    {
        Scope (\R021)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E5, PCNT))
                {
                    Return (\R021.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E6, PCNT))
    {
        Scope (\R022)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E6, PCNT))
                {
                    Return (\R022.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E7, PCNT))
    {
        Scope (\R023)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E7, PCNT))
                {
                    Return (\R023.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E8, PCNT))
    {
        Scope (\R024)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E8, PCNT))
                {
                    Return (\R024.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07E9, PCNT))
    {
        Scope (\R025)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07E9, PCNT))
                {
                    Return (\R025.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07EA, PCNT))
    {
        Scope (\R026)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07EA, PCNT))
                {
                    Return (\R026.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07EB, PCNT))
    {
        Scope (\R027)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07EB, PCNT))
                {
                    Return (\R027.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07EC, PCNT))
    {
        Scope (\R028)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07EC, PCNT))
                {
                    Return (\R028.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07ED, PCNT))
    {
        Scope (\R029)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07ED, PCNT))
                {
                    Return (\R029.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07EE, PCNT))
    {
        Scope (\R030)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07EE, PCNT))
                {
                    Return (\R030.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07EF, PCNT))
    {
        Scope (\R031)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07EF, PCNT))
                {
                    Return (\R031.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F0, PCNT))
    {
        Scope (\R032)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F0, PCNT))
                {
                    Return (\R032.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F1, PCNT))
    {
        Scope (\R033)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F1, PCNT))
                {
                    Return (\R033.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F2, PCNT))
    {
        Scope (\R034)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F2, PCNT))
                {
                    Return (\R034.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F3, PCNT))
    {
        Scope (\R035)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F3, PCNT))
                {
                    Return (\R035.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F4, PCNT))
    {
        Scope (\R036)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F4, PCNT))
                {
                    Return (\R036.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F5, PCNT))
    {
        Scope (\R037)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F5, PCNT))
                {
                    Return (\R037.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F6, PCNT))
    {
        Scope (\R038)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F6, PCNT))
                {
                    Return (\R038.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F7, PCNT))
    {
        Scope (\R039)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F7, PCNT))
                {
                    Return (\R039.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F8, PCNT))
    {
        Scope (\R040)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F8, PCNT))
                {
                    Return (\R040.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07F9, PCNT))
    {
        Scope (\R041)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07F9, PCNT))
                {
                    Return (\R041.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07FA, PCNT))
    {
        Scope (\R042)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07FA, PCNT))
                {
                    Return (\R042.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07FB, PCNT))
    {
        Scope (\R043)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07FB, PCNT))
                {
                    Return (\R043.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07FC, PCNT))
    {
        Scope (\R044)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07FC, PCNT))
                {
                    Return (\R044.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07FD, PCNT))
    {
        Scope (\R045)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07FD, PCNT))
                {
                    Return (\R045.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07FE, PCNT))
    {
        Scope (\R046)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07FE, PCNT))
                {
                    Return (\R046.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x07FF, PCNT))
    {
        Scope (\R047)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x07FF, PCNT))
                {
                    Return (\R047.XSTA())
                }

                Return (Zero)
            }
        }
    }

    If (LLessEqual (0x0800, PCNT))
    {
        Scope (\R048)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LLessEqual (0x0800, PCNT))
                {
                    Return (\R048.XSTA())
                }

                Return (Zero)
            }
        }
    }
}
