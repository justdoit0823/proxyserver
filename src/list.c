/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

#include "list.h"
#include "stdlib.h"

int addToList(List ** head, List * list)
{
  list->next = *head;
  list->prev = NULL;
  if(*head != NULL) (*head)->prev = list;
  *head = list;
  return 0;
}

int rmFromList(List ** head, List * list)
{
  if(list->prev == NULL){
    *head = list->next;
    if(*head != NULL) (*head)->prev = NULL;
  }
  else{
    list->prev->next = list->next;
    if(list->next != NULL) list->next->prev = list->prev;
  }
  return 0;
}

List * newListItem()
{
  List * nl = NULL;
  nl = malloc(sizeof(List));
  if(nl == NULL){
    logerr("malloc for new list error");
  }
  return nl;
}
