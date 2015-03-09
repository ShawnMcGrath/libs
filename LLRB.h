/*Linked-List Ring Buffer library by Shawn McGrath (shawn.mcgrath@gmail.com)
  (there might be a real name for this data structure, I have no idea)
	- useful for storing varying numbers of bytes in a ring buffer
		I used it for an undo buffer to fit within a fixed memory size
		can quickly jump to last inserted set and jump forwards and backwards
		but has no random access
	
	it's in the public domain, do whatever you want with it.

 ============================================================================
   You MUST                                                                  
                                                                             
      #define LLRB_DEFINE                                                     
                                                                             
   in EXACTLY _one_ C or C++ file that includes this header, BEFORE the
   include, like this:                                                                
                                                                             
      #define LLRB_DEFINE                                                     
      #include "LLRB.h"
      
   All other files should just #include "LLRB.h" without the #define.
 ============================================================================

 Version History:

 v0.1	(Mar 8, 2015)	-- first version, used as the N++ editor's undo buffer, no real testing done!


I stole how to do this easy portable library thing from Sean Barret (nothings.org) (he has better libs there)*/

#ifdef __cplusplus
   #define LLRB_EXTERN   extern "C"
#else
   #define LLRB_EXTERN   extern
#endif

// check for well-known debug define
#if defined(DEBUG) || defined(_DEBUG) || defined(DBG)
   #ifndef NDEBUG
      #define LLRB_DEBUG
   #endif
#endif


#ifdef LLRB_DEFINE
#ifdef LLRB_DEBUG
#include <assert.h>
#endif
#include <stdlib.h>
#endif

#ifdef LLRB_DEBUG
#define LLRB_ASSERT(x) assert(x)
#else
#define LLRB_ASSERT(x)
#endif

#ifndef LLRB_H_INCLUDE_GUARD_H_
#define LLRB_H_INCLUDE_GUARD_H_


struct LLRBHeader {
	int info; //for you to use
	int num_bytes;
	LLRBHeader *next;
	LLRBHeader *prev;
};

struct LLRB {
	unsigned char *data;
	size_t data_size;

	LLRBHeader *write_header;
	LLRBHeader *last_header;
};

LLRB_EXTERN void initLLRB(LLRB *llrb, unsigned char *data, size_t data_size);
LLRB_EXTERN void resetLLRB(LLRB *llrb);
LLRB_EXTERN unsigned char *pushLLRB(LLRB *llrb, unsigned char *data, size_t num_bytes, int info);
LLRB_EXTERN unsigned char *undoLLRB(LLRB *llrb);
LLRB_EXTERN unsigned char *redoLLRB(LLRB *llrb);
LLRB_EXTERN size_t getLLRBNumWriteBytes(LLRB *llrb); //returns the num bytes pointed to by the write header (useful for undo and redo);

#endif

#ifdef LLRB_DEFINE

void initLLRB(LLRB *llrb, unsigned char *data, size_t data_size) {
	if (data) {
		llrb->data = data;
	} else {
		llrb->data = (unsigned char *)malloc(data_size);
		LLRB_ASSERT(llrb->data);
	}
	llrb->data_size = data_size;

	llrb->write_header = 0;
	llrb->last_header = 0;
}

void resetLLRB(LLRB *llrb) {
	llrb->write_header = 0;
	llrb->last_header = 0;
}

unsigned char *pushLLRB(LLRB *llrb, unsigned char *data, size_t num_bytes, int info) {
	LLRB_ASSERT(num_bytes < llrb->data_size);
	if (num_bytes >= llrb->data_size) {
		return 0;
	}

	LLRBHeader *prev_header = llrb->write_header;
	if (!llrb->write_header) {
		//nothing's been written yet
		llrb->write_header = (LLRBHeader *)llrb->data;
		llrb->last_header = llrb->write_header;
	} else {
		unsigned char *data_start = (unsigned char *)llrb->write_header + sizeof(LLRBHeader) + llrb->write_header->num_bytes;
		if (data_start + num_bytes > llrb->data + llrb->data_size) {
			llrb->write_header = (LLRBHeader *)llrb->data;
		} else {
			//no wrapping, just setup the write header like normal
			llrb->write_header = (LLRBHeader *)data_start;
		}
		while ((unsigned char *)llrb->last_header - (unsigned char *)llrb->write_header < num_bytes + sizeof(LLRBHeader)) {
			if ((unsigned char *)llrb->write_header + sizeof(LLRBHeader) + num_bytes > llrb->data + llrb->data_size) {
				//we have to wrap
				llrb->write_header = (LLRBHeader *)llrb->data;
			}
			llrb->last_header = llrb->last_header->next;
			llrb->last_header->prev = 0;
		}
	}

	unsigned char *write_pos = (unsigned char *)llrb->write_header + sizeof(LLRBHeader);
	LLRB_ASSERT(write_pos + num_bytes < llrb->data + llrb->data_size);

	llrb->write_header->num_bytes = num_bytes;
	llrb->write_header->info = info;
	llrb->write_header->next = 0;
	llrb->write_header->prev = prev_header;
	if (prev_header) {
		prev_header->next = llrb->write_header;
	}

	if (data) {
		memcpy(write_pos, data, num_bytes);
	}
	return write_pos;
}

unsigned char *undoLLRB(LLRB *llrb) {
	if (llrb->write_header && llrb->write_header->prev) {
		llrb->write_header = llrb->write_header->prev;

		unsigned char *write_pos = (unsigned char *)llrb->write_header + sizeof(LLRBHeader);
		return write_pos;
	}
	return 0;
}

unsigned char *redoLLRB(LLRB *llrb) {
	if (llrb->write_header && llrb->write_header->next) {
		llrb->write_header = llrb->write_header->next;

		unsigned char *write_pos = (unsigned char *)llrb->write_header + sizeof(LLRBHeader);
		return write_pos;
	}
	return 0;
}

size_t getLLRBNumWriteBytes(LLRB *llrb) {
	if (!llrb->write_header) {
		return 0;
	}
	return llrb->write_header->num_bytes;
}

#endif //LLRB_DEFINE
