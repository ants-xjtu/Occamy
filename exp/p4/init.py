from ipaddress import ip_address

p4 = bfrt.occamy.pipe

myport = bfrt.port.port

# 132 1/0 ==100G== dc11:enp10s0f0np0
# 140 2/0 ==100G== dc11:enp10s0f1np1
# 164 5/0 ==10G==  dc6:enp4s0f0
# 172 6/0 ==10G==  dc7:enp4s0f1
myport.add(DEV_PORT=132,SPEED="BF_SPEED_100G",FEC="BF_FEC_TYP_REED_SOLOMON",N_LANES=4,PORT_ENABLE=True)
myport.add(DEV_PORT=140,SPEED="BF_SPEED_100G",FEC="BF_FEC_TYP_REED_SOLOMON",N_LANES=4,PORT_ENABLE=True)
myport.add(DEV_PORT=164,SPEED="BF_SPEED_10G",FEC="BF_FEC_TYP_NONE",N_LANES=1,PORT_ENABLE=True)
myport.add(DEV_PORT=172,SPEED="BF_SPEED_10G",FEC="BF_FEC_TYP_NONE",N_LANES=1,PORT_ENABLE=True)

# Flow table
ipv4_lpm=p4.Ingress.ipv4_lpm
ipv4_lpm.add_with_send(dst_addr=ip_address('192.168.3.21'),dst_addr_p_length=32,port=164)
ipv4_lpm.add_with_send(dst_addr=ip_address('192.168.3.22'),dst_addr_p_length=32,port=172)

# Specify q1...q3
p4.Ingress.queue1_len.add_with_get_q1len(ucast_egress_port=164, qid=2)
p4.Ingress.queue2_len.add_with_get_q2len(ucast_egress_port=164, qid=3)
p4.Ingress.queue3_len.add_with_get_q3len(ucast_egress_port=172, qid=2)

p4.Egress.queue1_len.add_with_set_q1len(egress_port=164, egress_qid=2)
p4.Egress.queue2_len.add_with_set_q2len(egress_port=164, egress_qid=3)
p4.Egress.queue3_len.add_with_set_q3len(egress_port=172, egress_qid=2)

# Sync action of which pipe, 68 for that in pipe 0, and 196 for pipe 1
p4.Egress.q1_to_sync.add_with_sync_q1(egress_port=196)
p4.Egress.q2_to_sync.add_with_sync_q2(egress_port=196)
p4.Egress.q3_to_sync.add_with_sync_q3(egress_port=196)
# p4.Egress.q1_to_sync.add_with_sync_q1(egress_port=68)

# Specify where sync packets mirror
bfrt.pre.node.add(1,1,[],[132])
bfrt.pre.mgid.add(1,[1],[False],[0])