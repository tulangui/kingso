export LD_LIBRARY_PATH=../lib
ulimit -c unlimited
rm -rf ../logs/*
rm -rf ../update_rundata/*
killall clustermap
../bin/clustermap 8866 8877 ../etc/cm.xml -d
../bin/ks_merger_server -c ../etc/merger_server.cfg -k restart -l ../etc/merger_log.cfg -d
../bin/ks_searcher_server -c ../etc/searcher_server.cfg -k restart -l ../etc/searcher_log.cfg -d
../bin/ks_detail_server -c ../etc/detail_server.cfg -k restart -l ../etc/detail_log.cfg -d
