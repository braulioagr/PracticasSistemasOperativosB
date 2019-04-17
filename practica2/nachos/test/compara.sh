#!/bin/bash
cont=0
for i in $(hexdump halt); do
	let mod=$(expr $cont % 9)
	if [ "$mod" -ne 0 ]
	then
		echo $i
	fi
	let cont=cont+1
done
