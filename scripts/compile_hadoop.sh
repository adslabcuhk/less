#!/bin/bash
# usage: compile HDFS
# NOTE: please ensure all packages are downloaded accordingly
# NOTE: please ensure that OpenEC has been compiled

source "./load_eval_settings.sh"

if [ -d $pkg_dir ];then
    echo "package directory $pkg_dir exist, continue"
else
    echo "error: package directory $pkg_dir not exit, abort"
    exit 0
fi

cd $proj_dir/hdfs3.3.4-integration
sed -i "s%^HADOOP\_SRC\_DIR.*%HADOOP\_SRC\_DIR=${pkg_dir}\/hadoop-3.3.4-src%g" install.sh
bash -i install.sh
cp -r $pkg_dir/hadoop-3.3.4-src/hadoop-dist/target/hadoop-3.3.4 $home_dir
cp -r $pkg_dir/hadoop-3.3.4-src/oeclib $home_dir/hadoop-3.3.4
