////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_driver.c
//  Description    : This file contains the driver code to be developed by
//                   the students of the 311 class.  See assignment details
//                   for additional information.
//
//   Author        : Abhinav
//   Last Modified : 12/11/20
//

// Include Files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>

// Project Includes
#include <sg_cache.h>

// Defines
int elapsedTime = 0;
float numHits = 0;
float totalQueries = 0;
int OID_assigner = 1000;
struct cache sg_cache = {NULL, 0, 0};

// Functional Prototypes

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : initSGCache
// Description  : Initialize the cache of block elements
//
// Inputs       : maxElements - maximum number of elements allowed
// Outputs      : 0 if successful, -1 if failure

int initSGCache( uint16_t maxElements ) {
	if(maxElements <= 0)
	{
		return -1;
	}
	if(maxElements>0 && sg_cache.head == NULL)
	{
		sg_cache.head = calloc(1, sizeof(struct c_node));
		sg_cache.head->info = NULL;
		sg_cache.size++;
	}
	struct c_node*tempNode = sg_cache.head;
	for(int i = 1; i < maxElements; i++)
	{
		tempNode->next = calloc(1, sizeof(struct c_node));
		tempNode->next->info = NULL;
		tempNode = tempNode->next;
		sg_cache.size++;
	}
    logMessage(LOG_INFO_LEVEL, "init_cmpsc311_cache: initialization complete [%d/%d]", sg_cache.size, sg_cache.size*SG_BLOCK_SIZE);
    logMessage(LOG_INFO_LEVEL, "Cache state [%d items, %d bytes used]", sg_cache.noOfItems, sg_cache.noOfItems*SG_BLOCK_SIZE);
    // Return successfully
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : closeSGCache
// Description  : Close the cache of block elements, clean up remaining data
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int closeSGCache( void ) {
	c_node*tempNode;
	int temp_hit = numHits;
	int temp_queries = totalQueries;
    logMessage(SGDriverLevel,"Closing cache: %d queries, %d hits (%f%% hit rate).", temp_queries, temp_hit, numHits*100/(totalQueries?totalQueries:1));
    logMessage(LOG_INFO_LEVEL, "Closed cmpsc311 cache, deleting %d items", sg_cache.noOfItems );
	while(sg_cache.head)
	{
		tempNode = sg_cache.head;
		sg_cache.head = sg_cache.head->next;
		free(tempNode->info);
		tempNode->info = NULL;
		free(tempNode);
		tempNode = NULL;
	}
    // Return successfully
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : getSGDataBlock
// Description  : Get the data block from the block cache
//
// Inputs       : nde - node ID to find
//                blk - block ID to find
// Outputs      : pointer to block or NULL if not found

char * getSGDataBlock( SG_Node_ID nde, SG_Block_ID blk ) {
	struct c_node*tempNode = sg_cache.head;
	while(tempNode->info)
	{
		if(tempNode->info->n_id == nde && tempNode->info->b_id == blk)
		{
			// Block found
			numHits++;
			tempNode->info->time_stamp = elapsedTime;
			tempNode->info->accessed = 1;
    		logMessage(LOG_INFO_LEVEL, "Getting found cache item OID %d, length %d", tempNode->info->OID,sizeof(tempNode->info->data_block));
			totalQueries++;
			return tempNode->info->data_block;
		}
		tempNode = tempNode->next;
	}
	logMessage(LOG_INFO_LEVEL, "Getting cache item OID %d (not found!)", OID_assigner);
	totalQueries++;
    // Block not found
    // Return successfully
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : putSGDataBlock
// Description  : Get the data block from the block cache
//
// Inputs       : nde - node ID to find
//                blk - block ID to find
//                block - block to insert into cache
// Outputs      : 0 if successful, -1 if failure

int putSGDataBlock( SG_Node_ID nde, SG_Block_ID blk, char *block ) {
	struct c_node*tempNode = sg_cache.head;
	if(sg_cache.noOfItems < sg_cache.size)
	{
		struct c_node*insNode = calloc(1, sizeof(struct c_node));
		insNode->info = calloc(1, sizeof(struct cache_line));
		insNode->info->OID = OID_assigner;
		insNode->info->n_id = nde;
		insNode->info->b_id = blk;
		memmove(insNode->info->data_block, block, SG_BLOCK_SIZE*sizeof(char));
		insNode->info->time_stamp = elapsedTime;
		insNode->info->accessed = 0;
		elapsedTime++;
		insNode->next = sg_cache.head;
		sg_cache.head = insNode;
		sg_cache.noOfItems++;
    	logMessage(LOG_INFO_LEVEL, "Cache state [%d items, %d bytes used]", sg_cache.noOfItems, sg_cache.noOfItems*SG_BLOCK_SIZE);
		logMessage(LOG_INFO_LEVEL, "Added cache item OID %d, length %d", insNode->info->OID, sizeof(insNode->info->data_block));
	}
	else
	{
		tempNode = sg_cache.head;
		struct c_node*less_time_node = sg_cache.head;
		while(tempNode->info)
		{
			if(tempNode->info->time_stamp < less_time_node->info->time_stamp && tempNode->info->accessed == 0)
			{
				less_time_node = tempNode;
			}
			tempNode = tempNode->next;
		}
		logMessage(LOG_INFO_LEVEL, "Ejecting cache item OID %d, length %d", less_time_node->info->OID, sizeof(less_time_node->info->data_block));
    	logMessage(LOG_INFO_LEVEL, "Cache state [%d items, %d bytes used]", sg_cache.noOfItems - 1, (sg_cache.noOfItems - 1)*SG_BLOCK_SIZE );
		less_time_node->info->n_id=nde;
		less_time_node->info->b_id=blk;
		memmove(less_time_node->info->data_block, block,SG_BLOCK_SIZE*sizeof(char));
		less_time_node->info->time_stamp = elapsedTime;
	    logMessage(LOG_INFO_LEVEL, "Cache state [%d items, %d bytes used]", sg_cache.noOfItems, sg_cache.noOfItems*SG_BLOCK_SIZE );
		logMessage(LOG_INFO_LEVEL, "Added cache item OID %d, length %d", less_time_node->info->OID, sizeof(less_time_node->info->data_block));
		elapsedTime++;
	}
	OID_assigner++;
    // Return successfully
    return( 0 );
}

