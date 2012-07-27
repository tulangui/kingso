./test_search query res index_lib_cfg.xml |grep "bin/search" > t
wget -i t -O t1
perl a.pl t1 > t2
