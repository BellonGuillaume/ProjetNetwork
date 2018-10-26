#include "packet_interface.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <time.h>

const clock_t TIMEOUT_TIME = 4;

typedef struct node {
  uint8_t seqnum;
  pkt_t* pkt;
  clock_t time_init;
} node_t;

typedef struct window {
  int length; //TAILLE DE LA WINDOW
  int size_used; //NOMBRE DE PLACES OCCUPEES
  node_t** buffer;
} window_t;

/*typedef struct receiving_window {
int length;
node_t** buffer;
} rwindow_t;*/

window_t* window_new(int length)
{
  window_t* window = malloc(sizeof(window_t));
  if(window == NULL) {
    //fprintf(stderr,"Error : malloc new window");
    return NULL;
  }
  window->length = length;
  window->size_used = 0;
  window->buffer = calloc(length,sizeof(node_t));
  int i;
  for(i = 0;i<window->length;i++)
  {
    window->buffer[i]=NULL;
  }
  return window;
}

node_t* node_new(pkt_t* pkt)
{
  node_t* node = malloc(sizeof(node_t));
  node->seqnum=pkt_get_seqnum(pkt);
  node->pkt=pkt;
  node->time_init=clock()/CLOCKS_PER_SEC;
  return node;
}

void node_del(node_t* node)
{
  if(node!=NULL)
  {
    if(node->pkt!=NULL)
    {
      pkt_del(node->pkt);
      node->pkt=NULL;
    }
    free(node);
  }
  return;
}

void window_del(window_t* window)
{
  if(window!=NULL)
  {
    if(window->buffer!=NULL)
    {
      int i;
      for(i=0;i<window->length;i++)
      {
        if(window->buffer[i]!=NULL)
        {
          node_del(window->buffer[i]);
        }
      }
      free(window->buffer);
      window->buffer=NULL;
    }
    free(window);
    window=NULL;
  }
  return;
}

node_t* window_node_with_seqnum(window_t* window, uint8_t r_seqnum)
{
  int i;
  for(i=0;i<window->length;i++)
  {
    if(window->buffer[i]!=NULL){
      if(((window->buffer[i])->seqnum) == r_seqnum)
      {
        return window->buffer[i];
      }
    }
  }
  return NULL;
}

node_t* window_check_RTT(window_t* window)
{
  int i;
  for(i=0;i<window->length;i++)
  {
    if(window->buffer[i]!=NULL){
      if(((window->buffer[i])->time_init)+TIMEOUT_TIME<(clock()/CLOCKS_PER_SEC))
      {
        return window->buffer[i];
      }
    }
  }
  return NULL;
}

int window_add(window_t* window, pkt_t* pkt, clock_t time_initialize)
{
  if(window->size_used<window->length)
  {
    //printf("%d,%d\n",window->size_used,window->length);
    node_t* node = node_new(pkt);
    window->buffer[window->size_used]=node;
    node->time_init = time_initialize;
    window->size_used++;
    return 0;
  }
  return -1;
}

pkt_t* window_find(window_t* window, int seqnum)
{
  int i;
  for(i=0;i<window->size_used;i++)
  {
    if(window->buffer[i]->seqnum==seqnum)
    {
      return window->buffer[i]->pkt;
    }
  }
  return NULL;
}

void window_remove_first(window_t* window)
{
  node_del(window->buffer[0]);
  int i;
  for(i=1;i<window->length;i++)
  {
    window->buffer[i-1]=window->buffer[i];
  }
  window->buffer[window->length-1]=NULL;
  window->size_used--;
}

void window_remove_until(window_t* window,int i)
{
  int j;
  for(j=0;j<=i;j++)
  {
    window_remove_first(window);
  }
}

void window_remove(window_t* window, int seqnum)
{
  if(window->size_used==0)
  return;
  int i;
  for(i=0;window->buffer[i]->seqnum!=seqnum && i<window->length;i++);
  if(i==window->length)
  return;
  window_remove_until(window,i);
}

int window_is_full(window_t* window)
{
  if(window->length==window->size_used)
  {
    return 1;
  }
  return 0;
}
