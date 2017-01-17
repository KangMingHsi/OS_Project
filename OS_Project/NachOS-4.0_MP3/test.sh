#! /bin/bash
cd /home/os2016/2016osteam12/NachOS-4.0_MP3/code/test
function pause(){
   read -p "$*"
}
echo
echo 'running ../build.linux/nachos -ep my_test1 100 -ep my_test1 90 '
echo
timeout 3 ../build.linux/nachos -ep my_test1 100 -ep my_test1 90 # >/home/os2016/2016osteam12/NachOS-4.0_MP3/report_11.log
pause 'Press [Enter] key to continue...'
echo
echo 'running ../build.linux/nachos -ep consoleIO_test2 95 -ep consoleIO_test2 99'
echo
timeout 3 ../build.linux/nachos -ep consoleIO_test2 95 -ep consoleIO_test2 99 # >/home/os2016/2016osteam12/NachOS-4.0_MP3/report_2.log
pause 'Press [Enter] key to continue...'
echo
echo 'running ../build.linux/nachos -ep my_test1 1 -ep my_test1 1'
echo
timeout 6 ../build.linux/nachos -ep my_test1 1 -ep my_test1 1 # >/home/os2016/2016osteam12/NachOS-4.0_MP3/report_3.log
pause 'Press [Enter] key to continue...'
echo
echo 'running ../build.linux/nachos -ep my_test1 100 -ep my_test1 10 '
echo
timeout 4 ../build.linux/nachos -ep my_test1 100 -ep my_test1 10 # >/home/os2016/2016osteam12/NachOS-4.0_MP3/report_4.log
pause 'Press [Enter] key to continue...'
echo
echo 'running ../build.linux/nachos -ep my_test2 100 -ep my_test2 10'
echo
timeout 6 ../build.linux/nachos -ep my_test2 100 -ep my_test2 10 # >/home/os2016/2016osteam12/NachOS-4.0_MP3/report_5.log
