#!/bin/bash

make clean; 
echo "Silently recompiling"
SANITIZE=1 make -j8 > /dev/zero || exit 1;
echo "Done!"

rm -rf Tests/Output || exit 1
mkdir -p Tests/Output || exit 1

code=0

printf "%s" "Test_1(Tests/Input/SSDT-x4_0.bin, \\_PR.PR00._PPC, 1): "
./ACPIe -f Tests/Input/SSDT-x4_0.bin \\_PR.PR00._PPC 1 > Tests/Output/test1_output.txt
diff -q Tests/Output/test1_output.txt Tests/Correct/test1_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test1_output.txt)
fi

printf "%s" "Test_2(Tests/Input/SSDT-x4_0.bin, \\_PR.PR00._PPC, 2): "
./ACPIe -f Tests/Input/SSDT-x4_0.bin \\_PR.PR00._PPC 2 > Tests/Output/test2_output.txt
diff -q Tests/Output/test2_output.txt Tests/Correct/test2_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test2_output.txt)
fi

printf "%s" "Test_3(Tests/Input/SSDT-1.bin, \\_SB.IETM._STA, 1): "
./ACPIe -f Tests/Input/SSDT-1.bin \\_SB.IETM._STA 1 > Tests/Output/test3_output.txt
diff -q Tests/Output/test3_output.txt Tests/Correct/test3_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test3_output.txt)
fi

printf "%s" "Test_4(Tests/Input/SSDT-0.bin, \\_SB.PCI0.SAT0.TMD0.DMA0, 1): "
./ACPIe -f Tests/Input/SSDT-0.bin \\_SB.PCI0.SAT0.TMD0.DMA0 1 > Tests/Output/test4_output.txt
diff -q Tests/Output/test4_output.txt Tests/Correct/test4_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test4_output.txt)
fi

printf "%s" "Test_5(Tests/Input/SSDT-0.bin, GTFB, 1): "
./ACPIe -f Tests/Input/SSDT-0.bin GTFB 1 > Tests/Output/test5_output.txt
diff -q Tests/Output/test5_output.txt Tests/Correct/test5_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test5_output.txt)
fi

printf "%s" "Test_6(Tests/Input/SSDT-0.bin, \\GTFB, 1): "
./ACPIe -f Tests/Input/SSDT-0.bin \\GTFB 1 > Tests/Output/test6_output.txt
diff -q Tests/Output/test6_output.txt Tests/Correct/test6_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test6_output.txt)
fi

printf "%s" "Test_7(Tests/Input/DSDT.bin, \\PSM.DPLC, 1): "
./ACPIe -f Tests/Input/DSDT.bin \\PSM.DPLC 1 > Tests/Output/test7_output.txt
diff -q Tests/Output/test7_output.txt Tests/Correct/test7_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test7_output.txt)
fi

printf "%s" "Test_8(Tests/Input/DSDT.bin, \\_SB.ABC..AHJK, 1): "
./ACPIe -f Tests/Input/DSDT.bin \\_SB.ABC..AHJK 1 > Tests/Output/test8_output.txt
diff -q Tests/Output/test8_output.txt Tests/Correct/test8_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test8_output.txt)
fi

printf "%s" "Test_9(Tests/Input/DSDT.bin, ..\\.., 1): "
./ACPIe -f Tests/Input/DSDT.bin ..\\.. 1 > Tests/Output/test9_output.txt
diff -q Tests/Output/test9_output.txt Tests/Correct/test9_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test9_output.txt)
fi

printf "%s" "Test_10(Tests/Input/ABCD.bin, \\_SB.PCI0.SAT0.TMD0.DMA0, 1): "
./ACPIe -f Tests/Input/ABCD.bin \\_SB.PCI0.SAT0.TMD0.DMA0 1 > Tests/Output/test10_output.txt
diff -q Tests/Output/test10_output.txt Tests/Correct/test10_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test10_output.txt)
fi

printf "%s" "Test_11(Tests/Input/RSDT.bin, \\_SB.PCI0, 1): "
./ACPIe -f Tests/Input/RSDT.bin \\_SB.PCI0 1 > Tests/Output/test11_output.txt
diff -q Tests/Output/test11_output.txt Tests/Correct/test11_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test11_output.txt)
fi

printf "%s" "Test_12(Tests/Input/DSDT.bin, \\MBAR, 1): "
./ACPIe -f Tests/Input/DSDT.bin \\MBAR 1 > Tests/Output/test12_output.txt
diff -q Tests/Output/test12_output.txt Tests/Correct/test12_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test12_output.txt)
fi

printf "%s" "Test_13(Tests/Input/DSDT.bin, _SB.PCI0.I2C1.ETPD._HID, 1): "
./ACPIe -f Tests/Input/DSDT.bin _SB.PCI0.I2C1.ETPD._HID 1 > Tests/Output/test13_output.txt
diff -q Tests/Output/test13_output.txt Tests/Correct/test13_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test13_output.txt)
fi

printf "%s" "Test_14(Tests/Input/DSDT.bin, _SB_.PCI0.B0D4._DSM, 1): "
./ACPIe -f Tests/Input/DSDT.bin _SB_.PCI0.B0D4._DSM > Tests/Output/test14_output.txt
diff -q Tests/Output/test14_output.txt Tests/Correct/test14_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test14_output.txt)
fi

printf "%s" "Test_15(Tests/Input/corrupt1.bin, _SB.PCI0.GFX0, 1): "
./ACPIe -f Tests/Input/corrupt1.bin _SB.PCI0.GFX0 > Tests/Output/test15_output.txt
diff -q Tests/Output/test15_output.txt Tests/Correct/test15_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test15_output.txt)
fi

printf "%s" "Test_16(Tests/Input/nesting.bin, _SB.PCI0.GFX0, 1): "
./ACPIe -f Tests/Input/nesting.bin _SB.PCI0.GFX0 > Tests/Output/test16_output.txt
diff -q Tests/Output/test16_output.txt Tests/Correct/test16_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test16_output.txt)
fi

printf "%s" "Test_17(Tests/Input/DSDT-legacy.bin, _SB.PCI0.GFX0, 1): "
./ACPIe -f Tests/Input/DSDT-legacy.bin \\_SB.PCI0.SBRG > Tests/Output/test17_output.txt
diff -q Tests/Output/test17_output.txt Tests/Correct/test17_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test17_output.txt)
fi

printf "%s" "Test_18(Tests/Input/DSDT-legacy.bin, \\_SB.PCI0.P0P9, 1): "
./ACPIe -f Tests/Input/DSDT-legacy.bin \\_SB.PCI0.P0P9 > Tests/Output/test18_output.txt
diff -q Tests/Output/test18_output.txt Tests/Correct/test18_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test18_output.txt)
fi

printf "%s" "Test_19(Tests/Input/DSDT-legacy.bin, \\HPET, 1): "
./ACPIe -f Tests/Input/DSDT-legacy.bin \\HPET > Tests/Output/test19_output.txt
diff -q Tests/Output/test19_output.txt Tests/Correct/test19_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test19_output.txt)
fi

printf "%s" "Test_20(Tests/Input/DSDT-legacy.bin, \\_SB.PCI0.SBRG.HPET, 1): "
./ACPIe -f Tests/Input/DSDT-legacy.bin \\_SB.PCI0.SBRG.HPET > Tests/Output/test20_output.txt
diff -q Tests/Output/test20_output.txt Tests/Output/test20_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test20_output.txt)
fi

printf "%s" "Test_21(Tests/Input/DSDT-XOSI.bin, \\_SB.PCI0.LPCB.HPET, 1): "
./ACPIe -f Tests/Input/DSDT-XOSI.bin \\_SB.PCI0.LPCB.HPET > Tests/Output/test21_output.txt
diff -q Tests/Output/test21_output.txt Tests/Output/test21_output.txt
if (($? == 1))
then echo FAIL && code=1
else (echo OK; rm -f Tests/Output/test21_output.txt)
fi

exit $code
