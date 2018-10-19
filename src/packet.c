#include "packet_interface.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <zlib.h>



struct __attribute__((__packed__)) pkt {
    uint8_t window:5;
    uint8_t tr:1;
    uint8_t type:2;
    uint8_t seqnum;
    uint16_t length;
    uint32_t timestamp;
    uint32_t crc1;
    char* payload;
    uint32_t crc2;
};

pkt_t* pkt_new(){

    pkt_t* ret=(pkt_t *)calloc(1,sizeof(pkt_t));
    if(ret==NULL){
        return NULL;
    }

    return ret;
}

void pkt_del(pkt_t *pkt)
{
    if(pkt!=NULL){
        if(pkt->payload!=NULL){
            free(pkt->payload);
            pkt->payload=NULL;
        }
        free(pkt);
        pkt=NULL;
    }
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt){
    
    pkt_status_code code;
    
    if(len < 4) return E_NOHEADER;
    if(len < 12) return E_UNCONSISTENT;
    
    memcpy(pkt, data, 12);
    
    //CRC1
    uint32_t crc1_test;
    crc1_test = crc32(0, (Bytef *) data, 8);
    //printf("crc1 decode %d\n",crc1_test);
    
    //NTOH LENGTH
    code=pkt_set_length(pkt,ntohs(pkt_get_length(pkt)));
    uint16_t length=pkt_get_length(pkt);
    
    //PAYLOAD
    pkt_set_payload(pkt,data+12,length);
    
    //CRC2
    memcpy(&(pkt->crc2),data+len-4,4);
    
    //VERIFICATIONS
    if((pkt_get_type(pkt) != PTYPE_DATA)&&(pkt_get_type(pkt) != PTYPE_ACK)&&(pkt_get_type(pkt) != PTYPE_NACK)){
        return E_TYPE;
    }
    if((length != len-4*sizeof(uint32_t))&&(length != len-3*sizeof(uint32_t))){
        return E_LENGTH;
    }
    if((pkt->tr == 1)&&(length!=0)){
        return E_TR;
    }
   
    //NTOH CRC1
    uint32_t crc1=ntohl(pkt->crc1);
    code=pkt_set_crc1(pkt,crc1);
    if(code!=PKT_OK)
        return code;
    uint32_t crc2;
    if(length>0){

        //NTOH CRC2
        crc2=ntohl(pkt_get_crc2(pkt));
        code=pkt_set_crc2(pkt,crc2);
        if(code!=PKT_OK)
            return code;
    }

    //TEST CRC1
    if(crc1_test != pkt_get_crc1(pkt))
    {
        return E_CRC;
    }
    if(code!=PKT_OK)
    {
        return code;
    }
    
    //TEST CRC2
    uint32_t crc2_test;
    if(length>0)
    {
        //CRC2
        crc2_test = crc32(0, (Bytef *) pkt->payload, length);
        //printf("crc2 decode %d\n",crc2_test);
        if(crc2_test != crc2)
        {
            return E_CRC;
        }
    }
    return PKT_OK;
}



pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
    //VERIFICATIONS
    if((pkt_get_crc2(pkt) == 0)&&(*len<pkt_get_length(pkt)+3*sizeof(uint32_t))){
        return E_NOMEM;
    }
    else if((pkt_get_crc2(pkt)!=0)&&(*len<pkt_get_length(pkt)+4*sizeof(uint32_t))){
        return E_NOMEM;
    }
    //LENGTHS
    uint16_t length=pkt_get_length(pkt);
    uint16_t nlength=htons(length);
    
    //MEMCPY HEADER
    memcpy(buf, pkt, 12);
    *len=12;
    pkt_t* temp=(pkt_t*) buf;
    uint32_t test_crc2=0;
    
    //HTON LENGTH
    temp->length=nlength;
    
    //CRC1
    uint32_t test_crc1=0;
    test_crc1=crc32(test_crc1,(Bytef *) buf, 8);
    temp->crc1=htonl(test_crc1);

    if(length>0)
    {
        //PAYLOAD
        memcpy(buf+12,pkt_get_payload(pkt),length);
        *len+=length;
        //CRC2
        test_crc2 = crc32(test_crc2, (Bytef *)(pkt->payload), length);
        test_crc2=htonl(test_crc2);
        memcpy(buf+12+length,&test_crc2,4);
        *len+=4;

    }
    //printf("crc1 encode %d\n",test_crc1);
    //printf("crc2 encode %d\n",ntohl(test_crc2));
    return PKT_OK;
}

ptypes_t pkt_get_type  (const pkt_t*pkt)
{
    return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t*pkt)
{
    return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t*pkt)
{
    return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t*pkt)
{
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t*pkt)
{
    return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t*pkt)
{
    return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t*pkt)
{
    return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t*pkt)
{
    return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t*pkt)
{
    return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
    if(type==PTYPE_DATA){
        pkt->type=1;
        return PKT_OK;
    }
    if(type==PTYPE_ACK){
        pkt->type=2;
        return PKT_OK;
    }
    if(type==PTYPE_NACK){
        pkt->type=3;
        return PKT_OK;
    }
    return E_TYPE;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
    if(tr==0||tr==1){
        pkt->tr=tr;
        return PKT_OK;
    }
    return E_TR;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
    if(window>31){
        return E_WINDOW;
    }
    pkt->window=window;
    return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
    //ERREUR COMPILATION
    //if(seqnum>255){
    //    return E_SEQNUM;
    //}
    pkt->seqnum=seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
    if(length>512){
        return E_LENGTH;
    }
    pkt->length=length;
    return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
    if(timestamp>512){
        return E_UNCONSISTENT;
    }
    pkt->timestamp=timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
    pkt->crc1=crc1;
    return PKT_OK;

}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
    pkt->crc2=crc2;
    return PKT_OK;

}

pkt_status_code pkt_set_payload(pkt_t *pkt,
                                const char *data,
                                const uint16_t length)
{

    if(length>MAX_PAYLOAD_SIZE)
    {
        return E_NOMEM;
    }
    if(pkt->payload!=NULL)
    {
        free(pkt->payload);
        pkt->payload=NULL;
        pkt->length=0;
    }
    if((data==NULL)||(length == 0))
    {
        return PKT_OK;
    }
    
    pkt->payload=malloc(length);
    
    if(pkt->payload==NULL)
    {
        return E_NOMEM;
    }
    
    memcpy(pkt->payload,data,length);
    pkt_set_length(pkt,length);    
    return PKT_OK;
}
