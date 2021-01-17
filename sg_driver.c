
// Include Files
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>

// Project Includes
#include <sg_driver.h>
#include <sg_service.h>
#include <sg_cache.h>

// Defines

//
// Global Data

// Driver file entry

// Global data
int sgDriverInitialized = 0; // The flag indicating the driver initialized
SG_Block_ID sgLocalNodeId;   // The local node identifier
SG_SeqNum sgLocalSeqno;      // The local sequence number

// Driver support functions
int sgInitEndpoint( void ); // Initialize the endpoint
typedef struct node_info {SG_Node_ID n_id; SG_Block_ID b_id; int position; char data_block[SG_BLOCK_SIZE];} node_info;
typedef struct nlist {struct node_info* data; int size;} nlist;
typedef struct file_info {SgFHandle file_id; int position; int size; char file_name[100]; nlist* file_data;} file_info;
typedef struct node {struct file_info* data; struct node* next;} node;
typedef struct list {struct node* head;} list;
list files_list = {NULL};
SgFHandle unique_id_value = 1;

typedef struct rnode {SG_Node_ID n_id; SG_SeqNum nseq_num; SG_SeqNum bseq_num;} rnode;
typedef struct table {rnode*nde_list; int size;} table;
table node_table = {NULL,0};

//
// Functions
//looks for file info and then inserts it
void insertFileInfo(const char*file_nm)
{
    struct file_info*temp_info = calloc(1, sizeof(file_info));
    temp_info->file_id = unique_id_value;
    strcpy(temp_info->file_name, file_nm);
    temp_info->file_data = calloc(1, sizeof(nlist));
    unique_id_value++;
    if (files_list.head == NULL)
    {
        files_list.head = calloc(1, sizeof(struct node));
        (files_list.head)->data = temp_info;
        (files_list.head)->next = NULL;
        return;
    }
    struct node*temp_node = calloc(1, sizeof(struct node));
    temp_node->data = temp_info;
    temp_node->next = files_list.head;
    files_list.head = temp_node;
}
//function to get the node
node* get_node(SgFHandle file_handle){
    node*temp = files_list.head;
    while (temp != NULL)
    {
        if ((temp->data->file_id) == file_handle)
        {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}
//looks for the file info and then removes it 
int removeFileInfo(node*file_node){
    node*temp = files_list.head, *rm_node;
    if (temp == NULL || get_node(file_node->data->file_id) == NULL)
    {
        return -1;
    }
    if (temp->next == NULL)
    {
        strcpy(temp->data->file_name, "");
        free(temp->data->file_data->data);
        free(temp->data->file_data);
        free(temp->data);
        temp->data = NULL;
        free(temp);
        temp = NULL;
        files_list.head = NULL;
        return 0;
    }
    while (temp->next != NULL)
    {
        if ((temp->next->data->file_id) == (file_node->data->file_id))
        {
            rm_node = temp->next;
            temp->next = temp->next->next;
            strcpy(rm_node->data->file_name,"");
            free(rm_node->data->file_data->data);
            free(rm_node->data->file_data);
            free(rm_node->data);
            rm_node->data = NULL;
            free(rm_node);
            temp = NULL;
            return 0;
        }
        temp = temp->next;
    }
    return -1;
}
void free_files_list(){
    node*temp;
    while(files_list.head != NULL)
    {
        temp = files_list.head;
        files_list.head = (files_list.head)->next;
        strcpy(temp->data->file_name, "");
        free(temp->data->file_data->data);
        free(temp->data->file_data);
        free(temp->data);
        temp->data = NULL;
        free(temp);
        temp = NULL;
    }
}
//
// File system interface implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

SgFHandle sgopen(const char *path) {

    // First check to see if we have been initialized
    if (!sgDriverInitialized) {

        // Call the endpoint initialization 
        if ( sgInitEndpoint() ) {
            logMessage( LOG_ERROR_LEVEL, "sgopen: Scatter/Gather endpoint initialization failed." );
            return( -1 );
        }

        // Set to initialized
        sgDriverInitialized = 1;
    }

// FILL IN THE REST OF THE CODE

    // Return the file handle
    insertFileInfo(path);
    logMessage(SGDriverLevel,"sgopen: opened file [%s] with file handle [%d]", files_list.head->data->file_name, files_list.head->data->file_id);
    return((files_list.head)->data->file_id);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgread
// Description  : Read data from the file
//
// Inputs       : fh - file handle for the file to read from
//                buf - place to put the data
//                len - the length of the read
// Outputs      : number of bytes read, -1 if failure

int sgread(SgFHandle fh, char *buf, size_t len) {
    node*temp_node = get_node(fh);
    //if file handle bad or file not open return -1
    if(temp_node == NULL || temp_node->data == NULL)
    {
        return -1;
    }
    if(temp_node->data->position >= temp_node->data->size)
    {
        return -1;
    }
    int length = temp_node->data->file_data->size,obt_position;
    SG_Node_ID obt_node_id;
    SG_Block_ID obt_block_id;
    char*obt_block = NULL;
    rnode*temp_rnode = NULL;
    for (int j = 0; j < length; j++)
    {
        if(temp_node->data->file_data->data[j].position == temp_node->data->position || temp_node->data->file_data->data[j].position==(temp_node->data->position-(temp_node->data->position%SG_BLOCK_SIZE)))
        {
            obt_node_id = temp_node->data->file_data->data[j].n_id;
            obt_block_id = temp_node->data->file_data->data[j].b_id;
            obt_block = temp_node->data->file_data->data[j].data_block;
            obt_position=temp_node->data->position;
            for(int j = 0; j < node_table.size; j++)
            {
                if(obt_node_id == node_table.nde_list[j].n_id)
                {
                    temp_rnode = &node_table.nde_list[j];
                }
            }
            break;
        }
    }
    memmove(buf, obt_block + obt_position%SG_BLOCK_SIZE, len*sizeof(char));
    //checks for number of bytes, if not enough bytes left return number of bytes read
    //if file handle bad or file not open then return -1
    char packet[SG_DATA_PACKET_SIZE], recvPacket[SG_DATA_PACKET_SIZE];
    size_t pktlen, rpktlen = SG_DATA_PACKET_SIZE;
    char*block = getSGDataBlock(obt_node_id,obt_block_id);
    size_t t_rpktlen=rpktlen;
    SG_Node_ID t_loc, t_rem;
    SG_Block_ID t_blkid;
    SG_SeqNum t_sloc, t_srem;
    SG_System_OP t_op;
    if(block == NULL)
    {
        if(serialize_sg_packet( sgLocalNodeId, obt_node_id,obt_block_id, SG_OBTAIN_BLOCK, sgLocalSeqno++, temp_rnode->bseq_num, buf, packet, &pktlen)!= SG_PACKT_OK)
        {
            return -1;
        }
        if(sgServicePost(packet, &pktlen, recvPacket, &rpktlen))
        {
            return -1;
        }
        logMessage(SGDriverLevel, "sgDriverObtainBlock: Obtained block [%ul] from node [%ul]", *(unsigned int*)(recvPacket + 20), *(unsigned int*)(recvPacket + 12));
        if(deserialize_sg_packet(&t_loc, &t_rem, &t_blkid, &t_op, &t_sloc, &t_srem, NULL, recvPacket, t_rpktlen) != SG_PACKT_OK)
        {
            return -1;
        }
        temp_rnode->bseq_num=t_srem + 1;
    }
    else
    {
        *(unsigned long int*)(recvPacket + 12) = obt_node_id;
        *(unsigned long int*)(recvPacket + 20) = obt_block_id;
        memmove((char*)(recvPacket + 37),obt_block,SG_BLOCK_SIZE);
        logMessage(SGDriverLevel,"sgDriverObtainBlock: Used cached block [%lu], node [%lu] in cache.", obt_node_id, obt_block_id);
    }
    logMessage(SGDriverLevel, "Read %s (%u bytes at offset %u)", temp_node->data->file_name, len, temp_node->data->position);
    temp_node->data->position += len;

    // Return the bytes processed
    return( len );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgwrite
// Description  : write data to the file
//
// Inputs       : fh - file handle for the file to write to
//                buf - pointer to data to write
//                len - the length of the write
// Outputs      : number of bytes written if successful test, -1 if failure

int sgwrite(SgFHandle fh, char *buf, size_t len) {
    node*temp_node = get_node(fh);
    //write count bytes into the file referenced
    //If the write goes beyond the end of the file the size is increased   
    if(temp_node == NULL || temp_node->data == NULL)
    {
        return -1;
    }
    int length = temp_node->data->file_data->size, is_found = 0,obt_position;
    SG_Node_ID obt_node_id = 0;
    SG_Block_ID obt_block_id;
    rnode*temp_rnode = NULL;
    char*obt_block = NULL;
    for(int j = 0;j < length; j++)
    {
        if(temp_node->data->file_data->data[j].position == temp_node->data->position || temp_node->data->file_data->data[j].position==(temp_node->data->position-(temp_node->data->position%SG_BLOCK_SIZE)))
        {
            obt_node_id = temp_node->data->file_data->data[j].n_id;
            obt_block_id = temp_node->data->file_data->data[j].b_id;
            obt_block = temp_node->data->file_data->data[j].data_block;
            obt_position=temp_node->data->position;
            is_found = 1;
            for(int j = 0; j < node_table.size; j++)
            {
                if(obt_node_id == node_table.nde_list[j].n_id)
                {
                    temp_rnode = &node_table.nde_list[j];
                }
            }
            break;
        }
    }
    //fails and return -1 if the file handle is bad or the file was not previously open
    if(is_found)
    {
        char packet[SG_DATA_PACKET_SIZE], recvPacket[SG_DATA_PACKET_SIZE];
        size_t pktlen, rpktlen = SG_DATA_PACKET_SIZE;
        char*block = getSGDataBlock(obt_node_id, obt_block_id);
        
        size_t t_rpktlen = rpktlen;
        SG_Node_ID t_loc, t_rem;
        SG_Block_ID t_blkid;
        SG_SeqNum t_sloc, t_srem;
        SG_System_OP t_op;
        if(block == NULL)
        {
            if(serialize_sg_packet(sgLocalNodeId, obt_node_id,obt_block_id, SG_OBTAIN_BLOCK, sgLocalSeqno++, temp_rnode->bseq_num, buf, packet, &pktlen) != SG_PACKT_OK)
            {
                return -1;
            }
            if(sgServicePost(packet, &pktlen, recvPacket, &rpktlen))
            {
                return -1;
            }
            if(deserialize_sg_packet(&t_loc, &t_rem, &t_blkid, &t_op, &t_sloc, &t_srem, NULL, recvPacket, t_rpktlen) != SG_PACKT_OK)
            {
                return -1;
            }
            temp_rnode->bseq_num = t_srem+1;
            putSGDataBlock(obt_node_id,obt_node_id, obt_block);
            logMessage(SGDriverLevel,"sgDriverObtainBlock: Obtained block [%ul] from node [%ul]", t_blkid, t_rem);
        }
        else
        {
            *(unsigned long int*)(recvPacket + 12) = obt_node_id;
            *(unsigned long int*)(recvPacket + 20) = obt_block_id;
            memmove((char*)(recvPacket + 37), obt_block, SG_BLOCK_SIZE);
            logMessage(SGDriverLevel,"sgDriverObtainBlock: Used cached block [%lu], node [%lu] in cache.",  obt_block_id, obt_node_id);
            logMessage(SGDriverLevel, "sgDriverUpdateBlock: Updated block [%lu], node [%lu] in cache.",  obt_block_id, obt_node_id);
        }
        if(serialize_sg_packet(sgLocalNodeId, *(unsigned long int*)(recvPacket + 12), *(unsigned long int*)(recvPacket + 20), SG_UPDATE_BLOCK, sgLocalSeqno++, temp_rnode->bseq_num, (char*)(recvPacket + 37), packet, &pktlen) != SG_PACKT_OK)
        {
            return -1;
        }
        if(sgServicePost(packet, &pktlen, recvPacket, &rpktlen))
        {
            return -1;
        }
        if(deserialize_sg_packet(&t_loc, &t_rem, &t_blkid, &t_op, &t_sloc, &t_srem, NULL, recvPacket, t_rpktlen) != SG_PACKT_OK)
        {
            return -1;
        }
        temp_rnode->bseq_num = t_srem+1;
        logMessage(SGDriverLevel, "sgDriverUpdateBlock: Updated block [%ul] on node [%ul]", t_blkid, t_rem);
        memmove(obt_block+obt_position%SG_BLOCK_SIZE, buf, len*sizeof(char));
        logMessage(SGDriverLevel, "Wrote %s (fh=%u, %u bytes at offset %u)", temp_node->data->file_name, temp_node->data->file_id, len, obt_position);
        temp_node->data->position += len;
    }
    else
    {
        char packet[SG_DATA_PACKET_SIZE], recvPacket[SG_DATA_PACKET_SIZE];
        size_t pktlen, rpktlen=SG_BASE_PACKET_SIZE;
        if(serialize_sg_packet( sgLocalNodeId, SG_NODE_UNKNOWN,SG_BLOCK_UNKNOWN, SG_CREATE_BLOCK, sgLocalSeqno++, SG_SEQNO_UNKNOWN, buf, packet, &pktlen)!=SG_PACKT_OK)
        {
            return -1;
        }
        if(sgServicePost(packet, &pktlen, recvPacket, &rpktlen))
        {
            return -1;
        }
        size_t t_rpktlen=rpktlen;
        SG_Node_ID t_loc, t_rem;
        SG_Block_ID t_blkid;
        SG_SeqNum t_sloc, t_srem;
        SG_System_OP t_op;
        if(deserialize_sg_packet(&t_loc, &t_rem, &t_blkid, &t_op, &t_sloc, &t_srem, NULL, recvPacket, t_rpktlen) != SG_PACKT_OK)
        {
            return -1;
        }
        int node_found = 0;
        for(int j = 0; j < node_table.size; j++)
        {
            if(t_rem == node_table.nde_list[j].n_id)
            {
                logMessage(SGDriverLevel,"Creating block on known node [%lu], in sequence [%u]",t_rem%10000,t_srem);
                node_table.nde_list[j].bseq_num = t_srem+1;
                node_found = 1;
            }
        }
        if(!node_found)
        {
            logMessage( LOG_ERROR_LEVEL, "sgAddNodeInfo: unable to find node in node table [%lu]",t_rem );
            logMessage(SGDriverLevel,"Added node (...%lu, seq=%u) to driver node table",t_rem%10000,t_sloc);
            logMessage(SGDriverLevel,"Creating block on unknown node [%lu], in sequence [%u]",t_rem%10000,t_srem);
            node_table.nde_list=realloc(node_table.nde_list,(node_table.size+1)*sizeof(rnode));
            node_table.nde_list[node_table.size].n_id=t_rem;
            node_table.nde_list[node_table.size].nseq_num = t_sloc;
            node_table.nde_list[node_table.size].bseq_num = t_srem+1;
            node_table.size++;
        }
        
        nlist*node_list = temp_node->data->file_data;
        node_list->data = realloc(node_list->data, (node_list->size + 1)*sizeof(struct node_info));
        node_list->data[node_list->size].n_id = t_rem;
        node_list->data[node_list->size].b_id = t_blkid;
        node_list->data[node_list->size].position = temp_node->data->position;
        obt_position=temp_node->data->position;
        obt_block=node_list->data[node_list->size].data_block;
        memmove(obt_block,buf,len*sizeof(char));
        char*block = getSGDataBlock(node_list->data[node_list->size].n_id,node_list->data[node_list->size].b_id);
        if(block == NULL)
        {
            putSGDataBlock(node_list->data[node_list->size].n_id,node_list->data[node_list->size].b_id, obt_block );
        }
        logMessage(SGDriverLevel,"sgDriverCreateBlock: Created block [%ul] on node [%ul]", t_blkid, t_rem);
        if(block == NULL)
        {
            logMessage(SGDriverLevel, "Inserted block [%lu], node [%lu] int cache.", t_blkid, t_rem);
        }
        logMessage(SGDriverLevel, "Wrote %s (fh=%u, %u bytes at offset %u)", temp_node->data->file_name, temp_node->data->file_id, len, obt_position);
        node_list->size++;
        temp_node->data->size += SG_BLOCK_SIZE;
        temp_node->data->position += len;
    }
    // Log the write, return bytes written
    return( len );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgseek
// Description  : Seek to a specific place in the file
//
// Inputs       : fh - the file handle of the file to seek in
//                off - offset within the file to seek to
// Outputs      : new position if successful, -1 if failure

int sgseek(SgFHandle fh, size_t off) {
    node*temp_node = get_node(fh);
    //check if beyond end of the file and return -1
    if(temp_node == NULL || off>(temp_node->data->size))
    {
        return -1;
    }
    logMessage(SGDriverLevel, "sgseek: %s (seeked to offset %ld)",temp_node->data->file_name,off);
    temp_node->data->position = off;

    // Return new position
    return( off );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgclose
// Description  : Close the file
//
// Inputs       : fh - the file handle of the file to close
// Outputs      : 0 if successful test, -1 if failure

int sgclose(SgFHandle fh) {
    node*temp_node = get_node(fh);
    //closes the file accessed then checks if handle is bad or if file not previously open
    if(temp_node == NULL || temp_node->data == NULL)
    {
        return -1;
    }
    removeFileInfo(temp_node);
    temp_node = NULL;

    // Return successfully
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgshutdown
// Description  : Shut down the filesystem
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int sgshutdown(void) {
    //shuts down the system and closes all of the files
    logMessage(LOG_INFO_LEVEL,"Stopping local endpoint ...");
    char packet[SG_BASE_PACKET_SIZE], recvPacket[SG_BASE_PACKET_SIZE];
    size_t pktlen, rpktlen = SG_BASE_PACKET_SIZE;
    if(serialize_sg_packet(sgLocalNodeId, SG_NODE_UNKNOWN,SG_BLOCK_UNKNOWN, SG_STOP_ENDPOINT, sgLocalSeqno++, SG_SEQNO_UNKNOWN, NULL, packet, &pktlen) != SG_PACKT_OK)
    {
        return -1;
    }
    if(sgServicePost(packet, &pktlen, recvPacket, &rpktlen))
    {
        return -1;
    }
    free(node_table.nde_list);
    node_table.nde_list = NULL;
    node_table.size = 0;
    logMessage(LOG_INFO_LEVEL, "Stopped local node (local node ID %lu)", sgLocalNodeId);
    free_files_list();
    closeSGCache();
    // Log, return successfully
    logMessage( LOG_INFO_LEVEL, "Shut down Scatter/Gather driver." );
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : serialize_sg_packet
// Description  : Serialize a ScatterGather packet (create packet)
//
// Inputs       : loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
//                data - the data block (of size SG_BLOCK_SIZE) or NULL
//                packet - the buffer to place the data
//                plen - the packet length (int bytes)
// Outputs      : 0 if successfully created, -1 if failure

SG_Packet_Status serialize_sg_packet(SG_Node_ID loc, SG_Node_ID rem, SG_Block_ID blk,
                                     SG_System_OP op, SG_SeqNum sseq, SG_SeqNum rseq, char *data,
                                     char *packet, size_t *plen) {

// REPLACE THIS CODE WITH YOU ASSIGNMENT #2
    //check if the inputs are valid or not
            char data_is_present='y';
            if(data==NULL)
                data_is_present='n';
            logMessage(SGDriverLevel, "Serialization loc=[..%lu], rem=[..%lu], op=[%d], sseq=[%d], rseq=[%d], data=[%c]", loc%10000,rem%10000,op,sseq,rseq,data_is_present);
            if (loc == 0)
            {
                return SG_PACKT_LOCID_BAD;
            }
            if (rem == 0)
            {
                return SG_PACKT_REMID_BAD;
            }
            if (blk == 0)
            {
                return SG_PACKT_BLKID_BAD;
            }
            if (op < SG_INIT_ENDPOINT || op > SG_MAXVAL_OP)
            {
                return SG_PACKT_OPERN_BAD;
            }
            if (sseq == 0)
            {
                return SG_PACKT_SNDSQ_BAD;
            }
            if (rseq == 0)
            {
                return SG_PACKT_RCVSQ_BAD;
            }
            //create the packet data from the elements that are passed in
            *(unsigned int*)(packet) = SG_MAGIC_VALUE;
            *((long unsigned int*)(packet + 4)) = loc;
            *((long unsigned int*)(packet + 12)) = rem;
            *((long unsigned int*)(packet + 20)) = blk;
            *((unsigned int*)(packet + 28)) = op;
            *((short unsigned int*)(packet + 32)) = sseq;
            *((short unsigned int*)(packet + 34)) = rseq;

            //check if there is any data
            //if there is data, set length to be equal to the data packet size
            //if there is no data, set length of packet to be the base packet size
            if (data != NULL)
            {
                packet[36] = 1;
                for (int i = 0; i < SG_BLOCK_SIZE; i++)
                {
                    packet[37 + i] = data[i];
                }
                *(unsigned int*)(packet + 37 + SG_BLOCK_SIZE) = SG_MAGIC_VALUE;
                *plen = SG_DATA_PACKET_SIZE;
            }
            else
            {
                packet[36] = 0;
                *(unsigned int*)(packet + 37) = SG_MAGIC_VALUE;
                *plen = SG_BASE_PACKET_SIZE;
            }

    // Return the system function return value
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : deserialize_sg_packet
// Description  : De-serialize a ScatterGather packet (unpack packet)
//
// Inputs       : loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
//                data - the data block (of size SG_BLOCK_SIZE) or NULL
//                packet - the buffer to place the data
//                plen - the packet length (int bytes)
// Outputs      : 0 if successfully created, -1 if failure

SG_Packet_Status deserialize_sg_packet(SG_Node_ID *loc, SG_Node_ID *rem, SG_Block_ID *blk,
                                       SG_System_OP *op, SG_SeqNum *sseq, SG_SeqNum *rseq, char *data,
                                       char *packet, size_t plen) {

// REPLACE THIS CODE WITH YOU ASSIGNMENT #2
    //create the packet data from the elemets passed in
            unsigned int tempOP = *((unsigned int*)(packet + 28));
            long unsigned int tempLOC =*((long unsigned int*)(packet + 4));
            long unsigned int tempREM = *((long unsigned int*)(packet + 12));
            long unsigned int tempBLK = *((long unsigned int*)(packet + 20));
            short unsigned int tempSSEQ = *((short unsigned int*)(packet + 32));
            short unsigned int tempRSEQ = *((short unsigned int*)(packet + 34));
            char tempDataIndicator = packet[36], *tempData = (packet + 37);
            //check if the inputs are valid
            if (tempLOC == 0)
            {
                return SG_PACKT_LOCID_BAD;
            }
            if (tempREM == 0)
            {
                return SG_PACKT_REMID_BAD;
            }
            if (tempBLK == 0)
            {
                return SG_PACKT_BLKID_BAD;
            }
            if (tempOP < SG_INIT_ENDPOINT || tempOP > SG_MAXVAL_OP)
            {
                return SG_PACKT_OPERN_BAD;
            }
            if (tempSSEQ == 0)
            {
                return SG_PACKT_SNDSQ_BAD;
            }
            if (tempRSEQ == 0)
            {
                return SG_PACKT_RCVSQ_BAD;
            }
            *loc = tempLOC;
            *rem = tempREM;
            *blk = tempBLK;
            *op = tempOP;
            *sseq = tempSSEQ;
            *rseq = tempRSEQ;

            //if the data indicator is 1 then create the packet data
            if (tempDataIndicator == 1 && data!=NULL)
            {
                if(data==NULL)
                {printf("\n\nData is Null\n\n");}
                for (int i = 0; i< SG_BLOCK_SIZE; i++)
                {
                    data[i] = tempData[i];
                }
            }
            char data_is_present = 'y';
            if(data == NULL)
                data_is_present = 'n';
            logMessage(SGDriverLevel, "Deserialization loc=[..%lu], rem=[..%lu], op=[%d], sseq=[..%d], rseq=[..%d], data=[%c]", *loc%10000,*rem%10000,*op,*sseq%100000,*rseq%100000,data_is_present);

    // Return the system function return value
    return( 0 );
}

// Driver support functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgInitEndpoint
// Description  : Initialize the endpoint
//
// Inputs       : none
// Outputs      : 0 if successfull, -1 if failure

int sgInitEndpoint( void ) {

    // Local variables
    char initPacket[SG_BASE_PACKET_SIZE], recvPacket[SG_BASE_PACKET_SIZE];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    // Local and do some initial setup
    logMessage( LOG_INFO_LEVEL, "Initializing local endpoint ..." );
    sgLocalSeqno = SG_INITIAL_SEQNO;

    // Setup the packet
    pktlen = SG_BASE_PACKET_SIZE;
    if ( (ret = serialize_sg_packet( SG_NODE_UNKNOWN, // Local ID
                                    SG_NODE_UNKNOWN,   // Remote ID
                                    SG_BLOCK_UNKNOWN,  // Block ID
                                    SG_INIT_ENDPOINT,  // Operation
                                    sgLocalSeqno++,    // Sender sequence number
                                    SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                    NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
        return( -1 );
    }

    // Send the packet
    rpktlen = SG_BASE_PACKET_SIZE;
    if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
        return( -1 );
    }

    // Unpack the recieived data
    if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                    &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
        return( -1 );
    }

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: bad local ID returned [%ul]", loc );
        return( -1 );
    }
    // Set the local node ID, log and return successfully
    sgLocalNodeId = loc;
    logMessage( LOG_INFO_LEVEL, "Completed initialization of node (local node ID %lu", sgLocalNodeId );
    initSGCache( SG_MAX_CACHE_ELEMENTS );
    return( 0 );
} 
