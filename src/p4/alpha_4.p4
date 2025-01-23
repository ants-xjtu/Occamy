/* -*- P4_16 -*- */

#include <core.p4>
#include <tna.p4>

/*************************************************************************
 ************* C O N S T A N T S    A N D   T Y P E S  *******************
**************************************************************************/
const bit<16> ETHERTYPE_TPID = 0x8100;
const bit<16> ETHERTYPE_IPV4 = 0x0800;
const bit<16> ETHERTYPE_SYNC = 0x5917;
const bit<8>  TYPE_TCP       = 0x06;

/* Table Sizes */
const int IPV4_HOST_SIZE = 65536;

#ifdef USE_ALPM
const int IPV4_LPM_SIZE  = 400*1024;
#else
const int IPV4_LPM_SIZE  = 12288;
#endif

const int Q_NUM = 3;
const int ALPHA = 2;
const int ALPHA__2 = 4;
const int MAX_LEN = 16384;
const int DATA_NUM = 8;

/*************************************************************************
 ***********************  H E A D E R S  *********************************
 *************************************************************************/

/*  Define all the headers the program will recognize             */
/*  The actual sets of headers processed by each gress can differ */

/* Standard ethernet header */
header ethernet_h {
    bit<48>   dst_addr;
    bit<48>   src_addr;
    bit<16>   ether_type;
}

header ipv4_h {
    bit<4>   version;
    bit<4>   ihl;
    bit<8>   diffserv;
    bit<16>  total_len;
    bit<16>  identification;
    bit<3>   flags;
    bit<13>  frag_offset;
    bit<8>   ttl;
    bit<8>   protocol;
    bit<16>  hdr_checksum;
    bit<32>  src_addr;
    bit<32>  dst_addr;
}

header tcp_h {
	bit<16>	srcPort;
	bit<16>	dstPort;
	bit<32>	seq;
	bit<32>	ack;
	bit<4>	headerLen;
	bit<6>	resv;
	bit<6>	ceuaprsf;
	bit<16>	winSize;
	bit<16>	tcpCheck;
	bit<16>	urgPtr;
}

typedef bit<4> q_index_t;
typedef bit<16> q_len_t;
typedef bit<32> sync_data_t;

header sync_h {
    bit<48>     time;
    q_len_t     q1;
    q_len_t     q2;
    q_len_t     q3;
    q_len_t     threshold;
    q_len_t     data;
}

/*************************************************************************
 **************  I N G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

    /***********************  H E A D E R S  ************************/

struct my_ingress_headers_t {
    ethernet_h      ethernet;
    ipv4_h          ipv4;
    tcp_h           tcp;
    sync_h          sync;
}

    /******  G L O B A L   I N G R E S S   M E T A D A T A  *********/

struct my_ingress_metadata_t {
    q_len_t length;
}

    /***********************  P A R S E R  **************************/
parser IngressParser(packet_in        pkt,
    /* User */
    out my_ingress_headers_t          hdr,
    out my_ingress_metadata_t         meta,
    /* Intrinsic */
    out ingress_intrinsic_metadata_t  ig_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition init_meta;
    }

    state init_meta {
        meta.length = 0;
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4:  parse_ipv4;
            ETHERTYPE_SYNC:  parse_sync;
            default: accept;
        }
    }

    state parse_sync {
        pkt.extract(hdr.sync);
        transition accept;
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol){
            TYPE_TCP: parse_tcp;
            default: accept;
        }
    }

    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }

}

    /***************** M A T C H - A C T I O N  *********************/

control Ingress(
    /* User */
    inout my_ingress_headers_t                       hdr,
    inout my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_t               ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{
    Register<q_len_t, q_index_t>(1) q1_reg;
    RegisterAction<q_len_t, q_index_t, q_len_t>(q1_reg) set_q1 = {
        void apply(inout q_len_t q_len) {
            q_len = hdr.sync.q1;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(q1_reg) get_q1 = {
        void apply(inout q_len_t q_len, out q_len_t result) {
            result = q_len;
        }
    };


    Register<q_len_t, q_index_t>(1) q2_reg;
    RegisterAction<q_len_t, q_index_t, q_len_t>(q2_reg) set_q2 = {
        void apply(inout q_len_t q_len) {
            q_len = hdr.sync.q2;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(q2_reg) get_q2 = {
        void apply(inout q_len_t q_len, out q_len_t result) {
            result = q_len;
        }
    };


    Register<q_len_t, q_index_t>(1) q3_reg;
    RegisterAction<q_len_t, q_index_t, q_len_t>(q3_reg) set_q3 = {
        void apply(inout q_len_t q_len) {
            q_len = hdr.sync.q3;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(q3_reg) get_q3 = {
        void apply(inout q_len_t q_len, out q_len_t result) {
            result = q_len;
        }
    };


    Register<q_len_t, q_index_t>(1) thres_reg;
    RegisterAction<q_len_t, q_index_t, q_len_t>(thres_reg) set_thres = {
        void apply(inout q_len_t thres) {
            thres = hdr.sync.threshold;
            // thres = hdr.sync.data;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(thres_reg) get_thres = {
        void apply(inout q_len_t thres, out q_len_t result) {
            result = thres;
        }
    };
    RegisterAction<q_len_t, q_index_t, bit<3>>(thres_reg) get_drop = {
        void apply(inout q_len_t thres, out bit<3> drop) {
            if (thres < meta.length + 20 || thres > MAX_LEN) {
                drop = 1;
            } else {
                drop = 0;
            }
        }
    };



    action drop() {
        ig_dprsr_md.drop_ctl = 1;
    }
    action send(PortId_t port, bit<5> qid) {
        ig_tm_md.ucast_egress_port = port;
        ig_tm_md.qid = qid;
    }
    table ipv4_lpm {
        key     = { hdr.ipv4.dst_addr : lpm; }
        actions = { send; drop; }
        default_action = drop();
        size           = IPV4_LPM_SIZE;
    }



    action get_q1len() {
        meta.length = get_q1.execute(0);
    }
    table queue1_len {
        key     = {
            ig_tm_md.ucast_egress_port : exact;
            ig_tm_md.qid : exact;
        }
        actions = {
            get_q1len;
            NoAction;
        }
        default_action = NoAction();
        size           = DATA_NUM;
    }

    action get_q2len() {
        meta.length = get_q2.execute(0);
    }
    table queue2_len {
        key     = {
            ig_tm_md.ucast_egress_port : exact;
            ig_tm_md.qid : exact;
        }
        actions = {
            get_q2len;
            NoAction;
        }
        default_action = NoAction();
        size           = DATA_NUM;
    }

    action get_q3len() {
        meta.length = get_q3.execute(0);
    }
    table queue3_len {
        key     = {
            ig_tm_md.ucast_egress_port : exact;
            ig_tm_md.qid : exact;
        }
        actions = {
            get_q3len;
            NoAction;
        }
        default_action = NoAction();
        size           = DATA_NUM;
    }



    apply {
        if (hdr.sync.isValid()) {       // 是同步包
            hdr.sync.time = ig_intr_md.ingress_mac_tstamp;
            ig_tm_md.qid = 5;           // 同步包之队列
            ig_tm_md.mcast_grp_a = 1;
            set_q1.execute(0);
            set_q2.execute(0);
            set_q3.execute(0);
            set_thres.execute(0);
            if (ig_intr_md.ingress_port == 196) {
                ig_tm_md.ucast_egress_port = 68;
            } else {
                ig_tm_md.ucast_egress_port = 196;
            }
        } else {
            meta.length = 0;
            if (hdr.ipv4.isValid()) {
                ipv4_lpm.apply();
            }
            ig_tm_md.qid = 2;
            queue1_len.apply();
            queue2_len.apply();
            queue3_len.apply();
            ig_dprsr_md.drop_ctl = get_drop.execute(0);
        }
    }
}

    /*********************  D E P A R S E R  ************************/

control IngressDeparser(packet_out pkt,
    /* User */
    inout my_ingress_headers_t                       hdr,
    in    my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
{
    apply {
        pkt.emit(hdr);
    }
}


/*************************************************************************
 ****************  E G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

    /***********************  H E A D E R S  ************************/

struct my_egress_headers_t {
    ethernet_h      ethernet;
    ipv4_h          ipv4;
    sync_h          sync;
}

    /********  G L O B A L   E G R E S S   M E T A D A T A  *********/

struct my_egress_metadata_t {
    q_len_t thres;
    q_len_t length;
    bool    queued;
}

    /***********************  P A R S E R  **************************/

parser EgressParser(packet_in        pkt,
    /* User */
    out my_egress_headers_t          hdr,
    out my_egress_metadata_t         meta,
    /* Intrinsic */
    out egress_intrinsic_metadata_t  eg_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(eg_intr_md);
        meta.thres  = MAX_LEN;
        meta.length = 0;
        // meta.queued = false;
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4:  parse_ipv4;
            ETHERTYPE_SYNC:  parse_sync;
            default: accept;
        }
    }

    state parse_sync {
        pkt.extract(hdr.sync);
        transition accept;
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

    /***************** M A T C H - A C T I O N  *********************/

control Egress(
    /* User */
    inout my_egress_headers_t                          hdr,
    inout my_egress_metadata_t                         meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_t                  eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t      eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t     eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t  eg_oport_md)
{
    Register<q_len_t, q_index_t>(1) q1_reg;
    RegisterAction<q_len_t, q_index_t, q_len_t>(q1_reg) set_q1 = {
        void apply(inout q_len_t q_len) {
            q_len = (q_len_t)eg_intr_md.deq_qdepth;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(q1_reg) set_q1_sync = {
        void apply(inout q_len_t q_len) {
            q_len = hdr.sync.q1;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(q1_reg) get_q1 = {
        void apply(inout q_len_t q_len, out q_len_t result) {
            result = q_len;
        }
    };

    Register<q_len_t, q_index_t>(1) q2_reg;
    RegisterAction<q_len_t, q_index_t, q_len_t>(q2_reg) set_q2 = {
        void apply(inout q_len_t q_len) {
            q_len = (q_len_t)eg_intr_md.deq_qdepth;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(q2_reg) set_q2_sync = {
        void apply(inout q_len_t q_len) {
            q_len = hdr.sync.q2;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(q2_reg) get_q2 = {
        void apply(inout q_len_t q_len, out q_len_t result) {
            result = q_len;
        }
    };

    Register<q_len_t, q_index_t>(1) q3_reg;
    RegisterAction<q_len_t, q_index_t, q_len_t>(q3_reg) set_q3 = {
        void apply(inout q_len_t q_len) {
            q_len = (q_len_t)eg_intr_md.deq_qdepth;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(q3_reg) set_q3_sync = {
        void apply(inout q_len_t q_len) {
            q_len = hdr.sync.q3;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(q3_reg) get_q3 = {
        void apply(inout q_len_t q_len, out q_len_t result) {
            result = q_len;
        }
    };

    Register<q_len_t, q_index_t>(1) thres_reg;
    RegisterAction<q_len_t, q_index_t, q_len_t>(thres_reg) set_thres = {
        void apply(inout q_len_t q_len) {
            q_len = hdr.sync.threshold;
        }
    };
    RegisterAction<q_len_t, q_index_t, q_len_t>(thres_reg) get_thres = {
        void apply(inout q_len_t q_len, out q_len_t result) {
            result = q_len;
        }
    };
    RegisterAction<q_len_t, q_index_t, bit<3>>(thres_reg) get_drop = {
        void apply(inout q_len_t q_len, out bit<3> result) {
            if (q_len <= eg_intr_md.deq_qdepth[15:0]) {
                result = 1;
            } else {
                result = 0;
            }
        }
    };



    action set_q1len() {
        set_q1.execute(0);
    }
    table queue1_len {
        key     = {
            eg_intr_md.egress_port : exact;
            eg_intr_md.egress_qid : exact;
        }
        actions = {
            set_q1len;
            NoAction;
        }
        default_action = NoAction();
        size           = DATA_NUM;
    }

    action set_q2len() {
        set_q2.execute(0);
    }
    table queue2_len {
        key     = {
            eg_intr_md.egress_port : exact;
            eg_intr_md.egress_qid : exact;
        }
        actions = {
            set_q2len;
            NoAction;
        }
        default_action = NoAction();
        size           = DATA_NUM;
    }

    action set_q3len() {
        set_q3.execute(0);
    }
    table queue3_len {
        key     = {
            eg_intr_md.egress_port : exact;
            eg_intr_md.egress_qid : exact;
        }
        actions = {
            set_q3len;
            NoAction;
        }
        default_action = NoAction();
        size           = DATA_NUM;
    }


    
    action sync_q1() {
        hdr.sync.q1 = get_q1.execute(0);
    }
    action sync_to_q1() {
        set_q1_sync.execute(0);
    }
    table q1_to_sync { // 若 q1 在另一条流水线，则同步
        key     = { eg_intr_md.egress_port : exact; }
        actions = {
            sync_q1;
            sync_to_q1;
        }
        default_action = sync_to_q1();
        size           = DATA_NUM;
    }

    action sync_q2() {
        hdr.sync.q2 = get_q2.execute(0);
    }
    action sync_to_q2() {
        set_q2_sync.execute(0);
    }
    table q2_to_sync { // 若 q2 在另一条流水线，则同步
        key     = { eg_intr_md.egress_port : exact; }
        actions = {
            sync_q2;
            sync_to_q2;
        }
        default_action = sync_to_q2();
        size           = DATA_NUM;
    }

    action sync_q3() {
        hdr.sync.q3 = get_q3.execute(0);
    }
    action sync_to_q3() {
        set_q3_sync.execute(0);
    }
    table q3_to_sync { // 若 q3 在另一条流水线，则同步
        key     = { eg_intr_md.egress_port : exact; }
        actions = {
            sync_q3;
            sync_to_q3;
        }
        default_action = sync_to_q3();
        size           = DATA_NUM;
    }



    action sync_thres() {
        hdr.sync.threshold = meta.thres << ALPHA;
    }

    action cmp() {
        meta.length = meta.length - eg_intr_md.deq_qdepth[15:0];
    }

    apply {
        if (hdr.sync.isValid()) {
            q1_to_sync.apply();
            meta.thres = meta.thres - hdr.sync.q1;
            q2_to_sync.apply();
            meta.thres = meta.thres - hdr.sync.q2;
            q3_to_sync.apply();
            meta.thres = meta.thres - hdr.sync.q3;
            if (meta.thres[15:15] == 1) {
                // 下溢，使用缓存大于 16384 cells（1.25 MiB）
                hdr.sync.threshold = 0;
            } else if (meta.thres[14:12] > 0) {
                hdr.sync.threshold = MAX_LEN;
            }
            else {
                // 应用 α 后，可用缓存大于上限
                hdr.sync.threshold = meta.thres << ALPHA;
            }
            set_thres.execute(0);
        } else {
            meta.length = get_thres.execute(0) + 40;
            queue1_len.apply();
            queue2_len.apply();
            queue3_len.apply();
#if HEAD_DROP
            cmp();
            eg_dprsr_md.drop_ctl[0:0] = meta.length[15:15];
#endif
        }
    }
}

    /*********************  D E P A R S E R  ************************/

control EgressDeparser(packet_out pkt,
    /* User */
    inout my_egress_headers_t                       hdr,
    in    my_egress_metadata_t                      meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_for_deparser_t  eg_dprsr_md)
{
    apply {
        pkt.emit(hdr);
    }
}


/************ F I N A L   P A C K A G E ******************************/
Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()
) pipe;

Switch(pipe) main;
