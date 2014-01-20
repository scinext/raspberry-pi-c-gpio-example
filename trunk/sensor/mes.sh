#!/bin/sh

a=0
if [ $1 = "con" ] ; then
	file=${2}-con-ret.txt
	sensor="sudo ./sensor -Dl >> ${file}"
else
	#ohm=`expr ${1} \* 1000`
	ohm=${1}
	file=${2}-${1}-registor-ret.txt
	sensor="sudo ./sensor -D -L ${ohm} >> ${file}"
fi
echo "exec " $sensor

echo "" > $file
while [ $a -ne 20 ]
do
	a=`expr $a + 1`
	echo "loop ${a}" >> ${file}
	eval ${sensor}
	echo "" >> ${file}
done