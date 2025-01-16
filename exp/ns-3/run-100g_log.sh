echo "hello, this is an experienment in small spine topo"

cd ../../src/ns-3/
SERVERS=16
LEAVES=8
SPINES=8
LINKS=1
SERVER_LEAF_CAP=100
LEAF_SPINE_CAP=100
LATENCY=10
BUFFER_PER_PORT_PER_GBPS=5.12


nPrior=2
requestFlowRate=200.0

bufferSize=$(python3 -c "print(int($BUFFER_PER_PORT_PER_GBPS*1024*8*$SERVER_LEAF_CAP))")
N_CORES=50
N=0
method_array=("DT")
tcpProtocol_array=("DCTCP")


webLoad_array=(0.2 0.4 0.9)
requestSizeRate_array=(0.4)
alpha=1.0
method_array=("DT")
for webLoad in ${webLoad_array[*]};do
    for requestSizeRate in ${requestSizeRate_array[*]};do        
        for tcpProtocol in ${tcpProtocol_array[*]};do
            for method in ${method_array[*]};do
                while [[ $(ps aux|grep "sig_100g_log-optimized"|wc -l) -gt $N_CORES ]];do
                    sleep 10;
                    echo "waiting for cores, $N running..."
                done
                N=$(( $N+1 ))
                ./ns3 run "scratch/sig_100g_log.cc --method=${method} --alpha=${alpha} --tcpProtocol=${tcpProtocol} --webLoad=${webLoad} --requestSizeRate=${requestSizeRate} --requestFlowRate=${requestFlowRate} --bufferSize=${bufferSize} --nPrior=${nPrior}" > /dev/null &
                sleep 2
                echo $N
            done
        done
    done
done

webLoad_array=(0.2 0.4 0.9)
requestSizeRate_array=(0.4)
alpha=0.5
for webLoad in ${webLoad_array[*]};do
    for requestSizeRate in ${requestSizeRate_array[*]};do        
        for tcpProtocol in ${tcpProtocol_array[*]};do
            for method in ${method_array[*]};do
                while [[ $(ps aux|grep "sig_100g_log-optimized"|wc -l) -gt $N_CORES ]];do
                    sleep 10;
                    echo "waiting for cores, $N running..."
                done
                N=$(( $N+1 ))
                ./ns3 run "scratch/sig_100g_log.cc --method=${method} --alpha=${alpha} --tcpProtocol=${tcpProtocol} --webLoad=${webLoad} --requestSizeRate=${requestSizeRate} --requestFlowRate=${requestFlowRate} --bufferSize=${bufferSize} --nPrior=${nPrior}" > /dev/null &
                sleep 2
                echo $N
            done
        done
    done
done



while [[ $(ps aux|grep "sig_100g_log-optimized"|wc -l) -gt 1 ]];do
	echo "Waiting for simulations to finish..."
	sleep 5
done


echo "##################################"
echo "#      FINISHED EXPERIMENTS      #"
echo "##################################"

