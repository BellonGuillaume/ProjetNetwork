#include "packet_interface.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

//const time_t TIMEOUT_TIME;

typedef struct node {
  uint8_t seqnum;
  pkt_t* pkt;
  //node_t next;
  //time_t time;
} node_t;

typedef struct window {
  int length; //TAILLE DE LA WINDOW
  int size_used; //NOMBRE DE PLACES OCCUPEES
  node_t** buffer;
} window_t;


window_t* window_new(int length)
{
  window_t* window = malloc(sizeof(window_t));
  window->length = length;
  window->size_used = 0;
  window->buffer = calloc(length,sizeof(node_t));
}

void window_del(window_t* window)
{
  if(window!=NULL)
  {
    if(window->buffer!=NULL)
    {
      free(window->buffer);
      window->buffer=NULL;
    }
    free(window);
    window=NULL;
  }
  return;
}

node_t* node_new(pkt_t* pkt)
{
  node_t* node = malloc(sizeof(node_t));
  node->seqnum=pkt_get_seqnum(pkt);
  node->pkt=pkt;
  //node->time=getTime();
  return node;
}

void node_del(node_t* node)
{
  if(node!=NULL)
  {
    if(node->pkt!=NULL)
    {
      free(node->pkt);
      node->pkt=NULL;
    }
    free(node);
    node=NULL;
  }
  return;
}
 /*
  * Checks if the RTT has expired on one of the window's node and resend it
  * @PRE: socket must be ready to be written on.
  * @POST: one of the elements that had its RTT expired has been sent.
  * @RETURN: 0 if none of the elements had its RTT expired, 1 if one was sent.
 */
/*int window_check_RTT(int sfd, window_t* window)
{
  int count=0;
  for(int i=0;i<window->length;i++)
  {
    if(((window->buffer[i])->time)+TIMEOUT_TIME<getTime())
    {
      send_pkt(sfd, (window->buffer[i])->pkt);
      count++;
      return count?
    }
  }
  return count;
}*/

int window_add(window_t* window, pkt_t* pkt)
{
  if(window->size_used<window->length)
  {
    int i;
    for(i=0;window->buffer[i]!=NULL;i++);
    node_t* node = node_new(pkt);
    window->buffer[i]=node;
    window->size_used++;
    return 0;
  }
  return -1;
}

int window_remove(window_t* window, int seqnum)
{
  if(window->size_used==0)
    return -1;
  int i;
  for(i=0;window->buffer[i]->seqnum!=seqnum;i++);
  if(i==window->length-1)
    return -1;
  window->buffer[i]=NULL;
  return 0;
}
