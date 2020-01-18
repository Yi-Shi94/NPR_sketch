if [ ! -d "data_tmp" ] 
then
    mkdir data_tmp
else
    echo "tmp dir exist"
fi

if [ ! -d "data_out" ] 
then
    mkdir data_out
else
    echo "out dir exist"
fi

if [ ! -d "build" ]
then 
    mkdir build 
fi
#rm -rf build
cd build && cmake .. && make -j && cd ..

in_dir=$1
name=$2
build/npr_sy_bin $name depth $in_dir
build/npr_sy_bin $name normal $in_dir
python py_src/edge.py $name data_out 