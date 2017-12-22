#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include "run.h"
#include "util.h"

void *base = 0;

p_meta find_meta(p_meta *last, size_t size) { 
  p_meta index = base; //current block
  p_meta result = NULL; 

  switch(fit_flag){
    case FIRST_FIT:
    {
       for(; index; index=index->next){
         if((index->size) >= size && index->free == 1){
           result = index;
           break;
         }
       *last=index;
       }
    }
    break;

    case BEST_FIT:
    {
      for(; index; index = index->next){
        if((index->size)>size && index->free == 1){
	  if(result == NULL) result = index;
          else if(result->size > index->size){
             result = index;
          }  
        }
	*last = index;
      } 

    }
    break;

    case WORST_FIT:
    {
      for(; index; index = index->next){
        if((index->size)>size && index->free == 1){
	  if(result == NULL) result = index;
          if(result->size < index->size){
            result = index;
          }
        }
      *last=index;
      }
    }
    break;

  }
  return result;
}

void *m_malloc(size_t size) {
  p_meta block;
  block = sbrk(0);
  if(size <= 0){
    return NULL;
  }
  int num = size/4;
  if(size%4 !=0)
    num+=1;
  size = num*4; 
  if(base == 0){ 
    block = sbrk(size + META_SIZE);
    if(!block){
      return NULL;
    }
    block->size = size;
    block->prev = NULL;
    block->next = NULL;
    block->free = 0;
    base = block;
  }else{
    p_meta last = base;
    block = find_meta(&last, size);
    if(!block){ 
      block = sbrk(size + META_SIZE);
      if(!block){
        return NULL;
      }
      block->size = size;
      block->prev = last;
      block->next = NULL;
      block->free = 0;
      last->next = block;
    }
    else{ // free == 1 && size can allocatable
      if((block->size) > size + META_SIZE){
        last = block;
        block = (void*)(last) + size + META_SIZE;
        block->size = last->size-META_SIZE-size;
        block->prev = last;
        block->next = last->next; 
        block->free = 1;
        block->next->prev = block;
        last->next = block;
        last->size = size;
        block = last;
      }      
    }
    block->free = 0;
  }
  return((void*)block+META_SIZE);
}

void m_free(void *ptr) {
  if(!ptr){
    return;
  }
  p_meta block_ptr = ptr - META_SIZE;
  block_ptr->free = 1;

  if(block_ptr->prev!=NULL)
  if(block_ptr->prev->free == 1){
    block_ptr->prev->size += block_ptr->size + META_SIZE;
    block_ptr->prev->next = block_ptr->next;
    block_ptr->next->prev = block_ptr->prev;
    block_ptr = block_ptr->prev;
  }
  if(block_ptr->next!=NULL)
  if(block_ptr->next->free == 1){
    block_ptr->size += block_ptr->next->size + META_SIZE;
    block_ptr->next->next->prev = block_ptr;
    block_ptr->next = block_ptr->next->next;
  }
  if(block_ptr->next == NULL){
    block_ptr->prev->next = NULL;
    brk(block_ptr->prev);//use up to the previous block of freed block
  }
}

void* m_realloc(void* ptr, size_t size)
{
  if(!ptr){
    return m_malloc(size);
  }
  p_meta last;   
  p_meta block_ptr = ptr-META_SIZE;
  int o_size = block_ptr->size;
  if(block_ptr->size >= size){
    int num = size/4;
    if(size%4 != 0) num++;
    size = num*4;
    if((block_ptr->size) > size+META_SIZE){
      p_meta last = block_ptr;
      block_ptr = (void*)(last) + size + META_SIZE;
      block_ptr->size = last->size - size - META_SIZE; 
      block_ptr->next = last->next;
      block_ptr->prev = last;
      block_ptr->free = 1;
      block_ptr->next->prev = block_ptr;
      last->next = block_ptr;
      last->size = size;
      block_ptr =last;
    }
    return ptr;
  }
  void *new_ptr;
  block_ptr = find_meta(&last, size);
  if(block_ptr==NULL){
    new_ptr = m_malloc(size);
    if(!new_ptr) {
      return NULL;
    }
    memcpy(new_ptr, ptr, o_size);
    m_free(ptr);
    return new_ptr;
  }else{
    m_free(ptr);
    new_ptr = m_malloc(size);
    if(!new_ptr){
      return NULL;
    }
    memcpy(new_ptr, ptr, block_ptr->size);
  }
  return new_ptr;
}
