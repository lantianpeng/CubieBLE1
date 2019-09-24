C:\Keil\ARM\ARMCC\bin\fromelf --bin --output=./outdir/rcu.bin  ./outdir/rcu.axf
C:\Keil\ARM\ARMCC\bin\fromelf -c ./outdir/rcu.axf -o ./outdir/rcu.txt
mkdir ..\..\outdir\bin\
copy .\outdir\rcu.bin ..\..\outdir\
