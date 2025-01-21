#!/usr/bin/python3

from scapy.layers.l2 import Ether, ARP
from scapy.all import wrpcap
from scapy.all import *

class Sync(Packet):
    name = "Sync"
    fields_desc = [
        ShortField("t1", 0),
        ShortField("t2", 0),
        ShortField("t3", 0),
        ShortField("q1", 0),
        ShortField("q2", 0),
        ShortField("q3", 0),
        ShortField("threshold", 0),
        ShortField("data", 0)
    ]
bind_layers(Ether, Sync, type=0x5917)

def generate_pcap(pcap_name, packet_list: list()):
    wrpcap(pcap_name, packet_list)


if __name__ == '__main__':

    ip_packet = Ether(src="00:00:00:00:00:01", dst="00:00:00:00:00:02")/IP(src="192.168.0.1", dst="192.168.0.2")
    packet_list = list()

    payload = "hello P4" 
    data = 0

    sync = Sync(threshold=16383)
    packet = Ether(src="00:00:00:00:00:01", dst="00:00:00:00:00:02")/sync/payload 
    ls(packet)

    # change the interface here
    sendp(packet, iface="enp7s0f0")
    packet.show()
