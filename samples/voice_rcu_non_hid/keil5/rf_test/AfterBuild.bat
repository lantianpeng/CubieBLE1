F:\Keil_v5\ARM\ARMCC\bin\fromelf --bin --output=./outdir/rf_test.bin  ./outdir/rf_test.axf
F:\Keil_v5\ARM\ARMCC\bin\fromelf -c ./outdir/rf_test.axf -o ./outdir/rf_test.txt
mkdir ..\..\outdir\bin\
copy .\outdir\rf_test.bin ..\..\outdir\
