#include<stdio.h>
struct node{
    int val;
    struct node *next;
};

extern void add(struct node* head, int value);
extern void print(struct node *head);
extern int getFirst(struct node *head);
extern void moveToEnd(struct node *head, int value);
