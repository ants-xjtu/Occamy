# NS3="/home/vamsi/ABM-ns3/ns-3.35"
# DIR="$NS3/examples/abm-evaluation"
# DUMP_DIR="/home/vamsi/ABM-ns3/dump_all"
source config.sh
DIR="$NS3/examples/ABM"
DUMP_DIR="$DIR/dump_sigcomm"
mkdir $DUMP_DIR

DT=101
FAB=102
CS=103
IB=104
ABM=110

RENO=0
CUBIC=1
DCTCP=2
HPCC=3
POWERTCP=4
HOMA=5
TIMELY=6
THETAPOWERTCP=7

BUF_ALGS=($DT $FAB $CS $IB $ABM)
# TCP_ALGS=($CUBIC $DCTCP $TIMELY $POWERTCP)
TCP_ALGS=($CUBIC $DCTCP $HPCC $POWERTCP $RENO $THETAPOWERTCP)
# TCP_ALGS=($CUBIC)

SERVERS=32
LEAVES=8
SPINES=8
LINKS=1
SERVER_LEAF_CAP=10
LEAF_SPINE_CAP=10
LATENCY=10

RED_MIN=65
RED_MAX=65

N_PRIO=2

ALPHAFILE="$DIR/alphas"
CDFFILE="$DIR/websearch.txt"
CDFNAME="WS"

ALPHA_UPDATE_INT=1 # 1 RTT


STATIC_BUFFER=0
# BUFFER=$(( 1000*1000*9  ))
BUFFER_PER_PORT_PER_GBPS=9.6 # https://baiwei0427.github.io/papers/bcc-ton.pdf (Trident 2)
BUFFER=$(python3 -c "print(int($BUFFER_PER_PORT_PER_GBPS*1024*($SERVERS+$LINKS*$SPINES)*$SERVER_LEAF_CAP))")

TCP=$CUBIC

START_TIME=10
END_TIME=24
FLOW_END_TIME=10.1

# SINGLE QUEUE - ALGS vs LOAD
BURST_SIZES=0.3
BURST_SIZE=$(python3 -c "print($BURST_SIZES*$BUFFER)")
BURST_FREQ=2

echo "short999fct short99fct short95fct shortavgfct shortmedfct incast999fct incast99fct incast95fct incastavgfct incastmedfct long999fct long99fct long95fct longavgfct longmedfct avgTh medTh bufmax buf999 buf99 buf95 avgBuf medBuf load burst alg tcp scenario nprio"

for TCP in ${TCP_ALGS[@]};do
	for ALG in $DT;do
		for LOAD in 0.2 0.4 0.6 0.8;do
			FLOWFILE="$DUMP_DIR/fcts-single-$TCP-$ALG-$LOAD-$BURST_SIZES-$BURST_FREQ.fct"
			TORFILE="$DUMP_DIR/tor-single-$TCP-$ALG-$LOAD-$BURST_SIZES-$BURST_FREQ.stat"
			python3 parseData-singleQ.py $FLOWFILE $TORFILE $LEAF_SPINE_CAP $(( $LATENCY*8 )) $LOAD $BURST_SIZES $ALG $TCP single $N_PRIO
		done
	done
done

