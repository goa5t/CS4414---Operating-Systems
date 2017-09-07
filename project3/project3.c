#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linkedlist.h"

//structs
struct page_table{
    int pages[16];
};

struct tlb_struct{
    int page;
    int frame;
};

//Globals

int memory_frames[8];
struct page_table pt;
int free_frames[8];
int frame;

//LRU for frames and FIFO for TLB
struct node *head;
struct node *tlb_head;

//TLB
struct tlb_struct tlb[4];


//Reading functions
int * readIn(int size){
    int * ret;
    int i=0;
    int c;
    FILE *f;
    ret = malloc(sizeof(int) * size);
    f = fopen("addresses.txt", "r");
    do
    {
        c = fscanf(f, "%d\n", &ret[i]);
        i++;
    }while(!feof(f));
    fclose(f);
    return ret;
}

size_t getSize(){
    size_t lines=1;
    int c=0;
    FILE *f;
    f = fopen("addresses.txt", "r");
    if(f==NULL){
        printf("Error in opening file.");
        return -1;
    }
    do
    {
        c = fgetc(f);
	if(c=='\n')
            lines++;
    }while(!feof(f));
    fclose(f);
    return lines;
    
}

int * pageNum(int * in, int size){
    int i=0;
    int * ret;
    ret = malloc(sizeof(int) * size);
    for(i=0; i<size; i++){
        ret[i] = in[i] >> 8;
    }
    return ret ;
}

int checkIfInPT(int pageN, struct page_table pt){
    if(pt.pages[pageN]!=-1)
        return 1;
    else
        return 0;
}
/*
void printArray(int * in, size_t size){
    printf("in printArray, size = %d\n", (int)size);
    int i=0;
    for(i=0;i<size; i++)
        printf("in[%d] = %d\n", i, in[i]);
}

void printPM(){
 int i=0;
	int p = 0;
 for(i=0;i<8; i++){
 for(p=0; p < 256; p +=sizeof(int)){
 printf("%d  ", physical_memory[i][p]);
 }
 printf("\n");
	
	}
 }*/
unsigned char * readBS(int pn){
    char * buffer = malloc(256);
    FILE *f;
    f = fopen("BACKING_STORE.bin", "rb");
    fseek(f, pn *256, SEEK_SET);
    fread(buffer, 1, 256, f);
    fclose(f);
    /*printf("Data = \n");
     for(i=0; i< 256 / sizeof(int); i++)
     printf("%d\n", buffer[i]);
     printf("%d is i \n", i);*/
    return buffer;
}

void updatePT(int pn, int frame){
    pt.pages[pn] = frame;
}

int isFull(){
	int i;
    for(i=0; i < 8; i++){
        if(free_frames[i] == 0)
            return 0;
    }
    return 1;
}

int freeLRU(){
    int pn = getFirst(head);
    int tmp = pt.pages[pn];
    free_frames[tmp] = 0;
    frame = tmp;
    pt.pages[pn] = -1;
    return pn;
}

void printPT(){
    int i=0;
    for(i=0; i < 16; i++){
        if(pt.pages[i]!=-1)
            printf("page: %d in frame: %d\n", i, pt.pages[i]);
        else
            printf("page: %d is not in memory. \n", i);
    }
}
void printMF(){
    int i=0;
    for(i=0; i < 8; i++){
        if(memory_frames[i]!=-1)
            printf("frame %d: page %d.\n", i, memory_frames[i]);
        else
            printf("frame %d: empty.\n", i);
    }
}

int main(){
    
    frame = 0;
    int i, p;
    int size = 0;
    int * input;
    int * pageNums;
    
    //tlb variables
    int tlb_hit = 0;
    int tlb_index;

    //counters
    int total_tlb_hit = 0;
    int total_page_faults = 0;
    
    unsigned char physical_memory[8][256];

	//initialize and malloc arrays
    
    memory_frames[0] = memory_frames[1] = memory_frames[2] = memory_frames[3] = memory_frames[4] = memory_frames[5] = memory_frames[6] = memory_frames[7] = -1;
    pt.pages[0] = pt.pages[1] = pt.pages[2] = pt.pages[3] = pt.pages[4] = pt.pages[5] = pt.pages[6] = pt.pages[7] = pt.pages[8] = pt.pages[9] = pt.pages[10] = pt.pages[11] = pt.pages[12] = pt.pages[13] = pt.pages[14] = pt.pages[15] = -1;
    unsigned char * tmp_buffer = malloc(256);
    tlb[0].page = tlb[1].page = tlb[2].page = tlb[3].page = -1;
    tlb[0].frame = tlb[1].frame = tlb[2].frame = tlb[3].frame = -1;
    head = malloc(sizeof(struct node));
	tlb_head = malloc(sizeof(struct node));

	//read in values
    size = getSize();
    input = malloc(sizeof(int) * size);
    pageNums = malloc(sizeof(int) * size);
    input = readIn(size);
    pageNums = pageNum(input, size);
    
    for(i=0; i<size; i++){
        int tmp = 0;
        tlb_hit = 0;
	tlb_index = 0;

	// pn is the page number for this VA
        int pn = pageNums[i];

	//Check the TLB first
        while(tlb_index < 4 && tlb_hit ==0){
            if(pn == tlb[tlb_index].page){
                total_tlb_hit++;
                tlb_hit = 1;
                printf("Page %d is stored in frame %d, which is stored in entry %d of the tlb.\n", pn, tlb[tlb_index].frame, tlb_index);
            }
            tlb_index++;
            
            
        }

        if(tlb_hit == 0){
	//TLB Miss
            printf("Frame number for page %d is missing from the TLB.\n", pn);
            tmp = checkIfInPT(pn, pt);
            //printf("checking Virtual Address = %d contained in page %d\n", input[i], pn);
            if(tmp == 0){
                
                //Page Fault
                printf("Virtual Address = %d contained in page %d causes a page fault.\n", input[i], pn);
                total_page_faults ++;
                if(isFull()){
			// No free frames, need to free 1 frame using LRU
                    int tmp2 = freeLRU();

                    //update TLB if page number evicted is in TLB
                    tlb_index = 0;
                    while(tlb_index < 4 && tlb[tlb_index].page!=tmp2){
                        tlb_index++;
                    }
                    if(tlb[tlb_index].page == tmp2){
                        tlb[tlb_index].page = -1;
                        tlb[tlb_index].frame = -1;
			removeItem(tlb_head, tlb[tlb_index].page);
                    }
                }
                
                tmp_buffer = readBS(pn);
                printf("Page %d is loaded into frame %d\n", pn, frame);
                
                updatePT(pn, frame);
                
                memory_frames[frame] = pn;
                // copy contents to physical_memory array
                for(p=0; p<256; p++){
                   physical_memory[frame][p] = tmp_buffer[p];
			//printf("%u  ", physical_memory[frame][p]);
		}

                free_frames[frame] = 1;
                
                //add page number to linked list to keep track of LRU
                add(head, pn);
                
                
                
                
            }
            else{
                printf("Page %d is already contained in frame %d.\n", pn, pt.pages[pn]);
                
                // move page number to the end of linked list to keep track of LRU
                moveToEnd(head, pn);
                
            }
            
            //Update TLB
            
            
		tlb_index = 0;
		while(tlb_index < 4 && tlb[tlb_index].page!=-1){
				tlb_index++;
		
		} 
		if(tlb[tlb_index].page != -1){
			//TLB is full
			int tmp_pn = getFirst(tlb_head);
			tlb_index = 0;
			while(tlb_index < 4 && tlb[tlb_index].page !=tmp_pn)
				tlb_index++;
		}
		tlb[tlb_index].page = pn;
            	tlb[tlb_index].frame = pt.pages[pn];
           	 printf("frame %d containing page %d is stored in entry %d of the TLB.\n",pt.pages[pn], pn, tlb_index);

		//add pn to the tlb_head linked list.
		add(tlb_head, pn);
		//increment frame so that it will find the first free frame 
            frame = (frame +1) % 8;
        }
        else{
            // TLB Hit, update LRU linked list
            moveToEnd(head, pn);
            
        }/*
        printf("Page Table:\n");
         printArray(pt.pages, 16);
         printf("TLB: \n");
         printf("tlb[0].page = %d, tlb[0].frame = %d\n", tlb[0].page, tlb[0].frame);
         printf("tlb[1].page = %d, tlb[1].frame = %d\n", tlb[1].page, tlb[1].frame);
         printf("tlb[2].page = %d, tlb[2].frame = %d\n", tlb[2].page, tlb[2].frame);
         printf("tlb[3].page = %d, tlb[3].frame = %d\n", tlb[3].page, tlb[3].frame);
         printf("Now printing the LRU linked list: \n");
         print(head);
	printf("Now printing the TLB FIFO linked list: \n");
	print(tlb_head);
         printf("free_frames:\n");
         printArray(free_frames, 8);
         
	printf("Physical Memory: \n");
	 for(i=0;i<8; i++){
 		for(p=0; p < 256; p +=sizeof(int)){
 			printf("%d  ", physical_memory[i][p]);
 		}
 		printf("\n");
	
	}*/
        printf("\n\n");
    }
    printf("The reference string generated %d page faults in memory out of %d references.\n", total_page_faults, size);
    printf("The TLB hit ratio is %d out of %d references. \n",  total_tlb_hit, size);
    printf("The contents of page table after simulation: \n");
    printPT();
    printf("\n\n");
    printf("The contents of memory after simulation:\n");
    printMF();
    
    
}
