/* *****************************************************************************
 *
 * Copyright (c) 2014 Alexis Naveros. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * *****************************************************************************
 */


/*
#define MM_HASH_DEBUG_STATISTICS
*/


enum
{
  MM_HASH_ENTRYCMP_INVALID,
  MM_HASH_ENTRYCMP_FOUND,
  MM_HASH_ENTRYCMP_SKIP
};

enum
{
  MM_HASH_ENTRYLIST_BREAK,
  MM_HASH_ENTRYLIST_CONTINUE
};

typedef struct
{
  /* Clear the entry so that entryvalid() returns zero */
  void (*clearentry)( void *entry );
  /* Returns non-zero if the entry is valid and existing */
  int (*entryvalid)( void *entry );
  /* Return key for an arbitrary set of user-defined data */
  uint32_t (*entrykey)( void *entry );
  /* Return MM_HASH_ENTRYCMP* to stop or continue the search */
  int (*entrycmp)( void *entry, void *entryref );
  /* Return MM_HASH_ENTRYLIST* to stop or continue the search */
  int (*entrylist)( void *opaque, void *entry, void *entryref );
} mmHashAccess;


/* Do not keep track of entry count, table will not be able to say when it needs to shrink or grow */
#define MM_HASH_FLAGS_NO_COUNT (0x1)


size_t mmHashRequiredSize( size_t entrysize, uint32_t hashbits, uint32_t pageshift );
void mmHashInit( void *hashtable, mmHashAccess *access, size_t entrysize, uint32_t hashbits, uint32_t pageshift, uint32_t flags );
int mmHashGetStatus( void *hashtable, int *rethashbits );

enum
{
  MM_HASH_STATUS_MUSTGROW,
  MM_HASH_STATUS_MUSTSHRINK,
  MM_HASH_STATUS_NORMAL,
  MM_HASH_STATUS_UNKNOWN
};


void *mmHashDirectFindEntry( void *hashtable, mmHashAccess *access, void *findentry );
void *mmHashLockFindEntry( void *hashtable, mmHashAccess *access, void *findentry );

void mmHashDirectListEntry( void *hashtable, mmHashAccess *access, void *listentry, void *opaque );
void mmHashLockListEntry( void *hashtable, mmHashAccess *access, void *listentry, void *opaque );

int mmHashDirectReadEntry( void *hashtable, mmHashAccess *access, void *readentry );
int mmHashLockReadEntry( void *hashtable, mmHashAccess *access, void *readentry );

int mmHashDirectCallEntry( void *hashtable, mmHashAccess *access, void *callentry, void (*callback)( void *opaque, void *entry, int newflag ), void *opaque, int addflag );
int mmHashLockCallEntry( void *hashtable, mmHashAccess *access, void *callentry, void (*callback)( void *opaque, void *entry, int newflag ), void *opaque, int addflag );

/* The hash key for replaced entries must remain the same! */
int mmHashDirectReplaceEntry( void *hashtable, mmHashAccess *access, void *replaceentry, int addflag );
int mmHashLockReplaceEntry( void *hashtable, mmHashAccess *access, void *replaceentry, int addflag );

int mmHashDirectAddEntry( void *hashtable, mmHashAccess *access, void *adddentry, int nodupflag );
int mmHashLockAddEntry( void *hashtable, mmHashAccess *access, void *adddentry, int nodupflag );

int mmHashDirectDeleteEntry( void *hashtable, mmHashAccess *access, void *deleteentry, int readflag );
int mmHashLockDeleteEntry( void *hashtable, mmHashAccess *access, void *deleteentry, int readflag );

void mmHashResize( void *newtable, void *oldtable, mmHashAccess *access, uint32_t hashbits, uint32_t pageshift );


enum
{
  MM_HASH_FAILURE,
  MM_HASH_SUCCESS,
  MM_HASH_TRYAGAIN /* Internal, can not be returned by any public call */
};



////



typedef struct
{
  uint32_t hashkey;
  uint32_t pagestart;
  uint32_t pagefinal;
  void *next;
} mmHashLockRange;

typedef struct
{
  void *hashtable;
  void *rangelist;
  int newcount;
} mmHashLock;


void mmHashLockInit( mmHashLock *hashlock, int newcount );
void mmHashLockAdd( void *hashtable, mmHashAccess *access, void *entry, mmHashLock *hashlock, mmHashLockRange *lockrange );
void mmHashLockAcquire( void *hashtable, mmHashAccess *access, mmHashLock *hashlock );
void mmHashLockRelease( void *hashtable, mmHashLock *hashlock );

void mmHashGlobalLockEnable( void *hashtable );
void mmHashGlobalLockDisable( void *hashtable );



////



void mmHashDirectDebugDuplicate( void *hashtable, mmHashAccess *access, void (*callback)( void *opaque, void *entry0, void *entry1 ), void *opaque );

void mmHashDirectDebugPages( void *hashtable );

void mmHashStatistics( void *hashtable, long *accesscount, long *collisioncount, long *relocationcount, long *entrycount, long *entrycountmax, long *hashsizemax );



