C:\Keil\ARM\ARMCC\bin\fromelf --bin --output=./outdir/rcu.bin  ./outdir/ble_module.axf
C:\Keil\ARM\ARMCC\bin\fromelf -c ./outdir/ble_module.axf -o ./outdir/ble_module.txt
mkdir ..\..\outdir\bin\
copy .\outdir\rcu.bin ..\..\outdir\
