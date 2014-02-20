#!/bin/sh

i=3
while [ $i -ne 5 ]
do
	i=`expr $i + 1`
	case $i in
	1)
		ohm=150
		name=150
		;;
	2)
		ohm=1000
		name=1K
		;;
	3)
		ohm=10000
		name=10K
		;;
	4)
		ohm=100000
		name=100K
		;;
	*)
		ohm=1000000
		name=1M
		;;
	esac
		
	file=${1}-${name}.txt
	sensor="sudo ./sensor -D -L ${ohm} >> ${file}"
	
	echo "exec " $sensor
	echo "" > $file
	echo "exec y/n"
	read ok
	
	case $ok in
	n)
		;;
	*)
		echo "exec"
		a=0
		while [ $a -ne 20 ]
		do
			a=`expr $a + 1`
			echo "loop ${a}" >> ${file}
			eval ${sensor}
			echo "" >> ${file}
		done
		;;
	esac
	
done