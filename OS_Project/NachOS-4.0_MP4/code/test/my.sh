make clean
make
../build.linux/nachos -f
../build.linux/nachos -cp FS_test1 /FS_test1
../build.linux/nachos -e /FS_test1
../build.linux/nachos -p /file1
../build.linux/nachos -cp FS_test2 /FS_test2
../build.linux/nachos -e /FS_test2
../build.linux/nachos -cp num_100.txt /100
../build.linux/nachos -cp num_1000.txt /1000	
../build.linux/nachos -p /1000
echo "========================================="
../build.linux/nachos -p /100
echo "========================================="
../build.linux/nachos -l /
../build.linux/nachos -mkdir /t0
../build.linux/nachos -mkdir /t1
../build.linux/nachos -mkdir /t2
../build.linux/nachos -cp num_100.txt /t0/f1
../build.linux/nachos -mkdir /t0/aa
../build.linux/nachos -mkdir /t0/bb
../build.linux/nachos -mkdir /t0/cc
../build.linux/nachos -cp num_100.txt /t0/bb/f1
../build.linux/nachos -cp num_100.txt /t0/bb/f2
../build.linux/nachos -cp num_100.txt /t0/bb/f3
../build.linux/nachos -cp num_100.txt /t0/bb/f4
echo "========================================="
../build.linux/nachos -l /
echo "========================================="
../build.linux/nachos -l /t0
echo "========================================="
../build.linux/nachos -r /t0/bb/f1
../build.linux/nachos -lr /
echo "========================================="
../build.linux/nachos -p /t0/f1
echo "========================================="
../build.linux/nachos -p /t0/bb/f3
echo "---------bonusIII----------"
#sh FS_bonusIII.sh
#echo "========================================="
../build.linux/nachos -mkdir /t0/dd
../build.linux/nachos -cp num_100.txt /t0/dd/f1
../build.linux/nachos -cp num_100.txt /t0/dd/f2
../build.linux/nachos -rr /t0/bb
../build.linux/nachos -lr /
echo "========================================="
../build.linux/nachos -rr /t0/dd
../build.linux/nachos -lr /
echo "========================================="
