#!/bin/sh

while getopts :m:s:d:l:t: OPTION; do
	case $OPTION in
		m) MASTER=$OPTARG
		;;
		s) SLAVE=$OPTARG
		;;
		d) WORK_HOME=$OPTARG
		;;
		l) LOGGER=$OPTARG
		;;
		t) TIME_SLOT=$OPTARG
		;;
		\?)
		echo "Usage: `basename $0` -m \$MASTER_HOST:\$MASTER_DIR -s \$SLAVE_HOST:\$SLAVE_DIR -d \$WORK_HOME -l \$LOG_FILE -t \$TIME_SLOT" >> /dev/stderr
		exit 1
		;;
	esac
done

MASTER_DIR=`echo "$MASTER" | awk -F":" '{print $2;}'`
MASTER=`echo "$MASTER" | awk -F":" '{print $1;}'`
SLAVE_DIR=`echo "$SLAVE" | awk -F":" '{print $2;}'`
SLAVE=`echo "$SLAVE" | awk -F":" '{print $1;}'`

if [ -z "$MASTER" ] || [ -z "$MASTER_DIR" ] || [ -z "$SLAVE" ] || [ -z "$SLAVE_DIR" ] ; then
	echo "Usage: `basename $0` -m \$MASTER_HOST:\$MASTER_DIR -s \$SLAVE_HOST:\$SLAVE_DIR -d \$WORK_HOME -l \$LOG_FILE -t \$TIME_SLOT" >> /dev/stderr
	exit 1
fi

if [ ! -z "$TIME_SLOT" ] && ! [[ "$TIME_SLOT" =~ "^[0-9]+$" ]]; then
	echo "[WARN] time slot -t $TIME_SLOT is not a number, using 10 second as default" >> /dev/stderr
	TIME_SLOT=""
fi

[ -z "$TIME_SLOT" ] && { TIME_SLOT=10; }

[ -z "$LOGGER" ] && { LOGGER=/dev/stdout; }

[ -z "$WORK_HOME" ] && { WORK_HOME=.; }

is_real_path() 
{
	[ $# -ne 1 ] && { return 0; }
	if [ `echo "$1" | awk '{if(index($1, "/") == 1) print "a";}' | wc -l` -eq 0 ]; then
		return 0;
	fi
	return 1
}

is_real_path "$WORK_HOME"
if [ $? -ne 1 ]; then
	WORK_HOME=`pwd`/$WORK_HOME
fi

is_real_path "$LOGGER"
if [ $? -ne 1 ]; then
	LOGGER=`pwd`/$LOGGER
fi

CTRL_CMD=a
SYNC=sync
MASTER_CTRL_FILE=cm_m.pipe
SLAVE_CTRL_FILE=cm_s.pipe

MASTER_OUT=m_clustermap.sys
MASTER_IN=m_clustermap.in.sys
SLAVE_OUT=s_clustermap.sys
SLAVE_IN=s_clustermap.in.sys

TMP_MO=$WORK_HOME/.master_out.$$
TMP_SO=$WORK_HOME/.slave_out.$$

echo "[INFO] Starting cm_sync monitor at `date`" >> $LOGGER
echo "[INFO] Master CM Server: $MASTER ($MASTER_DIR)" >> $LOGGER
echo "[INFO] Slave CM Server: $SLAVE ($SLAVE_DIR)" >> $LOGGER
echo "[INFO] Working Home: $WORK_HOME" >> $LOGGER
echo "[INFO] Monitor per $TIME_SLOT seconds" >> $LOGGER
echo "" >> $LOGGER
while [ 1 -eq 1 ]; do
	rm -f $TMP_MO $TMP_SO
	scp $MASTER:$MASTER_DIR/$MASTER_OUT $TMP_MO 2>/dev/null 1>/dev/null
	if [ $? -eq 0 ] && [ -f $TMP_MO ]; then
		echo -e "\n[INFO] new master change from $MASTER:$MASTER_DIR/$MASTER_OUT at `date`" >> $LOGGER
		scp $TMP_MO $SLAVE:$SLAVE_DIR/$MASTER_IN 2>/dev/null 1>/dev/null
		if [ $? -ne 0 ]; then
			echo "[ERROR] \"scp $TMP_MO $SLAVE:$SLAVE_DIR/$MASTER_IN\" failed" >> $LOGGER
		else
			ssh $SLAVE "$SLAVE_DIR/$SYNC \"$CTRL_CMD\" $SLAVE_DIR/$SLAVE_CTRL_FILE" 2>/dev/null 1>/dev/null
			if [ $? -ne 0 ]; then
				echo "[ERROR] touch off slave: $SLAVE failed" >> $LOGGER
			else
				echo "[INFO] touch off slave: $SLAVE successfully" >> $LOGGER
			fi
		fi
	fi
	scp $SLAVE:$SLAVE_DIR/$SLAVE_OUT $TMP_SO 2>/dev/null 1>/dev/null
	if [ $? -eq 0 ] && [ -f $TMP_SO ]; then
		echo -e "\n[INFO] new slave change from $SLAVE:$SLAVE_DIR/$SLAVE_OUT at `date`" >> $LOGGER
		scp $TMP_SO $MASTER:$MASTER_DIR/$SLAVE_IN 2>/dev/null 1>/dev/null
		if [ $? -ne 0 ]; then
			echo "[ERROR] \"scp $TMP_SO $MASTER:$MASTER_DIR/$SLAVE_IN\" failed" >> $LOGGER
		else
			ssh $MASTER "$MASTER_DIR/$SYNC \"$CTRL_CMD\" $MASTER_DIR/$MASTER_CTRL_FILE" 2>/dev/null 1>/dev/null
			if [ $? -ne 0 ]; then
				echo "[ERROR] touch off master: $MASTER failed" >> $LOGGER
			else
				echo "[INFO] touch off master: $MASTER successfully" >>$LOGGER
			fi
		fi
	fi
	sleep $TIME_SLOT
done
echo "[INFO] Stopping cm_sync monitor at `data`" >> $LOGGER
