input_path=$1
output_dir=$2
name=$3
scale=$4
if [ ! -d "data_depth" ]
then
    mkdir data_depth
else
    echo "depth dir exist"
fi

if [ ! -d "data_normal" ]
then
    mkdir data_normal
else
    echo "normal dir exist"
fi

if [ ! -d "data_sketch" ]
then
    mkdir data_sketch
else
    echo "out dir exist"
fi

if [ ! -d "build" ]
then 
    mkdir build 
    cd build && cmake .. && make -j && cd ..
fi

build/npr_sy_bin ${input_path} data_normal/${name} normal ${scale}
build/npr_sy_bin ${input_path} data_depth/${name} depth ${scale}
python py_src/edge.py $name data_sketch
