/*

  Author: Just do it.
  Email: justdoit920823@gmail.com
  Website: http://www.shareyou.net.cn/

*/

/*
  A simple double linked list implement.

*/


#ifndef LIST_H
#define LIST_H

#include "stdio.h"
#include "http.h"

typedef struct HTTPConnection ListItem;

typedef struct _List{
  ListItem * item;
  struct _List * prev;
  struct _List * next;
} List;
       
int addToList(List ** head, List * list);

int rmFromList(List ** head, List * list);

List * findListNode(List * search, ListItem * item);

List * newListNode(void);

#endif
