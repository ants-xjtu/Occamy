#include "ns3/log.h"
#include "switch.h"
#include "iostream"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Switch");

NS_OBJECT_ENSURE_REGISTERED (Switch);

TypeId Switch::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Switch")
        .SetParent<Object> ()
        .SetGroupName("Network")
    ;
    return tid;
}

Switch::Switch(){
    m_alpha = 0.5;
    m_maxBuffer = 10000;
    m_usedBuffer = 0;
    m_threshold = m_alpha*(m_maxBuffer-m_usedBuffer);
    m_ecn_threshold = 0;
    m_methodType = 0;
    m_enqueueMethod = Switch::DT;
    m_dequeueMethod = Switch::NORMAL_DEQUEUE;

    for(int i=0;i<qCnt;i++){
        qAlpha[i] = 1;
        pAlpha[i] = 10;
        qToP[i] = i;
        qThreshold[i] = qAlpha[i]*(m_maxBuffer-m_usedBuffer);
        pThreshold[i] = pAlpha[i]*(m_maxBuffer-m_usedBuffer);
        pUsedBuffer[i] = 0;        

        for(uint32_t j = 0; j<portCnt;j++){
            saturated[j][i] = 0;        //there is on congestion queue at the beginning, so every port every queue is 0
        }
    }

    for(uint32_t i=0;i<priorityCnt;i++){
        N[i] = 0;      //the NP is 0, there is no congestion queue at the beginning
    }
}

Switch::~Switch(){
    NS_LOG_FUNCTION (this);
}

void Switch::SetBuffer(int maxBuffer,int usedBuffer){

    m_maxBuffer = maxBuffer;
    m_usedBuffer = usedBuffer;
  
}

void Switch::AddUsed(int size, int qNo){      //this means a packet enqueue
    m_usedBuffer = m_usedBuffer + size;
    int priority = qToP[qNo];
    pUsedBuffer[priority] = pUsedBuffer[priority] + size;
  
}

void Switch::DeleteUsed(int size, int qNo){
    m_usedBuffer = m_usedBuffer - size;           //the total used buffer
    int priority = qToP[qNo];
    pUsedBuffer[priority] = pUsedBuffer[priority] - size;             //the used buffer of priority that qNo queue belonging to  
  
}

int Switch::GetQueueThreshold(int qNo){
    double threshold =  qAlpha[qNo] * (m_maxBuffer - m_usedBuffer);
    return int(threshold);
}

uint64_t Switch::GetPriorityThreshold(int qNo){
    int priority = qToP[qNo];
    uint64_t threshold =  pAlpha[priority] * (m_maxBuffer - m_usedBuffer);
    return uint64_t(threshold);
}

int Switch::GetEcnThreshold(){
    return m_ecn_threshold;
}

void Switch::SetEcnThreshold(int ecn_threshold){
    m_ecn_threshold = ecn_threshold;
}

int Switch::GetUsedBuffer(){
    return m_usedBuffer;
}

int Switch::GetMaxBuffer(){
    return m_maxBuffer;
}

void 
Switch::SetEnqueueMethod(EnqueueMethod eMethod){
    m_enqueueMethod = eMethod;
}

Switch::EnqueueMethod 
Switch::GetEnqueueMethod(){
    return m_enqueueMethod;
}

void 
Switch::SetDequeueMethod(DequeueMethod dMethod){
    m_dequeueMethod = dMethod;
}

Switch::DequeueMethod 
Switch::GetDequeueMethod(){
    return m_dequeueMethod;
}

void
Switch::SetQueueAlpha(double array[]){
    for(int i=0; i<qCnt; i++){
        qAlpha[i] = array[i];
    }
}

void
Switch::SetPriorityAlpha(double array[]){
    for(int i=0; i<qCnt; i++){
        pAlpha[i] = array[i];
    }
}

void
Switch::SetQueueToPriority(int array[]){
    for(int i=0; i<qCnt; i++){
        qToP[i] = array[i];
    }
}

uint64_t
Switch::GetPrioritySize(int qNo){
    int priority = qToP[qNo];
    return pUsedBuffer[priority];
}

void
Switch::SetSaturated(uint32_t port,uint32_t index, double satLevel){
    
    // N[priority] += satLevel - saturated[port][priority];
    // if(saturated[port][priority]==0 && satLevel==1){
    //     N[priority]++;
    // }else if(saturated[port][priority]==1 && satLevel==0){
    //     N[priority]--;
    // }
    // saturated[port][priority]=satLevel;
    
    if(saturated[port][index] != satLevel){
        uint32_t priority = qToP[index];
        N[priority] = N[priority] + satLevel - saturated[port][index];   //change NP 0->1 or 1->0
        saturated[port][index] = satLevel;                               //store new NP
    }
}

double
Switch::GetNPriority(uint32_t index){
    uint32_t priority = qToP[index];
    return N[priority];
}

}