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

# BUF_ALGS=($DT $FAB $CS $IB $ABM)
BUF_ALGS=($DT)
TCP_ALGS=($CUBIC $DCTCP $HPCC $POWERTCP $RENO $THETAPOWERTCP)

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
BUFFER_PER_PORT_PER_GBPS=5.12 # https://baiwei0427.github.io/papers/bcc-ton.pdf (Trident 2)
BUFFER=$(python3 -c "print(int($BUFFER_PER_PORT_PER_GBPS*1024*($SERVERS+$LINKS*$SPINES)*$SERVER_LEAF_CAP))")

TCP=$CUBIC

START_TIME=10
END_TIME=24
FLOW_END_TIME=13


cd $NS3


N=0

##################################################################################
# Performance of DCTCP, TIMELY and PowerTCP under Incast workload at different request sizes (burst size) and request rate of 2. 
##################################################################################

# Total simulations = 30
BURST_SIZES=0.3
BURST_SIZE=$(python3 -c "print($BURST_SIZES*$BUFFER)")
BURST_FREQ=2
TCP=1
for TCP in ${TCP_ALGS[@]};do
	for ALG in ${BUF_ALGS[@]};do
		for LOAD in 0.2 0.4 0.6 0.8;do
			FLOW_END_TIME=11
			FLOWFILE="$DUMP_DIR/fcts-single-$TCP-$ALG-$LOAD-$BURST_SIZES-$BURST_FREQ.fct"
			TORFILE="$DUMP_DIR/tor-single-$TCP-$ALG-$LOAD-$BURST_SIZES-$BURST_FREQ.stat"
			while [[ $(ps aux | grep abm-optimized | wc -l) -gt $N_CORES ]];do
				sleep 30;
				echo "waiting for cores, $N running..."
			done
			N=$(( $N+1 ))
			(time ./ns3 run "abm --load=$LOAD --StartTime=$START_TIME --EndTime=$END_TIME --FlowLaunchEndTime=$FLOW_END_TIME --serverCount=$SERVERS --spineCount=$SPINES --leafCount=$LEAVES --linkCount=$LINKS --spineLeafCapacity=$LEAF_SPINE_CAP --leafServerCapacity=$SERVER_LEAF_CAP --linkLatency=$LATENCY --TcpProt=$TCP --BufferSize=$BUFFER --statBuf=$STATIC_BUFFER --algorithm=$ALG --RedMinTh=$RED_MIN --RedMaxTh=$RED_MAX --request=$BURST_SIZE --queryRequestRate=$BURST_FREQ --nPrior=$N_PRIO --alphasFile=$ALPHAFILE --cdfFileName=$CDFFILE --alphaUpdateInterval=$ALPHA_UPDATE_INT --fctOutFile=$FLOWFILE --torOutFile=$TORFILE"; echo "$FLOWFILE")&
			sleep 2
		done
	done
done


echo "##################################"
echo "#      FINISHED EXPERIMENTS      #"
echo "##################################"

