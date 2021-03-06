#include "inode_manager.h"
#include <vector>
using std::vector;

const int RATE = 4;

// block layer -----------------------------------------

pthread_mutex_t block_manager::mutex = PTHREAD_MUTEX_INITIALIZER;

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
	/*
	 * your lab1 code goes here.
	 * note: you should mark the corresponding bit in block bitmap when alloc.
	 * you need to think about which block you can start to be allocated.

alloc
	 *hint: use macro IBLOCK and BBLOCK.
					use bit operation.
					remind yourself of the layout of disk.
	 */
	pthread_mutex_lock(&mutex);
	char buf[BLOCK_SIZE];
	// find the start block index for files
	// start_index = super_block number + bitmap block number + inode block number
	blockid_t bnum = 2 + sb.nblocks / BPB + sb.ninodes / IPB;
	while(bnum < sb.nblocks) {
		read_block(BBLOCK(bnum), buf);
		uint32_t index = BLOCK_BITMAP_INDEX(bnum);
		uint32_t offset = BLOCK_BITMAP_OFFSET(bnum); 
		if(BIT_STATE(buf[index], offset) == 0) {
			buf[index] = (buf[index] | (1 << offset));
			write_block(BBLOCK(bnum), buf);
			pthread_mutex_unlock(&mutex);
			return bnum;
		}
		bnum++;
	}
	printf("block_manager::alloc_block::no more storage.\n");
	pthread_mutex_unlock(&mutex);
	return -1;
}

// used for test
void printBit(char c) {
	for(int i = 7; i >= 0; --i){
		if(c & (1<<i))
			printf("1");
		else 
			printf("0");
	}
	printf("\n");
}

void
block_manager::free_block(uint32_t id)
{
	/* 
	 * your lab1 code goes here.
	 * note: you should unmark the corresponding bit in the block bitmap when free.
	 */
	if(id < 0 || id > sb.nblocks) {
		printf("block_manager::free_block::block id is out of range:%u\n", id);
		return;
	}
	
	pthread_mutex_lock(&mutex);
	char buf[BLOCK_SIZE];

	// unmark bitmap
	read_block(BBLOCK(id), buf);
	uint32_t index = BLOCK_BITMAP_INDEX(id);
	uint32_t offset = BLOCK_BITMAP_OFFSET(id);
	buf[index] = (buf[index] & (~(1 << offset)));
	// update bitmap
	write_block(BBLOCK(id), buf);
	pthread_mutex_unlock(&mutex);
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
	d = new disk();

	// format the disk
	sb.size = BLOCK_SIZE * BLOCK_NUM;
	sb.nblocks = BLOCK_NUM / RATE;
	sb.ninodes = INODE_NUM;
}

/**
 * using low 4 bit of char A to construct char with parity bit.
 * @param  A 0000abcd 
 * @return   (abc)(abd)(acd)(bcd)abcd
 */
static char constructCodedChar(char A) {
	char t = 0;
	char a = (A>>3)&1; //a
	char b = (A>>2)&1; //b
	char c = (A>>1)&1; //c
	char d = (A>>0)&1; //d
	t = t | ((b^c^d) << 4);
	t = t | ((a^c^d) << 5);
	t = t | ((a^b^d) << 6);
	t = t | ((a^b^c) << 7);
	t = t | (A & ((1<<4) - 1));

	return t;
}

/**
 * check the correctness of the char with parity bit
 * @param  A (abc)(abd)(acd)(bcd)abcd
 * @return   0  correct, ans will equal to  0000abcd
 *           1  one bit error, ans A will be corrected, 
 *           	ans will be the correct origin char equal to 0000abcd
 *           2  two bit error
 */
static int checkCharCorrectness(char A, char &ans) {
	vector<int> p;  // d, c, b, a, bcd, acd, abd, abc
	for(int i = 0; i < 8; ++i) {
		p.push_back((A>>i)&1);
	}
	bool p1 = p[0]^p[1]^p[2]^p[4];
	bool p2 = p[3]^p[1]^p[0]^p[5];
	bool p3 = p[3]^p[2]^p[0]^p[6];
	bool p4 = p[3]^p[2]^p[1]^p[7];

	int ret = 1;
	if(p1 && p2 && !p3 && p4) {
		p[1] = p[1] ^ 1;
	} else if(p1 && !p2 && p3 && p4) {
		p[2] = p[2] ^ 1;
	} else if(!p1 && p2 && p3 && p4) {
		p[3] = p[3] ^ 1;
	} else if(p1 && p2 && p3 && !p4) {
		p[0] = p[0] ^ 1;
	} else if(!p1 && !p2 && !p3 && !p4) {
		ret = 0;
	} else if(!p1 && !p2 && !p3 && p4){
		ret = 1;
	} else if(!p1 && !p2 && p3 && !p4){
		ret = 1;
	} else if(!p1 && p2 && !p3 && !p4){
		ret = 1;
	} else if(p1 && !p2 && !p3 && !p4){
		ret = 1;
	} else {
		ret = 2;
	}

	ans = (p[3] << 3) | (p[2] << 2) | (p[1] << 1) | (p[0]);
	return ret;
}



void
block_manager::read_block(uint32_t id, char *buf)
{
	//printf("block_manager::read_block: inum:%u\n", id);
	char codedBuf[BLOCK_SIZE];
	d->read_block(id, codedBuf);

	
	bool flag = 0;
	int bufIndex = 0;
	for(int k = 0; k < RATE; ++k) {
		d->read_block(id * RATE + k, codedBuf);
		int currentIndex = 0;
		
		for(int i = 0; i < BLOCK_SIZE / RATE; ++i) {
			int tAns = 0;
			char t;
			int ret = checkCharCorrectness(codedBuf[currentIndex], t);
			if(ret != 0) {
				flag = true;
			}
			if(ret == 2) {
				printf("!@#!@$!@#!@#!@# hhhhhhhhhhhhhhhhhhhhhhh\n");
				checkCharCorrectness(codedBuf[currentIndex + 1], t);
			} 
			tAns = t << 4;
			ret = checkCharCorrectness(codedBuf[currentIndex + 2], t);
			if(ret != 0) {
				flag = true;
			}
			if(ret == 2) {
				printf("!@#!@$!@#!@#!@# hhhhhhhhhhhhhhhhhhhhhhh\n");
				checkCharCorrectness(codedBuf[currentIndex + 3], t);
			} 
			buf[bufIndex] = tAns | t;
			++bufIndex;
			currentIndex += 4;
		}
	}

	if(flag) {
		write_block(id, buf);
	}
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
	//printf("block_manager::write_block: inum:%u\n", id);
	// printf("--write:inum:%u  content:%s\n", id, buf);
	char codedBuf[BLOCK_SIZE * RATE];
	int currentIndex = 0;
	char mask = (1 << 4) - 1;
	for(int i = 0; i < BLOCK_SIZE; ++i) {
		codedBuf[currentIndex] = constructCodedChar((buf[i] >> 4) & mask);
		codedBuf[currentIndex + 1] = codedBuf[currentIndex];
		codedBuf[currentIndex + 2] = constructCodedChar(buf[i] & mask);
		codedBuf[currentIndex + 3] = codedBuf[currentIndex + 2];
		currentIndex += RATE;
	}

	for(int i = 0; i < RATE; ++i) {
		d->write_block(id * RATE + i, codedBuf + i*BLOCK_SIZE);	
	}
}

// inode layer -----------------------------------------
inode_manager::inode_manager()
{
	bm = new block_manager();
	uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);

	printf("im: init root_dir\n");
	if (root_dir != 1) {
		printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
		exit(0);
	}
}

unsigned int
inode_manager::getTime()
{
// struct timesepc {
//  time_t tv_sec; //whole seconds (valid values are >= 0) 
//  long tv_nsec;  //nanoseconds (valid values are [0, 999999999])  
// }
	struct timespec t;
	if(clock_gettime(CLOCK_REALTIME, &t) == 0)
		return t.tv_sec;
	else 
		return -1;
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
	/* 
	 * your lab1 code goes here.
	 * note: the normal inode block should begin from the 2nd inode block.
	 * the 1st is used for root_dir, see inode_manager::inode_manager().
	
	 * if you get some heap memory, do not forget to free it.
	 */

	struct inode *ino_disk;
	char buf[BLOCK_SIZE];

	// start from 1 root_dir
	int inum = 1;
	while(inum <= INODE_NUM) {
		bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
		ino_disk = (struct inode*)buf + inum%IPB;
		if(ino_disk->type == 0) {
			
			ino_disk->ctime = getTime();
			ino_disk->mtime = ino_disk->ctime;
			ino_disk->atime = ino_disk->ctime;
			ino_disk->type = type;
			bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
			return inum;
		}
		inum++;
	}

	// no more inode available.
	printf("no block for inode to allocate\n");
	return -1;
}

void
inode_manager::free_inode(uint32_t inum)
{
	/* 
	 * your lab1 code goes here.
	 * note: you need to check if the inode is already a freed one;
	 * if not, clear it, and remember to write back to disk.
	 * do not forget to free memory if necessary.
	 */
	printf("inode_manager::free_inode: inum %u\n", inum);
	if (inum < 0 || inum >= bm->sb.ninodes) {
		printf("inode_manager::free_inode: inum out of range:%u\n", inum);
		return ;
	}

	// clear block data
	char buf[BLOCK_SIZE];
	memset(buf, 0, BLOCK_SIZE);
	bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
	// call free block to free inode block, unmark bitmap
	bm->free_block(IBLOCK(inum, bm->sb.nblocks));
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{

	printf("\tim: get_inode %d\n", inum);
	
	if (inum < 0 || inum >= bm->sb.ninodes) {
		printf("\tim: inum out of range\n");
		return NULL;
	}

	struct inode *ino, *ino_disk;
	char buf[BLOCK_SIZE];

	bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
	// printf("%s:%d\n", __FILE__, __LINE__);

	ino_disk = (struct inode*)buf + inum%IPB;
	if (ino_disk->type == 0) {
		printf("\tim: inode not exist\n");
		return NULL;
	}

	ino = (struct inode*)malloc(sizeof(struct inode));
	*ino = *ino_disk;

	return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
	char buf[BLOCK_SIZE];
	struct inode *ino_disk;

	printf("\tim: put_inode %d\n", inum);
	if (ino == NULL)
		return;

	bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
	ino_disk = (struct inode*)buf + inum%IPB;
	*ino_disk = *ino;
	bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))


/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
  /*
   * your lab1 code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_out
   */
  
  // printf("inode_manager::read_file : inum:%u  size:%d\n", inum, *size);
  if(inum < 0 || inum > bm->sb.ninodes || buf_out == NULL || size ==NULL || *size < 0) {
    printf("inode_manager::read_file::parameters error\n");
    return;
  }

  // inode buf, block buf
  char inodeBuf[BLOCK_SIZE], blockBuf[BLOCK_SIZE];
  struct inode *ino_disk;
  
  // get inode
  bm->read_block(IBLOCK(inum, bm->sb.nblocks), inodeBuf);
  ino_disk = (struct inode*)inodeBuf + inum % IPB;
  
  // update access time
  ino_disk->atime = getTime();
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), inodeBuf);

  // get correct size, initialize content buffer
  if(*size == 0) {
    *size = ino_disk->size;
  }
  *size = MIN((uint32_t)*size, ino_disk->size);
  char *content = (char*)malloc(sizeof(char)*(*size));
  // printf("inode_manager::read_file : file-size:%u\n", ino_disk->size);

  // p : current content pointer, 
  int p = 0;

  // get block content according to direct blocks
  for(int i = 0; i < NDIRECT && p < *size; ++i) {
    bm->read_block(ino_disk->blocks[i], blockBuf);
    // printf("inode_manager::read_file: bnum:%u\n", ino_disk->blocks[i]);
    int tSize = MIN(BLOCK_SIZE, *size - p);
    memcpy(content + p, blockBuf, tSize);
    p += tSize;
  }

  // if there is more content in indirect blocks
  if(p < *size) {
    // get the indirect block
    char indirectINode[BLOCK_SIZE];
    bm->read_block(ino_disk->blocks[NDIRECT], indirectINode);
    uint *blocks = (uint*)indirectINode;

    for(uint32_t i = 0; i < NINDIRECT && p < *size; ++i) {
      bm->read_block( blocks[i], blockBuf);
      int tSize = MIN(BLOCK_SIZE, *size - p);
      memcpy(content + p, blockBuf, tSize);
      p += tSize;
    }
  }

  // set return value
  *buf_out = content;
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your lab1 code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode.
   * you should free some blocks if necessary.
   */
  // printf("inode_manager::write_file, inum:%u - size :%d\n", inum, size);
  
  if(inum < 0 || inum > bm->sb.ninodes || buf == NULL || size < 0) {
    printf("inode_manager::write_file:error parameters\n");
    return;
  }
  if((uint32_t)size > MAXFILE * BLOCK_SIZE) {
    printf("inode_manager::write_file:file to write is too big, size:%d\n", size);
    return; 
  }

  char inodeBuf[BLOCK_SIZE], blockBuf[BLOCK_SIZE];
  struct inode *ino_disk;
  
  // get inode
  bm->read_block(IBLOCK(inum, bm->sb.nblocks), inodeBuf);
  ino_disk = (struct inode*)inodeBuf + inum % IPB;
  
  // update modification time();
  ino_disk->atime = getTime();
  ino_disk->mtime = ino_disk->atime;
  ino_disk->ctime = ino_disk->atime;
  std::cout<<"inode_manager::write_file::time:"<<ino_disk->atime<<std::endl;


  int originBlockNum = (ino_disk->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  if(ino_disk->size == 0) {
    originBlockNum = 0;
  }
  // p : current content pointer, 
  int p = 0;
  
  // store content into direct blocks
  int i = 0;
  blockid_t bnum;
  for(; i < NDIRECT && p < size; ++i) {
    int tSize = MIN(BLOCK_SIZE, size - p);
    memcpy(blockBuf, buf + p, tSize);
    if(i < originBlockNum) {
      bnum = ino_disk->blocks[i];
    } else {
      bnum = bm->alloc_block();
      ino_disk->blocks[i] = bnum;  
    }
    // printf("inode_manager::write_file: write block bnum:%d \n", bnum);
    bm->write_block(bnum, blockBuf);
    p = p + tSize;
  }
  // if there is more content to write, store in indirect blocks
  if(p < size) {
    if(i > originBlockNum) {
      ino_disk->blocks[NDIRECT] = bm->alloc_block();
    }
    
    // get the indirect block
    char indirectINode[BLOCK_SIZE];
    bm->read_block(ino_disk->blocks[NDIRECT], indirectINode);
    uint *blocks = (uint*)indirectINode;
    uint32_t index = 0;
    while(p < size) {
      int tSize = MIN(BLOCK_SIZE, size - p);
      memcpy(blockBuf, buf + p, tSize);
      if(i < originBlockNum) {
        bnum = blocks[index];
      } else {
        bnum = bm->alloc_block();
        blocks[index] = bnum;
      }
      bm->write_block(bnum, blockBuf);
      p = p + tSize;
      index++;
      i++;
    }
    bm->write_block(ino_disk->blocks[NDIRECT], indirectINode);
  }

  //free unused block
  int blockCount = i + 1;
  if(size == 0) {
    blockCount = 0;
  }
  if(originBlockNum > blockCount) {
    // free direct blocks
    for(int j = blockCount; j < NDIRECT && j < originBlockNum; ++j) {
      bm->free_block(ino_disk->blocks[j]);
    }
    // free indirect blocks
    if(originBlockNum >= NDIRECT) {
      char indirectINode[BLOCK_SIZE];
      bm->read_block(ino_disk->blocks[NDIRECT], indirectINode);
      uint *blocks = (uint*)indirectINode;
      int j = blockCount >= NDIRECT ? blockCount - NDIRECT + 1 : NDIRECT - NDIRECT;
      while(NDIRECT + j < originBlockNum) {
        bm->free_block(blocks[j]);
        j++;
      }
      if(blockCount < NDIRECT) {
        bm->free_block(ino_disk->blocks[NDIRECT]);
      }
    }
  }

  ino_disk->size = size;
  // bm->write_block(IBLOCK(inum), ino_disk);
  put_inode(inum, ino_disk);
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
	/*
	 * your lab1 code goes here.
	 * note: get the attributes of inode inum.
	 * you can refer to "struct attr" in extent_protocol.h
	 */
	if(inum < 0 || inum > INODE_NUM) {
		printf("inode_manager::getattr:: inum is out of range: %u\n", inum);
		return;
	}

	// get inode, can not use get_inode(), the test program will crash because it may return NULL.
	char buf[BLOCK_SIZE];
	bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
	struct inode* t = (struct inode*)buf + inum%IPB;
	a.type = t->type;
	a.atime = t->atime; 
	a.mtime = t->mtime;
	a.ctime = t->ctime;
	a.size = t->size;
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your lab1 code goes here
   * note: you need to consider about both the data block and inode of the file
   * do not forget to free memory if necessary.
   */
  
  // get inode
  char inodeBuf[BLOCK_SIZE];
  struct inode* ino_disk;
  bm->read_block(IBLOCK(inum, bm->sb.nblocks), inodeBuf);
  ino_disk = (struct inode*)inodeBuf + inum%IPB;
  // printf("inode_manager::remove_file: inum:%u\n", inum);

  int fileSize = ino_disk->size;
  int p = 0;

  // free inode first, for sake of security.
  free_inode(inum);
  
  for(int i = 0; i < NDIRECT && p < fileSize; ++i) {
    // printf("%inode_manager::remove_file: bnum:%d\n", ino_disk->blocks[i]);
    bm->free_block(ino_disk->blocks[i]);
    p += BLOCK_SIZE;
  }

  // there are more data to release
  if( p < fileSize) {
    // get the indirect block
    char indirectINode[BLOCK_SIZE];
    bm->read_block(ino_disk->blocks[NDIRECT], indirectINode);
    // need to free the indirect inode first, for sake of security
    bm->free_block(ino_disk->blocks[NDIRECT]);

    uint* blocks = (uint *)indirectINode;
    int index = 0;
    while(p < fileSize) {
      bm->free_block(blocks[index]);
      index++;
      p += BLOCK_SIZE;
    }
  }
}
