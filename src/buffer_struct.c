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
  }
  return;
}

/*int window_check_RTT(window_t* window)
{
  int count=0;
  for(int i=0;i<window->length;i++)
  {
    if(((window->buffer[i])->time)+TIMEOUT_TIME<getTime())
    {
      send_socket((window->buffer[i])->pkt);
      count++;
      //return count?
    }
  }
  return count;
}*/

int window_add(window_t* window, pkt_t* pkt)
{
  if(window->size_used<window->length)
  {
    int i;
    for(i=0;window->buffer[i]!=NULL;i++){};
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
  window->size_used--;
  return 0;
}

int window_

/*

int main(int argc, char const *argv[]) {
  int length = 10;
  window_t* test = window_new(length);
  if (test == NULL){
    printf("Error : the created window is NULL\n");
    return EXIT_FAILURE;
  }
  else if(test->length == length && test->size_used == 0 && test->buffer != NULL){
    printf("Success : the window is correctly created\n");
  }
  else {
    printf("Error : the created window is not initialized correctly\n");
    return EXIT_FAILURE;
  }
  pkt_t* pktest = pkt_new();
  node_t* nodetest = node_new(pktest);
  if(nodetest==NULL)
  {
    printf("Error : the node test was not created\n");
    return EXIT_FAILURE;
  }
  else if(nodetest->pkt == pktest && nodetest->seqnum == pkt_get_seqnum(pktest)){
    printf("Success : the node test is correctly created\n");
  }
  else{
    printf("Error : the created node is not initialized correctly\n");
    return EXIT_FAILURE;
  }
   if(window_add(test, pktest) < 0){
     printf("Error : the window was not added to the buffer\n");
     return EXIT_FAILURE;
   }
   else if(test->length == length && test->size_used == 1 && test->buffer != NULL){
     printf("Success : the pkt was correctly added to the buffer\n");
   }
   else{
     printf("Error : the pkt wasn't correctly added to the buffer\n");
     return EXIT_FAILURE;
   }
   node_del(nodetest);
   if(window_remove(test, pkt_get_seqnum(pktest)) < 0){
     printf("Error : the node is not removed from the buffer\n");
     return EXIT_FAILURE;
   }
   else if(test->size_used == 0 && test->buffer != NULL){
     printf("Success : the node was correctly removed from the buffer\n");
   }
   else{
     printf("Error : the window wasn't updated --- Size : %d\n",test->size_used);
     return EXIT_FAILURE;
   }
   window_del(test);

   printf("Success : all the steps are correctly effectued\n");
  return EXIT_SUCCESS;
}
*/
