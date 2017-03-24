/*
 * Author: ERIC SHEEN
 */
/* we need this for uintptr_t */
#include <stdint.h>
/* we need this for memcpy/memset */
#include <string.h>
/* we need this for my_sbrk */
#include "my_sbrk.h"
/* we need this for the metadata_t struct definition */
#include "my_malloc.h"

/* You *MUST* use this macro when calling my_sbrk to allocate the
 * appropriate size. Failure to do so may result in an incorrect
 * grading!
 */
#define SBRK_SIZE 2048

/* If you want to use debugging printouts, it is HIGHLY recommended
 * to use this macro or something similar. If you produce output from
 * your code then you may receive a 20 point deduction. You have been
 * warned.
 */
#ifdef DEBUG
#define DEBUG_PRINT(x) printf(x)
#else
#define DEBUG_PRINT(x)
#endif

/* Our freelist structure - this is where the current freelist of
 * blocks will be maintained. failure to maintain the list inside
 * of this structure will result in no credit, as the grader will
 * expect it to be maintained here.
 * DO NOT CHANGE the way this structure is declared
 * or it will break the autograder.
 */

void setCanary(metadata_t* meta);
 
typedef int bool;
#define true 1
#define false 0
 
metadata_t* freelist = NULL;

void* my_malloc(size_t size) {
	/////////////////////////
	if (size < 0) { // in case having negative size.
       	return NULL; 
   	}

	int neededsize = sizeof(metadata_t) + sizeof(int) + size;
	
	if (neededsize > SBRK_SIZE) { // For size request that is TOO big!
		ERRNO = SINGLE_REQUEST_TOO_LARGE; // Give the Error
		return NULL;
	} else {
		ERRNO = NO_ERROR; // Give the Error
	}
		
	metadata_t* freelistptr = freelist;
	metadata_t* prevNode = NULL;
	while (freelistptr != NULL) {
		if (freelistptr->block_size >= neededsize) break;
		prevNode = freelistptr;
		freelistptr = freelistptr->next;
	}
	
//	int tempcount = 0; //MINE
	
	//when there is no block large enough
	if (freelistptr == NULL) {
		freelistptr = (metadata_t*)my_sbrk(SBRK_SIZE);
		
		if (freelistptr == NULL) {
			ERRNO = OUT_OF_MEMORY;
			return NULL;
		}

		//tempCount++; //MINE
		
		if (freelist != NULL) {
			metadata_t* fptr = freelist;
			metadata_t* fprev = NULL;
			while (fptr != NULL) {
				if ((uintptr_t) (fptr) > (uintptr_t)(freelistptr)) {
					break;
				}
				fprev = fptr;
				fptr = fptr->next;
			}
			
			if (fprev == NULL) {
				freelistptr->next = freelist;
				freelist = freelistptr;
			} else {
				freelistptr->next = fptr;
				fprev->next = freelistptr;
				prevNode = fprev;
			}
			
		} else {
			freelistptr->next = NULL;
			freelist = freelistptr;
		}
		freelistptr->block_size = SBRK_SIZE;
		///////XOR
		freelistptr->request_size = 0;
		setCanary(freelistptr);
	}
	
	//use found block at freelistptr

	metadata_t* user = freelistptr;
	metadata_t* tempPtr = user->next;
	user->next = NULL;
	
	int tempSize = user->block_size;
	user->block_size = neededsize;
	
	user->request_size = size;
	setCanary(user);
	
	//move freelistptr to new front of free space
	if (neededsize < (tempSize-sizeof(metadata_t) - sizeof(int))) {
		freelistptr = (metadata_t*)((char*)freelistptr + neededsize);
		freelistptr->next = tempPtr;
		freelistptr->block_size = tempSize - neededsize;
		
		freelistptr->request_size = 0;
		setCanary(freelistptr);
		
		if (prevNode == NULL)
			freelist = freelistptr;
		else
			prevNode->next = freelistptr;
	} else {
		if (prevNode == NULL)
			freelist = tempPtr;
		else
			prevNode->next = tempPtr;
	}
	
	
	void* user_MEM = ((void*) (((char*) user) + sizeof(metadata_t) ));
	//ERRNO = NO_ERROR;
	return user_MEM;
}
	

void* my_realloc(void* ptr, size_t new_size) {

	if (sizeof(metadata_t) + sizeof(int) + new_size > SBRK_SIZE) { // For size request that is TOO big!
		ERRNO = SINGLE_REQUEST_TOO_LARGE; // Give the Error
		return NULL;
	}
	
	void* tempPtr = ptr;
	my_free(tempPtr);
	
	if (tempPtr != NULL) {
		tempPtr = (metadata_t*)((char*)tempPtr - sizeof(metadata_t));
	}
	if (new_size != 0) {
		tempPtr = my_malloc(new_size);
		return tempPtr;	
	} 
	
	
	
	return NULL;

}

void* my_calloc(size_t nmemb, size_t size) {
	/////////////////////////
	void *memory = my_malloc(nmemb * size); 
	if (memory == NULL) {  
		return NULL;
	}
	char *temp = (char *) memory; 
	char *endTemp = temp + (nmemb * size); 
	while (temp < endTemp) { // go from beginning to end
		*temp = 0;
		temp++;
	}
	ERRNO = NO_ERROR; // shows that there is not an error
	return memory;
}

	
void my_free(void* ptr) {

	if (ptr == NULL) {
		return;
	}
	
    // Make a pointer to the metadata
    metadata_t* metaData = (metadata_t*) ((char*) ptr - sizeof(metadata_t));
    unsigned short requestSize = metaData->request_size;
    
//	metaData->block_size = (int) (sizeof(int) + sizeof(metaData));
	
  //  blockPtr = (char*) metaData;
    if ((unsigned int)(((((int)metaData->block_size) << 16) | ((int)metaData->request_size)) ^ (int)(uintptr_t)metaData) != metaData->canary) {
        // Canary #1 is corrupted
        ERRNO = CANARY_CORRUPTED;
        return;
    } else {
		ERRNO = NO_ERROR;
	}

	metaData->request_size = 0;

	int* secondCanary = (int*) ((char*)ptr + requestSize);
	
	if  (*secondCanary != metaData->canary) {
	    ERRNO = CANARY_CORRUPTED;
        return;
    } else {
		ERRNO = NO_ERROR;
	}
	
	metadata_t* freelistptr = freelist;
	metadata_t* prev = NULL;
	while (freelistptr != NULL) {
		if ((uintptr_t) (freelistptr) > (uintptr_t)(metaData)) {
			break;
		}
		prev = freelistptr;
		freelistptr = freelistptr->next;
		
	}
	
	metadata_t* left = prev;
	metadata_t* right = freelistptr;
	
	
	if (left!=NULL && right!=NULL) {
		if  (((uintptr_t) ((char*) metaData + metaData->block_size) == (uintptr_t) right) && ((uintptr_t) ((char*) left + left->block_size) == (uintptr_t) metaData)) {
			left->next = right->next;
			left->block_size += metaData->block_size + right->block_size;
			setCanary(left);
		} else if ((uintptr_t) ((char*) metaData + metaData->block_size) == (uintptr_t) right) {
			if (left == NULL) {
				freelist = metaData;
			} else {
				left->next = metaData;
			}
			metaData->block_size = metaData->block_size + right->block_size;
			metaData->next = right->next;
			setCanary(metaData);
			
		} else if ((uintptr_t) ((char*) left + left->block_size) == (uintptr_t) metaData) {
			
			left->block_size = left->block_size + metaData->block_size;
			setCanary(left);
		} else {
			left->next = metaData;
			metaData->next = right;
			setCanary(metaData);
		}
		
	} else {
		if (left != NULL) {		
			if ((uintptr_t) ((char*) left + left->block_size) == (uintptr_t) metaData) {	
				left->block_size = left->block_size + metaData->block_size;
				setCanary(left);
			} else {
				metaData->next = NULL;
				left->next  = metaData;
				setCanary(metaData);
			}
		} else if (right != NULL) {
			if ((uintptr_t) ((char*) metaData + metaData->block_size) == (uintptr_t) right) {
				if (left == NULL) {
					freelist = metaData;
				} else {
					left->next = metaData;
				}
				metaData->block_size = metaData->block_size + right->block_size;
				metaData->next = right->next;
				setCanary(metaData);				
			} else {
				freelist = metaData;
				metaData->next = right;
				setCanary(metaData);
			}
		} else {
			freelist = metaData;
			metaData->next = NULL;
			setCanary(metaData);
		}
	}
	
	
	
}


void setCanary(metadata_t* meta) {
	meta->canary = (unsigned int)(((((int)meta->block_size) << 16) | ((int)meta->request_size)) ^ (int)(uintptr_t)meta); 
	
	unsigned int* Rcanary = (unsigned int*)((char*)meta + (meta->block_size - sizeof(int)));
	*Rcanary = meta->canary;
}
		
 
