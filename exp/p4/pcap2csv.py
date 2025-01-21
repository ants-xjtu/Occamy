#!/usr/bin/python3
from scapy.all import *
from scapy.utils import PcapReader
from scapy.layers.l2 import Ether, ARP
import sys

if len(sys.argv) > 1:
    file = sys.argv[1]
else:
    print("No pcap file. Exited.")
    exit()

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


PCAP_FILE = file + '.pcap'
CSV_FILE = file + '.csv'

with PcapReader(PCAP_FILE) as packets, open(CSV_FILE, "w") as f:
    f.write("time,q1,q2,q3,thres\n")
    for data in packets:
        if 'Sync' in data:
            t = (((data[Sync].t1 << 16) + data[Sync].t2) << 16) + data[Sync].t3
            f.write(f"{t},{data[Sync].q1},{data[Sync].q2},{data[Sync].q3},{data[Sync].threshold}\n")