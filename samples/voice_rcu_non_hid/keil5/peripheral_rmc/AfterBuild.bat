F:\Keil_v5\ARM\ARMCC\bin\fromelf --bin --output=./outdir/rcu.bin  ./outdir/rcu.axf
F:\Keil_v5\ARM\ARMCC\bin\fromelf -c ./outdir/rcu.axf -o ./outdir/rcu.txt
mkdir ..\..\outdir\bin\
copy .\outdir\rcu.bin ..\..\outdir\
