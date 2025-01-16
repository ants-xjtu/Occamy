#ifndef SWITCH_H
#define SWITCH_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "iostream"

namespace ns3 {

class Switch : public Object
{
  public:
    static TypeId GetTypeId (void);
    Switch();
    virtual ~Switch();
    void SetBuffer(int maxBuffer, int usedBuffer);
    
    void DeleteUsed(int size, int qNo);
    void AddUsed(int size, int qNo);


    int GetQueueThreshold(int qNo);
    uint64_t GetPriorityThreshold(int qNo);
    int GetUsedBuffer();
    int GetMaxBuffer();

    void SetEcnThreshold(int ecn_threshold);
    int GetEcnThreshold();

    typedef enum {
        NORMAL_ENQUEUE,
        DT,
        MULTILAYER_DT,
        PUSHOUT,
        ABM,
    } EnqueueMethod;

    typedef enum {
        NORMAL_DEQUEUE,
        DROP
    } DequeueMethod;

    EnqueueMethod m_enqueueMethod;
    DequeueMethod m_dequeueMethod;

    void SetEnqueueMethod(EnqueueMethod eMethod);
    EnqueueMethod GetEnqueueMethod(); 

    void SetDequeueMethod(DequeueMethod dMethod);
    DequeueMethod GetDequeueMethod();

    void SetQueueAlpha(double array[]);
    void SetPriorityAlpha(double array[]);
    void SetQueueToPriority(int array[]);
    uint64_t GetPrioritySize(int qNo);

    //level dt
    static const int qCnt = 8;
    double qAlpha[qCnt];
    double pAlpha[qCnt];
    double qToP[qCnt];               //queue to priority
    double qThreshold[qCnt];
    double pThreshold[qCnt];
    uint64_t pUsedBuffer[qCnt];

    //ABM
    static const uint32_t portCnt = 300;
    static const uint32_t priorityCnt = 8;
    double N[priorityCnt]; // N corresponds to each queue (one-one mapping with priority) at each port. 8 queues exist at each port.
    double saturated[portCnt][qCnt]; // 100 ports and 8 queues per node which share the buffer, are supported for now.
    void SetSaturated(uint32_t port,uint32_t index, double satLevel);
    double GetNPriority(uint32_t index);

    //pbufferplus setting
    int64_t last_head_drop_time = -1000000001;
    int64_t head_drop_internal = 1000000000;

    //token
    bool first_flag = false;
    int64_t token_num = 0;
    int64_t max_token_num = 8;        //inorder to avoid adding too many toke
    int64_t last_pkt_send_time = 1000000000;          //start time
    int64_t port_num = 8;
    int64_t bw = 100;
    int64_t ceil_size = 200;

    uint32_t last_head_drop_port = 1;
    uint32_t last_head_drop_queue = 0;

    //send_msg
    uint64_t send_size = 0;
    uint64_t send_rate = 0;

  private:
    int m_maxBuffer;
    int m_usedBuffer;
    double m_alpha;
    double m_threshold;
    int m_ecn_threshold;

    int m_methodType;   // 1--DT 2--ADT
};
  


}

#endif