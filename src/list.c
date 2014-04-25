/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#include "list.h"
#include "stdlib.h"

int addToList(List ** head, List * list)
{
  /*
    add list node to the double linked list.
  */

  if(list == NULL) return -1;
  list->next = *head;
  list->prev = NULL;
  if(*head != NULL) (*head)->prev = list;
  *head = list;
  return 0;
}

int rmFromList(List ** head, List * list)
{
  if(list == NULL) return -1;
  if(list->prev == NULL){
    *head = list->next;
    if(*head != NULL) (*head)->prev = NULL;
  }
  else{
    list->prev->next = list->next;
    if(list->next != NULL) list->next->prev = list->prev;
  }
  list->prev = NULL;
  list->next = NULL;
  return 0;
}

List * findListNode(List * search, ListItem * item)
{
  if(item == NULL) return NULL;
  for(; search != NULL; search=search->next){
    if(search->item == item) return search;
  }
  return NULL;
}

List * newListNode()
{
  List * nl = NULL;
  nl = malloc(sizeof(List));
  if(nl == NULL){
    logerr("malloc for new list error");
  }
  return nl;
}
