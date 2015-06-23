/*********************************************************************
*
* Filename      :   ts_psc.h
* Description   :   fundamental operation of psi protocol of DVB
* edited by     :   Jensen Zhen(JensenZhen@zhaoxin.com)
*
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ts_psi.h>
#include <ts_list.h>
#include <print_debug.h>


/*  
 *  Function    : Init the global pat_program_list and pmt_stream list
 */
void init_ts_psi_list(void)
{
    INIT_LIST_HEAD(&__ts_pat_program_list.list);
    INIT_LIST_HEAD(&__ts_pmt_stream_list.list);
}


/*  
 *  Function    : Parse the header of ts packet header.
 */
int parse_ts_packet_header(TS_PACKET_HEADER *packet_head, unsigned char *buffer)
{
    packet_head->sync_byte                      =   buffer[0];
    packet_head->transport_error_indicator      =   buffer[1] >> 7;
    packet_head->payload_unit_start_indicator   =   buffer[1] >> 6 & 0x01;
    packet_head->transport_priority             =   buffer[1] >> 5 & 0x01;
    packet_head->pid                            =   ((buffer[1] & 0x1F) << 8) | buffer[2];
    packet_head->transport_scrambling_control   =   buffer[3] >> 6;
    packet_head->adaptation_field_control       =   (buffer[3] >> 4) & 0x03;

    packet_head->continuity_counter             =   buffer[3] & 0x0f;
    
    return 0;
}

int show_ts_packet_header(TS_PACKET_HEADER *packet_head)
{
    uprintf("transport_packet()\n");
    uprintf("sync_byte                      :  0x%X\n", packet_head->sync_byte);
    uprintf("transport_error_indicator      :  0x%X\n", packet_head->transport_error_indicator);
    uprintf("payload_unit_start_indicator   :  0x%X\n", packet_head->payload_unit_start_indicator);
    uprintf("transport_priority             :  0x%X\n", packet_head->transport_priority);
    uprintf("pid                            :  0x%X\n", packet_head->pid);
    uprintf("transport_scrambling_control   :  0x%X\n", packet_head->transport_scrambling_control);
    uprintf("adaptation_field_control       :  0x%X\n", packet_head->adaptation_field_control);
    uprintf("continuity_counter             :  0x%X\n", packet_head->continuity_counter);
    
    return 0;
}


/*  
 *  Funciton : locate the first real packet data offset. 
 *             
 *  psiFlag  : to distinguish the PSI/SI packet and PES packet.
 *             0 PSI/SI packet.     not 0 : PES packet.
 *  
 *  pes_start : meaning the packet_head->payload_unit_start_indicator.
 *              0  : not pes start.
 *              1  : pes start.
 */
unsigned int locate_offset(TS_PACKET_HEADER *packet_head, unsigned char *buffer,
        unsigned int psiFlag, unsigned int * pes_start)
{
    unsigned int offset_length = 0;
    *pes_start = 0;

    if (0 == psiFlag)
    {
        //Have data

        if ((0x03 == packet_head->adaptation_field_control) || (0x01 == packet_head->adaptation_field_control))
        {
            if( 1 == packet_head->payload_unit_start_indicator )
            {
                if (0x03 == packet_head->adaptation_field_control)
                {
                    //1 + buffer[4] :   point + point_field_length
                    //buffer[4] + 5 : the index of adaptation_field_length;
                    offset_length = TS_BASE_PACKET_HEADER_SIZE + (1 + buffer[4]) + (1 + buffer[buffer[4] + 5]);
                }
                else
                {
                    offset_length = TS_BASE_PACKET_HEADER_SIZE + (1 + buffer[4]);
                }
            }
            else
            {
                if (0x03 == packet_head->adaptation_field_control)//1+buffer[4] : adaptation_field + adaptation_field_length
                {
                    offset_length = TS_BASE_PACKET_HEADER_SIZE + (1 + buffer[4]);
                }
                else
                {
                    offset_length = TS_BASE_PACKET_HEADER_SIZE; 
                }
            }
    
        }
    }
    else //this is pes packet
    {
        //Have data

        if ((0x03 == packet_head->adaptation_field_control) || (0x01 == packet_head->adaptation_field_control))
        {
            if( 1 == packet_head->payload_unit_start_indicator )
            {
//                uprintf("This is a PES packet start.\n");
                *pes_start = 1;
            }

            if (0x03 == packet_head->adaptation_field_control)//1+buffer[4] : adaptation_field + adaptation_field_length
            {
                offset_length = TS_BASE_PACKET_HEADER_SIZE + (1 + buffer[4]);
            }
            else
            {
                offset_length = TS_BASE_PACKET_HEADER_SIZE; 
            }
        } 
    
    }
    //uprintf("the value of offset_length is  %d\n",offset_length);

    return offset_length;
}








/*  
 *  Function    : Parse the pat table and add the pat_program info into
 *                global list------ts_pat_program_list
 */
int parse_pat_table(unsigned char * pBuffer,TS_PAT_TABLE * psiPAT)
{

    int pat_len = 0;
    unsigned short program_num;
    int ret=0, n = 0;
    P_TS_PAT_Program tmp = NULL;
    unsigned int pes_start;
    
    TS_PACKET_HEADER tsPacketHeader;
    P_TS_PACKET_HEADER ptsPacketHeader = &tsPacketHeader;

    parse_ts_packet_header(ptsPacketHeader, pBuffer);

    unsigned int offset = locate_offset(ptsPacketHeader, pBuffer, 0, &pes_start);
    unsigned char * buffer = pBuffer + offset;


    psiPAT->table_id                    = buffer[0];  
    psiPAT->section_syntax_indicator    = buffer[1] >> 7;  
    psiPAT->zero                        = buffer[1] >> 6 & 0x1;  
    psiPAT->reserved_1                  = buffer[1] >> 4 & 0x3;  
    psiPAT->section_length              = (buffer[1] & 0x0F) << 8 | buffer[2];   
    psiPAT->transport_stream_id         = buffer[3] << 8 | buffer[4];
    psiPAT->reserved_2                  = buffer[5] >> 6;  
    psiPAT->version_number              = buffer[5] >> 1 &  0x1F;  
    psiPAT->current_next_indicator      = (buffer[5] << 7) >> 7;  
    psiPAT->section_number              = buffer[6];  
    psiPAT->last_section_number         = buffer[7];   
  
    // the 3 meaning that the section_length offset of buffer head is 3 byte.
    // and the len length is the total length of buffer. 
    pat_len = 3 + psiPAT->section_length;  
    psiPAT->CRC_32                      = (buffer[pat_len-4] & 0x000000FF) << 24 
                                        | (buffer[pat_len-3] & 0x000000FF) << 16  
                                        | (buffer[pat_len-2] & 0x000000FF) << 8      
                                        | (buffer[pat_len-1] & 0x000000FF);   

    
    //Parse the PAT_Program_
    // 12 = 8 pat relate table + 4 CRC
    
    //uprintf("the psiPAT->section_length value is %d\n",psiPAT->section_length);
    
    for ( n = 0; n < psiPAT->section_length - 12; n += 4 )  
    {  
        program_num   =   buffer[8 + n ] << 8 | buffer[9 + n ];    
        psiPAT->reserved_3      =   buffer[10 + n ] >> 5;   
        
        psiPAT->network_pid     =   0x00;  
        if ( 0x00 == program_num)  
        {    
            psiPAT->network_pid = (buffer[10 + n ] & 0x1F) << 8 | buffer[11 + n ];  
            //TS_network_Pid = psiPAT->network_PID; //记录该TS流的网络PID  
        }  
        else  
        {  
            tmp = (P_TS_PAT_Program)malloc(sizeof(TS_PAT_Program));
            tmp->program_number = program_num;
            tmp->program_map_pid = (buffer[10 + n] & 0x1F) << 8| buffer[11 + n];
            list_add(&(tmp->list), &(__ts_pat_program_list.list));
        }
    }

    #ifdef DEBUG
        show_pat_program_info();
    #endif

    return ret;
}

int show_pat_program_info(void)
{
    struct list_head *pos;
    P_TS_PAT_Program tmp = (P_TS_PAT_Program)malloc(sizeof(TS_PAT_Program));
    P_TS_PAT_Program pFreetmp = tmp;

    list_for_each(pos, &__ts_pat_program_list.list)
    {
        tmp = list_entry(pos,TS_PAT_Program, list);
        uprintf("-------------------------------------------\n");
        uprintf("the program_num is  0x%X(%d)\n",tmp->program_number, tmp->program_number);
        uprintf("the program_map_pid 0x%X(%d)\n",tmp->program_map_pid, tmp->program_map_pid);
        uprintf("-------------------------------------------\n");
    }

    free(pFreetmp);
    
    return 0;
}




/*  
 *  Function    : Parse the pmt table and add the pmt stream info into
 *                global list------ts_pmt_stream_list
 */
int parse_pmt_table (unsigned char * pBuffer,unsigned int programNumber,
        TS_PMT_TABLE * psiPMT)  
{   
    int pmt_len;
    int pos_offset;
    P_TS_PMT_Stream tmp,freetmp;
    unsigned int pes_start;
    TS_PACKET_HEADER tsPacketHeader;
    P_TS_PACKET_HEADER ptsPacketHeader = &tsPacketHeader;
    parse_ts_packet_header(ptsPacketHeader, pBuffer);

    unsigned int offset = locate_offset(ptsPacketHeader,pBuffer, 0, &pes_start);
    unsigned char * buffer = pBuffer + offset;
    struct list_head *pos;

    psiPMT->table_id                            = buffer[0];  
    psiPMT->section_syntax_indicator            = buffer[1] >> 7;  
    psiPMT->zero                                = buffer[1] >> 6 & 0x01;   
    psiPMT->reserved_1                          = buffer[1] >> 4 & 0x03;  
    psiPMT->section_length                      = (buffer[1] & 0x0F) << 8 | buffer[2];      
    psiPMT->program_number                      = buffer[3] << 8 | buffer[4];  
    psiPMT->reserved_2                          = buffer[5] >> 6;  
    psiPMT->version_number                      = buffer[5] >> 1 & 0x1F;  
    psiPMT->current_next_indicator              = (buffer[5] << 7) >> 7;  
    psiPMT->section_number                      = buffer[6];  
    psiPMT->last_section_number                 = buffer[7];  
    psiPMT->reserved_3                          = buffer[8] >> 5;  
    psiPMT->PCR_PID                             = ((buffer[8] << 8) | buffer[9]) & 0x1FFF;  
    psiPMT->reserved_4                          = buffer[10] >> 4;  
    psiPMT->program_info_length                 = (buffer[10] & 0x0F) << 8 | buffer[11];   
    // Get CRC_32    
    pmt_len = psiPMT->section_length + 3;      
    psiPMT->CRC_32  = (buffer[pmt_len-4] & 0x000000FF) << 24  
                    | (buffer[pmt_len-3] & 0x000000FF) << 16  
                    | (buffer[pmt_len-2] & 0x000000FF) << 8  
                    | (buffer[pmt_len-1] & 0x000000FF);   
   
    pos_offset = 12;  
    // program info descriptor  
    if ( psiPMT->program_info_length != 0 )  
        pos_offset += psiPMT->program_info_length;      
        // Get stream type and PID
    

    //judge to add to __ts_pmt_stream_list or not.
    tmp =  (P_TS_PMT_Stream)malloc(sizeof(TS_PMT_Stream));
    freetmp = tmp;
    list_for_each(pos,&__ts_pmt_stream_list.list)
    {
        tmp = list_entry(pos,TS_PMT_Stream, list);

        if(tmp->program_number == programNumber)
        {
            free(freetmp);
            return 0;
        }
    }
    

    //section_length + 2 --> pmt start.  -4 --> CRC
    for ( ; pos_offset <= (psiPMT->section_length + 2 ) - 4; pos_offset += 5 )  
    {  
        tmp = (P_TS_PMT_Stream)malloc(sizeof(TS_PMT_Stream));
        tmp->stream_type    =  buffer[pos_offset];  
        tmp->reserved_5     =   buffer[pos_offset+1] >> 5;  
        tmp->elementary_PID =  ((buffer[pos_offset+1] << 8) | buffer[pos_offset+2]) & 0x1FFF;  
        tmp->reserved_6     =   buffer[pos_offset+3] >> 4;  
        tmp->ES_info_length =   (buffer[pos_offset+3] & 0x0F) << 8 | buffer[pos_offset+4];  
        tmp->program_number = programNumber;

        list_add(&(tmp->list), &(__ts_pmt_stream_list.list));

        if (tmp->ES_info_length != 0)  
            pos_offset += tmp->ES_info_length;    
    }  
    
    #ifdef DEBUG
//        show_pmt_stream_info();
    #endif
    
    return 0;  
}  

//very simple  unknown 0 Video =1  Audio 2
int judge_media_type(P_TS_PMT_Stream ptsPmtStream)
{   
    int mediaType = 0;
    switch (ptsPmtStream->stream_type)
    {
        case 0x01:
        case 0x02:
        case 0x1b:
            //Video
            mediaType = 1;
            break;
        case 0x03:
        case 0x04:
        case 0x11:
        case 0x0f:
            //Audio
            mediaType = 2;
            break;
        default:
            break;
    }

    return mediaType;
}



int show_pmt_stream_info(void)
{
    struct list_head *pos;
    P_TS_PMT_Stream tmp = (P_TS_PMT_Stream)malloc(sizeof(TS_PMT_Stream));
    P_TS_PMT_Stream pFreetmp = tmp;
    int mediaType = -1;
    char * mediaTypeString[3] = {"Unknown","Video","Audio"};
    
    list_for_each(pos, &__ts_pmt_stream_list.list)
    {
        tmp = list_entry(pos,TS_PMT_Stream, list);
        mediaType = judge_media_type(tmp);

        uprintf("-------------------------------------------\n");
        uprintf("the program_number is 0x%X(%d)\n",tmp->program_number,tmp->program_number);
        uprintf("the stream_type is    0x%X(%d)(%s)\n",tmp->stream_type,tmp->stream_type, mediaTypeString[mediaType]);
        uprintf("the elementary_PID is 0x%X,(%d)\n",tmp->elementary_PID, tmp->elementary_PID);
        uprintf("-------------------------------------------\n");
    }

    free(pFreetmp);

    return 0;
}

int setup_pmt_stream_list(FILE *pFile, unsigned int packetLength)
{
    struct list_head *pos;
    P_TS_PAT_Program tmp_pat_program = (P_TS_PAT_Program)malloc(sizeof(TS_PAT_Program));
    P_TS_PAT_Program pFreetmp = tmp_pat_program;
    TS_PMT_TABLE mtsPmtTable;
        
    unsigned char * pPacketBuffer = (unsigned char *)malloc(packetLength);
    unsigned char * pFreebuffer = pPacketBuffer;

    list_for_each(pos, &__ts_pat_program_list.list)
    {
        tmp_pat_program = list_entry(pos,TS_PAT_Program, list);
        find_given_table(pFile, pPacketBuffer, packetLength, tmp_pat_program->program_map_pid);
        parse_pmt_table(pPacketBuffer, tmp_pat_program->program_number, &mtsPmtTable); 
    }

    show_pmt_stream_info();

    free(pFreebuffer);
    free(pFreetmp);
    
    fseek(pFile, 0, SEEK_SET);
    
    return 0;
}




/*  
 *  Function    : Find the given table on the basis mUserPid.
 *  Description : 1. when find, copy the given table to storeBuffer.
 *                2. store the given table header to ptsPacketHeader.
 *  Note        : because some table use some sections to store it.
 *                So we need store a total table. 
 *                This function can't use to find pes data.
 */
int find_given_table(FILE *pFile, unsigned char *storeBuffer, 
        unsigned int mPacketLength, unsigned int mUserPid)
{
    int ret = -1;
    unsigned int offsetLength = 0;
    unsigned int sectionNumber = 0,lastSectionNumber = 0;
    unsigned char * tmpbuffer = (unsigned char *)malloc(mPacketLength);
    unsigned char *ptmpbuffer = tmpbuffer;
    unsigned int sectionCount = 0;
    unsigned int pes_start;

    TS_PACKET_HEADER tsPacketHeader;
    P_TS_PACKET_HEADER ptsPacketHeader = &tsPacketHeader;

    while((fread(tmpbuffer, mPacketLength, 1, pFile) == 1))
    {
        parse_ts_packet_header(ptsPacketHeader, tmpbuffer);

        if (mUserPid == ptsPacketHeader->pid)
        {
            offsetLength = locate_offset(ptsPacketHeader, tmpbuffer, 1, &pes_start);
            sectionNumber = tmpbuffer[offsetLength + 7];
            lastSectionNumber = tmpbuffer[offsetLength + 8];
            
            ret = lastSectionNumber+1;//we divide the table into ret section. 

            if(0 == lastSectionNumber)
            {
                memcpy(storeBuffer, tmpbuffer, mPacketLength);
                break;
            }
            
            if(sectionCount <= lastSectionNumber)
            {
                memcpy(storeBuffer[sectionNumber], tmpbuffer, mPacketLength);
                sectionCount++;
                if(sectionNumber == lastSectionNumber && sectionCount == lastSectionNumber + 1)
                    break;
            }
        }
    }

    
    if(ret == -1)
    {
        uprintf("Can't find the given table\n");
    }
    //return to the SEEK_SET position of pFile.
    fseek(pFile, 0, SEEK_SET);
    free(ptmpbuffer);

    return ret;
}

/*  
 *  Function    : Find the given table on the basis mUserPid.
 *  Description : 1. when find, copy the given table to storeBuffer.
 *                2. store the given table header to ptsPacketHeader.
 *  Note        : because some table use some sections to store it.
 *                So we need store a total table. 
 *                This function can't use to find pes data.
 *
 *                For SDT table . with more sections situation. Need to deal with.
 */


int find_given_table_more(FILE *pFile, unsigned char *storeBuffer, 
        unsigned int mPacketLength, unsigned int mUserPid,unsigned int tableId)
{
    int ret = -1;
    unsigned int offsetLength = 0;
    unsigned int sectionNumber = 0,lastSectionNumber = 0;
    unsigned char * tmpbuffer = (unsigned char *)malloc(mPacketLength);
    unsigned char *ptmpbuffer = tmpbuffer;
    unsigned char *pstoreBuffer = storeBuffer;
    unsigned int section_length = 0;
    unsigned int continuity_counter = 0;
    unsigned int pes_start;

    unsigned int copy_loop_total = 0;
    unsigned int copy_count = 0;

    TS_PACKET_HEADER tsPacketHeader;
    P_TS_PACKET_HEADER ptsPacketHeader = &tsPacketHeader;

    while((fread(tmpbuffer, mPacketLength, 1, pFile) == 1))
    {
        parse_ts_packet_header(ptsPacketHeader, tmpbuffer);

        offsetLength = locate_offset(ptsPacketHeader, tmpbuffer, 1, &pes_start);
        
        //distinguish the SDT BAT ST
        if (mUserPid == ptsPacketHeader->pid && tableId == tmpbuffer[offsetLength + 1])
        {
            ret = 1;
            continuity_counter = ptsPacketHeader->continuity_counter;

            section_length = ((tmpbuffer[offsetLength + 2] & 0x0f)<<8) | (tmpbuffer[offsetLength + 3]);
            //uprintf("the value of section_length is  0x%x\n",section_length);


            //3 meaning before section_length bytes.    
            copy_loop_total = (section_length + 3 + offsetLength) / mPacketLength;
            //uprintf("the value of copy_loop_total is  %d\n",copy_loop_total);


            if(0 == copy_loop_total)
                memcpy(storeBuffer, &ptmpbuffer[0], mPacketLength);
            else
            {
                for (; copy_count <= copy_loop_total;)
                {
                    if(0 == copy_count)
                    {
                        memcpy(pstoreBuffer, &ptmpbuffer[0], mPacketLength);
                        copy_count++;
                        pstoreBuffer += mPacketLength;
                    }
                    else
                    {
                        if(fread(tmpbuffer, mPacketLength, 1, pFile) !=1 )
                            fseek(pFile, 0, SEEK_SET);
                        parse_ts_packet_header(ptsPacketHeader, tmpbuffer);
                        offsetLength = locate_offset(ptsPacketHeader, tmpbuffer, 1, &pes_start);

                        if(mUserPid == ptsPacketHeader->pid && ptsPacketHeader->continuity_counter == continuity_counter + copy_count) 
                        {
                        # if 0
                            uprintf("the value of copy_count is %d\n",copy_count);
                            uprintf("the value of continuity_counter is %d\n",continuity_counter);
                            uprintf("the value of ptsPacketHeader->continuity_counter is %d\n",ptsPacketHeader->continuity_counter);
                        #endif
                            memcpy(pstoreBuffer,tmpbuffer+offsetLength, mPacketLength-offsetLength);
                            copy_count++;
                            pstoreBuffer += mPacketLength-offsetLength;
                        }
                    }
                }
            }
            break;
        }
    }


    
    if(ret == -1)
    {
        uprintf("Can't find the given table\n");
    }
    //return to the SEEK_SET position of pFile.
    fseek(pFile, 0, SEEK_SET);
    free(ptmpbuffer);

    return ret;
}


int search_given_program_info(unsigned int searchProgramId)
{
    int programPid = -1;
    
    struct list_head *pos;
    
    P_TS_PAT_Program tmp = (P_TS_PAT_Program)malloc(sizeof(TS_PAT_Program));
    P_TS_PAT_Program pFreetmp = tmp;

    list_for_each(pos, &__ts_pat_program_list.list)
    {
        tmp = list_entry(pos,TS_PAT_Program, list);
        if (tmp->program_number == searchProgramId)
        {
            programPid = tmp->program_map_pid;
            break;
        }
    }

    if(-1 == programPid)
        uprintf("Can't find the program info pid\n");
    else
        uprintf("We find the given program info pid\n");

    free(pFreetmp);

    return programPid;    
}


int identify_pes_start_prfix(unsigned char * p_tmpBuffer, unsigned int offsetLength)
{
    unsigned int packetStartCodePrefix = 0;
    unsigned char * buffer = p_tmpBuffer + offsetLength;
    int ret = -1;
    
    packetStartCodePrefix   = buffer[0] << 16 | buffer[1] << 8 | buffer[2];
    //uprintf("the packetStartCodePrefix value is %d\n",packetStartCodePrefix);
    if (PACKET_START_CODE_PREFIX == packetStartCodePrefix)
    {
        uprintf("identify_right successful\n");
        ret = 0;
    }

    return ret;
}



int store_pes_stream(FILE *pFile, FILE *storeFile,
        unsigned int packetLength, unsigned int mElementaryPid)
{
    TS_PACKET_HEADER mtsPacketHeader;
    P_TS_PACKET_HEADER ptsPacketHeader = &mtsPacketHeader;

    unsigned char * tmpBuffer = (unsigned char *)malloc(packetLength);
    unsigned char * pFreetmpBuffer = tmpBuffer;
    unsigned int pes_start;
    unsigned int offsetLength = 0;
    int ret;
        
    while((fread(tmpBuffer, packetLength, 1, pFile) == 1))
    {

        parse_ts_packet_header(&mtsPacketHeader, tmpBuffer);
       
        //       if (mElementaryPid == ptsPacketHeader->pid &&
        //               1 == ptsPacketHeader->payload_unit_start_indicator)
        if (mElementaryPid == ptsPacketHeader->pid)
        {
            offsetLength = locate_offset(ptsPacketHeader, tmpBuffer, 1, &pes_start);
#ifdef DEBUG_DETAIL
            uprintf("the offsetLength is %d\n",offsetLength);
            show_ts_packet_header(ptsPacketHeader);
            show_packet_memory(tmpBuffer,packetLength);
#endif
            if(1 != fwrite(tmpBuffer + offsetLength, packetLength - offsetLength, 1, storeFile))
            {
                uprintf("fwrite failed \n");
                return 1;
            }
        }
    }

    free(pFreetmpBuffer);
    return 0;
}


void show_packet_memory(unsigned char * buffer, unsigned int packetLength)
{
    int index = 0;
    printf("\nPrepare to show the  memory info\n");
    for(; index < packetLength; index++)
    {
            uprintf("%02X ",buffer[index]);
            if((index+1)%16 == 0)
                uprintf("\n");
    }
    uprintf("\n");
}









































