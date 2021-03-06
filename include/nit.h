/*********************************************************************
*
* Filename      :   nit.h
* Description   :   
* edited by     :   Jensen Zhen(JensenZhen@zhaoxin.com)
*
*********************************************************************/

#ifndef  _MPEG_TS_SI_NIT_H_
#define  _MPEG_TS_SI_NIT_H_

#include <descriptor_common.h>
/*********************************************************************
 *              NIT releated struction
 *   struction : NIT table --> transport_stream --> descriptor
 *   
 *   architecture:
 *
 *              NIT
 *              {
 *                  for(i=0; i < N; i++)
 *                      descriptor;
 *
 *                  for(i=0; i < N; i++)
 *                      transport_stream
 *                      {
 *                          for(i=0; i < N; i++)
 *                              descriptor;
 *                      }
 *              }
 *
 *
 *********************************************************************/
#define PID_TS_SI_NIT               (0x0010)
#define TABLE_ID_NIT_ACTUAL         (0x40)
#define TABLE_ID_NIT_OTHER          (0x41)


typedef struct transport_stream {
    unsigned transport_stream_id : 16;
    unsigned original_network_id : 16;
    unsigned reserved_future_use : 4;
    unsigned transport_descriptors_length : 12;
    
    DESCRIPTOR_COMMON * first_desc; // point to descriptor list
    struct transport_stream * next_transport_stream;
}TRANSPORT_STREAM, *P_TRANSPORT_STREAM;

#define NIT_TRANSPORT_STREAM_ID(b)                  ((b[0] << 8) | b[1])
#define NIT_TRANSPORT_ORIGINAL_NETWORK_ID(b)        ((b[2] << 8) | b[3])
#define NIT_TRANSPORT_DESC_LENGTH(b)                (((b[4] & 0x0f) << 4) | b[5])

//NIT table
typedef struct ts_nit_table {
	unsigned table_id : 8;
	unsigned section_syntax_indicator : 1;
	unsigned reserved_future_use_1 : 1;
	unsigned reserved_1 : 2;
	unsigned section_length : 12;
	unsigned network_id : 16;
	unsigned reserved_2 : 2;
	unsigned version_number : 5;
	unsigned current_next_indicator : 1;
	unsigned section_number : 8;
	unsigned last_section_number : 8;
	unsigned reserved_future_use_2 : 4;
	unsigned network_descriptors_length : 12;
    DESCRIPTOR_COMMON * nit_first_desc;// add by myself
	unsigned reserved_futrue_use_3 : 4;
    unsigned transport_stream_loop_length : 12;

    TRANSPORT_STREAM * first_transport_stream;
    struct ts_sdt_table * sdt_next_section;

	unsigned CRC_32;
}TS_NIT_TABLE,*P_TS_NIT_TABLE;

//releated operation

#define	NIT_TABLE_ID(b)				                (b[0])			/*  */

#define NIT_SECTION_SYNTAX_INDICATOR(b)			    ((b[1] & 0x80) >> 7)				/*  */

#define	NIT_RESERVED_FUTURE_USE_1(b)		        ((b[1] & 0x40) >> 6)			/*  */

#define	NIT_RESERVED_1(b)			                ((b[1] & 0x30) >> 4)			/*  */

#define	NIT_SECTION_LENGTH(b)			            (((b[1] & 0x0f) << 8) | b[2])			/*  */

#define	NIT_NETWORK_ID(b)				            ((b[3] << 8) | b[4])			/*  */

#define	NIT_RESERVED_2(b)			                ((b[5] & 0xc0) >> 6)			/*  */

#define	NIT_VERSION_NUMBER(b)			            ((b[5] & 0x3e) >> 1)			/*  */

#define	NIT_CURRENT_NEXT_INDICATOR(b)			    (b[5] & 0x01)			/*  */

#define	NIT_SECTION_NUMBER(b)			            (b[6])		/*  */

#define	NIT_LAST_SECTION_NUMBER(b)			        (b[7])			/*  */

#define	NIT_RESERVED_FUTURE_USE_2(b)		        ((b[8] & 0xf0) >> 4)			/*  */

#define	NIT_NETWORK_DESC_LENGTH(b)		            ((b[8] & 0x0f) << 8 | b[9])			/*  */
////////////////////////////////////////////////////
#define	NIT_CRC_32(b)				    ({ \
							int n = NIT_SECTION_LENGTH(b); \
							int l = n - 4; \
							int base = 3 + l; \
							((b[base] << 24) | (b[base + 1] << 16) | (b[base + 2] << 8) | b[base + 3]); \
							})


P_TRANSPORT_STREAM insert_nit_transport_stream_node(TRANSPORT_STREAM * Header, TRANSPORT_STREAM * node);
int decode_nit_transport_stream(unsigned char * byteptr, int this_section_length, TRANSPORT_STREAM * pNitTranstream);
TS_NIT_TABLE * parse_nit_table(FILE *pFile, unsigned int packetLength, unsigned int NIT_TABLE_ID);
TS_NIT_TABLE * parse_nit_table_onesection(unsigned char *byteptr, TS_NIT_TABLE * pNitTable);

void show_nit_transport_stream_info(TRANSPORT_STREAM * Header);

int show_nit_table_info(TS_NIT_TABLE * pNitTable);
void show_nit_descriptors_info(DESCRIPTOR_COMMON *header);
void show_nit_transport_stream_descriptors_info(TRANSPORT_STREAM * nitTransStream);

void free_transport_stream(TS_NIT_TABLE * nit);
void free_nit_table(TS_NIT_TABLE * nit_table_header);

/***************************** End of NIT releated struction **********************/

#endif   /* ----- #ifndef _MPEG_TS_SI_H_  ----- */

