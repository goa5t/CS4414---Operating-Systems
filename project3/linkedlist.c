#include<stdio.h>
#include<stdlib.h>
#include "linkedlist.h"


void add(struct node* head, int value){
    struct node * current = head;
    while(current->next !=NULL)
        current = current->next;
    
    struct node * new_node = malloc(sizeof(struct node));
    new_node->val = value;
    current->next = new_node;
    new_node->next = NULL;
    
}


void print(struct node *head){
    while(head!=NULL){
        printf("Node is %d\n", head->val);
        head = head->next;
    }
}
int getFirst(struct node *head){
    int ret = 0;
    struct node * tmp = head;
    tmp = tmp->next;
    ret = tmp->val;
    
    tmp=tmp->next;
    head->next=tmp;
    
    return ret;
    
}

void moveToEnd(struct node *head, int value){
    int found = 0;
    struct node * current = head;
    struct node * tmp = NULL;
    while(current->next !=NULL){
       // printf("checking node: %d\n", current->next->val);
        if(current->next->val == value && found==0){
            tmp = current->next;
           // printf("tmp val is %d\n", tmp->val);
            current->next = current->next->next;
           // printf("current->next->val is %d\n", current->next->val);
           // printf("current->next->next->val is %d\n", current->next->next->val);
            tmp->next = NULL;
            found = 1;
        }
            current = current->next;
       // printf("test sf\n");
    }
    if(found==1)
        current->next = tmp;
    
}

void removeItem(struct node *head, int value){
struct node * current = head;
struct node * prev = NULL;

while(current != NULL){
	
	if(current->val == value){
		if(prev==NULL)
			head = current->next;
		else{
			prev->next = current->next;
		}
		free(current);
		return;
	}
	prev = current;
	current = current->next;
}
}

/*int main(){
    struct node *head = NULL;
    head = malloc(sizeof(struct node));
    printf("before add\n");
    add(head, 1);
    add(head, 2);
    add(head, 3);
    add(head, 4);
    add(head, 5);
    add(head, 6);
    add(head, 7);
    add(head, 8);
    add(head, 9);
    add(head, 10);
    printf("after add\n");
    print(head);
    int first = getFirst(head);
    printf("first item is %d\n", first);
    print(head);
	printf("removing 5\n");
	removeItem(head, 5);
	print(head);
	printf("removing 2\n");
	removeItem(head, 2);
	print(head);
	printf("removing 9\n");
	removeItem(head, 9);
	print(head);
    first = getFirst(head);
    printf("first item is %d\n", first);
    print(head);
    moveToEnd(head, 5);
    printf("After moveToEnd: \n");
    print(head);
    moveToEnd(head, 3);
    printf("After moveToEnd: \n");
    print(head);
    moveToEnd(head, 9);
    printf("After moveToEnd: \n");
    print(head);
    moveToEnd(head, 16);
    printf("After moveToEnd: \n");
    print(head);
    
    return 0;
}*/
