C:\Keil\ARM\ARMCC\bin\fromelf --bin --output=./outdir/  ./outdir/loader.axf
C:\Keil\ARM\ARMCC\bin\fromelf -c ./outdir/loader.axf -o ./outdir/loader.txt
mkdir ..\..\outdir\bin\
copy .\outdir\ER_IROM ..\..\outdir\loader.bin
