C:\Keil\ARM\ARMCC\bin\fromelf --bin --output=./outdir/ancs.bin  ./outdir/ancs.axf
C:\Keil\ARM\ARMCC\bin\fromelf -c ./outdir/ancs.axf -o ./outdir/ancs.txt
mkdir ..\..\outdir\bin\
copy .\outdir\ancs.bin ..\..\outdir\
