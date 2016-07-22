#!/bin/bash

time java -jar juicebox_tools.jar dump observed NONE /Users/nchernia/Downloads/intra_nofrag.hic 1:1000000:6400000 1:1000000:6400000 BP 25000 /Users/nchernia/Downloads/tmp.txt

time java -jar JuiceboxReader.jar dump observed NONE /Users/nchernia/Downloads/intra_nofrag.hic 1:1000000:6400000 1:1000000:6400000 BP 25000 /Users/nchernia/Downloads/tmp.txt

time ./juicebox-quick-dump NONE /Users/nchernia/Downloads/intra_nofrag.hic 1:1000000:6400000 1:1000000:6400000 BP 25000 > /Users/nchernia/Downloads/tmp2.txt

res=$(paste /Users/nchernia/Downloads/tmp2.txt /Users/nchernia/Downloads/tmp.txt | awk 'function abs(v){return v<0?-v:v;} abs($3-$6)>0.0000000001')

if [ -n "$res" ]
then
    echo "Files not equal"
else
    echo "Files equal"
fi

time java -jar juicebox_tools.jar dump observed NONE /Users/nchernia/Downloads/intra_nofrag.hic 19:0:1280000 19:0:1280000 BP 5000 /Users/nchernia/Downloads/tmp.txt

time java -jar JuiceboxReader.jar dump observed NONE /Users/nchernia/Downloads/intra_nofrag.hic 19:0:1280000 19:0:1280000 BP 5000 /Users/nchernia/Downloads/tmp.txt

time ./juicebox-quick-dump NONE /Users/nchernia/Downloads/intra_nofrag.hic 19:0:1280000 19:0:1280000 BP 5000 > /Users/nchernia/Downloads/tmp2.txt

res=$(paste /Users/nchernia/Downloads/tmp2.txt /Users/nchernia/Downloads/tmp.txt | awk 'function abs(v){return v<0?-v:v;} abs($3-$6)>0.0000000001')

if [ -n "$res" ]
then
    echo "Files not equal"
else
    echo "Files equal"
fi

time java -jar juicebox_tools.jar dump observed NONE /Users/nchernia/Downloads/HIC001.hic 1:1000000:6400000 1:1000000:6400000 BP 25000 /Users/nchernia/Downloads/tmp.txt

time java -jar JuiceboxReader.jar dump observed NONE /Users/nchernia/Downloads/HIC001.hic 1:1000000:6400000 1:1000000:6400000 BP 25000 /Users/nchernia/Downloads/tmp.txt

time ./juicebox-quick-dump NONE /Users/nchernia/Downloads/HIC001.hic 1:1000000:6400000 1:1000000:6400000 BP 25000 > /Users/nchernia/Downloads/tmp2.txt

res=$(paste /Users/nchernia/Downloads/tmp2.txt /Users/nchernia/Downloads/tmp.txt | awk 'function abs(v){return v<0?-v:v;} abs($3-$6)>0.0000000001')

if [ -n "$res" ]
then
    echo "Files not equal"
else
    echo "Files equal"
fi

time java -jar juicebox_tools.jar dump observed NONE /Users/nchernia/Downloads/drosophila.hic arm_2L arm_2L BP 100000 /Users/nchernia/Downloads/tmp.txt

time java -jar JuiceboxReader.jar dump observed NONE /Users/nchernia/Downloads/drosophila.hic arm_2L arm_2L BP 100000 /Users/nchernia/Downloads/tmp.txt

time ./juicebox-quick-dump NONE /Users/nchernia/Downloads/drosophila.hic arm_2L arm_2L BP 100000 > /Users/nchernia/Downloads/tmp2.txt

res=$(paste /Users/nchernia/Downloads/tmp2.txt /Users/nchernia/Downloads/tmp.txt | awk 'function abs(v){return v<0?-v:v;} abs($3-$6)>0.0000000001')

if [ -n "$res" ]
then
    echo "Files not equal"
else
    echo "Files equal"
fi