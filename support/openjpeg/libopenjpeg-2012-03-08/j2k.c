/*
 * Copyright (c) 2002-2007, Communications and Remote Sensing Laboratory, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2007, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux and Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * Copyright (c) 2008, Jerome Fimes, Communications & Systemes <jerome.fimes@c-s.fr>
 * Copyright (c) 2006-2007, Parvatha Elangovan
 * Copyright (c) 2010-2011, Kaori Hagihara
 * Copyright (c) 2011, Mickael Savinaud, Communications & Systemes <mickael.savinaud@c-s.fr>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opj_includes.h"

/** @defgroup J2K J2K - JPEG-2000 codestream reader/writer */
/*@{*/

/** @name Local static functions */
/*@{*/

/**
 * Sets up the procedures to do on reading header. Developpers wanting to extend the library can add their own reading procedures.
 */
void j2k_setup_header_reading (opj_j2k_v2_t *p_j2k);

/**
 * The read header procedure.
 */
opj_bool j2k_read_header_procedure(
							    opj_j2k_v2_t *p_j2k,
								struct opj_stream_private *p_stream,
								struct opj_event_mgr * p_manager);

/**
 * The default decoding validation procedure without any extension.
 *
 * @param	p_j2k			the jpeg2000 codec to validate.
 * @param	p_stream				the input stream to validate.
 * @param	p_manager		the user event manager.
 *
 * @return true if the parameters are correct.
 */
opj_bool j2k_decoding_validation (
								opj_j2k_v2_t * p_j2k,
								opj_stream_private_t *p_stream,
								opj_event_mgr_t * p_manager
							);

/**
 * Sets up the validation ,i.e. adds the procedures to lauch to make sure the codec parameters
 * are valid. Developpers wanting to extend the library can add their own validation procedures.
 */
static void j2k_setup_decoding_validation (opj_j2k_v2_t *p_j2k);

/**
 * Builds the tcd decoder to use to decode tile.
 */
opj_bool j2k_build_decoder (
						opj_j2k_v2_t * p_j2k,
						opj_stream_private_t *p_stream,
						opj_event_mgr_t * p_manager
						);

/**
 * Excutes the given procedures on the given codec.
 *
 * @param	p_procedure_list	the list of procedures to execute
 * @param	p_j2k					the jpeg2000 codec to execute the procedures on.
 * @param	p_stream					the stream to execute the procedures on.
 * @param	p_manager			the user manager.
 *
 * @return	true				if all the procedures were successfully executed.
 */
static opj_bool j2k_exec (
					opj_j2k_v2_t * p_j2k,
					opj_procedure_list_t * p_procedure_list,
					opj_stream_private_t *p_stream,
					opj_event_mgr_t * p_manager
				  );

/**
 * Copies the decoding tile parameters onto all the tile parameters.
 * Creates also the tile decoder.
 */
opj_bool j2k_copy_default_tcp_and_create_tcd (	opj_j2k_v2_t * p_j2k,
												opj_stream_private_t *p_stream,
												opj_event_mgr_t * p_manager );

/**
 * Reads the lookup table containing all the marker, status and action, and returns the handler associated
 * with the marker value.
 * @param	p_id		Marker value to look up
 *
 * @return	the handler associated with the id.
*/
static const struct opj_dec_memory_marker_handler * j2k_get_marker_handler (OPJ_UINT32 p_id);

/**
 * Destroys a tile coding parameter structure.
 *
 * @param	p_tcp		the tile coding parameter to destroy.
 */
static void j2k_tcp_destroy (opj_tcp_v2_t *p_tcp);

/**
 * Destroys the data inside a tile coding parameter structure.
 *
 * @param	p_tcp		the tile coding parameter which contain data to destroy.
 */
static void j2k_tcp_data_destroy (opj_tcp_v2_t *p_tcp);

/**
 * Destroys a coding parameter structure.
 *
 * @param	p_cp		the coding parameter to destroy.
 */
static void j2k_cp_destroy (opj_cp_v2_t *p_cp);


/**
 * Reads a SPCod or SPCoc element, i.e. the coding style of a given component of a tile.
 * @param	p_header_data	the data contained in the COM box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the COM marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_SPCod_SPCoc(
							opj_j2k_v2_t *p_j2k,
							OPJ_UINT32 compno,
							OPJ_BYTE * p_header_data,
							OPJ_UINT32 * p_header_size,
							struct opj_event_mgr * p_manager
							);

/**
 * Reads a SQcd or SQcc element, i.e. the quantization values of a band in the QCD or QCC.
 *
 * @param	p_tile_no		the tile to output.
 * @param	p_comp_no		the component number to output.
 * @param	p_data			the data buffer.
 * @param	p_header_size	pointer to the size of the data buffer, it is changed by the function.
 * @param	p_j2k				J2K codec.
 * @param	p_manager		the user event manager.
 *
*/
static opj_bool j2k_read_SQcd_SQcc(
							opj_j2k_v2_t *p_j2k,
							OPJ_UINT32 compno,
							OPJ_BYTE * p_header_data,
							OPJ_UINT32 * p_header_size,
							struct opj_event_mgr * p_manager
					);

/**
 * Copies the tile component parameters of all the component from the first tile component.
 *
 * @param		p_j2k		the J2k codec.
 */
static void j2k_copy_tile_component_parameters(
							opj_j2k_v2_t *p_j2k
							);

/**
 * Copies the tile quantization parameters of all the component from the first tile component.
 *
 * @param		p_j2k		the J2k codec.
 */
static void j2k_copy_tile_quantization_parameters(
							opj_j2k_v2_t *p_j2k
							);

/**
 * Reads the tiles.
 */
opj_bool j2k_decode_tiles (		opj_j2k_v2_t *p_j2k,
								opj_stream_private_t *p_stream,
								opj_event_mgr_t * p_manager);

static opj_bool j2k_update_image_data (opj_tcd_v2_t * p_tcd, OPJ_BYTE * p_data, opj_image_t* p_output_image);


/*
 * -----------------------------------------------------------------------
 * -----------------------------------------------------------------------
 * -----------------------------------------------------------------------
 */

/**
Write the SOC marker (Start Of Codestream)
@param j2k J2K handle
*/
static void j2k_write_soc(opj_j2k_t *j2k);
/**
Read the SOC marker (Start of Codestream)
@param j2k J2K handle
*/
static void j2k_read_soc(opj_j2k_t *j2k);

/**
 * Reads a SOC marker (Start of Codestream)
 * @param	p_header_data	the data contained in the SOC box.
 * @param	jp2				the jpeg2000 file codec.
 * @param	p_header_size	the size of the data contained in the SOC marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_soc_v2(
					opj_j2k_v2_t *p_j2k,
					struct opj_stream_private *p_stream,
					struct opj_event_mgr * p_manager
				 );

/**
Write the SIZ marker (image and tile size)
@param j2k J2K handle
*/
static void j2k_write_siz(opj_j2k_t *j2k);
/**
Read the SIZ marker (image and tile size)
@param j2k J2K handle
*/
static void j2k_read_siz(opj_j2k_t *j2k);

/**
 * Reads a SIZ marker (image and tile size)
 * @param	p_header_data	the data contained in the SIZ box.
 * @param	jp2				the jpeg2000 file codec.
 * @param	p_header_size	the size of the data contained in the SIZ marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_siz_v2 (
						  opj_j2k_v2_t *p_j2k,
						  OPJ_BYTE * p_header_data,
						  OPJ_UINT32 p_header_size,
						  struct opj_event_mgr * p_manager
					);

/**
Write the COM marker (comment)
@param j2k J2K handle
*/
static void j2k_write_com(opj_j2k_t *j2k);
/**
Read the COM marker (comment)
@param j2k J2K handle
*/
static void j2k_read_com(opj_j2k_t *j2k);
/**
 * Reads a COM marker (comments)
 * @param	p_header_data	the data contained in the COM box.
 * @param	jp2				the jpeg2000 file codec.
 * @param	p_header_size	the size of the data contained in the COM marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_com_v2 (
					opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager
					);
/**
Write the value concerning the specified component in the marker COD and COC
@param j2k J2K handle
@param compno Number of the component concerned by the information written
*/
static void j2k_write_cox(opj_j2k_t *j2k, int compno);
/**
Read the value concerning the specified component in the marker COD and COC
@param j2k J2K handle
@param compno Number of the component concerned by the information read
*/
static void j2k_read_cox(opj_j2k_t *j2k, int compno);
/**
Write the COD marker (coding style default)
@param j2k J2K handle
*/
static void j2k_write_cod(opj_j2k_t *j2k);
/**
Read the COD marker (coding style default)
@param j2k J2K handle
*/
static void j2k_read_cod(opj_j2k_t *j2k);

/**
 * Reads a COD marker (Coding Styke defaults)
 * @param	p_header_data	the data contained in the COD box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the COD marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_cod_v2 (
					opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager
					);

/**
Write the COC marker (coding style component)
@param j2k J2K handle
@param compno Number of the component concerned by the information written
*/
static void j2k_write_coc(opj_j2k_t *j2k, int compno);
/**
Read the COC marker (coding style component)
@param j2k J2K handle
*/
static void j2k_read_coc(opj_j2k_t *j2k);

/**
 * Reads a COC marker (Coding Style Component)
 * @param	p_header_data	the data contained in the COC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the COC marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_coc_v2 (
					opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager
					);

/**
Write the value concerning the specified component in the marker QCD and QCC
@param j2k J2K handle
@param compno Number of the component concerned by the information written
*/
static void j2k_write_qcx(opj_j2k_t *j2k, int compno);
/**
Read the value concerning the specified component in the marker QCD and QCC
@param j2k J2K handle
@param compno Number of the component concern by the information read
@param len Length of the information in the QCX part of the marker QCD/QCC
*/
static void j2k_read_qcx(opj_j2k_t *j2k, int compno, int len);
/**
Write the QCD marker (quantization default)
@param j2k J2K handle
*/
static void j2k_write_qcd(opj_j2k_t *j2k);
/**
Read the QCD marker (quantization default)
@param j2k J2K handle
*/
static void j2k_read_qcd(opj_j2k_t *j2k);

/**
 * Reads a QCD marker (Quantization defaults)
 * @param	p_header_data	the data contained in the QCD box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the QCD marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_qcd_v2 (
					opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager
					);

/**
Write the QCC marker (quantization component)
@param j2k J2K handle
@param compno Number of the component concerned by the information written
*/
static void j2k_write_qcc(opj_j2k_t *j2k, int compno);
/**
Read the QCC marker (quantization component)
@param j2k J2K handle
*/
static void j2k_read_qcc(opj_j2k_t *j2k);
/**
 * Reads a QCC marker (Quantization component)
 * @param	p_header_data	the data contained in the QCC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the QCC marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_qcc_v2(
							opj_j2k_v2_t *p_j2k,
							OPJ_BYTE * p_header_data,
							OPJ_UINT32 p_header_size,
							struct opj_event_mgr * p_manager);

/**
Write the POC marker (progression order change)
@param j2k J2K handle
*/
static void j2k_write_poc(opj_j2k_t *j2k);
/**
Read the POC marker (progression order change)
@param j2k J2K handle
*/
static void j2k_read_poc(opj_j2k_t *j2k);
/**
 * Reads a POC marker (Progression Order Change)
 *
 * @param	p_header_data	the data contained in the POC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the POC marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_poc_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					);
/**
Read the CRG marker (component registration)
@param j2k J2K handle
*/
static void j2k_read_crg(opj_j2k_t *j2k);
/**
 * Reads a CRG marker (Component registration)
 *
 * @param	p_header_data	the data contained in the TLM box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the TLM marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_crg_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					);
/**
Read the TLM marker (tile-part lengths)
@param j2k J2K handle
*/
static void j2k_read_tlm(opj_j2k_t *j2k);
/**
 * Reads a TLM marker (Tile Length Marker)
 *
 * @param	p_header_data	the data contained in the TLM box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the TLM marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_tlm_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					);
/**
Read the PLM marker (packet length, main header)
@param j2k J2K handle
*/
static void j2k_read_plm(opj_j2k_t *j2k);

/**
 * Reads a PLM marker (Packet length, main header marker)
 *
 * @param	p_header_data	the data contained in the TLM box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the TLM marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_plm_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					);
/**
Read the PLT marker (packet length, tile-part header)
@param j2k J2K handle
*/
static void j2k_read_plt(opj_j2k_t *j2k);
/**
 * Reads a PLT marker (Packet length, tile-part header)
 *
 * @param	p_header_data	the data contained in the PLT box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the PLT marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_plt_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					);
/**
Read the PPM marker (packet packet headers, main header)
@param j2k J2K handle
*/
static void j2k_read_ppm(opj_j2k_t *j2k);
/**
 * Reads a PPM marker (Packed packet headers, main header)
 *
 * @param	p_header_data	the data contained in the POC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the POC marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_ppm_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					);

static opj_bool j2k_read_ppm_v3 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					);

/**
Read the PPT marker (packet packet headers, tile-part header)
@param j2k J2K handle
*/
static void j2k_read_ppt(opj_j2k_t *j2k);
/**
 * Reads a PPT marker (Packed packet headers, tile-part header)
 *
 * @param	p_header_data	the data contained in the PPT box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the PPT marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_ppt_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					);
/**
Write the TLM marker (Mainheader)
@param j2k J2K handle
*/
static void j2k_write_tlm(opj_j2k_t *j2k);
/**
Write the SOT marker (start of tile-part)
@param j2k J2K handle
*/
static void j2k_write_sot(opj_j2k_t *j2k);
/**
Read the SOT marker (start of tile-part)
@param j2k J2K handle
*/
static void j2k_read_sot(opj_j2k_t *j2k);

/**
 * Reads a PPT marker (Packed packet headers, tile-part header)
 *
 * @param	p_header_data	the data contained in the PPT box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the PPT marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_sot_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					);
/**
Write the SOD marker (start of data)
@param j2k J2K handle
@param tile_coder Pointer to a TCD handle
*/
static void j2k_write_sod(opj_j2k_t *j2k, void *tile_coder);
/**
Read the SOD marker (start of data)
@param j2k J2K handle
*/
static void j2k_read_sod(opj_j2k_t *j2k);

/**
 * Reads a SOD marker (Start Of Data)
 *
 * @param	p_header_data	the data contained in the SOD box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the SOD marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_sod_v2 (
						opj_j2k_v2_t *p_j2k,
						struct opj_stream_private *p_stream,
						struct opj_event_mgr * p_manager
					);

/**
Write the RGN marker (region-of-interest)
@param j2k J2K handle
@param compno Number of the component concerned by the information written
@param tileno Number of the tile concerned by the information written
*/
static void j2k_write_rgn(opj_j2k_t *j2k, int compno, int tileno);
/**
Read the RGN marker (region-of-interest)
@param j2k J2K handle
*/
static void j2k_read_rgn(opj_j2k_t *j2k);

/**
 * Reads a RGN marker (Region Of Interest)
 *
 * @param	p_header_data	the data contained in the POC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the POC marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_rgn_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					) ;

/**
Write the EOC marker (end of codestream)
@param j2k J2K handle
*/
static void j2k_write_eoc(opj_j2k_t *j2k);
/**
Read the EOC marker (end of codestream)
@param j2k J2K handle
*/
static void j2k_read_eoc(opj_j2k_t *j2k);

/**
 * Reads a EOC marker (End Of Codestream)
 *
 * @param	p_header_data	the data contained in the SOD box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the SOD marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_eoc_v2 (
					    opj_j2k_v2_t *p_j2k,
						struct opj_stream_private *p_stream,
						struct opj_event_mgr * p_manager
					) ;

/**
Read an unknown marker
@param j2k J2K handle
*/
static void j2k_read_unk(opj_j2k_t *j2k);
/**
Add main header marker information
@param cstr_info Codestream information structure
@param type marker type
@param pos byte offset of marker segment
@param len length of marker segment
 */
static void j2k_add_mhmarker(opj_codestream_info_t *cstr_info, unsigned short int type, int pos, int len);

static void j2k_add_mhmarker_v2(opj_codestream_index_t *cstr_index, OPJ_UINT32 type, OPJ_OFF_T pos, OPJ_UINT32 len) ;
/**
Add tile header marker information
@param tileno tile index number
@param cstr_info Codestream information structure
@param type marker type
@param pos byte offset of marker segment
@param len length of marker segment
 */
static void j2k_add_tlmarker( int tileno, opj_codestream_info_t *cstr_info, unsigned short int type, int pos, int len);

static void j2k_add_tlmarker_v2(OPJ_UINT32 tileno, opj_codestream_index_t *cstr_index, OPJ_UINT32 type, OPJ_OFF_T pos, OPJ_UINT32 len);

/**
 * Reads an unknown marker
 *
 * @param	p_stream		the stream object to read from.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_manager		the user event manager.
 *
 * @return	true			if the marker could be deduced.
*/
static opj_bool j2k_read_unk_v2 (	opj_j2k_v2_t *p_j2k,
									struct opj_stream_private *p_stream,
									OPJ_UINT32 *output_marker,
									struct opj_event_mgr * p_manager );

/**
 * Reads a MCT marker (Multiple Component Transform)
 *
 * @param	p_header_data	the data contained in the MCT box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the MCT marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_mct (	opj_j2k_v2_t *p_j2k,
								OPJ_BYTE * p_header_data,
								OPJ_UINT32 p_header_size,
								struct opj_event_mgr * p_manager );

/**
 * Reads a MCC marker (Multiple Component Collection)
 *
 * @param	p_header_data	the data contained in the MCC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the MCC marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_mcc (	opj_j2k_v2_t *p_j2k,
							OPJ_BYTE * p_header_data,
							OPJ_UINT32 p_header_size,
							struct opj_event_mgr * p_manager );

/**
 * Reads a MCO marker (Multiple Component Transform Ordering)
 *
 * @param	p_header_data	the data contained in the MCO box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the MCO marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_mco (	opj_j2k_v2_t *p_j2k,
								OPJ_BYTE * p_header_data,
								OPJ_UINT32 p_header_size,
								struct opj_event_mgr * p_manager );

static opj_bool j2k_add_mct(opj_tcp_v2_t * p_tcp, opj_image_t * p_image, OPJ_UINT32 p_index);

static void  j2k_read_int16_to_float (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);
static void  j2k_read_int32_to_float (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);
static void  j2k_read_float32_to_float (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);
static void  j2k_read_float64_to_float (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);

static void  j2k_read_int16_to_int32 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);
static void  j2k_read_int32_to_int32 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);
static void  j2k_read_float32_to_int32 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);
static void  j2k_read_float64_to_int32 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);

static void  j2k_write_float_to_int16 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);
static void  j2k_write_float_to_int32 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);
static void  j2k_write_float_to_float (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);
static void  j2k_write_float_to_float64 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);

/**
 * Reads a CBD marker (Component bit depth definition)
 * @param	p_header_data	the data contained in the CBD box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the CBD marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_cbd (	opj_j2k_v2_t *p_j2k,
							OPJ_BYTE * p_header_data,
							OPJ_UINT32 p_header_size,
							struct opj_event_mgr * p_manager);


static void j2k_dump_MH_info(opj_j2k_v2_t* p_j2k, FILE* out_stream);

static void j2k_dump_MH_index(opj_j2k_v2_t* p_j2k, FILE* out_stream);

static opj_codestream_index_t* j2k_create_cstr_index(void);

/*@}*/

/*@}*/

/* ----------------------------------------------------------------------- */
typedef struct j2k_prog_order{
	OPJ_PROG_ORDER enum_prog;
	char str_prog[5];
}j2k_prog_order_t;

j2k_prog_order_t j2k_prog_order_list[] = {
	{CPRL, "CPRL"},
	{LRCP, "LRCP"},
	{PCRL, "PCRL"},
	{RLCP, "RLCP"},
	{RPCL, "RPCL"},
	{(OPJ_PROG_ORDER)-1, ""}
};



/**
 * FIXME DOC
 */
const OPJ_UINT32 MCT_ELEMENT_SIZE [] =
{
	2,
	4,
	4,
	8
};

typedef void (* j2k_mct_function) (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem);

const j2k_mct_function j2k_mct_read_functions_to_float [] =
{
	j2k_read_int16_to_float,
	j2k_read_int32_to_float,
	j2k_read_float32_to_float,
	j2k_read_float64_to_float
};

const j2k_mct_function j2k_mct_read_functions_to_int32 [] =
{
	j2k_read_int16_to_int32,
	j2k_read_int32_to_int32,
	j2k_read_float32_to_int32,
	j2k_read_float64_to_int32
};

const j2k_mct_function j2k_mct_write_functions_from_float [] =
{
	j2k_write_float_to_int16,
	j2k_write_float_to_int32,
	j2k_write_float_to_float,
	j2k_write_float_to_float64
};

typedef struct opj_dec_memory_marker_handler
{
	/** marker value */
	OPJ_UINT32 id;
	/** value of the state when the marker can appear */
	OPJ_UINT32 states;
	/** action linked to the marker */
	opj_bool (*handler) (
					opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager
						);
}
opj_dec_memory_marker_handler_t;

const opj_dec_memory_marker_handler_t j2k_memory_marker_handler_tab [] =
{
#ifdef TODO_MS
  {J2K_MS_SOT, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPHSOT, j2k_read_sot},
  {J2K_MS_COD, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_cod},
  {J2K_MS_COC, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_coc},
  {J2K_MS_RGN, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_rgn},
  {J2K_MS_QCD, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_qcd},
  {J2K_MS_QCC, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_qcc},
  {J2K_MS_POC, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_poc},
  {J2K_MS_SIZ, J2K_DEC_STATE_MHSIZ , j2k_read_siz},
  {J2K_MS_TLM, J2K_DEC_STATE_MH, j2k_read_tlm},
  {J2K_MS_PLM, J2K_DEC_STATE_MH, j2k_read_plm},
  {J2K_MS_PLT, J2K_DEC_STATE_TPH, j2k_read_plt},
  {J2K_MS_PPM, J2K_DEC_STATE_MH, j2k_read_ppm},
  {J2K_MS_PPT, J2K_DEC_STATE_TPH, j2k_read_ppt},
  {J2K_MS_SOP, 0, 0},
  {J2K_MS_CRG, J2K_DEC_STATE_MH, j2k_read_crg},
  {J2K_MS_COM, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_com},
  {J2K_MS_MCT, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_mct},
  {J2K_MS_CBD, J2K_DEC_STATE_MH , j2k_read_cbd},
  {J2K_MS_MCC, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_mcc},
  {J2K_MS_MCO, J2K_DEC_STATE_MH | J2K_DEC_STATE_TPH, j2k_read_mco},
#endif
  {J2K_MS_SOT, J2K_STATE_MH | J2K_STATE_TPHSOT, j2k_read_sot_v2},
  {J2K_MS_COD, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_cod_v2},
  {J2K_MS_COC, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_coc_v2},
  {J2K_MS_RGN, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_rgn_v2},
  {J2K_MS_QCD, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_qcd_v2},
  {J2K_MS_QCC, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_qcc_v2},
  {J2K_MS_POC, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_poc_v2},
  {J2K_MS_SIZ, J2K_STATE_MHSIZ , j2k_read_siz_v2},
  {J2K_MS_TLM, J2K_STATE_MH, j2k_read_tlm_v2},
  {J2K_MS_PLM, J2K_STATE_MH, j2k_read_plm_v2},
  {J2K_MS_PLT, J2K_STATE_TPH, j2k_read_plt_v2},
  {J2K_MS_PPM, J2K_STATE_MH, j2k_read_ppm_v3},
  {J2K_MS_PPT, J2K_STATE_TPH, j2k_read_ppt_v2},
  {J2K_MS_SOP, 0, 0},
  {J2K_MS_CRG, J2K_STATE_MH, j2k_read_crg_v2},
  {J2K_MS_COM, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_com_v2},
  {J2K_MS_MCT, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_mct},
  {J2K_MS_CBD, J2K_STATE_MH , j2k_read_cbd},
  {J2K_MS_MCC, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_mcc},
  {J2K_MS_MCO, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_mco},
#ifdef USE_JPWL
#ifdef TODO_MS /* FIXME */
  {J2K_MS_EPC, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_epc},
  {J2K_MS_EPB, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_epb},
  {J2K_MS_ESD, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_esd},
  {J2K_MS_RED, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_red},
#endif
#endif /* USE_JPWL */
#ifdef USE_JPSEC
  {J2K_MS_SEC, J2K_DEC_STATE_MH, j2k_read_sec},
  {J2K_MS_INSEC, 0, j2k_read_insec}
#endif /* USE_JPSEC */
  {J2K_MS_UNK, J2K_STATE_MH | J2K_STATE_TPH, 0}/*j2k_read_unk_v2}*/
};



void  j2k_read_int16_to_float (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_src_data = (OPJ_BYTE *) p_src_data;
	OPJ_FLOAT32 * l_dest_data = (OPJ_FLOAT32 *) p_dest_data;
	OPJ_UINT32 i;
	OPJ_UINT32 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		opj_read_bytes(l_src_data,&l_temp,2);

		l_src_data+=sizeof(OPJ_INT16);

		*(l_dest_data++) = (OPJ_FLOAT32) l_temp;
	}
}

void  j2k_read_int32_to_float (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_src_data = (OPJ_BYTE *) p_src_data;
	OPJ_FLOAT32 * l_dest_data = (OPJ_FLOAT32 *) p_dest_data;
	OPJ_UINT32 i;
	OPJ_UINT32 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		opj_read_bytes(l_src_data,&l_temp,4);

		l_src_data+=sizeof(OPJ_INT32);

		*(l_dest_data++) = (OPJ_FLOAT32) l_temp;
	}
}

void  j2k_read_float32_to_float (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_src_data = (OPJ_BYTE *) p_src_data;
	OPJ_FLOAT32 * l_dest_data = (OPJ_FLOAT32 *) p_dest_data;
	OPJ_UINT32 i;
	OPJ_FLOAT32 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		opj_read_float(l_src_data,&l_temp);

		l_src_data+=sizeof(OPJ_FLOAT32);

		*(l_dest_data++) = l_temp;
	}
}

void  j2k_read_float64_to_float (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_src_data = (OPJ_BYTE *) p_src_data;
	OPJ_FLOAT32 * l_dest_data = (OPJ_FLOAT32 *) p_dest_data;
	OPJ_UINT32 i;
	OPJ_FLOAT64 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		opj_read_double(l_src_data,&l_temp);

		l_src_data+=sizeof(OPJ_FLOAT64);

		*(l_dest_data++) = (OPJ_FLOAT32) l_temp;
	}
}

void  j2k_read_int16_to_int32 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_src_data = (OPJ_BYTE *) p_src_data;
	OPJ_INT32 * l_dest_data = (OPJ_INT32 *) p_dest_data;
	OPJ_UINT32 i;
	OPJ_UINT32 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		opj_read_bytes(l_src_data,&l_temp,2);

		l_src_data+=sizeof(OPJ_INT16);

		*(l_dest_data++) = (OPJ_INT32) l_temp;
	}
}

void  j2k_read_int32_to_int32 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_src_data = (OPJ_BYTE *) p_src_data;
	OPJ_INT32 * l_dest_data = (OPJ_INT32 *) p_dest_data;
	OPJ_UINT32 i;
	OPJ_UINT32 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		opj_read_bytes(l_src_data,&l_temp,4);

		l_src_data+=sizeof(OPJ_INT32);

		*(l_dest_data++) = (OPJ_INT32) l_temp;
	}
}

void  j2k_read_float32_to_int32 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_src_data = (OPJ_BYTE *) p_src_data;
	OPJ_INT32 * l_dest_data = (OPJ_INT32 *) p_dest_data;
	OPJ_UINT32 i;
	OPJ_FLOAT32 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		opj_read_float(l_src_data,&l_temp);

		l_src_data+=sizeof(OPJ_FLOAT32);

		*(l_dest_data++) = (OPJ_INT32) l_temp;
	}
}

void  j2k_read_float64_to_int32 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_src_data = (OPJ_BYTE *) p_src_data;
	OPJ_INT32 * l_dest_data = (OPJ_INT32 *) p_dest_data;
	OPJ_UINT32 i;
	OPJ_FLOAT64 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		opj_read_double(l_src_data,&l_temp);

		l_src_data+=sizeof(OPJ_FLOAT64);

		*(l_dest_data++) = (OPJ_INT32) l_temp;
	}
}

void  j2k_write_float_to_int16 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_dest_data = (OPJ_BYTE *) p_dest_data;
	OPJ_FLOAT32 * l_src_data = (OPJ_FLOAT32 *) p_src_data;
	OPJ_UINT32 i;
	OPJ_UINT32 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		l_temp = (OPJ_UINT32) *(l_src_data++);

		opj_write_bytes(l_dest_data,l_temp,sizeof(OPJ_INT16));

		l_dest_data+=sizeof(OPJ_INT16);
	}
}

void  j2k_write_float_to_int32 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_dest_data = (OPJ_BYTE *) p_dest_data;
	OPJ_FLOAT32 * l_src_data = (OPJ_FLOAT32 *) p_src_data;
	OPJ_UINT32 i;
	OPJ_UINT32 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		l_temp = (OPJ_UINT32) *(l_src_data++);

		opj_write_bytes(l_dest_data,l_temp,sizeof(OPJ_INT32));

		l_dest_data+=sizeof(OPJ_INT32);
	}
}

void  j2k_write_float_to_float (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_dest_data = (OPJ_BYTE *) p_dest_data;
	OPJ_FLOAT32 * l_src_data = (OPJ_FLOAT32 *) p_src_data;
	OPJ_UINT32 i;
	OPJ_FLOAT32 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		l_temp = (OPJ_FLOAT32) *(l_src_data++);

		opj_write_float(l_dest_data,l_temp);

		l_dest_data+=sizeof(OPJ_FLOAT32);
	}
}

void  j2k_write_float_to_float64 (const void * p_src_data, void * p_dest_data, OPJ_UINT32 p_nb_elem)
{
	OPJ_BYTE * l_dest_data = (OPJ_BYTE *) p_dest_data;
	OPJ_FLOAT32 * l_src_data = (OPJ_FLOAT32 *) p_src_data;
	OPJ_UINT32 i;
	OPJ_FLOAT64 l_temp;

	for (i=0;i<p_nb_elem;++i) {
		l_temp = (OPJ_FLOAT64) *(l_src_data++);

		opj_write_double(l_dest_data,l_temp);

		l_dest_data+=sizeof(OPJ_FLOAT64);
	}
}


/**
 * Converts an enum type progression order to string type.
 *
 * @param prg_order		the progression order to get.
 *
 * @return	the string representation of the given progression order.
 */
char *j2k_convert_progression_order(OPJ_PROG_ORDER prg_order){
	j2k_prog_order_t *po;
	for(po = j2k_prog_order_list; po->enum_prog != -1; po++ ){
		if(po->enum_prog == prg_order){
			return po->str_prog;
		}
	}
	return po->str_prog;
}

/* ----------------------------------------------------------------------- */
static int j2k_get_num_tp(opj_cp_t *cp,int pino,int tileno){
	char *prog;
	int i;
	int tpnum=1,tpend=0;
	opj_tcp_t *tcp = &cp->tcps[tileno];
	prog = j2k_convert_progression_order(tcp->prg);
	
	if(cp->tp_on == 1){
		for(i=0;i<4;i++){
			if(tpend!=1){
				if( cp->tp_flag == prog[i] ){
					tpend=1;cp->tp_pos=i;
				}
				switch(prog[i]){
				case 'C':
					tpnum= tpnum * tcp->pocs[pino].compE;
					break;
				case 'R':
					tpnum= tpnum * tcp->pocs[pino].resE;
					break;
				case 'P':
					tpnum= tpnum * tcp->pocs[pino].prcE;
					break;
				case 'L':
					tpnum= tpnum * tcp->pocs[pino].layE;
					break;
				}
			}
		}
	}else{
		tpnum=1;
	}
	return tpnum;
}

/**	mem allocation for TLM marker*/
int j2k_calculate_tp(opj_cp_t *cp,int img_numcomp,opj_image_t *image,opj_j2k_t *j2k ){
	int pino,tileno,totnum_tp=0;

	OPJ_ARG_NOT_USED(img_numcomp);

	j2k->cur_totnum_tp = (int *) opj_malloc(cp->tw * cp->th * sizeof(int));
	for (tileno = 0; tileno < cp->tw * cp->th; tileno++) {
		int cur_totnum_tp = 0;
		opj_tcp_t *tcp = &cp->tcps[tileno];
		for(pino = 0; pino <= tcp->numpocs; pino++) {
			int tp_num=0;
			opj_pi_iterator_t *pi = pi_initialise_encode(image, cp, tileno,FINAL_PASS);
			if(!pi) { return -1;}
			tp_num = j2k_get_num_tp(cp,pino,tileno);
			totnum_tp = totnum_tp + tp_num;
			cur_totnum_tp = cur_totnum_tp + tp_num;
			pi_destroy(pi, cp, tileno);
		}
		j2k->cur_totnum_tp[tileno] = cur_totnum_tp;
		/* INDEX >> */
		if (j2k->cstr_info) {
			j2k->cstr_info->tile[tileno].num_tps = cur_totnum_tp;
			j2k->cstr_info->tile[tileno].tp = (opj_tp_info_t *) opj_malloc(cur_totnum_tp * sizeof(opj_tp_info_t));
		}
		/* << INDEX */
	}
	return totnum_tp;
}

static void j2k_write_soc(opj_j2k_t *j2k) {
	opj_cio_t *cio = j2k->cio;
	cio_write(cio, J2K_MS_SOC, 2);

	if(j2k->cstr_info)
	  j2k_add_mhmarker(j2k->cstr_info, J2K_MS_SOC, cio_tell(cio), 0);

/* UniPG>> */
#ifdef USE_JPWL

	/* update markers struct */
	j2k_add_marker(j2k->cstr_info, J2K_MS_SOC, cio_tell(cio) - 2, 2);
#endif /* USE_JPWL */
/* <<UniPG */
}

static void j2k_read_soc(opj_j2k_t *j2k) {	
	j2k->state = J2K_STATE_MHSIZ;
	/* Index */
	if (j2k->cstr_info) {
		j2k->cstr_info->main_head_start = cio_tell(j2k->cio) - 2;
		j2k->cstr_info->codestream_size = cio_numbytesleft(j2k->cio) + 2 - j2k->cstr_info->main_head_start;
	}
}

/**
 * Reads a SOC marker (Start of Codestream)
 * @param	p_header_data	the data contained in the SOC box.
 * @param	jp2				the jpeg2000 file codec.
 * @param	p_header_size	the size of the data contained in the SOC marker.
 * @param	p_manager		the user event manager.
*/
static opj_bool j2k_read_soc_v2(	opj_j2k_v2_t *p_j2k,
									struct opj_stream_private *p_stream,
									struct opj_event_mgr * p_manager )
{
	OPJ_BYTE l_data [2];
	OPJ_UINT32 l_marker;

	/* preconditions */
	assert(p_j2k != 00);
	assert(p_manager != 00);
	assert(p_stream != 00);

	if (opj_stream_read_data(p_stream,l_data,2,p_manager) != 2) {
		return OPJ_FALSE;
	}

	opj_read_bytes(l_data,&l_marker,2);
	if (l_marker != J2K_MS_SOC) {
		return OPJ_FALSE;
	}

	/* Next marker should be a SIZ marker in the main header */
	p_j2k->m_specific_param.m_decoder.m_state = J2K_STATE_MHSIZ;

	/* FIXME move it in a index structure included in p_j2k*/
	p_j2k->cstr_index->main_head_start = opj_stream_tell(p_stream) - 2;

	opj_event_msg_v2(p_manager, EVT_INFO, "Start to read j2k main header (%d).\n", p_j2k->cstr_index->main_head_start);

	/* Add the marker to the codestream index*/
	j2k_add_mhmarker_v2(p_j2k->cstr_index, J2K_MS_SOC, p_j2k->cstr_index->main_head_start, 2);

	return OPJ_TRUE;
}

static void j2k_write_siz(opj_j2k_t *j2k) {
	int i;
	int lenp, len;

	opj_cio_t *cio = j2k->cio;
	opj_image_t *image = j2k->image;
	opj_cp_t *cp = j2k->cp;

	cio_write(cio, J2K_MS_SIZ, 2);	/* SIZ */
	lenp = cio_tell(cio);
	cio_skip(cio, 2);
	cio_write(cio, cp->rsiz, 2);			/* Rsiz (capabilities) */
	cio_write(cio, image->x1, 4);	/* Xsiz */
	cio_write(cio, image->y1, 4);	/* Ysiz */
	cio_write(cio, image->x0, 4);	/* X0siz */
	cio_write(cio, image->y0, 4);	/* Y0siz */
	cio_write(cio, cp->tdx, 4);		/* XTsiz */
	cio_write(cio, cp->tdy, 4);		/* YTsiz */
	cio_write(cio, cp->tx0, 4);		/* XT0siz */
	cio_write(cio, cp->ty0, 4);		/* YT0siz */
	cio_write(cio, image->numcomps, 2);	/* Csiz */
	for (i = 0; i < image->numcomps; i++) {
		cio_write(cio, image->comps[i].prec - 1 + (image->comps[i].sgnd << 7), 1);	/* Ssiz_i */
		cio_write(cio, image->comps[i].dx, 1);	/* XRsiz_i */
		cio_write(cio, image->comps[i].dy, 1);	/* YRsiz_i */
	}
	len = cio_tell(cio) - lenp;
	cio_seek(cio, lenp);
	cio_write(cio, len, 2);		/* Lsiz */
	cio_seek(cio, lenp + len);
	
	if(j2k->cstr_info)
	  j2k_add_mhmarker(j2k->cstr_info, J2K_MS_SIZ, lenp, len);
}

static void j2k_read_siz(opj_j2k_t *j2k) {
	int len, i;
	
	opj_cio_t *cio = j2k->cio;
	opj_image_t *image = j2k->image;
	opj_cp_t *cp = j2k->cp;
	
	len = cio_read(cio, 2);			/* Lsiz */
	cio_read(cio, 2);				/* Rsiz (capabilities) */
	image->x1 = cio_read(cio, 4);	/* Xsiz */
	image->y1 = cio_read(cio, 4);	/* Ysiz */
	image->x0 = cio_read(cio, 4);	/* X0siz */
	image->y0 = cio_read(cio, 4);	/* Y0siz */
	cp->tdx = cio_read(cio, 4);		/* XTsiz */
	cp->tdy = cio_read(cio, 4);		/* YTsiz */
	cp->tx0 = cio_read(cio, 4);		/* XT0siz */
	cp->ty0 = cio_read(cio, 4);		/* YT0siz */
	
	if ((image->x0<0)||(image->x1<0)||(image->y0<0)||(image->y1<0)) {
		opj_event_msg(j2k->cinfo, EVT_ERROR,
									"%s: invalid image size (x0:%d, x1:%d, y0:%d, y1:%d)\n",
									image->x0,image->x1,image->y0,image->y1);
		return;
	}
	
	image->numcomps = cio_read(cio, 2);	/* Csiz */

#ifdef USE_JPWL
	if (j2k->cp->correct) {
		/* if JPWL is on, we check whether TX errors have damaged
		  too much the SIZ parameters */
		if (!(image->x1 * image->y1)) {
			opj_event_msg(j2k->cinfo, EVT_ERROR,
				"JPWL: bad image size (%d x %d)\n",
				image->x1, image->y1);
			if (!JPWL_ASSUME || JPWL_ASSUME) {
				opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
				return;
			}
		}
		if (image->numcomps != ((len - 38) / 3)) {
			opj_event_msg(j2k->cinfo, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
				"JPWL: Csiz is %d => space in SIZ only for %d comps.!!!\n",
				image->numcomps, ((len - 38) / 3));
			if (!JPWL_ASSUME) {
				opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
				return;
			}
			/* we try to correct */
			opj_event_msg(j2k->cinfo, EVT_WARNING, "- trying to adjust this\n");
			if (image->numcomps < ((len - 38) / 3)) {
				len = 38 + 3 * image->numcomps;
				opj_event_msg(j2k->cinfo, EVT_WARNING, "- setting Lsiz to %d => HYPOTHESIS!!!\n",
					len);				
			} else {
				image->numcomps = ((len - 38) / 3);
				opj_event_msg(j2k->cinfo, EVT_WARNING, "- setting Csiz to %d => HYPOTHESIS!!!\n",
					image->numcomps);				
			}
		}

		/* update components number in the jpwl_exp_comps filed */
		cp->exp_comps = image->numcomps;
	}
#endif /* USE_JPWL */

	image->comps = (opj_image_comp_t*) opj_calloc(image->numcomps, sizeof(opj_image_comp_t));
	for (i = 0; i < image->numcomps; i++) {
		int tmp, w, h;
		tmp = cio_read(cio, 1);		/* Ssiz_i */
		image->comps[i].prec = (tmp & 0x7f) + 1;
		image->comps[i].sgnd = tmp >> 7;
		image->comps[i].dx = cio_read(cio, 1);	/* XRsiz_i */
		image->comps[i].dy = cio_read(cio, 1);	/* YRsiz_i */
		
#ifdef USE_JPWL
		if (j2k->cp->correct) {
		/* if JPWL is on, we check whether TX errors have damaged
			too much the SIZ parameters, again */
			if (!(image->comps[i].dx * image->comps[i].dy)) {
				opj_event_msg(j2k->cinfo, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
					"JPWL: bad XRsiz_%d/YRsiz_%d (%d x %d)\n",
					i, i, image->comps[i].dx, image->comps[i].dy);
				if (!JPWL_ASSUME) {
					opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
					return;
				}
				/* we try to correct */
				opj_event_msg(j2k->cinfo, EVT_WARNING, "- trying to adjust them\n");
				if (!image->comps[i].dx) {
					image->comps[i].dx = 1;
					opj_event_msg(j2k->cinfo, EVT_WARNING, "- setting XRsiz_%d to %d => HYPOTHESIS!!!\n",
						i, image->comps[i].dx);
				}
				if (!image->comps[i].dy) {
					image->comps[i].dy = 1;
					opj_event_msg(j2k->cinfo, EVT_WARNING, "- setting YRsiz_%d to %d => HYPOTHESIS!!!\n",
						i, image->comps[i].dy);
				}
			}
			
		}
#endif /* USE_JPWL */

		/* TODO: unused ? */
		w = int_ceildiv(image->x1 - image->x0, image->comps[i].dx);
		h = int_ceildiv(image->y1 - image->y0, image->comps[i].dy);

		image->comps[i].resno_decoded = 0;	/* number of resolution decoded */
		image->comps[i].factor = cp->reduce; /* reducing factor per component */
	}
	
	cp->tw = int_ceildiv(image->x1 - cp->tx0, cp->tdx);
	cp->th = int_ceildiv(image->y1 - cp->ty0, cp->tdy);

#ifdef USE_JPWL
	if (j2k->cp->correct) {
		/* if JPWL is on, we check whether TX errors have damaged
		  too much the SIZ parameters */
		if ((cp->tw < 1) || (cp->th < 1) || (cp->tw > cp->max_tiles) || (cp->th > cp->max_tiles)) {
			opj_event_msg(j2k->cinfo, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
				"JPWL: bad number of tiles (%d x %d)\n",
				cp->tw, cp->th);
			if (!JPWL_ASSUME) {
				opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
				return;
			}
			/* we try to correct */
			opj_event_msg(j2k->cinfo, EVT_WARNING, "- trying to adjust them\n");
			if (cp->tw < 1) {
				cp->tw= 1;
				opj_event_msg(j2k->cinfo, EVT_WARNING, "- setting %d tiles in x => HYPOTHESIS!!!\n",
					cp->tw);
			}
			if (cp->tw > cp->max_tiles) {
				cp->tw= 1;
				opj_event_msg(j2k->cinfo, EVT_WARNING, "- too large x, increase expectance of %d\n"
					"- setting %d tiles in x => HYPOTHESIS!!!\n",
					cp->max_tiles, cp->tw);
			}
			if (cp->th < 1) {
				cp->th= 1;
				opj_event_msg(j2k->cinfo, EVT_WARNING, "- setting %d tiles in y => HYPOTHESIS!!!\n",
					cp->th);
			}
			if (cp->th > cp->max_tiles) {
				cp->th= 1;
				opj_event_msg(j2k->cinfo, EVT_WARNING, "- too large y, increase expectance of %d to continue\n",
					"- setting %d tiles in y => HYPOTHESIS!!!\n",
					cp->max_tiles, cp->th);
			}
		}
	}
#endif /* USE_JPWL */

	cp->tcps = (opj_tcp_t*) opj_calloc(cp->tw * cp->th, sizeof(opj_tcp_t));
	cp->tileno = (int*) opj_malloc(cp->tw * cp->th * sizeof(int));
	cp->tileno_size = 0;
	
#ifdef USE_JPWL
	if (j2k->cp->correct) {
		if (!cp->tcps) {
			opj_event_msg(j2k->cinfo, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
				"JPWL: could not alloc tcps field of cp\n");
			if (!JPWL_ASSUME || JPWL_ASSUME) {
				opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
				return;
			}
		}
	}
#endif /* USE_JPWL */

	for (i = 0; i < cp->tw * cp->th; i++) {
		cp->tcps[i].POC = 0;
		cp->tcps[i].numpocs = 0;
		cp->tcps[i].first = 1;
	}
	
	/* Initialization for PPM marker */
	cp->ppm = 0;
	cp->ppm_data = NULL;
	cp->ppm_data_first = NULL;
	cp->ppm_previous = 0;
	cp->ppm_store = 0;

	j2k->default_tcp->tccps = (opj_tccp_t*) opj_calloc(image->numcomps, sizeof(opj_tccp_t));
	for (i = 0; i < cp->tw * cp->th; i++) {
		cp->tcps[i].tccps = (opj_tccp_t*) opj_malloc(image->numcomps * sizeof(opj_tccp_t));
	}	
	j2k->tile_data = (unsigned char**) opj_calloc(cp->tw * cp->th, sizeof(unsigned char*));
	j2k->tile_len = (int*) opj_calloc(cp->tw * cp->th, sizeof(int));
	j2k->state = J2K_STATE_MH;

	/* Index */
	if (j2k->cstr_info) {
		opj_codestream_info_t *cstr_info = j2k->cstr_info;
		cstr_info->image_w = image->x1 - image->x0;
		cstr_info->image_h = image->y1 - image->y0;
		cstr_info->numcomps = image->numcomps;
		cstr_info->tw = cp->tw;
		cstr_info->th = cp->th;
		cstr_info->tile_x = cp->tdx;	
		cstr_info->tile_y = cp->tdy;	
		cstr_info->tile_Ox = cp->tx0;	
		cstr_info->tile_Oy = cp->ty0;			
		cstr_info->tile = (opj_tile_info_t*) opj_calloc(cp->tw * cp->th, sizeof(opj_tile_info_t));		
	}
}


/**
 * Reads a SIZ marker (image and tile size)
 * @param	p_header_data	the data contained in the SIZ box.
 * @param	jp2				the jpeg2000 file codec.
 * @param	p_header_size	the size of the data contained in the SIZ marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_siz_v2 (
				    opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager
					)
{
	OPJ_UINT32 l_size, i;
	OPJ_UINT32 l_nb_comp;
	OPJ_UINT32 l_nb_comp_remain;
	OPJ_UINT32 l_remaining_size;
	OPJ_UINT32 l_nb_tiles;
	OPJ_UINT32 l_tmp;
	opj_image_t *l_image = 00;
	opj_cp_v2_t *l_cp = 00;
	opj_image_comp_t * l_img_comp = 00;
	opj_tcp_v2_t * l_current_tile_param = 00;

	/* preconditions */
	assert(p_j2k != 00);
	assert(p_manager != 00);
	assert(p_header_data != 00);

	l_image = p_j2k->m_private_image;
	l_cp = &(p_j2k->m_cp);

	/* minimum size == 39 - 3 (= minimum component parameter) */
	if (p_header_size < 36) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error with SIZ marker size\n");
		return OPJ_FALSE;
	}

	l_remaining_size = p_header_size - 36;
	l_nb_comp = l_remaining_size / 3;
	l_nb_comp_remain = l_remaining_size % 3;
	if (l_nb_comp_remain != 0){
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error with SIZ marker size\n");
		return OPJ_FALSE;
	}

	l_size = p_header_size + 2;										/* Lsiz */

	opj_read_bytes(p_header_data,&l_tmp ,2);						/* Rsiz (capabilities) */
	p_header_data+=2;
	l_cp->rsiz = (OPJ_RSIZ_CAPABILITIES) l_tmp;
	opj_read_bytes(p_header_data, (OPJ_UINT32*) &l_image->x1, 4);	/* Xsiz */
	p_header_data+=4;
	opj_read_bytes(p_header_data, (OPJ_UINT32*) &l_image->y1, 4);	/* Ysiz */
	p_header_data+=4;
	opj_read_bytes(p_header_data, (OPJ_UINT32*) &l_image->x0, 4);	/* X0siz */
	p_header_data+=4;
	opj_read_bytes(p_header_data, (OPJ_UINT32*) &l_image->y0, 4);	/* Y0siz */
	p_header_data+=4;
	opj_read_bytes(p_header_data, (OPJ_UINT32*) &l_cp->tdx, 4);		/* XTsiz */
	p_header_data+=4;
	opj_read_bytes(p_header_data, (OPJ_UINT32*) &l_cp->tdy, 4);		/* YTsiz */
	p_header_data+=4;
	opj_read_bytes(p_header_data, (OPJ_UINT32*) &l_cp->tx0, 4);		/* XT0siz */
	p_header_data+=4;
	opj_read_bytes(p_header_data, (OPJ_UINT32*) &l_cp->ty0, 4);		/* YT0siz */
	p_header_data+=4;
	opj_read_bytes(p_header_data, (OPJ_UINT32*) &l_tmp, 2);			/* Csiz */
	p_header_data+=2;
	if (l_tmp < 16385)
		l_image->numcomps = (OPJ_UINT16) l_tmp;
	else {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error with SIZ marker: number of component is illegal -> %d\n", l_tmp);
		return OPJ_FALSE;
	}

	if (l_image->numcomps != l_nb_comp) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error with SIZ marker: number of component is not compatible with the remaining number of parameters ( %d vs %d)\n", l_image->numcomps, l_nb_comp);
		return OPJ_FALSE;
	}

#ifdef USE_JPWL
	if (l_cp->correct) {
		/* if JPWL is on, we check whether TX errors have damaged
		  too much the SIZ parameters */
		if (!(l_image->x1 * l_image->y1)) {
			opj_event_msg_v2(p_manager, EVT_ERROR,
				"JPWL: bad image size (%d x %d)\n",
				l_image->x1, l_image->y1);
			if (!JPWL_ASSUME || JPWL_ASSUME) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "JPWL: giving up\n");
				return OPJ_FALSE;
			}
		}

	/* FIXME check previously in the function so why keep this piece of code ? Need by the norm ?
		if (l_image->numcomps != ((len - 38) / 3)) {
			opj_event_msg_v2(p_manager, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
				"JPWL: Csiz is %d => space in SIZ only for %d comps.!!!\n",
				l_image->numcomps, ((len - 38) / 3));
			if (!JPWL_ASSUME) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "JPWL: giving up\n");
				return OPJ_FALSE;
			}
	*/		/* we try to correct */
	/*		opj_event_msg_v2(p_manager, EVT_WARNING, "- trying to adjust this\n");
			if (l_image->numcomps < ((len - 38) / 3)) {
				len = 38 + 3 * l_image->numcomps;
				opj_event_msg_v2(p_manager, EVT_WARNING, "- setting Lsiz to %d => HYPOTHESIS!!!\n",
					len);
			} else {
				l_image->numcomps = ((len - 38) / 3);
				opj_event_msg_v2(p_manager, EVT_WARNING, "- setting Csiz to %d => HYPOTHESIS!!!\n",
					l_image->numcomps);
			}
		}
	*/

		/* update components number in the jpwl_exp_comps filed */
		l_cp->exp_comps = l_image->numcomps;
	}
#endif /* USE_JPWL */

	/* Allocate the resulting image components */
	l_image->comps = (opj_image_comp_t*) opj_calloc(l_image->numcomps, sizeof(opj_image_comp_t));
	if (l_image->comps == 00){
		l_image->numcomps = 0;
		opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory to take in charge SIZ marker\n");
		return OPJ_FALSE;
	}

	memset(l_image->comps,0,l_image->numcomps * sizeof(opj_image_comp_t));
	l_img_comp = l_image->comps;

	/* Read the component information */
	for (i = 0; i < l_image->numcomps; ++i){
		OPJ_UINT32 tmp;
		opj_read_bytes(p_header_data,&tmp,1);	/* Ssiz_i */
		++p_header_data;
		l_img_comp->prec = (tmp & 0x7f) + 1;
		l_img_comp->sgnd = tmp >> 7;
		opj_read_bytes(p_header_data,&tmp,1);	/* XRsiz_i */
		++p_header_data;
		l_img_comp->dx = (OPJ_INT32)tmp; /* should be between 1 and 255 */
		opj_read_bytes(p_header_data,&tmp,1);	/* YRsiz_i */
		++p_header_data;
		l_img_comp->dy = (OPJ_INT32)tmp; /* should be between 1 and 255 */

#ifdef USE_JPWL
		if (l_cp->correct) {
		/* if JPWL is on, we check whether TX errors have damaged
			too much the SIZ parameters, again */
			if (!(l_image->comps[i].dx * l_image->comps[i].dy)) {
				opj_event_msg_v2(p_manager, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
					"JPWL: bad XRsiz_%d/YRsiz_%d (%d x %d)\n",
					i, i, l_image->comps[i].dx, l_image->comps[i].dy);
				if (!JPWL_ASSUME) {
					opj_event_msg_v2(p_manager, EVT_ERROR, "JPWL: giving up\n");
					return OPJ_FALSE;
				}
				/* we try to correct */
				opj_event_msg_v2(p_manager, EVT_WARNING, "- trying to adjust them\n");
				if (!l_image->comps[i].dx) {
					l_image->comps[i].dx = 1;
					opj_event_msg_v2(p_manager, EVT_WARNING, "- setting XRsiz_%d to %d => HYPOTHESIS!!!\n",
						i, l_image->comps[i].dx);
				}
				if (!l_image->comps[i].dy) {
					l_image->comps[i].dy = 1;
					opj_event_msg_v2(p_manager, EVT_WARNING, "- setting YRsiz_%d to %d => HYPOTHESIS!!!\n",
						i, l_image->comps[i].dy);
				}
			}
		}
#endif /* USE_JPWL */
		l_img_comp->resno_decoded = 0;								/* number of resolution decoded */
		l_img_comp->factor = l_cp->m_specific_param.m_dec.m_reduce; /* reducing factor per component */
		++l_img_comp;
	}

	/* Compute the number of tiles */
	l_cp->tw = int_ceildiv(l_image->x1 - l_cp->tx0, l_cp->tdx);
	l_cp->th = int_ceildiv(l_image->y1 - l_cp->ty0, l_cp->tdy);
	l_nb_tiles = l_cp->tw * l_cp->th;

	/* Define the tiles which will be decoded */
	if (p_j2k->m_specific_param.m_decoder.m_discard_tiles) {
		p_j2k->m_specific_param.m_decoder.m_start_tile_x = (p_j2k->m_specific_param.m_decoder.m_start_tile_x - l_cp->tx0) / l_cp->tdx;
		p_j2k->m_specific_param.m_decoder.m_start_tile_y = (p_j2k->m_specific_param.m_decoder.m_start_tile_y - l_cp->ty0) / l_cp->tdy;
		p_j2k->m_specific_param.m_decoder.m_end_tile_x = int_ceildiv((p_j2k->m_specific_param.m_decoder.m_end_tile_x - l_cp->tx0), l_cp->tdx);
		p_j2k->m_specific_param.m_decoder.m_end_tile_y = int_ceildiv((p_j2k->m_specific_param.m_decoder.m_end_tile_y - l_cp->ty0), l_cp->tdy);
	}
	else {
		p_j2k->m_specific_param.m_decoder.m_start_tile_x = 0;
		p_j2k->m_specific_param.m_decoder.m_start_tile_y = 0;
		p_j2k->m_specific_param.m_decoder.m_end_tile_x = l_cp->tw;
		p_j2k->m_specific_param.m_decoder.m_end_tile_y = l_cp->th;
	}

#ifdef USE_JPWL
	if (l_cp->correct) {
		/* if JPWL is on, we check whether TX errors have damaged
		  too much the SIZ parameters */
		if ((l_cp->tw < 1) || (l_cp->th < 1) || (l_cp->tw > l_cp->max_tiles) || (l_cp->th > l_cp->max_tiles)) {
			opj_event_msg_v2(p_manager, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
				"JPWL: bad number of tiles (%d x %d)\n",
				l_cp->tw, l_cp->th);
			if (!JPWL_ASSUME) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "JPWL: giving up\n");
				return OPJ_FALSE;
			}
			/* we try to correct */
			opj_event_msg_v2(p_manager, EVT_WARNING, "- trying to adjust them\n");
			if (l_cp->tw < 1) {
				l_cp->tw= 1;
				opj_event_msg_v2(p_manager, EVT_WARNING, "- setting %d tiles in x => HYPOTHESIS!!!\n",
						l_cp->tw);
			}
			if (l_cp->tw > l_cp->max_tiles) {
				l_cp->tw= 1;
				opj_event_msg_v2(p_manager, EVT_WARNING, "- too large x, increase expectance of %d\n"
					"- setting %d tiles in x => HYPOTHESIS!!!\n",
					l_cp->max_tiles, l_cp->tw);
			}
			if (l_cp->th < 1) {
				l_cp->th= 1;
				opj_event_msg_v2(p_manager, EVT_WARNING, "- setting %d tiles in y => HYPOTHESIS!!!\n",
						l_cp->th);
			}
			if (l_cp->th > l_cp->max_tiles) {
				l_cp->th= 1;
				opj_event_msg_v2(p_manager, EVT_WARNING, "- too large y, increase expectance of %d to continue\n",
					"- setting %d tiles in y => HYPOTHESIS!!!\n",
					l_cp->max_tiles, l_cp->th);
			}
		}
	}
#endif /* USE_JPWL */

	/* memory allocations */
	l_cp->tcps = (opj_tcp_v2_t*) opj_calloc(l_nb_tiles, sizeof(opj_tcp_v2_t));
	if (l_cp->tcps == 00) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory to take in charge SIZ marker\n");
		return OPJ_FALSE;
	}
	memset(l_cp->tcps,0,l_nb_tiles*sizeof(opj_tcp_t));

#ifdef USE_JPWL
	if (l_cp->correct) {
		if (!l_cp->tcps) {
			opj_event_msg_v2(p_manager, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
				"JPWL: could not alloc tcps field of cp\n");
			if (!JPWL_ASSUME || JPWL_ASSUME) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "JPWL: giving up\n");
				return OPJ_FALSE;
			}
		}
	}
#endif /* USE_JPWL */

	p_j2k->m_specific_param.m_decoder.m_default_tcp->tccps =
			(opj_tccp_t*) opj_calloc(l_image->numcomps, sizeof(opj_tccp_t));
	if(p_j2k->m_specific_param.m_decoder.m_default_tcp->tccps  == 00) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory to take in charge SIZ marker\n");
		return OPJ_FALSE;
	}
	memset(p_j2k->m_specific_param.m_decoder.m_default_tcp->tccps ,0,l_image->numcomps*sizeof(opj_tccp_t));

	p_j2k->m_specific_param.m_decoder.m_default_tcp->m_mct_records =
			(opj_mct_data_t*)opj_malloc(J2K_MCT_DEFAULT_NB_RECORDS * sizeof(opj_mct_data_t));

	if (! p_j2k->m_specific_param.m_decoder.m_default_tcp->m_mct_records) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory to take in charge SIZ marker\n");
		return OPJ_FALSE;
	}
	memset(p_j2k->m_specific_param.m_decoder.m_default_tcp->m_mct_records,0,J2K_MCT_DEFAULT_NB_RECORDS * sizeof(opj_mct_data_t));
	p_j2k->m_specific_param.m_decoder.m_default_tcp->m_nb_max_mct_records = J2K_MCT_DEFAULT_NB_RECORDS;

	p_j2k->m_specific_param.m_decoder.m_default_tcp->m_mcc_records =
			(opj_simple_mcc_decorrelation_data_t*)
			opj_malloc(J2K_MCC_DEFAULT_NB_RECORDS * sizeof(opj_simple_mcc_decorrelation_data_t));

	if (! p_j2k->m_specific_param.m_decoder.m_default_tcp->m_mcc_records) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory to take in charge SIZ marker\n");
		return OPJ_FALSE;
	}
	memset(p_j2k->m_specific_param.m_decoder.m_default_tcp->m_mcc_records,0,J2K_MCC_DEFAULT_NB_RECORDS * sizeof(opj_simple_mcc_decorrelation_data_t));
	p_j2k->m_specific_param.m_decoder.m_default_tcp->m_nb_max_mcc_records = J2K_MCC_DEFAULT_NB_RECORDS;

	/* set up default dc level shift */
	for (i=0;i<l_image->numcomps;++i) {
		if (! l_image->comps[i].sgnd) {
			p_j2k->m_specific_param.m_decoder.m_default_tcp->tccps[i].m_dc_level_shift = 1 << (l_image->comps[i].prec - 1);
		}
	}

	l_current_tile_param = l_cp->tcps;
	for	(i = 0; i < l_nb_tiles; ++i) {
		l_current_tile_param->tccps = (opj_tccp_t*) opj_malloc(l_image->numcomps * sizeof(opj_tccp_t));
		if (l_current_tile_param->tccps == 00) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory to take in charge SIZ marker\n");
			return OPJ_FALSE;
		}
		memset(l_current_tile_param->tccps,0,l_image->numcomps * sizeof(opj_tccp_t));

		++l_current_tile_param;
	}

	p_j2k->m_specific_param.m_decoder.m_state =  J2K_STATE_MH; /* FIXME J2K_DEC_STATE_MH; */
	opj_image_comp_header_update(l_image,l_cp);

	return OPJ_TRUE;
}



static void j2k_write_com(opj_j2k_t *j2k) {
	unsigned int i;
	int lenp, len;

	if(j2k->cp->comment) {
		opj_cio_t *cio = j2k->cio;
		char *comment = j2k->cp->comment;

		cio_write(cio, J2K_MS_COM, 2);
		lenp = cio_tell(cio);
		cio_skip(cio, 2);
		cio_write(cio, 1, 2);		/* General use (IS 8859-15:1999 (Latin) values) */
		for (i = 0; i < strlen(comment); i++) {
			cio_write(cio, comment[i], 1);
		}
		len = cio_tell(cio) - lenp;
		cio_seek(cio, lenp);
		cio_write(cio, len, 2);
		cio_seek(cio, lenp + len);

		
		if(j2k->cstr_info)
		  j2k_add_mhmarker(j2k->cstr_info, J2K_MS_COM, lenp, len);

	}
}

static void j2k_read_com(opj_j2k_t *j2k) {
	int len;
	
	opj_cio_t *cio = j2k->cio;

	len = cio_read(cio, 2);
	cio_skip(cio, len - 2);  
}
/**
 * Reads a COM marker (comments)
 * @param	p_header_data	the data contained in the COM box.
 * @param	jp2				the jpeg2000 file codec.
 * @param	p_header_size	the size of the data contained in the COM marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_com_v2 (
					opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager
					)
{
	/* preconditions */
	assert(p_j2k != 00);
	assert(p_manager != 00);
	assert(p_header_data != 00);

	return OPJ_TRUE;
}

static void j2k_write_cox(opj_j2k_t *j2k, int compno) {
	int i;

	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = &cp->tcps[j2k->curtileno];
	opj_tccp_t *tccp = &tcp->tccps[compno];
	opj_cio_t *cio = j2k->cio;
	
	cio_write(cio, tccp->numresolutions - 1, 1);	/* SPcox (D) */
	cio_write(cio, tccp->cblkw - 2, 1);				/* SPcox (E) */
	cio_write(cio, tccp->cblkh - 2, 1);				/* SPcox (F) */
	cio_write(cio, tccp->cblksty, 1);				/* SPcox (G) */
	cio_write(cio, tccp->qmfbid, 1);				/* SPcox (H) */
	
	if (tccp->csty & J2K_CCP_CSTY_PRT) {
		for (i = 0; i < tccp->numresolutions; i++) {
			cio_write(cio, tccp->prcw[i] + (tccp->prch[i] << 4), 1);	/* SPcox (I_i) */
		}
	}
}

static void j2k_read_cox(opj_j2k_t *j2k, int compno) {
	int i;

	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = j2k->state == J2K_STATE_TPH ? &cp->tcps[j2k->curtileno] : j2k->default_tcp;
	opj_tccp_t *tccp = &tcp->tccps[compno];
	opj_cio_t *cio = j2k->cio;

	tccp->numresolutions = cio_read(cio, 1) + 1;	/* SPcox (D) */

	/* If user wants to remove more resolutions than the codestream contains, return error*/
	if (cp->reduce >= tccp->numresolutions) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Error decoding component %d.\nThe number of resolutions to remove is higher than the number "
					"of resolutions of this component\nModify the cp_reduce parameter.\n\n", compno);
		j2k->state |= J2K_STATE_ERR;
	}

	tccp->cblkw = cio_read(cio, 1) + 2;	/* SPcox (E) */
	tccp->cblkh = cio_read(cio, 1) + 2;	/* SPcox (F) */
	tccp->cblksty = cio_read(cio, 1);	/* SPcox (G) */
	tccp->qmfbid = cio_read(cio, 1);	/* SPcox (H) */
	if (tccp->csty & J2K_CP_CSTY_PRT) {
		for (i = 0; i < tccp->numresolutions; i++) {
			int tmp = cio_read(cio, 1);	/* SPcox (I_i) */
			tccp->prcw[i] = tmp & 0xf;
			tccp->prch[i] = tmp >> 4;
		}
	}

	/* INDEX >> */
	if(j2k->cstr_info && compno == 0) {
		for (i = 0; i < tccp->numresolutions; i++) {
			if (tccp->csty & J2K_CP_CSTY_PRT) {
				j2k->cstr_info->tile[j2k->curtileno].pdx[i] = tccp->prcw[i];
				j2k->cstr_info->tile[j2k->curtileno].pdy[i] = tccp->prch[i];
			}
			else {
				j2k->cstr_info->tile[j2k->curtileno].pdx[i] = 15;
				j2k->cstr_info->tile[j2k->curtileno].pdx[i] = 15;
			}
		}
	}
	/* << INDEX */
}

static void j2k_write_cod(opj_j2k_t *j2k) {
	opj_cp_t *cp = NULL;
	opj_tcp_t *tcp = NULL;
	int lenp, len;

	opj_cio_t *cio = j2k->cio;
	
	cio_write(cio, J2K_MS_COD, 2);	/* COD */
	
	lenp = cio_tell(cio);
	cio_skip(cio, 2);
	
	cp = j2k->cp;
	tcp = &cp->tcps[j2k->curtileno];

	cio_write(cio, tcp->csty, 1);		/* Scod */
	cio_write(cio, tcp->prg, 1);		/* SGcod (A) */
	cio_write(cio, tcp->numlayers, 2);	/* SGcod (B) */
	cio_write(cio, tcp->mct, 1);		/* SGcod (C) */
	
	j2k_write_cox(j2k, 0);
	len = cio_tell(cio) - lenp;
	cio_seek(cio, lenp);
	cio_write(cio, len, 2);		/* Lcod */
	cio_seek(cio, lenp + len);

	if(j2k->cstr_info)
	  j2k_add_mhmarker(j2k->cstr_info, J2K_MS_COD, lenp, len);

}

static void j2k_read_cod(opj_j2k_t *j2k) {
	int len, i, pos;
	
	opj_cio_t *cio = j2k->cio;
	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = j2k->state == J2K_STATE_TPH ? &cp->tcps[j2k->curtileno] : j2k->default_tcp;
	opj_image_t *image = j2k->image;
	
	len = cio_read(cio, 2);				/* Lcod */
	tcp->csty = cio_read(cio, 1);		/* Scod */
	tcp->prg = (OPJ_PROG_ORDER)cio_read(cio, 1);		/* SGcod (A) */
	tcp->numlayers = cio_read(cio, 2);	/* SGcod (B) */
	tcp->mct = cio_read(cio, 1);		/* SGcod (C) */
	
	pos = cio_tell(cio);
	for (i = 0; i < image->numcomps; i++) {
		tcp->tccps[i].csty = tcp->csty & J2K_CP_CSTY_PRT;
		cio_seek(cio, pos);
		j2k_read_cox(j2k, i);
	}

	/* Index */
	if (j2k->cstr_info) {
		opj_codestream_info_t *cstr_info = j2k->cstr_info;
		cstr_info->prog = tcp->prg;
		cstr_info->numlayers = tcp->numlayers;
		cstr_info->numdecompos = (int*) opj_malloc(image->numcomps * sizeof(int));
		for (i = 0; i < image->numcomps; i++) {
			cstr_info->numdecompos[i] = tcp->tccps[i].numresolutions - 1;
		}
	}
}

/**
 * Reads a COD marker (Coding Styke defaults)
 * @param	p_header_data	the data contained in the COD box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the COD marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_cod_v2 (
					opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager
					)
{
	/* loop */
	OPJ_UINT32 i;
	OPJ_UINT32 l_tmp;
	opj_cp_v2_t *l_cp = 00;
	opj_tcp_v2_t *l_tcp = 00;
	opj_image_t *l_image = 00;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	l_image = p_j2k->m_private_image;
	l_cp = &(p_j2k->m_cp);

	/* If we are in the first tile-part header of the current tile */
	l_tcp = (p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH) ?
				&l_cp->tcps[p_j2k->m_current_tile_number] :
				p_j2k->m_specific_param.m_decoder.m_default_tcp;

	/* Make sure room is sufficient */
	if (p_header_size < 5) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading COD marker\n");
		return OPJ_FALSE;
	}

	opj_read_bytes(p_header_data,&l_tcp->csty,1);		/* Scod */
	++p_header_data;
	opj_read_bytes(p_header_data,&l_tmp,1);				/* SGcod (A) */
	++p_header_data;
	l_tcp->prg = (OPJ_PROG_ORDER) l_tmp;
	opj_read_bytes(p_header_data,&l_tcp->numlayers,2);	/* SGcod (B) */
	p_header_data+=2;

	/* If user didn't set a number layer to decode take the max specify in the codestream. */
	if	(l_cp->m_specific_param.m_dec.m_layer) {
		l_tcp->num_layers_to_decode = l_cp->m_specific_param.m_dec.m_layer;
	}
	else {
		l_tcp->num_layers_to_decode = l_tcp->numlayers;
	}

	opj_read_bytes(p_header_data,&l_tcp->mct,1);		/* SGcod (C) */
	++p_header_data;

	p_header_size -= 5;
	for	(i = 0; i < l_image->numcomps; ++i) {
		l_tcp->tccps[i].csty = l_tcp->csty & J2K_CCP_CSTY_PRT;
	}

	if (! j2k_read_SPCod_SPCoc(p_j2k,0,p_header_data,&p_header_size,p_manager)) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading COD marker\n");
		return OPJ_FALSE;
	}

	if (p_header_size != 0) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading COD marker\n");
		return OPJ_FALSE;
	}

	/* Apply the coding style to other components of the current tile or the m_default_tcp*/
	j2k_copy_tile_component_parameters(p_j2k);

	/* Index */
#ifdef WIP_REMOVE_MSD
	if (p_j2k->cstr_info) {
		/*opj_codestream_info_t *l_cstr_info = p_j2k->cstr_info;*/
		p_j2k->cstr_info->prog = l_tcp->prg;
		p_j2k->cstr_info->numlayers = l_tcp->numlayers;
		p_j2k->cstr_info->numdecompos = (OPJ_INT32*) opj_malloc(l_image->numcomps * sizeof(OPJ_UINT32));
		for	(i = 0; i < l_image->numcomps; ++i) {
			p_j2k->cstr_info->numdecompos[i] = l_tcp->tccps[i].numresolutions - 1;
		}
	}
#endif

	return OPJ_TRUE;
}

static void j2k_write_coc(opj_j2k_t *j2k, int compno) {
	int lenp, len;

	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = &cp->tcps[j2k->curtileno];
	opj_image_t *image = j2k->image;
	opj_cio_t *cio = j2k->cio;
	
	cio_write(cio, J2K_MS_COC, 2);	/* COC */
	lenp = cio_tell(cio);
	cio_skip(cio, 2);
	cio_write(cio, compno, image->numcomps <= 256 ? 1 : 2);	/* Ccoc */
	cio_write(cio, tcp->tccps[compno].csty, 1);	/* Scoc */
	j2k_write_cox(j2k, compno);
	len = cio_tell(cio) - lenp;
	cio_seek(cio, lenp);
	cio_write(cio, len, 2);			/* Lcoc */
	cio_seek(cio, lenp + len);
}

static void j2k_read_coc(opj_j2k_t *j2k) {
	int len, compno;

	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = j2k->state == J2K_STATE_TPH ? &cp->tcps[j2k->curtileno] : j2k->default_tcp;
	opj_image_t *image = j2k->image;
	opj_cio_t *cio = j2k->cio;
	
	len = cio_read(cio, 2);		/* Lcoc */
	compno = cio_read(cio, image->numcomps <= 256 ? 1 : 2);	/* Ccoc */
	tcp->tccps[compno].csty = cio_read(cio, 1);	/* Scoc */
	j2k_read_cox(j2k, compno);
}

/**
 * Reads a COC marker (Coding Style Component)
 * @param	p_header_data	the data contained in the COC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the COC marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_coc_v2 (
					opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager
					)
{
	opj_cp_v2_t *l_cp = NULL;
	opj_tcp_v2_t *l_tcp = NULL;
	opj_image_t *l_image = NULL;
	OPJ_UINT32 l_comp_room;
	OPJ_UINT32 l_comp_no;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	l_cp = &(p_j2k->m_cp);
	l_tcp = (p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH ) ? /*FIXME J2K_DEC_STATE_TPH*/
				&l_cp->tcps[p_j2k->m_current_tile_number] :
				p_j2k->m_specific_param.m_decoder.m_default_tcp;
	l_image = p_j2k->m_private_image;

	l_comp_room = l_image->numcomps <= 256 ? 1 : 2;

	/* make sure room is sufficient*/
	if (p_header_size < l_comp_room + 1) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading COC marker\n");
		return OPJ_FALSE;
	}
	p_header_size -= l_comp_room + 1;

	opj_read_bytes(p_header_data,&l_comp_no,l_comp_room);			/* Ccoc */
	p_header_data += l_comp_room;
	if (l_comp_no >= l_image->numcomps) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading COC marker (bad number of components)\n");
		return OPJ_FALSE;
	}

	opj_read_bytes(p_header_data,&l_tcp->tccps[l_comp_no].csty,1);			/* Scoc */
	++p_header_data ;

	if (! j2k_read_SPCod_SPCoc(p_j2k,l_comp_no,p_header_data,&p_header_size,p_manager)) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading COC marker\n");
		return OPJ_FALSE;
	}

	if (p_header_size != 0) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading COC marker\n");
		return OPJ_FALSE;
	}
	return OPJ_TRUE;
}

static void j2k_write_qcx(opj_j2k_t *j2k, int compno) {
	int bandno, numbands;
	int expn, mant;
	
	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = &cp->tcps[j2k->curtileno];
	opj_tccp_t *tccp = &tcp->tccps[compno];
	opj_cio_t *cio = j2k->cio;
	
	cio_write(cio, tccp->qntsty + (tccp->numgbits << 5), 1);	/* Sqcx */
	numbands = tccp->qntsty == J2K_CCP_QNTSTY_SIQNT ? 1 : tccp->numresolutions * 3 - 2;
	
	for (bandno = 0; bandno < numbands; bandno++) {
		expn = tccp->stepsizes[bandno].expn;
		mant = tccp->stepsizes[bandno].mant;
		
		if (tccp->qntsty == J2K_CCP_QNTSTY_NOQNT) {
			cio_write(cio, expn << 3, 1);	/* SPqcx_i */
		} else {
			cio_write(cio, (expn << 11) + mant, 2);	/* SPqcx_i */
		}
	}
}

static void j2k_read_qcx(opj_j2k_t *j2k, int compno, int len) {
	int tmp;
	int bandno, numbands;

	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = j2k->state == J2K_STATE_TPH ? &cp->tcps[j2k->curtileno] : j2k->default_tcp;
	opj_tccp_t *tccp = &tcp->tccps[compno];
	opj_cio_t *cio = j2k->cio;

	tmp = cio_read(cio, 1);		/* Sqcx */
	tccp->qntsty = tmp & 0x1f;
	tccp->numgbits = tmp >> 5;
	numbands = (tccp->qntsty == J2K_CCP_QNTSTY_SIQNT) ? 
		1 : ((tccp->qntsty == J2K_CCP_QNTSTY_NOQNT) ? len - 1 : (len - 1) / 2);

#ifdef USE_JPWL
	if (j2k->cp->correct) {

		/* if JPWL is on, we check whether there are too many subbands */
		if ((numbands < 0) || (numbands >= J2K_MAXBANDS)) {
			opj_event_msg(j2k->cinfo, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
				"JPWL: bad number of subbands in Sqcx (%d)\n",
				numbands);
			if (!JPWL_ASSUME) {
				opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
				return;
			}
			/* we try to correct */
			numbands = 1;
			opj_event_msg(j2k->cinfo, EVT_WARNING, "- trying to adjust them\n"
				"- setting number of bands to %d => HYPOTHESIS!!!\n",
				numbands);
		};

	};

#else
	/* We check whether there are too many subbands */
	if ((numbands < 0) || (numbands >= J2K_MAXBANDS)) {
		opj_event_msg(j2k->cinfo, EVT_WARNING ,
					"bad number of subbands in Sqcx (%d) regarding to J2K_MAXBANDS (%d) \n"
				    "- limiting number of bands to J2K_MAXBANDS and try to move to the next markers\n", numbands, J2K_MAXBANDS);
	}

#endif /* USE_JPWL */

	for (bandno = 0; bandno < numbands; bandno++) {
		int expn, mant;
		if (tccp->qntsty == J2K_CCP_QNTSTY_NOQNT) {
			expn = cio_read(cio, 1) >> 3;	/* SPqcx_i */
			mant = 0;
		} else {
			tmp = cio_read(cio, 2);	/* SPqcx_i */
			expn = tmp >> 11;
			mant = tmp & 0x7ff;
		}
		if (bandno < J2K_MAXBANDS){
			tccp->stepsizes[bandno].expn = expn;
			tccp->stepsizes[bandno].mant = mant;
		}
	}
	
	/* Add Antonin : if scalar_derived -> compute other stepsizes */
	if (tccp->qntsty == J2K_CCP_QNTSTY_SIQNT) {
		for (bandno = 1; bandno < J2K_MAXBANDS; bandno++) {
			tccp->stepsizes[bandno].expn = 
				((tccp->stepsizes[0].expn) - ((bandno - 1) / 3) > 0) ? 
					(tccp->stepsizes[0].expn) - ((bandno - 1) / 3) : 0;
			tccp->stepsizes[bandno].mant = tccp->stepsizes[0].mant;
		}
	}
	/* ddA */
}

static void j2k_write_qcd(opj_j2k_t *j2k) {
	int lenp, len;

	opj_cio_t *cio = j2k->cio;
	
	cio_write(cio, J2K_MS_QCD, 2);	/* QCD */
	lenp = cio_tell(cio);
	cio_skip(cio, 2);
	j2k_write_qcx(j2k, 0);
	len = cio_tell(cio) - lenp;
	cio_seek(cio, lenp);
	cio_write(cio, len, 2);			/* Lqcd */
	cio_seek(cio, lenp + len);

	if(j2k->cstr_info)
	  j2k_add_mhmarker(j2k->cstr_info, J2K_MS_QCD, lenp, len);
}

static void j2k_read_qcd(opj_j2k_t *j2k) {
	int len, i, pos;

	opj_cio_t *cio = j2k->cio;
	opj_image_t *image = j2k->image;
	
	len = cio_read(cio, 2);		/* Lqcd */
	pos = cio_tell(cio);
	for (i = 0; i < image->numcomps; i++) {
		cio_seek(cio, pos);
		j2k_read_qcx(j2k, i, len - 2);
	}
}

/**
 * Reads a QCD marker (Quantization defaults)
 * @param	p_header_data	the data contained in the QCD box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the QCD marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_qcd_v2 (
				    opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager
					)
{
	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	if (! j2k_read_SQcd_SQcc(p_j2k,0,p_header_data,&p_header_size,p_manager)) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading QCD marker\n");
		return OPJ_FALSE;
	}

	if (p_header_size != 0) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading QCD marker\n");
		return OPJ_FALSE;
	}

	/* Apply the quantization parameters to other components of the current tile or the m_default_tcp */
	j2k_copy_tile_quantization_parameters(p_j2k);

	return OPJ_TRUE;
}

static void j2k_write_qcc(opj_j2k_t *j2k, int compno) {
	int lenp, len;

	opj_cio_t *cio = j2k->cio;
	
	cio_write(cio, J2K_MS_QCC, 2);	/* QCC */
	lenp = cio_tell(cio);
	cio_skip(cio, 2);
	cio_write(cio, compno, j2k->image->numcomps <= 256 ? 1 : 2);	/* Cqcc */
	j2k_write_qcx(j2k, compno);
	len = cio_tell(cio) - lenp;
	cio_seek(cio, lenp);
	cio_write(cio, len, 2);			/* Lqcc */
	cio_seek(cio, lenp + len);
}

static void j2k_read_qcc(opj_j2k_t *j2k) {
	int len, compno;
	int numcomp = j2k->image->numcomps;
	opj_cio_t *cio = j2k->cio;

	len = cio_read(cio, 2);	/* Lqcc */
	compno = cio_read(cio, numcomp <= 256 ? 1 : 2);	/* Cqcc */

#ifdef USE_JPWL
	if (j2k->cp->correct) {

		static int backup_compno = 0;

		/* compno is negative or larger than the number of components!!! */
		if ((compno < 0) || (compno >= numcomp)) {
			opj_event_msg(j2k->cinfo, EVT_ERROR,
				"JPWL: bad component number in QCC (%d out of a maximum of %d)\n",
				compno, numcomp);
			if (!JPWL_ASSUME) {
				opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
				return;
			}
			/* we try to correct */
			compno = backup_compno % numcomp;
			opj_event_msg(j2k->cinfo, EVT_WARNING, "- trying to adjust this\n"
				"- setting component number to %d\n",
				compno);
		}

		/* keep your private count of tiles */
		backup_compno++;
	};
#endif /* USE_JPWL */

	j2k_read_qcx(j2k, compno, len - 2 - (numcomp <= 256 ? 1 : 2));
}

/**
 * Reads a QCC marker (Quantization component)
 * @param	p_header_data	the data contained in the QCC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the QCC marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_qcc_v2(	opj_j2k_v2_t *p_j2k,
							OPJ_BYTE * p_header_data,
							OPJ_UINT32 p_header_size,
							struct opj_event_mgr * p_manager)
{
	OPJ_UINT32 l_num_comp,l_comp_no;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	l_num_comp = p_j2k->m_private_image->numcomps;

	if (l_num_comp <= 256) {
		if (p_header_size < 1) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading QCC marker\n");
			return OPJ_FALSE;
		}
		opj_read_bytes(p_header_data,&l_comp_no,1);
		++p_header_data;
		--p_header_size;
	}
	else {
		if (p_header_size < 2) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading QCC marker\n");
			return OPJ_FALSE;
		}
		opj_read_bytes(p_header_data,&l_comp_no,2);
		p_header_data+=2;
		p_header_size-=2;
	}

#ifdef USE_JPWL
	if (p_j2k->m_cp.correct) {

		static OPJ_UINT32 backup_compno = 0;

		/* compno is negative or larger than the number of components!!! */
		if ((l_comp_no < 0) || (l_comp_no >= l_num_comp)) {
			opj_event_msg_v2(p_manager, EVT_ERROR,
				"JPWL: bad component number in QCC (%d out of a maximum of %d)\n",
				l_comp_no, l_num_comp);
			if (!JPWL_ASSUME) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "JPWL: giving up\n");
				return OPJ_FALSE;
			}
			/* we try to correct */
			l_comp_no = backup_compno % l_num_comp;
			opj_event_msg_v2(p_manager, EVT_WARNING, "- trying to adjust this\n"
				"- setting component number to %d\n",
				l_comp_no);
		}

		/* keep your private count of tiles */
		backup_compno++;
	};
#endif /* USE_JPWL */

	if (! j2k_read_SQcd_SQcc(p_j2k,l_comp_no,p_header_data,&p_header_size,p_manager)) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading QCC marker\n");
		return OPJ_FALSE;
	}

	if (p_header_size != 0) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading QCC marker\n");
		return OPJ_FALSE;
	}

	return OPJ_TRUE;
}


static void j2k_write_poc(opj_j2k_t *j2k) {
	int len, numpchgs, i;

	int numcomps = j2k->image->numcomps;
	
	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = &cp->tcps[j2k->curtileno];
	opj_tccp_t *tccp = &tcp->tccps[0];
	opj_cio_t *cio = j2k->cio;

	numpchgs = 1 + tcp->numpocs;
	cio_write(cio, J2K_MS_POC, 2);	/* POC  */
	len = 2 + (5 + 2 * (numcomps <= 256 ? 1 : 2)) * numpchgs;
	cio_write(cio, len, 2);		/* Lpoc */
	for (i = 0; i < numpchgs; i++) {
		opj_poc_t *poc = &tcp->pocs[i];
		cio_write(cio, poc->resno0, 1);	/* RSpoc_i */
		cio_write(cio, poc->compno0, (numcomps <= 256 ? 1 : 2));	/* CSpoc_i */
		cio_write(cio, poc->layno1, 2);	/* LYEpoc_i */
		poc->layno1 = int_min(poc->layno1, tcp->numlayers);
		cio_write(cio, poc->resno1, 1);	/* REpoc_i */
		poc->resno1 = int_min(poc->resno1, tccp->numresolutions);
		cio_write(cio, poc->compno1, (numcomps <= 256 ? 1 : 2));	/* CEpoc_i */
		poc->compno1 = int_min(poc->compno1, numcomps);
		cio_write(cio, poc->prg, 1);	/* Ppoc_i */
	}
}

static void j2k_read_poc(opj_j2k_t *j2k) {
	int len, numpchgs, i, old_poc;

	int numcomps = j2k->image->numcomps;
	
	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = j2k->state == J2K_STATE_TPH ? &cp->tcps[j2k->curtileno] : j2k->default_tcp;
	opj_cio_t *cio = j2k->cio;
	
	old_poc = tcp->POC ? tcp->numpocs + 1 : 0;
	tcp->POC = 1;
	len = cio_read(cio, 2);		/* Lpoc */
	numpchgs = (len - 2) / (5 + 2 * (numcomps <= 256 ? 1 : 2));
	
	for (i = old_poc; i < numpchgs + old_poc; i++) {
		opj_poc_t *poc;
		poc = &tcp->pocs[i];
		poc->resno0 = cio_read(cio, 1);	/* RSpoc_i */
		poc->compno0 = cio_read(cio, numcomps <= 256 ? 1 : 2);	/* CSpoc_i */
		poc->layno1 = cio_read(cio, 2);    /* LYEpoc_i */
		poc->resno1 = cio_read(cio, 1);    /* REpoc_i */
		poc->compno1 = int_min(
			cio_read(cio, numcomps <= 256 ? 1 : 2), (unsigned int) numcomps);	/* CEpoc_i */
		poc->prg = (OPJ_PROG_ORDER)cio_read(cio, 1);	/* Ppoc_i */
	}
	
	tcp->numpocs = numpchgs + old_poc - 1;
}

/**
 * Reads a POC marker (Progression Order Change)
 *
 * @param	p_header_data	the data contained in the POC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the POC marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_poc_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager)
{
	OPJ_UINT32 i, l_nb_comp, l_tmp;
	opj_image_t * l_image = 00;
	OPJ_UINT32 l_old_poc_nb, l_current_poc_nb, l_current_poc_remaining;
	OPJ_UINT32 l_chunk_size, l_comp_room;

	opj_cp_v2_t *l_cp = 00;
	opj_tcp_v2_t *l_tcp = 00;
	opj_poc_t *l_current_poc = 00;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	l_image = p_j2k->m_private_image;
	l_nb_comp = l_image->numcomps;
	if (l_nb_comp <= 256) {
		l_comp_room = 1;
	}
	else {
		l_comp_room = 2;
	}
	l_chunk_size = 5 + 2 * l_comp_room;
	l_current_poc_nb = p_header_size / l_chunk_size;
	l_current_poc_remaining = p_header_size % l_chunk_size;

	if ((l_current_poc_nb <= 0) || (l_current_poc_remaining != 0)) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading POC marker\n");
		return OPJ_FALSE;
	}

	l_cp = &(p_j2k->m_cp);
	l_tcp = (p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH) ?
				&l_cp->tcps[p_j2k->m_current_tile_number] :
				p_j2k->m_specific_param.m_decoder.m_default_tcp;
	l_old_poc_nb = l_tcp->POC ? l_tcp->numpocs + 1 : 0;
	l_current_poc_nb += l_old_poc_nb;

	assert(l_current_poc_nb < 32);

	/* now poc is in use.*/
	l_tcp->POC = 1;

	l_current_poc = &l_tcp->pocs[l_old_poc_nb];
	for	(i = l_old_poc_nb; i < l_current_poc_nb; ++i) {
		opj_read_bytes(p_header_data,&(l_current_poc->resno0),1);				/* RSpoc_i */
		++p_header_data;
		opj_read_bytes(p_header_data,&(l_current_poc->compno0),l_comp_room);	/* CSpoc_i */
		p_header_data+=l_comp_room;
		opj_read_bytes(p_header_data,&(l_current_poc->layno1),2);				/* LYEpoc_i */
		p_header_data+=2;
		opj_read_bytes(p_header_data,&(l_current_poc->resno1),1);				/* REpoc_i */
		++p_header_data;
		opj_read_bytes(p_header_data,&(l_current_poc->compno1),l_comp_room);	/* CEpoc_i */
		p_header_data+=l_comp_room;
		opj_read_bytes(p_header_data,&l_tmp,1);									/* Ppoc_i */
		++p_header_data;
		l_current_poc->prg = (OPJ_PROG_ORDER) l_tmp;
		/* make sure comp is in acceptable bounds */
		l_current_poc->compno1 = uint_min(l_current_poc->compno1, l_nb_comp);
		++l_current_poc;
	}

	l_tcp->numpocs = l_current_poc_nb - 1;
	return OPJ_TRUE;
}

static void j2k_read_crg(opj_j2k_t *j2k) {
	int len, i, Xcrg_i, Ycrg_i;
	
	opj_cio_t *cio = j2k->cio;
	int numcomps = j2k->image->numcomps;
	
	len = cio_read(cio, 2);			/* Lcrg */
	for (i = 0; i < numcomps; i++) {
		Xcrg_i = cio_read(cio, 2);	/* Xcrg_i */
		Ycrg_i = cio_read(cio, 2);	/* Ycrg_i */
	}
}

/**
 * Reads a CRG marker (Component registration)
 *
 * @param	p_header_data	the data contained in the TLM box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the TLM marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_crg_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					)
{
	OPJ_UINT32 l_nb_comp;
	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	l_nb_comp = p_j2k->m_private_image->numcomps;

	if (p_header_size != l_nb_comp *4) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading CRG marker\n");
		return OPJ_FALSE;
	}
	/* Do not care of this at the moment since only local variables are set here */
	/*
	for
		(i = 0; i < l_nb_comp; ++i)
	{
		opj_read_bytes(p_header_data,&l_Xcrg_i,2);				// Xcrg_i
		p_header_data+=2;
		opj_read_bytes(p_header_data,&l_Ycrg_i,2);				// Xcrg_i
		p_header_data+=2;
	}
	*/
	return OPJ_TRUE;
}

static void j2k_read_tlm(opj_j2k_t *j2k) {
	int len, Ztlm, Stlm, ST, SP, tile_tlm, i;
	long int Ttlm_i, Ptlm_i;

	opj_cio_t *cio = j2k->cio;
	
	len = cio_read(cio, 2);		/* Ltlm */
	Ztlm = cio_read(cio, 1);	/* Ztlm */
	Stlm = cio_read(cio, 1);	/* Stlm */
	ST = ((Stlm >> 4) & 0x01) + ((Stlm >> 4) & 0x02);
	SP = (Stlm >> 6) & 0x01;
	tile_tlm = (len - 4) / ((SP + 1) * 2 + ST);
	for (i = 0; i < tile_tlm; i++) {
		Ttlm_i = cio_read(cio, ST);	/* Ttlm_i */
		Ptlm_i = cio_read(cio, SP ? 4 : 2);	/* Ptlm_i */
	}
}

/**
 * Reads a TLM marker (Tile Length Marker)
 *
 * @param	p_header_data	the data contained in the TLM box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the TLM marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_tlm_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					)
{
	OPJ_UINT32 l_Ztlm, l_Stlm, l_ST, l_SP, l_tot_num_tp, l_tot_num_tp_remaining, l_quotient, l_Ptlm_size;
	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	if (p_header_size < 2) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading TLM marker\n");
		return OPJ_FALSE;
	}
	p_header_size -= 2;

	opj_read_bytes(p_header_data,&l_Ztlm,1);				/* Ztlm */
	++p_header_data;
	opj_read_bytes(p_header_data,&l_Stlm,1);				/* Stlm */
	++p_header_data;

	l_ST = ((l_Stlm >> 4) & 0x3);
	l_SP = (l_Stlm >> 6) & 0x1;

	l_Ptlm_size = (l_SP + 1) * 2;
	l_quotient = l_Ptlm_size + l_ST;

	l_tot_num_tp = p_header_size / l_quotient;
	l_tot_num_tp_remaining = p_header_size % l_quotient;

	if (l_tot_num_tp_remaining != 0) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading TLM marker\n");
		return OPJ_FALSE;
	}
	/* FIXME Do not care of this at the moment since only local variables are set here */
	/*
	for
		(i = 0; i < l_tot_num_tp; ++i)
	{
		opj_read_bytes(p_header_data,&l_Ttlm_i,l_ST);				// Ttlm_i
		p_header_data += l_ST;
		opj_read_bytes(p_header_data,&l_Ptlm_i,l_Ptlm_size);		// Ptlm_i
		p_header_data += l_Ptlm_size;
	}*/
	return OPJ_TRUE;
}

static void j2k_read_plm(opj_j2k_t *j2k) {
	int len, i, Zplm, Nplm, add, packet_len = 0;
	
	opj_cio_t *cio = j2k->cio;

	len = cio_read(cio, 2);		/* Lplm */
	Zplm = cio_read(cio, 1);	/* Zplm */
	len -= 3;
	while (len > 0) {
		Nplm = cio_read(cio, 4);		/* Nplm */
		len -= 4;
		for (i = Nplm; i > 0; i--) {
			add = cio_read(cio, 1);
			len--;
			packet_len = (packet_len << 7) + add;	/* Iplm_ij */
			if ((add & 0x80) == 0) {
				/* New packet */
				packet_len = 0;
			}
			if (len <= 0)
				break;
		}
	}
}

/**
 * Reads a PLM marker (Packet length, main header marker)
 *
 * @param	p_header_data	the data contained in the TLM box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the TLM marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_plm_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					)
{
	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	if (p_header_size < 1) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading PLM marker\n");
		return OPJ_FALSE;
	}
	/* Do not care of this at the moment since only local variables are set here */
	/*
	opj_read_bytes(p_header_data,&l_Zplm,1);					// Zplm
	++p_header_data;
	--p_header_size;

	while
		(p_header_size > 0)
	{
		opj_read_bytes(p_header_data,&l_Nplm,1);				// Nplm
		++p_header_data;
		p_header_size -= (1+l_Nplm);
		if
			(p_header_size < 0)
		{
			opj_event_msg(p_manager, EVT_ERROR, "Error reading PLM marker\n");
			return false;
		}
		for
			(i = 0; i < l_Nplm; ++i)
		{
			opj_read_bytes(p_header_data,&l_tmp,1);				// Iplm_ij
			++p_header_data;
			// take only the last seven bytes
			l_packet_len |= (l_tmp & 0x7f);
			if
				(l_tmp & 0x80)
			{
				l_packet_len <<= 7;
			}
			else
			{
                // store packet length and proceed to next packet
				l_packet_len = 0;
			}
		}
		if
			(l_packet_len != 0)
		{
			opj_event_msg(p_manager, EVT_ERROR, "Error reading PLM marker\n");
			return false;
		}
	}
	*/
	return OPJ_TRUE;
}

static void j2k_read_plt(opj_j2k_t *j2k) {
	int len, i, Zplt, packet_len = 0, add;
	
	opj_cio_t *cio = j2k->cio;
	
	len = cio_read(cio, 2);		/* Lplt */
	Zplt = cio_read(cio, 1);	/* Zplt */
	for (i = len - 3; i > 0; i--) {
		add = cio_read(cio, 1);
		packet_len = (packet_len << 7) + add;	/* Iplt_i */
		if ((add & 0x80) == 0) {
			/* New packet */
			packet_len = 0;
		}
	}
}

/**
 * Reads a PLT marker (Packet length, tile-part header)
 *
 * @param	p_header_data	the data contained in the PLT box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the PLT marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_plt_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					)
{
	OPJ_UINT32 l_Zplt, l_tmp, l_packet_len = 0, i;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	if (p_header_size < 1) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading PLM marker\n");
		return OPJ_FALSE;
	}

	opj_read_bytes(p_header_data,&l_Zplt,1);		/* Zplt */
	++p_header_data;
	--p_header_size;

	for (i = 0; i < p_header_size; ++i) {
		opj_read_bytes(p_header_data,&l_tmp,1);		/* Iplt_ij */
		++p_header_data;
		/* take only the last seven bytes */
		l_packet_len |= (l_tmp & 0x7f);
		if (l_tmp & 0x80) {
			l_packet_len <<= 7;
		}
		else {
            /* store packet length and proceed to next packet */
			l_packet_len = 0;
		}
	}

	if (l_packet_len != 0) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading PLM marker\n");
		return OPJ_FALSE;
	}

	return OPJ_TRUE;
}

static void j2k_read_ppm(opj_j2k_t *j2k) {
	int len, Z_ppm, i, j;
	int N_ppm;

	opj_cp_t *cp = j2k->cp;
	opj_cio_t *cio = j2k->cio;
	
	len = cio_read(cio, 2);
	cp->ppm = 1;
	
	Z_ppm = cio_read(cio, 1);	/* Z_ppm */
	len -= 3;
	while (len > 0) {
		if (cp->ppm_previous == 0) {
			N_ppm = cio_read(cio, 4);	/* N_ppm */
			len -= 4;
		} else {
			N_ppm = cp->ppm_previous;
		}
		j = cp->ppm_store;
		if (Z_ppm == 0) {	/* First PPM marker */
			cp->ppm_data = (unsigned char *) opj_malloc(N_ppm * sizeof(unsigned char));
			cp->ppm_data_first = cp->ppm_data;
			cp->ppm_len = N_ppm;
		} else {			/* NON-first PPM marker */
			cp->ppm_data = (unsigned char *) opj_realloc(cp->ppm_data, (N_ppm +	cp->ppm_store) * sizeof(unsigned char));

#ifdef USE_JPWL
			/* this memory allocation check could be done even in non-JPWL cases */
			if (cp->correct) {
				if (!cp->ppm_data) {
					opj_event_msg(j2k->cinfo, EVT_ERROR,
						"JPWL: failed memory allocation during PPM marker parsing (pos. %x)\n",
						cio_tell(cio));
					if (!JPWL_ASSUME || JPWL_ASSUME) {
						opj_free(cp->ppm_data);
						opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
						return;
					}
				}
			}
#endif

			cp->ppm_data_first = cp->ppm_data;
			cp->ppm_len = N_ppm + cp->ppm_store;
		}
		for (i = N_ppm; i > 0; i--) {	/* Read packet header */
			cp->ppm_data[j] = cio_read(cio, 1);
			j++;
			len--;
			if (len == 0)
				break;			/* Case of non-finished packet header in present marker but finished in next one */
		}
		cp->ppm_previous = i - 1;
		cp->ppm_store = j;
	}
}
/**
 * Reads a PPM marker (Packed packet headers, main header)
 *
 * @param	p_header_data	the data contained in the POC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the POC marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_ppm_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					)
{

	opj_cp_v2_t *l_cp = 00;
	OPJ_UINT32 l_remaining_data, l_Z_ppm, l_N_ppm;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	if (p_header_size < 1) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading PPM marker\n");
		return OPJ_FALSE;
	}

	l_cp = &(p_j2k->m_cp);
	l_cp->ppm = 1;

	opj_read_bytes(p_header_data,&l_Z_ppm,1);		/* Z_ppm */
	++p_header_data;
	--p_header_size;

	/* First PPM marker */
	if (l_Z_ppm == 0) {
		if (p_header_size < 4) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading PPM marker\n");
			return OPJ_FALSE;
		}

		opj_read_bytes(p_header_data,&l_N_ppm,4);		/* N_ppm */
		p_header_data+=4;
		p_header_size-=4;

		/* First PPM marker: Initialization */
		l_cp->ppm_len = l_N_ppm;
		l_cp->ppm_data_size = 0;

		l_cp->ppm_buffer = (OPJ_BYTE *) opj_malloc(l_cp->ppm_len);
		if (l_cp->ppm_buffer == 00) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory reading ppm marker\n");
			return OPJ_FALSE;
		}
		memset(l_cp->ppm_buffer,0,l_cp->ppm_len);

		l_cp->ppm_data = l_cp->ppm_buffer;
	}

	while (1) {
		if (l_cp->ppm_data_size == l_cp->ppm_len) {
			if (p_header_size >= 4) {
				/* read a N_ppm */
				opj_read_bytes(p_header_data,&l_N_ppm,4);		/* N_ppm */
				p_header_data+=4;
				p_header_size-=4;
				l_cp->ppm_len += l_N_ppm ;

				l_cp->ppm_buffer = (OPJ_BYTE *) opj_realloc(l_cp->ppm_buffer, l_cp->ppm_len);
				if (l_cp->ppm_buffer == 00) {
					opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory reading ppm marker\n");
					return OPJ_FALSE;
				}
				memset(l_cp->ppm_buffer+l_cp->ppm_data_size,0,l_N_ppm);

				l_cp->ppm_data = l_cp->ppm_buffer;
			}
			else {
				return OPJ_FALSE;
			}
		}

		l_remaining_data = l_cp->ppm_len - l_cp->ppm_data_size;

		if (l_remaining_data <= p_header_size) {
			/* we must store less information than available in the packet */
			memcpy(l_cp->ppm_buffer + l_cp->ppm_data_size , p_header_data , l_remaining_data);
			l_cp->ppm_data_size = l_cp->ppm_len;
			p_header_size -= l_remaining_data;
			p_header_data += l_remaining_data;
		}
		else {
			memcpy(l_cp->ppm_buffer + l_cp->ppm_data_size , p_header_data , p_header_size);
			l_cp->ppm_data_size += p_header_size;
			p_header_data += p_header_size;
			p_header_size = 0;
			break;
		}
	}

	return OPJ_TRUE;
}



/**
 * Reads a PPM marker (Packed packet headers, main header)
 *
 * @param	p_header_data	the data contained in the POC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the POC marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_ppm_v3 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					)
{
	opj_cp_v2_t *l_cp = 00;
	OPJ_UINT32 l_remaining_data, l_Z_ppm, l_N_ppm;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	/* Minimum size of PPM marker is equal to the size of Zppm element */
	if (p_header_size < 1) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading PPM marker\n");
		return OPJ_FALSE;
	}

	l_cp = &(p_j2k->m_cp);
	l_cp->ppm = 1;

	opj_read_bytes(p_header_data,&l_Z_ppm,1);		/* Z_ppm */
	++p_header_data;
	--p_header_size;

	/* First PPM marker */
	if (l_Z_ppm == 0) {
		/* We need now at least the Nppm^0 element */
		if (p_header_size < 4) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading PPM marker\n");
			return OPJ_FALSE;
		}

		opj_read_bytes(p_header_data,&l_N_ppm,4);		/* First N_ppm */
		p_header_data+=4;
		p_header_size-=4;

		/* First PPM marker: Initialization */
		l_cp->ppm_len = l_N_ppm;
		l_cp->ppm_data_read = 0;

		l_cp->ppm_data = (OPJ_BYTE *) opj_malloc(l_cp->ppm_len);
		if (l_cp->ppm_data == 00) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory reading ppm marker\n");
			return OPJ_FALSE;
		}
		memset(l_cp->ppm_data,0,l_cp->ppm_len);

		l_cp->ppm_data_current = l_cp->ppm_data;

		/*l_cp->ppm_data = l_cp->ppm_buffer;*/
	}
	else {
		if (p_header_size < 4) {
			opj_event_msg_v2(p_manager, EVT_WARNING, "Empty PPM marker\n");
			return OPJ_TRUE;
		}
		else {
			/* Uncompleted Ippm series in the previous PPM marker?*/
			if (l_cp->ppm_data_read < l_cp->ppm_len) {
				/* Get the place where add the remaining Ippm series*/
				l_cp->ppm_data_current = &(l_cp->ppm_data[l_cp->ppm_data_read]);
				l_N_ppm = l_cp->ppm_len - l_cp->ppm_data_read;
			}
			else {
				opj_read_bytes(p_header_data,&l_N_ppm,4);		/* First N_ppm */
				p_header_data+=4;
				p_header_size-=4;

				/* Increase the size of ppm_data to add the new Ippm series*/
				l_cp->ppm_data = (OPJ_BYTE *) opj_realloc(l_cp->ppm_data, l_cp->ppm_len + l_N_ppm);

				/* Keep the position of the place where concatenate the new series*/
				l_cp->ppm_data_current = &(l_cp->ppm_data[l_cp->ppm_len]);
				l_cp->ppm_len += l_N_ppm;
			}
		}
	}

	l_remaining_data = p_header_size;

	while (l_remaining_data >= l_N_ppm) {
		/* read a complete Ippm series*/
		memcpy(l_cp->ppm_data_current, p_header_data, l_N_ppm);
		p_header_size -= l_N_ppm;
		p_header_data += l_N_ppm;

		l_cp->ppm_data_read += l_N_ppm; /* Increase the number of data read*/

		if (p_header_size)
		{
			opj_read_bytes(p_header_data,&l_N_ppm,4);		/* N_ppm^i */
			p_header_data+=4;
			p_header_size-=4;
		}
		else {
			l_remaining_data = p_header_size;
			break;
		}

		l_remaining_data = p_header_size;

		/* Next Ippm series is a complete series ?*/
		if (l_remaining_data > l_N_ppm) {
			/* Increase the size of ppm_data to add the new Ippm series*/
			l_cp->ppm_data = (OPJ_BYTE *) opj_realloc(l_cp->ppm_data, l_cp->ppm_len + l_N_ppm);

			/* Keep the position of the place where concatenate the new series */
			l_cp->ppm_data_current = &(l_cp->ppm_data[l_cp->ppm_len]);
			l_cp->ppm_len += l_N_ppm;
		}

	}

	/* Need to read an incomplete Ippm series*/
	if (l_remaining_data) {
		l_cp->ppm_data = (OPJ_BYTE *) opj_realloc(l_cp->ppm_data, l_cp->ppm_len + l_N_ppm);

		/* Keep the position of the place where concatenate the new series*/
		l_cp->ppm_data_current = &(l_cp->ppm_data[l_cp->ppm_len]);
		l_cp->ppm_len += l_N_ppm;

		/* Read incomplete Ippm series*/
		memcpy(l_cp->ppm_data_current, p_header_data, l_remaining_data);
		p_header_size -= l_remaining_data;
		p_header_data += l_remaining_data;

		l_cp->ppm_data_read += l_remaining_data; /* Increase the number of data read*/
	}

#ifdef CLEAN_MSD

		if (l_cp->ppm_data_size == l_cp->ppm_len) {
			if (p_header_size >= 4) {
				/* read a N_ppm*/
				opj_read_bytes(p_header_data,&l_N_ppm,4);		/* N_ppm */
				p_header_data+=4;
				p_header_size-=4;
				l_cp->ppm_len += l_N_ppm ;

				l_cp->ppm_buffer = (OPJ_BYTE *) opj_realloc(l_cp->ppm_buffer, l_cp->ppm_len);
				if (l_cp->ppm_buffer == 00) {
					opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory reading ppm marker\n");
					return OPJ_FALSE;
				}
				memset(l_cp->ppm_buffer+l_cp->ppm_data_size,0,l_N_ppm);

				l_cp->ppm_data = l_cp->ppm_buffer;
			}
			else {
				return OPJ_FALSE;
			}
		}

		l_remaining_data = l_cp->ppm_len - l_cp->ppm_data_size;

		if (l_remaining_data <= p_header_size) {
			/* we must store less information than available in the packet */
			memcpy(l_cp->ppm_buffer + l_cp->ppm_data_size , p_header_data , l_remaining_data);
			l_cp->ppm_data_size = l_cp->ppm_len;
			p_header_size -= l_remaining_data;
			p_header_data += l_remaining_data;
		}
		else {
			memcpy(l_cp->ppm_buffer + l_cp->ppm_data_size , p_header_data , p_header_size);
			l_cp->ppm_data_size += p_header_size;
			p_header_data += p_header_size;
			p_header_size = 0;
			break;
		}
	}
#endif
	return OPJ_TRUE;
}

static void j2k_read_ppt(opj_j2k_t *j2k) {
	int len, Z_ppt, i, j = 0;

	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = cp->tcps + j2k->curtileno;
	opj_cio_t *cio = j2k->cio;

	len = cio_read(cio, 2);
	Z_ppt = cio_read(cio, 1);
	tcp->ppt = 1;
	if (Z_ppt == 0) {		/* First PPT marker */
		tcp->ppt_data = (unsigned char *) opj_malloc((len - 3) * sizeof(unsigned char));
		tcp->ppt_data_first = tcp->ppt_data;
		tcp->ppt_store = 0;
		tcp->ppt_len = len - 3;
	} else {			/* NON-first PPT marker */
		tcp->ppt_data =	(unsigned char *) opj_realloc(tcp->ppt_data, (len - 3 + tcp->ppt_store) * sizeof(unsigned char));
		tcp->ppt_data_first = tcp->ppt_data;
		tcp->ppt_len = len - 3 + tcp->ppt_store;
	}
	j = tcp->ppt_store;
	for (i = len - 3; i > 0; i--) {
		tcp->ppt_data[j] = cio_read(cio, 1);
		j++;
	}
	tcp->ppt_store = j;
}

/**
 * Reads a PPT marker (Packed packet headers, tile-part header)
 *
 * @param	p_header_data	the data contained in the PPT box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the PPT marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_ppt_v2 (	opj_j2k_v2_t *p_j2k,
							OPJ_BYTE * p_header_data,
							OPJ_UINT32 p_header_size,
							struct opj_event_mgr * p_manager )
{
	opj_cp_v2_t *l_cp = 00;
	opj_tcp_v2_t *l_tcp = 00;
	OPJ_UINT32 l_Z_ppt;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	/* We need to have the Z_ppt element at minimum */
	if (p_header_size < 1) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading PPT marker\n");
		return OPJ_FALSE;
	}

	l_cp = &(p_j2k->m_cp);
	if (l_cp->ppm){
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading PPT marker: packet header have been previously found in the main header (PPM marker).\n");
		return OPJ_FALSE;
	}

	l_tcp = &(l_cp->tcps[p_j2k->m_current_tile_number]);
	l_tcp->ppt = 1;

	opj_read_bytes(p_header_data,&l_Z_ppt,1);		/* Z_ppt */
	++p_header_data;
	--p_header_size;

	/* Allocate buffer to read the packet header */
	if (l_Z_ppt == 0) {
		/* First PPT marker */
		l_tcp->ppt_data_size = 0;
		l_tcp->ppt_len = p_header_size;

		l_tcp->ppt_buffer = (OPJ_BYTE *) opj_calloc(l_tcp->ppt_len, sizeof(OPJ_BYTE) );
		if (l_tcp->ppt_buffer == 00) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory reading PPT marker\n");
			return OPJ_FALSE;
		}
		l_tcp->ppt_data = l_tcp->ppt_buffer;

		/* memset(l_tcp->ppt_buffer,0,l_tcp->ppt_len); */
	}
	else {
		l_tcp->ppt_len += p_header_size;

		l_tcp->ppt_buffer = (OPJ_BYTE *) opj_realloc(l_tcp->ppt_buffer,l_tcp->ppt_len);
		if (l_tcp->ppt_buffer == 00) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Not enough memory reading PPT marker\n");
			return OPJ_FALSE;
		}
		l_tcp->ppt_data = l_tcp->ppt_buffer;

		memset(l_tcp->ppt_buffer+l_tcp->ppt_data_size,0,p_header_size);
	}

	/* Read packet header from buffer */
	memcpy(l_tcp->ppt_buffer+l_tcp->ppt_data_size,p_header_data,p_header_size);

	l_tcp->ppt_data_size += p_header_size;

	return OPJ_TRUE;
}

static void j2k_write_tlm(opj_j2k_t *j2k){
	int lenp;
	opj_cio_t *cio = j2k->cio;
	j2k->tlm_start = cio_tell(cio);
	cio_write(cio, J2K_MS_TLM, 2);/* TLM */
	lenp = 4 + (5*j2k->totnum_tp);
	cio_write(cio,lenp,2);				/* Ltlm */
	cio_write(cio, 0,1);					/* Ztlm=0*/
	cio_write(cio,80,1);					/* Stlm ST=1(8bits-255 tiles max),SP=1(Ptlm=32bits) */
	cio_skip(cio,5*j2k->totnum_tp);
}

static void j2k_write_sot(opj_j2k_t *j2k) {
	int lenp, len;

	opj_cio_t *cio = j2k->cio;

	j2k->sot_start = cio_tell(cio);
	cio_write(cio, J2K_MS_SOT, 2);		/* SOT */
	lenp = cio_tell(cio);
	cio_skip(cio, 2);					/* Lsot (further) */
	cio_write(cio, j2k->curtileno, 2);	/* Isot */
	cio_skip(cio, 4);					/* Psot (further in j2k_write_sod) */
	cio_write(cio, j2k->cur_tp_num , 1);	/* TPsot */
	cio_write(cio, j2k->cur_totnum_tp[j2k->curtileno], 1);		/* TNsot */
	len = cio_tell(cio) - lenp;
	cio_seek(cio, lenp);
	cio_write(cio, len, 2);				/* Lsot */
	cio_seek(cio, lenp + len);

	/* UniPG>> */
#ifdef USE_JPWL
	/* update markers struct */
	j2k_add_marker(j2k->cstr_info, J2K_MS_SOT, j2k->sot_start, len + 2);
#endif /* USE_JPWL */
	/* <<UniPG */

	if( j2k->cstr_info && j2k->cur_tp_num==0){
	  j2k_add_tlmarker( j2k->curtileno, j2k->cstr_info, J2K_MS_SOT, lenp, len);
	}
}

static void j2k_read_sot(opj_j2k_t *j2k) {
	int len, tileno, totlen, partno, numparts, i;
	opj_tcp_t *tcp = NULL;
	char status = 0;

	opj_cp_t *cp = j2k->cp;
	opj_cio_t *cio = j2k->cio;

	len = cio_read(cio, 2);
	tileno = cio_read(cio, 2);

#ifdef USE_JPWL
	if (j2k->cp->correct) {

		static int backup_tileno = 0;

		/* tileno is negative or larger than the number of tiles!!! */
		if ((tileno < 0) || (tileno > (cp->tw * cp->th))) {
			opj_event_msg(j2k->cinfo, EVT_ERROR,
				"JPWL: bad tile number (%d out of a maximum of %d)\n",
				tileno, (cp->tw * cp->th));
			if (!JPWL_ASSUME) {
				opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
				return;
			}
			/* we try to correct */
			tileno = backup_tileno;
			opj_event_msg(j2k->cinfo, EVT_WARNING, "- trying to adjust this\n"
				"- setting tile number to %d\n",
				tileno);
		}

		/* keep your private count of tiles */
		backup_tileno++;
	};
#endif /* USE_JPWL */
	
	if (cp->tileno_size == 0) {
		cp->tileno[cp->tileno_size] = tileno;
		cp->tileno_size++;
	} else {
		i = 0;
		while (i < cp->tileno_size && status == 0) {
			status = cp->tileno[i] == tileno ? 1 : 0;
			i++;
		}
		if (status == 0) {
			cp->tileno[cp->tileno_size] = tileno;
			cp->tileno_size++;
		}
	}
	
	totlen = cio_read(cio, 4);

#ifdef USE_JPWL
	if (j2k->cp->correct) {

		/* totlen is negative or larger than the bytes left!!! */
		if ((totlen < 0) || (totlen > (cio_numbytesleft(cio) + 8))) {
			opj_event_msg(j2k->cinfo, EVT_ERROR,
				"JPWL: bad tile byte size (%d bytes against %d bytes left)\n",
				totlen, cio_numbytesleft(cio) + 8);
			if (!JPWL_ASSUME) {
				opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
				return;
			}
			/* we try to correct */
			totlen = 0;
			opj_event_msg(j2k->cinfo, EVT_WARNING, "- trying to adjust this\n"
				"- setting Psot to %d => assuming it is the last tile\n",
				totlen);
		}

	};
#endif /* USE_JPWL */

	if (!totlen)
		totlen = cio_numbytesleft(cio) + 8;
	
	partno = cio_read(cio, 1);
	numparts = cio_read(cio, 1);
  
  if (partno >= numparts) {
    opj_event_msg(j2k->cinfo, EVT_WARNING, "SOT marker inconsistency in tile %d: tile-part index greater (%d) than number of tile-parts (%d)\n", tileno, partno, numparts);
    numparts = partno+1;
  }
	
	j2k->curtileno = tileno;
	j2k->cur_tp_num = partno;
	j2k->eot = cio_getbp(cio) - 12 + totlen;
	j2k->state = J2K_STATE_TPH;
	tcp = &cp->tcps[j2k->curtileno];

	/* Index */
	if (j2k->cstr_info) {
		if (tcp->first) {
			if (tileno == 0) 
				j2k->cstr_info->main_head_end = cio_tell(cio) - 13;
			j2k->cstr_info->tile[tileno].tileno = tileno;
			j2k->cstr_info->tile[tileno].start_pos = cio_tell(cio) - 12;
			j2k->cstr_info->tile[tileno].end_pos = j2k->cstr_info->tile[tileno].start_pos + totlen - 1;				
    } else {
			j2k->cstr_info->tile[tileno].end_pos += totlen;
		}
    j2k->cstr_info->tile[tileno].num_tps = numparts;
    if (numparts)
      j2k->cstr_info->tile[tileno].tp = (opj_tp_info_t *) opj_realloc(j2k->cstr_info->tile[tileno].tp, numparts * sizeof(opj_tp_info_t));
    else
      j2k->cstr_info->tile[tileno].tp = (opj_tp_info_t *) opj_realloc(j2k->cstr_info->tile[tileno].tp, 10 * sizeof(opj_tp_info_t)); /* Fixme (10)*/
		j2k->cstr_info->tile[tileno].tp[partno].tp_start_pos = cio_tell(cio) - 12;
		j2k->cstr_info->tile[tileno].tp[partno].tp_end_pos = 
			j2k->cstr_info->tile[tileno].tp[partno].tp_start_pos + totlen - 1;
	}
	
	if (tcp->first == 1) {		
		/* Initialization PPT */
		opj_tccp_t *tmp = tcp->tccps;
		memcpy(tcp, j2k->default_tcp, sizeof(opj_tcp_t));
		tcp->ppt = 0;
		tcp->ppt_data = NULL;
		tcp->ppt_data_first = NULL;
		tcp->tccps = tmp;

		for (i = 0; i < j2k->image->numcomps; i++) {
			tcp->tccps[i] = j2k->default_tcp->tccps[i];
		}
		cp->tcps[j2k->curtileno].first = 0;
	}
}

/**
 * Reads a PPT marker (Packed packet headers, tile-part header)
 *
 * @param	p_header_data	the data contained in the PPT box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the PPT marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_sot_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					)
{

	opj_cp_v2_t *l_cp = 00;
	opj_tcp_v2_t *l_tcp = 00;
	OPJ_UINT32 l_tot_len, l_num_parts = 0;
	OPJ_UINT32 l_current_part;
	OPJ_UINT32 l_tile_x,l_tile_y;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	/* Size of this marker is fixed = 12 (we have already read marker and its size)*/
	if (p_header_size != 8) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading SOT marker\n");
		return OPJ_FALSE;
	}

	l_cp = &(p_j2k->m_cp);
	opj_read_bytes(p_header_data,&(p_j2k->m_current_tile_number),2);		/* Isot */
	p_header_data+=2;

	l_tcp = &l_cp->tcps[p_j2k->m_current_tile_number];
	l_tile_x = p_j2k->m_current_tile_number % l_cp->tw;
	l_tile_y = p_j2k->m_current_tile_number / l_cp->tw;

#ifdef USE_JPWL
	if (l_cp->correct) {

		int tileno = p_j2k->m_current_tile_number;
		static int backup_tileno = 0;

		/* tileno is negative or larger than the number of tiles!!! */
		if ((tileno < 0) || (tileno > (l_cp->tw * l_cp->th))) {
			opj_event_msg_v2(p_manager, EVT_ERROR,
				"JPWL: bad tile number (%d out of a maximum of %d)\n",
				tileno, (l_cp->tw * l_cp->th));
			if (!JPWL_ASSUME) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "JPWL: giving up\n");
				return OPJ_FALSE;
			}
			/* we try to correct */
			tileno = backup_tileno;
			opj_event_msg_v2(p_manager, EVT_WARNING, "- trying to adjust this\n"
				"- setting tile number to %d\n",
				tileno);
		}

		/* keep your private count of tiles */
		backup_tileno++;
	};
#endif /* USE_JPWL */

	/* look for the tile in the list of already processed tile (in parts). */
	/* Optimization possible here with a more complex data structure and with the removing of tiles */
	/* since the time taken by this function can only grow at the time */

	opj_read_bytes(p_header_data,&l_tot_len,4);		/* Psot */
	p_header_data+=4;

	/* PSot should be equal to zero or >=14 or <= 2^32-1 */
	if ((l_tot_len !=0 ) && (l_tot_len < 14) )
	{
		opj_event_msg_v2(p_manager, EVT_ERROR, "Psot value (%d) is not correct regards to the JPEG2000 norm!\n", l_tot_len);
		return OPJ_FALSE;
	}


#ifdef USE_JPWL
	if (l_cp->correct) {

		/* totlen is negative or larger than the bytes left!!! */
		if ((l_tot_len < 0) || (l_tot_len > p_header_size ) ) { /* FIXME it seems correct; for info in V1 -> (p_stream_numbytesleft(p_stream) + 8))) { */
			opj_event_msg_v2(p_manager, EVT_ERROR,
				"JPWL: bad tile byte size (%d bytes against %d bytes left)\n",
				l_tot_len, p_header_size ); /* FIXME it seems correct; for info in V1 -> p_stream_numbytesleft(p_stream) + 8); */
			if (!JPWL_ASSUME) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "JPWL: giving up\n");
				return OPJ_FALSE;
			}
			/* we try to correct */
			l_tot_len = 0;
			opj_event_msg_v2(p_manager, EVT_WARNING, "- trying to adjust this\n"
				"- setting Psot to %d => assuming it is the last tile\n",
				l_tot_len);
		}
	};
#endif /* USE_JPWL */

	/* Ref A.4.2: Psot could be equal zero if it is the last tile-part of the codestream.*/
	if (!l_tot_len) {
		opj_event_msg_v2(p_manager, EVT_INFO, "Psot value of the current tile-part is equal to zero, "
				"we assuming it is the last tile-part of the codestream.\n");
		p_j2k->m_specific_param.m_decoder.m_last_tile_part = 1;
	}

	opj_read_bytes(p_header_data,&l_current_part ,1);	/* TPsot */
	++p_header_data;

	opj_read_bytes(p_header_data,&l_num_parts ,1);		/* TNsot */
	++p_header_data;

	if (l_num_parts != 0) { /* Number of tile-part header is provided by this tile-part header */
		/* Useful to manage the case of textGBR.jp2 file because two values of TNSot are allowed: the correct numbers of
		 * tile-parts for that tile and zero (A.4.2 of 15444-1 : 2002). */
		if (l_tcp->m_nb_tile_parts) {
			if (l_current_part >= l_tcp->m_nb_tile_parts){
				opj_event_msg_v2(p_manager, EVT_ERROR, "In SOT marker, TPSot (%d) is not valid regards to the current "
						"number of tile-part (%d), giving up\n", l_current_part, l_tcp->m_nb_tile_parts );
				p_j2k->m_specific_param.m_decoder.m_last_tile_part = 1;
				return OPJ_FALSE;
			}
		}
		l_tcp->m_nb_tile_parts = l_num_parts;
	}

	/* If know the number of tile part header we will check if we didn't read the last*/
	if (l_tcp->m_nb_tile_parts) {
		if (l_tcp->m_nb_tile_parts == (l_current_part + 1)) {
			p_j2k->m_specific_param.m_decoder.m_can_decode = 1; /* Process the last tile-part header*/
		}
	}

	if (!p_j2k->m_specific_param.m_decoder.m_last_tile_part){
		/* Keep the size of data to skip after this marker */
		p_j2k->m_specific_param.m_decoder.m_sot_length = l_tot_len - 12; /* SOT_marker_size = 12 */
	}
	else {
		/* FIXME: need to be computed from the number of bytes remaining in the codestream */
		p_j2k->m_specific_param.m_decoder.m_sot_length = 0;
	}

	p_j2k->m_specific_param.m_decoder.m_state = J2K_STATE_TPH;

	/* Check if the current tile is outside the area we want decode or not corresponding to the tile index*/
	if (p_j2k->m_specific_param.m_decoder.m_tile_ind_to_dec == -1) {
		p_j2k->m_specific_param.m_decoder.m_skip_data =
				(l_tile_x < p_j2k->m_specific_param.m_decoder.m_start_tile_x)
			||	(l_tile_x >= p_j2k->m_specific_param.m_decoder.m_end_tile_x)
			||  (l_tile_y < p_j2k->m_specific_param.m_decoder.m_start_tile_y)
			||	(l_tile_y >= p_j2k->m_specific_param.m_decoder.m_end_tile_y);
	}
	else
		p_j2k->m_specific_param.m_decoder.m_skip_data =
			(p_j2k->m_current_tile_number != p_j2k->m_specific_param.m_decoder.m_tile_ind_to_dec);

	/* Index */
	if (p_j2k->cstr_index)
	{
		p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].tileno = p_j2k->m_current_tile_number;
		p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].current_tpsno = l_current_part;

		if (l_num_parts != 0){
			p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].nb_tps = l_num_parts;
			p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].current_nb_tps = l_num_parts;


			if (!p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index)
				p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index =
					(opj_tp_index_t*)opj_calloc(l_num_parts, sizeof(opj_tp_index_t));
			else
				p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index =
					(opj_tp_index_t*)opj_realloc(
							p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index,
							l_num_parts* sizeof(opj_tp_index_t));
		}
		else{
			/*if (!p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index)*/ {

				if (!p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index) {
					p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].current_nb_tps = 10;
					p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index =
						(opj_tp_index_t*)opj_calloc( p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].current_nb_tps,
													 sizeof(opj_tp_index_t));
				}

				if ( l_current_part >= p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].current_nb_tps ){
					p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].current_nb_tps += 10;
					p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index =
						(opj_tp_index_t*)opj_realloc( p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index,
													  p_j2k->cstr_index->tile_index[p_j2k->m_current_tile_number].current_nb_tps
													  * sizeof(opj_tp_index_t));
				}
			}

		}

	}


	/* FIXME move this onto a separate method to call before reading any SOT, remove part about main_end header, use a index struct inside p_j2k */
	/* if (p_j2k->cstr_info) {
		if (l_tcp->first) {
			if (tileno == 0) {
				p_j2k->cstr_info->main_head_end = p_stream_tell(p_stream) - 13;
			}

			p_j2k->cstr_info->tile[tileno].tileno = tileno;
			p_j2k->cstr_info->tile[tileno].start_pos = p_stream_tell(p_stream) - 12;
			p_j2k->cstr_info->tile[tileno].end_pos = p_j2k->cstr_info->tile[tileno].start_pos + totlen - 1;
			p_j2k->cstr_info->tile[tileno].num_tps = numparts;

			if (numparts) {
				p_j2k->cstr_info->tile[tileno].tp = (opj_tp_info_t *) opj_malloc(numparts * sizeof(opj_tp_info_t));
			}
			else {
				p_j2k->cstr_info->tile[tileno].tp = (opj_tp_info_t *) opj_malloc(10 * sizeof(opj_tp_info_t)); // Fixme (10)
			}
		}
		else {
			p_j2k->cstr_info->tile[tileno].end_pos += totlen;
		}

		p_j2k->cstr_info->tile[tileno].tp[partno].tp_start_pos = p_stream_tell(p_stream) - 12;
		p_j2k->cstr_info->tile[tileno].tp[partno].tp_end_pos =
		p_j2k->cstr_info->tile[tileno].tp[partno].tp_start_pos + totlen - 1;
	}*/
	return OPJ_TRUE;
}

static void j2k_write_sod(opj_j2k_t *j2k, void *tile_coder) {
	int l, layno;
	int totlen;
	opj_tcp_t *tcp = NULL;
	opj_codestream_info_t *cstr_info = NULL;
	
	opj_tcd_t *tcd = (opj_tcd_t*)tile_coder;	/* cast is needed because of conflicts in header inclusions */
	opj_cp_t *cp = j2k->cp;
	opj_cio_t *cio = j2k->cio;

	tcd->tp_num = j2k->tp_num ;
	tcd->cur_tp_num = j2k->cur_tp_num;
	
	/* INDEX >> - PLT */
#define ROUND_UPTO(a, quanta)   (((a) + ((quanta) - 1)) & ~((quanta) - 1))
	int marker_plt_start;
	cstr_info = j2k->cstr_info;
	if (cstr_info) {
		int precno = 0;
		for (int compno = 0; compno < cstr_info->numcomps; compno++)
			for (int resno = 0; resno <= cstr_info->numdecompos[compno]; resno++) {
				int pw = 1 << cp->tcps->tccps->prcw[resno];
				int ph = 1 << cp->tcps->tccps->prch[resno];

				int ww = (cstr_info->image_w >> resno);
				ww = ROUND_UPTO(ww, pw) / pw;

				int hh = (cstr_info->image_h >> resno);
				hh = ROUND_UPTO(hh, ph) / ph;

				precno += (ww < 1 ? 1 : ww) * (hh < 1 ? 1 : hh);
			}
		int plt_num = cstr_info->numlayers * precno;

		cio_write(cio, J2K_MS_PLT, 2);
		cio_write(cio, plt_num * 3 + 2 + 1, 2);
		cio_write(cio, 0, 1);

		marker_plt_start = cio_tell(cio);
		for (int i = 0; i < plt_num * 3; ++i)
            cio_write(cio, 0, 1);
	}
	/* PLT - << INDEX */
	
	cio_write(cio, J2K_MS_SOD, 2);

	if( j2k->cstr_info && j2k->cur_tp_num==0){
	  j2k_add_tlmarker( j2k->curtileno, j2k->cstr_info, J2K_MS_SOD, cio_tell(cio), 0);
	}

	if (j2k->curtileno == 0) {
		j2k->sod_start = cio_tell(cio) + j2k->pos_correction;
	}

	/* INDEX >> */
	cstr_info = j2k->cstr_info;
	if (cstr_info) {
		if (!j2k->cur_tp_num ) {
			cstr_info->tile[j2k->curtileno].end_header = cio_tell(cio) + j2k->pos_correction - 1;
			j2k->cstr_info->tile[j2k->curtileno].tileno = j2k->curtileno;
		}
		else{
			if(cstr_info->tile[j2k->curtileno].packet[cstr_info->packno - 1].end_pos < cio_tell(cio))
				cstr_info->tile[j2k->curtileno].packet[cstr_info->packno].start_pos = cio_tell(cio);
		}
		/* UniPG>> */
#ifdef USE_JPWL
		/* update markers struct */
		j2k_add_marker(j2k->cstr_info, J2K_MS_SOD, j2k->sod_start, 2);
#endif /* USE_JPWL */
		/* <<UniPG */
	}
	/* << INDEX */
	
	tcp = &cp->tcps[j2k->curtileno];
	for (layno = 0; layno < tcp->numlayers; layno++) {
		if (tcp->rates[layno]>(j2k->sod_start / (cp->th * cp->tw))) {
			tcp->rates[layno]-=(j2k->sod_start / (cp->th * cp->tw));
		} else if (tcp->rates[layno]) {
			tcp->rates[layno]=1;
		}
	}
	if(j2k->cur_tp_num == 0){
		tcd->tcd_image->tiles->packno = 0;
		if(cstr_info)
			cstr_info->packno = 0;
	}
	
	l = tcd_encode_tile(tcd, j2k->curtileno, cio_getbp(cio), cio_numbytesleft(cio) - 2, cstr_info);
	
	/* INDEX >> - PLT */
	cstr_info = j2k->cstr_info;
	if (cstr_info) {
		int marker_end = cio_tell(cio);
		cio_seek(cio, marker_plt_start);
		for (int i = 0; i < cstr_info->packno; ++i) {
			int plen = cstr_info->tile[j2k->curtileno].packet[i].end_pos - cstr_info->tile[j2k->curtileno].packet[i].start_pos + 1;

			int b0 = plen & 0x7F;
			int b1 = (plen >> 7) & 0x7F;
			int b2 = (plen >> 14) & 0x7F;

			if (b2) {
				cio_write(cio, b2 | 0x80, 1);
				cio_write(cio, b1 | 0x80, 1);
			} else if (b1)
				cio_write(cio, b1 | 0x80, 1);
			cio_write(cio, b0, 1);
		}
		cio_seek(cio, marker_end);
	}
	/* PLT - << INDEX */
	
	/* Writing Psot in SOT marker */
	totlen = cio_tell(cio) + l - j2k->sot_start;
	cio_seek(cio, j2k->sot_start + 6);
	cio_write(cio, totlen, 4);
	cio_seek(cio, j2k->sot_start + totlen);
	/* Writing Ttlm and Ptlm in TLM marker */
	if(cp->cinema){
		cio_seek(cio, j2k->tlm_start + 6 + (5*j2k->cur_tp_num));
		cio_write(cio, j2k->curtileno, 1);
		cio_write(cio, totlen, 4);
	}
	cio_seek(cio, j2k->sot_start + totlen);
}

static void j2k_read_sod(opj_j2k_t *j2k) {
	int len, truncate = 0, i;
	unsigned char *data = NULL, *data_ptr = NULL;

	opj_cio_t *cio = j2k->cio;
	int curtileno = j2k->curtileno;

	/* Index */
	if (j2k->cstr_info) {
		j2k->cstr_info->tile[j2k->curtileno].tp[j2k->cur_tp_num].tp_end_header =
			cio_tell(cio) + j2k->pos_correction - 1;
		if (j2k->cur_tp_num == 0)
			j2k->cstr_info->tile[j2k->curtileno].end_header = cio_tell(cio) + j2k->pos_correction - 1;
		j2k->cstr_info->packno = 0;
	}
	
	len = int_min(j2k->eot - cio_getbp(cio), cio_numbytesleft(cio) + 1);

	if (len == cio_numbytesleft(cio) + 1) {
		truncate = 1;		/* Case of a truncate codestream */
	}	

	data = j2k->tile_data[curtileno];
	data = (unsigned char*) opj_realloc(data, (j2k->tile_len[curtileno] + len) * sizeof(unsigned char));

	data_ptr = data + j2k->tile_len[curtileno];
	for (i = 0; i < len; i++) {
		data_ptr[i] = cio_read(cio, 1);
	}

	j2k->tile_len[curtileno] += len;
	j2k->tile_data[curtileno] = data;
	
	if (!truncate) {
		j2k->state = J2K_STATE_TPHSOT;
	} else {
		j2k->state = J2K_STATE_NEOC;	/* RAJOUTE !! */
	}
	j2k->cur_tp_num++;
}

/**
 * Reads a SOD marker (Start Of Data)
 *
 * @param	p_header_data	the data contained in the SOD box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the SOD marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_sod_v2 (
						opj_j2k_v2_t *p_j2k,
						struct opj_stream_private *p_stream,
						struct opj_event_mgr * p_manager
					)
{
	OPJ_UINT32 l_current_read_size;
	opj_codestream_index_t * l_cstr_index = 00;
	OPJ_BYTE ** l_current_data = 00;
	opj_tcp_v2_t * l_tcp = 00;
	OPJ_UINT32 * l_tile_len = 00;

	/* preconditions */
	assert(p_j2k != 00);
	assert(p_manager != 00);
	assert(p_stream != 00);

	l_tcp = &(p_j2k->m_cp.tcps[p_j2k->m_current_tile_number]);

	if (p_j2k->m_specific_param.m_decoder.m_last_tile_part) {
		/* opj_stream_get_number_byte_left returns OPJ_OFF_T
		// but we are in the last tile part,
		// so its result will fit on OPJ_UINT32 unless we find
		// a file with a single tile part of more than 4 GB...*/
		p_j2k->m_specific_param.m_decoder.m_sot_length = (OPJ_UINT32)(opj_stream_get_number_byte_left(p_stream) - 2);
	}
	else
		p_j2k->m_specific_param.m_decoder.m_sot_length -= 2;

	l_current_data = &(l_tcp->m_data);
	l_tile_len = &l_tcp->m_data_size;

	if (! *l_current_data) {
		*l_current_data = (OPJ_BYTE*) opj_malloc(p_j2k->m_specific_param.m_decoder.m_sot_length);
	}
	else {
		*l_current_data = (OPJ_BYTE*) opj_realloc(*l_current_data, *l_tile_len + p_j2k->m_specific_param.m_decoder.m_sot_length);
	}

	if (*l_current_data == 00) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Cannot decode tile\n");
		return OPJ_FALSE;
	}


	/* Index */
	l_cstr_index = p_j2k->cstr_index;
	if (l_cstr_index) {
		OPJ_OFF_T l_current_pos = opj_stream_tell(p_stream) - 2;

		OPJ_UINT32 l_current_tile_part = l_cstr_index->tile_index[p_j2k->m_current_tile_number].current_tpsno;
		l_cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index[l_current_tile_part].end_header =
				l_current_pos;
		l_cstr_index->tile_index[p_j2k->m_current_tile_number].tp_index[l_current_tile_part].end_pos =
				l_current_pos + p_j2k->m_specific_param.m_decoder.m_sot_length + 2;

		j2k_add_tlmarker_v2(p_j2k->m_current_tile_number,
							l_cstr_index,
							J2K_MS_SOD,
							l_current_pos,
							p_j2k->m_specific_param.m_decoder.m_sot_length + 2);

		/*l_cstr_index->packno = 0;*/
	}

	l_current_read_size = opj_stream_read_data(	p_stream,
												*l_current_data + *l_tile_len,
												p_j2k->m_specific_param.m_decoder.m_sot_length,
												p_manager);

	if (l_current_read_size != p_j2k->m_specific_param.m_decoder.m_sot_length) {
		p_j2k->m_specific_param.m_decoder.m_state = J2K_STATE_NEOC;
	}
	else {
		p_j2k->m_specific_param.m_decoder.m_state = J2K_STATE_TPHSOT;
	}

	*l_tile_len +=  l_current_read_size;

	return OPJ_TRUE;
}


static void j2k_write_rgn(opj_j2k_t *j2k, int compno, int tileno) {
	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = &cp->tcps[tileno];
	opj_cio_t *cio = j2k->cio;
	int numcomps = j2k->image->numcomps;
	
	cio_write(cio, J2K_MS_RGN, 2);						/* RGN  */
	cio_write(cio, numcomps <= 256 ? 5 : 6, 2);			/* Lrgn */
	cio_write(cio, compno, numcomps <= 256 ? 1 : 2);	/* Crgn */
	cio_write(cio, 0, 1);								/* Srgn */
	cio_write(cio, tcp->tccps[compno].roishift, 1);		/* SPrgn */
}

static void j2k_read_rgn(opj_j2k_t *j2k) {
	int len, compno, roisty;

	opj_cp_t *cp = j2k->cp;
	opj_tcp_t *tcp = j2k->state == J2K_STATE_TPH ? &cp->tcps[j2k->curtileno] : j2k->default_tcp;
	opj_cio_t *cio = j2k->cio;
	int numcomps = j2k->image->numcomps;

	len = cio_read(cio, 2);										/* Lrgn */
	compno = cio_read(cio, numcomps <= 256 ? 1 : 2);			/* Crgn */
	roisty = cio_read(cio, 1);									/* Srgn */

#ifdef USE_JPWL
	if (j2k->cp->correct) {
		/* totlen is negative or larger than the bytes left!!! */
		if (compno >= numcomps) {
			opj_event_msg(j2k->cinfo, EVT_ERROR,
				"JPWL: bad component number in RGN (%d when there are only %d)\n",
				compno, numcomps);
			if (!JPWL_ASSUME || JPWL_ASSUME) {
				opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL: giving up\n");
				return;
			}
		}
	};
#endif /* USE_JPWL */

	tcp->tccps[compno].roishift = cio_read(cio, 1);				/* SPrgn */
}

static void j2k_write_eoc(opj_j2k_t *j2k) {
	opj_cio_t *cio = j2k->cio;
	/* opj_event_msg(j2k->cinfo, "%.8x: EOC\n", cio_tell(cio) + j2k->pos_correction); */
	cio_write(cio, J2K_MS_EOC, 2);

/* UniPG>> */
#ifdef USE_JPWL
	/* update markers struct */
	j2k_add_marker(j2k->cstr_info, J2K_MS_EOC, cio_tell(cio) - 2, 2);
#endif /* USE_JPWL */
/* <<UniPG */
}

/**
 * Reads a RGN marker (Region Of Interest)
 *
 * @param	p_header_data	the data contained in the POC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the POC marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_rgn_v2 (
						opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager
					)
{
	OPJ_UINT32 l_nb_comp;
	opj_image_t * l_image = 00;

	opj_cp_v2_t *l_cp = 00;
	opj_tcp_v2_t *l_tcp = 00;
	OPJ_UINT32 l_comp_room, l_comp_no, l_roi_sty;

	/* preconditions*/
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	l_image = p_j2k->m_private_image;
	l_nb_comp = l_image->numcomps;

	if (l_nb_comp <= 256) {
		l_comp_room = 1; }
	else {
		l_comp_room = 2; }

	if (p_header_size != 2 + l_comp_room) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading RGN marker\n");
		return OPJ_FALSE;
	}

	l_cp = &(p_j2k->m_cp);
	l_tcp = (p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH) ?
				&l_cp->tcps[p_j2k->m_current_tile_number] :
				p_j2k->m_specific_param.m_decoder.m_default_tcp;

	opj_read_bytes(p_header_data,&l_comp_no,l_comp_room);		/* Crgn */
	p_header_data+=l_comp_room;
	opj_read_bytes(p_header_data,&l_roi_sty,1);					/* Srgn */
	++p_header_data;

#ifdef USE_JPWL
	if (l_cp->correct) {
		/* totlen is negative or larger than the bytes left!!! */
		if (l_comp_room >= l_nb_comp) {
			opj_event_msg_v2(p_manager, EVT_ERROR,
				"JPWL: bad component number in RGN (%d when there are only %d)\n",
				l_comp_room, l_nb_comp);
			if (!JPWL_ASSUME || JPWL_ASSUME) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "JPWL: giving up\n");
				return OPJ_FALSE;
			}
		}
	};
#endif /* USE_JPWL */

	opj_read_bytes(p_header_data,(OPJ_UINT32 *) (&(l_tcp->tccps[l_comp_no].roishift)),1);	/* SPrgn */
	++p_header_data;

	return OPJ_TRUE;

}

static void j2k_read_eoc(opj_j2k_t *j2k) {
	int i, tileno;
	opj_bool success;

	/* if packets should be decoded */
	if (j2k->cp->limit_decoding != DECODE_ALL_BUT_PACKETS) {
		opj_tcd_t *tcd = tcd_create(j2k->cinfo);
		tcd_malloc_decode(tcd, j2k->image, j2k->cp);
		for (i = 0; i < j2k->cp->tileno_size; i++) {
			tcd_malloc_decode_tile(tcd, j2k->image, j2k->cp, i, j2k->cstr_info);
			tileno = j2k->cp->tileno[i];
			success = tcd_decode_tile(tcd, j2k->tile_data[tileno], j2k->tile_len[tileno], tileno, j2k->cstr_info);
			opj_free(j2k->tile_data[tileno]);
			j2k->tile_data[tileno] = NULL;
			tcd_free_decode_tile(tcd, i);
			if (success == OPJ_FALSE) {
				j2k->state |= J2K_STATE_ERR;
				break;
			}
		}
		tcd_free_decode(tcd);
		tcd_destroy(tcd);
	}
	/* if packets should not be decoded  */
	else {
		for (i = 0; i < j2k->cp->tileno_size; i++) {
			tileno = j2k->cp->tileno[i];
			opj_free(j2k->tile_data[tileno]);
			j2k->tile_data[tileno] = NULL;
		}
	}	
	if (j2k->state & J2K_STATE_ERR)
		j2k->state = J2K_STATE_MT + J2K_STATE_ERR;
	else
		j2k->state = J2K_STATE_MT; 
}

/**
 * Reads a EOC marker (End Of Codestream)
 *
 * @param	p_header_data	the data contained in the SOD box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the SOD marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_eoc_v2 (	opj_j2k_v2_t *p_j2k,
							struct opj_stream_private *p_stream,
							struct opj_event_mgr * p_manager )
{
	OPJ_UINT32 i;
	opj_tcd_v2_t * l_tcd = 00;
	OPJ_UINT32 l_nb_tiles;
	opj_tcp_v2_t * l_tcp = 00;
	opj_bool l_success;

	/* preconditions */
	assert(p_j2k != 00);
	assert(p_manager != 00);
	assert(p_stream != 00);

	l_nb_tiles = p_j2k->m_cp.th * p_j2k->m_cp.tw;
	l_tcp = p_j2k->m_cp.tcps;

	l_tcd = tcd_create_v2(OPJ_TRUE);
	if (l_tcd == 00) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Cannot decode tile, memory error\n");
		return OPJ_FALSE;
	}

	for (i = 0; i < l_nb_tiles; ++i) {
		if (l_tcp->m_data) {
			if (! tcd_init_decode_tile(l_tcd, i)) {
				tcd_destroy_v2(l_tcd);
				opj_event_msg_v2(p_manager, EVT_ERROR, "Cannot decode tile, memory error\n");
				return OPJ_FALSE;
			}

			l_success = tcd_decode_tile_v2(l_tcd, l_tcp->m_data, l_tcp->m_data_size, i, p_j2k->cstr_index);
			/* cleanup */

			if (! l_success) {
				p_j2k->m_specific_param.m_decoder.m_state |= J2K_STATE_ERR;
				break;
			}
		}

		j2k_tcp_destroy(l_tcp);
		++l_tcp;
	}

	tcd_destroy_v2(l_tcd);
	return OPJ_TRUE;
}

typedef struct opj_dec_mstabent {
	/** marker value */
	int id;
	/** value of the state when the marker can appear */
	int states;
	/** action linked to the marker */
	void (*handler) (opj_j2k_t *j2k);
} opj_dec_mstabent_t;

opj_dec_mstabent_t j2k_dec_mstab[] = {
  {J2K_MS_SOC, J2K_STATE_MHSOC, j2k_read_soc},
  {J2K_MS_SOT, J2K_STATE_MH | J2K_STATE_TPHSOT, j2k_read_sot},
  {J2K_MS_SOD, J2K_STATE_TPH, j2k_read_sod},
  {J2K_MS_EOC, J2K_STATE_TPHSOT, j2k_read_eoc},
  {J2K_MS_SIZ, J2K_STATE_MHSIZ, j2k_read_siz},
  {J2K_MS_COD, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_cod},
  {J2K_MS_COC, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_coc},
  {J2K_MS_RGN, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_rgn},
  {J2K_MS_QCD, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_qcd},
  {J2K_MS_QCC, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_qcc},
  {J2K_MS_POC, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_poc},
  {J2K_MS_TLM, J2K_STATE_MH, j2k_read_tlm},
  {J2K_MS_PLM, J2K_STATE_MH, j2k_read_plm},
  {J2K_MS_PLT, J2K_STATE_TPH, j2k_read_plt},
  {J2K_MS_PPM, J2K_STATE_MH, j2k_read_ppm},
  {J2K_MS_PPT, J2K_STATE_TPH, j2k_read_ppt},
  {J2K_MS_SOP, 0, 0},
  {J2K_MS_CRG, J2K_STATE_MH, j2k_read_crg},
  {J2K_MS_COM, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_com},

#ifdef USE_JPWL
  {J2K_MS_EPC, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_epc},
  {J2K_MS_EPB, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_epb},
  {J2K_MS_ESD, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_esd},
  {J2K_MS_RED, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_red},
#endif /* USE_JPWL */
#ifdef USE_JPSEC
  {J2K_MS_SEC, J2K_STATE_MH, j2k_read_sec},
  {J2K_MS_INSEC, 0, j2k_read_insec},
#endif /* USE_JPSEC */

  {0, J2K_STATE_MH | J2K_STATE_TPH, j2k_read_unk}
};

static void j2k_read_unk(opj_j2k_t *j2k) {
	opj_event_msg(j2k->cinfo, EVT_WARNING, "Unknown marker\n");

#ifdef USE_JPWL
	if (j2k->cp->correct) {
		int m = 0, id, i;
		int min_id = 0, min_dist = 17, cur_dist = 0, tmp_id;
		cio_seek(j2k->cio, cio_tell(j2k->cio) - 2);
		id = cio_read(j2k->cio, 2);
		opj_event_msg(j2k->cinfo, EVT_ERROR,
			"JPWL: really don't know this marker %x\n",
			id);
		if (!JPWL_ASSUME) {
			opj_event_msg(j2k->cinfo, EVT_ERROR,
				"- possible synch loss due to uncorrectable codestream errors => giving up\n");
			return;
		}
		/* OK, activate this at your own risk!!! */
		/* we look for the marker at the minimum hamming distance from this */
		while (j2k_dec_mstab[m].id) {
			
			/* 1's where they differ */
			tmp_id = j2k_dec_mstab[m].id ^ id;

			/* compute the hamming distance between our id and the current */
			cur_dist = 0;
			for (i = 0; i < 16; i++) {
				if ((tmp_id >> i) & 0x0001) {
					cur_dist++;
				}
			}

			/* if current distance is smaller, set the minimum */
			if (cur_dist < min_dist) {
				min_dist = cur_dist;
				min_id = j2k_dec_mstab[m].id;
			}
			
			/* jump to the next marker */
			m++;
		}

		/* do we substitute the marker? */
		if (min_dist < JPWL_MAXIMUM_HAMMING) {
			opj_event_msg(j2k->cinfo, EVT_ERROR,
				"- marker %x is at distance %d from the read %x\n",
				min_id, min_dist, id);
			opj_event_msg(j2k->cinfo, EVT_ERROR,
				"- trying to substitute in place and crossing fingers!\n");
			cio_seek(j2k->cio, cio_tell(j2k->cio) - 2);
			cio_write(j2k->cio, min_id, 2);

			/* rewind */
			cio_seek(j2k->cio, cio_tell(j2k->cio) - 2);

		}

	};
#endif /* USE_JPWL */

}

/**
 * Reads an unknown marker
 *
 * @param	p_stream				the stream object to read from.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_manager		the user event manager.
 *
 * @return	true			if the marker could be deduced.
*/
opj_bool j2k_read_unk_v2 (	opj_j2k_v2_t *p_j2k,
							struct opj_stream_private *p_stream,
							OPJ_UINT32 *output_marker,
							struct opj_event_mgr * p_manager
							)
{
	OPJ_UINT32 l_unknown_marker;
	const opj_dec_memory_marker_handler_t * l_marker_handler;
	OPJ_UINT32 l_size_unk = 2;

	/* preconditions*/
	assert(p_j2k != 00);
	assert(p_manager != 00);
	assert(p_stream != 00);

	opj_event_msg_v2(p_manager, EVT_WARNING, "Unknown marker\n");

	while(1) {
		/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer*/
		if (opj_stream_read_data(p_stream,p_j2k->m_specific_param.m_decoder.m_header_data,2,p_manager) != 2) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
			return OPJ_FALSE;
		}

		/* read 2 bytes as the new marker ID*/
		opj_read_bytes(p_j2k->m_specific_param.m_decoder.m_header_data,&l_unknown_marker,2);

		if (!(l_unknown_marker < 0xff00)) {

			/* Get the marker handler from the marker ID*/
			l_marker_handler = j2k_get_marker_handler(l_unknown_marker);

			if (!(p_j2k->m_specific_param.m_decoder.m_state & l_marker_handler->states)) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Marker is not compliant with its position\n");
				return OPJ_FALSE;
			}
			else {
				if (l_marker_handler->id != J2K_MS_UNK) {
					/* Add the marker to the codestream index*/
					if (l_marker_handler->id != J2K_MS_SOT)
						j2k_add_mhmarker_v2(p_j2k->cstr_index, J2K_MS_UNK,
											(OPJ_UINT32) opj_stream_tell(p_stream) - l_size_unk,
											l_size_unk);
					break; /* next marker is known and well located */
				}
				else
					l_size_unk += 2;
			}
		}
	}

	*output_marker = l_marker_handler->id ;

	return OPJ_TRUE;
}

/**
Read the lookup table containing all the marker, status and action
@param id Marker value
*/
static opj_dec_mstabent_t *j2k_dec_mstab_lookup(int id) {
	opj_dec_mstabent_t *e;
	for (e = j2k_dec_mstab; e->id != 0; e++) {
		if (e->id == id) {
			break;
		}
	}
	return e;
}


/**
 * Reads a MCT marker (Multiple Component Transform)
 *
 * @param	p_header_data	the data contained in the MCT box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the MCT marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_mct (	opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager )
{
	OPJ_UINT32 i;
	opj_tcp_v2_t *l_tcp = 00;
	OPJ_UINT32 l_tmp;
	OPJ_UINT32 l_indix;
	opj_mct_data_t * l_mct_data;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);

	l_tcp = p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH ?
			&p_j2k->m_cp.tcps[p_j2k->m_current_tile_number] :
			p_j2k->m_specific_param.m_decoder.m_default_tcp;

	if (p_header_size < 2) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCT marker\n");
		return OPJ_FALSE;
	}

	/* first marker */
	opj_read_bytes(p_header_data,&l_tmp,2);				/* Zmct */
	p_header_data += 2;
	if (l_tmp != 0) {
		opj_event_msg_v2(p_manager, EVT_WARNING, "Cannot take in charge mct data within multiple MCT records\n");
		return OPJ_TRUE;
	}

	if(p_header_size <= 6) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCT marker\n");
		return OPJ_FALSE;
	}

	/* Imct -> no need for other values, take the first, type is double with decorrelation x0000 1101 0000 0000*/
	opj_read_bytes(p_header_data,&l_tmp,2); 			/* Imct */
	p_header_data += 2;

	l_indix = l_tmp & 0xff;
	l_mct_data = l_tcp->m_mct_records;

	for (i=0;i<l_tcp->m_nb_mct_records;++i) {
		if (l_mct_data->m_index == l_indix) {
			break;
		}
		++l_mct_data;
	}

	/* NOT FOUND */
	if (i == l_tcp->m_nb_mct_records) {
		if (l_tcp->m_nb_mct_records == l_tcp->m_nb_max_mct_records) {
			l_tcp->m_nb_max_mct_records += J2K_MCT_DEFAULT_NB_RECORDS;

			l_tcp->m_mct_records = (opj_mct_data_t*)opj_realloc(l_tcp->m_mct_records,l_tcp->m_nb_max_mct_records * sizeof(opj_mct_data_t));
			if(! l_tcp->m_mct_records) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCT marker\n");
				return OPJ_FALSE;
			}

			l_mct_data = l_tcp->m_mct_records + l_tcp->m_nb_mct_records;
			memset(l_mct_data ,0,(l_tcp->m_nb_max_mct_records - l_tcp->m_nb_mct_records) * sizeof(opj_mct_data_t));
		}

		l_mct_data = l_tcp->m_mct_records + l_tcp->m_nb_mct_records;
	}

	if (l_mct_data->m_data) {
		opj_free(l_mct_data->m_data);
		l_mct_data->m_data = 00;
	}

	l_mct_data->m_index = l_indix;
	l_mct_data->m_array_type = (J2K_MCT_ARRAY_TYPE)((l_tmp  >> 8) & 3);
	l_mct_data->m_element_type = (J2K_MCT_ELEMENT_TYPE)((l_tmp  >> 10) & 3);

	opj_read_bytes(p_header_data,&l_tmp,2);				/* Ymct */
	p_header_data+=2;
	if (l_tmp != 0) {
		opj_event_msg_v2(p_manager, EVT_WARNING, "Cannot take in charge multiple MCT markers\n");
		return OPJ_TRUE;
	}

	p_header_size -= 6;

	l_mct_data->m_data = (OPJ_BYTE*)opj_malloc(p_header_size);
	if (! l_mct_data->m_data) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCT marker\n");
		return OPJ_FALSE;
	}
	memcpy(l_mct_data->m_data,p_header_data,p_header_size);

	l_mct_data->m_data_size = p_header_size;
	++l_tcp->m_nb_mct_records;

	return OPJ_TRUE;
}

/**
 * Reads a MCC marker (Multiple Component Collection)
 *
 * @param	p_header_data	the data contained in the MCC box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the MCC marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_mcc (	opj_j2k_v2_t *p_j2k,
					OPJ_BYTE * p_header_data,
					OPJ_UINT32 p_header_size,
					struct opj_event_mgr * p_manager )
{
	OPJ_UINT32 i,j;
	OPJ_UINT32 l_tmp;
	OPJ_UINT32 l_indix;
	opj_tcp_v2_t * l_tcp;
	opj_simple_mcc_decorrelation_data_t * l_mcc_record;
	opj_mct_data_t * l_mct_data;
	OPJ_UINT32 l_nb_collections;
	OPJ_UINT32 l_nb_comps;
	OPJ_UINT32 l_nb_bytes_by_comp;


	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	l_tcp = p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH ?
			&p_j2k->m_cp.tcps[p_j2k->m_current_tile_number] :
			p_j2k->m_specific_param.m_decoder.m_default_tcp;

	if (p_header_size < 2) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCC marker\n");
		return OPJ_FALSE;
	}

	/* first marker */
	opj_read_bytes(p_header_data,&l_tmp,2);				/* Zmcc */
	p_header_data += 2;
	if (l_tmp != 0) {
		opj_event_msg_v2(p_manager, EVT_WARNING, "Cannot take in charge multiple data spanning\n");
		return OPJ_TRUE;
	}

	if (p_header_size < 7) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCC marker\n");
		return OPJ_FALSE;
	}

	opj_read_bytes(p_header_data,&l_indix,1); /* Imcc -> no need for other values, take the first */
	++p_header_data;

	l_mcc_record = l_tcp->m_mcc_records;

	for(i=0;i<l_tcp->m_nb_mcc_records;++i) {
		if (l_mcc_record->m_index == l_indix) {
			break;
		}
		++l_mcc_record;
	}

	/** NOT FOUND */
	if (i == l_tcp->m_nb_mcc_records) {
		if (l_tcp->m_nb_mcc_records == l_tcp->m_nb_max_mcc_records) {
			l_tcp->m_nb_max_mcc_records += J2K_MCC_DEFAULT_NB_RECORDS;

			l_tcp->m_mcc_records = (opj_simple_mcc_decorrelation_data_t*)
					opj_realloc(l_tcp->m_mcc_records,l_tcp->m_nb_max_mcc_records * sizeof(opj_simple_mcc_decorrelation_data_t));
			if (! l_tcp->m_mcc_records) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCC marker\n");
				return OPJ_FALSE;
			}
			l_mcc_record = l_tcp->m_mcc_records + l_tcp->m_nb_mcc_records;
			memset(l_mcc_record,0,(l_tcp->m_nb_max_mcc_records-l_tcp->m_nb_mcc_records) * sizeof(opj_simple_mcc_decorrelation_data_t));
		}
		l_mcc_record = l_tcp->m_mcc_records + l_tcp->m_nb_mcc_records;
	}
	l_mcc_record->m_index = l_indix;

	/* only one marker atm */
	opj_read_bytes(p_header_data,&l_tmp,2);				/* Ymcc */
	p_header_data+=2;
	if (l_tmp != 0) {
		opj_event_msg_v2(p_manager, EVT_WARNING, "Cannot take in charge multiple data spanning\n");
		return OPJ_TRUE;
	}

	opj_read_bytes(p_header_data,&l_nb_collections,2);				/* Qmcc -> number of collections -> 1 */
	p_header_data+=2;

	if (l_nb_collections > 1) {
		opj_event_msg_v2(p_manager, EVT_WARNING, "Cannot take in charge multiple collections\n");
		return OPJ_TRUE;
	}

	p_header_size -= 7;

	for (i=0;i<l_nb_collections;++i) {
		if (p_header_size < 3) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCC marker\n");
			return OPJ_FALSE;
		}

		opj_read_bytes(p_header_data,&l_tmp,1);	/* Xmcci type of component transformation -> array based decorrelation */
		++p_header_data;

		if (l_tmp != 1) {
			opj_event_msg_v2(p_manager, EVT_WARNING, "Cannot take in charge collections other than array decorrelation\n");
			return OPJ_TRUE;
		}

		opj_read_bytes(p_header_data,&l_nb_comps,2);

		p_header_data+=2;
		p_header_size-=3;

		l_nb_bytes_by_comp = 1 + (l_nb_comps>>15);
		l_mcc_record->m_nb_comps = l_nb_comps & 0x7fff;

		if (p_header_size < (l_nb_bytes_by_comp * l_mcc_record->m_nb_comps + 2)) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCC marker\n");
			return OPJ_FALSE;
		}

		p_header_size -= (l_nb_bytes_by_comp * l_mcc_record->m_nb_comps + 2);

		for (j=0;j<l_mcc_record->m_nb_comps;++j) {
			opj_read_bytes(p_header_data,&l_tmp,l_nb_bytes_by_comp);	/* Cmccij Component offset*/
			p_header_data+=l_nb_bytes_by_comp;

			if (l_tmp != j) {
				opj_event_msg_v2(p_manager, EVT_WARNING, "Cannot take in charge collections with indix shuffle\n");
				return OPJ_TRUE;
			}
		}

		opj_read_bytes(p_header_data,&l_nb_comps,2);
		p_header_data+=2;

		l_nb_bytes_by_comp = 1 + (l_nb_comps>>15);
		l_nb_comps &= 0x7fff;

		if (l_nb_comps != l_mcc_record->m_nb_comps) {
			opj_event_msg_v2(p_manager, EVT_WARNING, "Cannot take in charge collections without same number of indixes\n");
			return OPJ_TRUE;
		}

		if (p_header_size < (l_nb_bytes_by_comp * l_mcc_record->m_nb_comps + 3)) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCC marker\n");
			return OPJ_FALSE;
		}

		p_header_size -= (l_nb_bytes_by_comp * l_mcc_record->m_nb_comps + 3);

		for (j=0;j<l_mcc_record->m_nb_comps;++j) {
			opj_read_bytes(p_header_data,&l_tmp,l_nb_bytes_by_comp);	/* Wmccij Component offset*/
			p_header_data+=l_nb_bytes_by_comp;

			if (l_tmp != j) {
				opj_event_msg_v2(p_manager, EVT_WARNING, "Cannot take in charge collections with indix shuffle\n");
				return OPJ_TRUE;
			}
		}

		opj_read_bytes(p_header_data,&l_tmp,3);	/* Wmccij Component offset*/
		p_header_data += 3;

		l_mcc_record->m_is_irreversible = ! ((l_tmp>>16) & 1);
		l_mcc_record->m_decorrelation_array = 00;
		l_mcc_record->m_offset_array = 00;

		l_indix = l_tmp & 0xff;
		if (l_indix != 0) {
			l_mct_data = l_tcp->m_mct_records;
			for (j=0;j<l_tcp->m_nb_mct_records;++j) {
				if (l_mct_data->m_index == l_indix) {
					l_mcc_record->m_decorrelation_array = l_mct_data;
					break;
				}
				++l_mct_data;
			}

			if (l_mcc_record->m_decorrelation_array == 00) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCC marker\n");
				return OPJ_FALSE;
			}
		}

		l_indix = (l_tmp >> 8) & 0xff;
		if (l_indix != 0) {
			l_mct_data = l_tcp->m_mct_records;
			for (j=0;j<l_tcp->m_nb_mct_records;++j) {
				if (l_mct_data->m_index == l_indix) {
					l_mcc_record->m_offset_array = l_mct_data;
					break;
				}
				++l_mct_data;
			}

			if (l_mcc_record->m_offset_array == 00) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCC marker\n");
				return OPJ_FALSE;
			}
		}
	}

	if (p_header_size != 0) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCC marker\n");
		return OPJ_FALSE;
	}

	++l_tcp->m_nb_mcc_records;

	return OPJ_TRUE;
}

/**
 * Reads a MCO marker (Multiple Component Transform Ordering)
 *
 * @param	p_header_data	the data contained in the MCO box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the MCO marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_mco (	opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager )
{
	OPJ_UINT32 l_tmp, i;
	OPJ_UINT32 l_nb_stages;
	opj_tcp_v2_t * l_tcp;
	opj_tccp_t * l_tccp;
	opj_image_t * l_image;
	opj_image_comp_t * l_img_comp;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	l_image = p_j2k->m_private_image;
	l_tcp = p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH ?
			&p_j2k->m_cp.tcps[p_j2k->m_current_tile_number] :
			p_j2k->m_specific_param.m_decoder.m_default_tcp;

	if (p_header_size < 1) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading MCO marker\n");
		return OPJ_FALSE;
	}

	opj_read_bytes(p_header_data,&l_nb_stages,1);				/* Nmco : only one tranform stage*/
	++p_header_data;

	if (l_nb_stages > 1) {
		opj_event_msg_v2(p_manager, EVT_WARNING, "Cannot take in charge multiple transformation stages.\n");
		return OPJ_TRUE;
	}

	if (p_header_size != l_nb_stages + 1) {
		opj_event_msg_v2(p_manager, EVT_WARNING, "Error reading MCO marker\n");
		return OPJ_FALSE;
	}

	l_tccp = l_tcp->tccps;
	l_img_comp = l_image->comps;

	for (i=0;i<l_image->numcomps;++i) {
		l_tccp->m_dc_level_shift = 0;
		++l_tccp;
	}

	if (l_tcp->m_mct_decoding_matrix) {
		opj_free(l_tcp->m_mct_decoding_matrix);
		l_tcp->m_mct_decoding_matrix = 00;
	}

	for (i=0;i<l_nb_stages;++i) {
		opj_read_bytes(p_header_data,&l_tmp,1);
		++p_header_data;

		if (! j2k_add_mct(l_tcp,p_j2k->m_private_image,l_tmp)) {
			return OPJ_FALSE;
		}
	}

	return OPJ_TRUE;
}

opj_bool j2k_add_mct(opj_tcp_v2_t * p_tcp, opj_image_t * p_image, OPJ_UINT32 p_index)
{
	OPJ_UINT32 i;
	opj_simple_mcc_decorrelation_data_t * l_mcc_record;
	opj_mct_data_t * l_deco_array, * l_offset_array;
	OPJ_UINT32 l_data_size,l_mct_size, l_offset_size;
	OPJ_UINT32 l_nb_elem;
	OPJ_UINT32 * l_offset_data, * l_current_offset_data;
	opj_tccp_t * l_tccp;

	/* preconditions */
	assert(p_tcp != 00);

	l_mcc_record = p_tcp->m_mcc_records;

	for (i=0;i<p_tcp->m_nb_mcc_records;++i) {
		if (l_mcc_record->m_index == p_index) {
			break;
		}
	}

	if (i==p_tcp->m_nb_mcc_records) {
		/** element discarded **/
		return OPJ_TRUE;
	}

	if (l_mcc_record->m_nb_comps != p_image->numcomps) {
		/** do not support number of comps != image */
		return OPJ_TRUE;
	}

	l_deco_array = l_mcc_record->m_decorrelation_array;

	if (l_deco_array) {
		l_data_size = MCT_ELEMENT_SIZE[l_deco_array->m_element_type] * p_image->numcomps * p_image->numcomps;
		if (l_deco_array->m_data_size != l_data_size) {
			return OPJ_FALSE;
		}

		l_nb_elem = p_image->numcomps * p_image->numcomps;
		l_mct_size = l_nb_elem * sizeof(OPJ_FLOAT32);
		p_tcp->m_mct_decoding_matrix = (OPJ_FLOAT32*)opj_malloc(l_mct_size);

		if (! p_tcp->m_mct_decoding_matrix ) {
			return OPJ_FALSE;
		}

		j2k_mct_read_functions_to_float[l_deco_array->m_element_type](l_deco_array->m_data,p_tcp->m_mct_decoding_matrix,l_nb_elem);
	}

	l_offset_array = l_mcc_record->m_offset_array;

	if (l_offset_array) {
		l_data_size = MCT_ELEMENT_SIZE[l_offset_array->m_element_type] * p_image->numcomps;
		if (l_offset_array->m_data_size != l_data_size) {
			return OPJ_FALSE;
		}

		l_nb_elem = p_image->numcomps;
		l_offset_size = l_nb_elem * sizeof(OPJ_UINT32);
		l_offset_data = (OPJ_UINT32*)opj_malloc(l_offset_size);

		if (! l_offset_data ) {
			return OPJ_FALSE;
		}

		j2k_mct_read_functions_to_int32[l_offset_array->m_element_type](l_offset_array->m_data,l_offset_data,l_nb_elem);

		l_tccp = p_tcp->tccps;
		l_current_offset_data = l_offset_data;

		for (i=0;i<p_image->numcomps;++i) {
			l_tccp->m_dc_level_shift = *(l_current_offset_data++);
			++l_tccp;
		}

		opj_free(l_offset_data);
	}

	return OPJ_TRUE;
}

/**
 * Reads a CBD marker (Component bit depth definition)
 * @param	p_header_data	the data contained in the CBD box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the CBD marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_cbd (	opj_j2k_v2_t *p_j2k,
						OPJ_BYTE * p_header_data,
						OPJ_UINT32 p_header_size,
						struct opj_event_mgr * p_manager)
{
	OPJ_UINT32 l_nb_comp,l_num_comp;
	OPJ_UINT32 l_comp_def;
	OPJ_UINT32 i;
	opj_image_comp_t * l_comp = 00;

	/* preconditions */
	assert(p_header_data != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	l_num_comp = p_j2k->m_private_image->numcomps;

	if (p_header_size != (p_j2k->m_private_image->numcomps + 2)) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Crror reading CBD marker\n");
		return OPJ_FALSE;
	}

	opj_read_bytes(p_header_data,&l_nb_comp,2);				/* Ncbd */
	p_header_data+=2;

	if (l_nb_comp != l_num_comp) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Crror reading CBD marker\n");
		return OPJ_FALSE;
	}

	l_comp = p_j2k->m_private_image->comps;
	for (i=0;i<l_num_comp;++i) {
		opj_read_bytes(p_header_data,&l_comp_def,1);			/* Component bit depth */
		++p_header_data;
        l_comp->sgnd = (l_comp_def>>7) & 1;
		l_comp->prec = (l_comp_def&0x7f) + 1;
		++l_comp;
	}

	return OPJ_TRUE;
}


/* ----------------------------------------------------------------------- */
/* J2K / JPT decoder interface                                             */
/* ----------------------------------------------------------------------- */

opj_j2k_t* j2k_create_decompress(opj_common_ptr cinfo) {
	opj_j2k_t *j2k = (opj_j2k_t*) opj_calloc(1, sizeof(opj_j2k_t));
	if(!j2k)
		return NULL;

	j2k->default_tcp = (opj_tcp_t*) opj_calloc(1, sizeof(opj_tcp_t));
	if(!j2k->default_tcp) {
		opj_free(j2k);
		return NULL;
	}

	j2k->cinfo = cinfo;
	j2k->tile_data = NULL;

	return j2k;
}

void j2k_destroy_decompress(opj_j2k_t *j2k) {
	int i = 0;

	if(j2k->tile_len != NULL) {
		opj_free(j2k->tile_len);
	}
	if(j2k->tile_data != NULL) {
		opj_free(j2k->tile_data);
	}
	if(j2k->default_tcp != NULL) {
		opj_tcp_t *default_tcp = j2k->default_tcp;
		if(default_tcp->ppt_data_first != NULL) {
			opj_free(default_tcp->ppt_data_first);
		}
		if(j2k->default_tcp->tccps != NULL) {
			opj_free(j2k->default_tcp->tccps);
		}
		opj_free(j2k->default_tcp);
	}
	if(j2k->cp != NULL) {
		opj_cp_t *cp = j2k->cp;
		if(cp->tcps != NULL) {
			for(i = 0; i < cp->tw * cp->th; i++) {
				if(cp->tcps[i].ppt_data_first != NULL) {
					opj_free(cp->tcps[i].ppt_data_first);
				}
				if(cp->tcps[i].tccps != NULL) {
					opj_free(cp->tcps[i].tccps);
				}
			}
			opj_free(cp->tcps);
		}
		if(cp->ppm_data_first != NULL) {
			opj_free(cp->ppm_data_first);
		}
		if(cp->tileno != NULL) {
			opj_free(cp->tileno);  
		}
		if(cp->comment != NULL) {
			opj_free(cp->comment);
		}

		opj_free(cp);
	}
	opj_free(j2k);
}

void j2k_setup_decoder(opj_j2k_t *j2k, opj_dparameters_t *parameters) {
	if(j2k && parameters) {
		/* create and initialize the coding parameters structure */
		opj_cp_t *cp = (opj_cp_t*) opj_calloc(1, sizeof(opj_cp_t));
		cp->reduce = parameters->cp_reduce;	
		cp->layer = parameters->cp_layer;
		cp->limit_decoding = parameters->cp_limit_decoding;

#ifdef USE_JPWL
		cp->correct = parameters->jpwl_correct;
		cp->exp_comps = parameters->jpwl_exp_comps;
		cp->max_tiles = parameters->jpwl_max_tiles;
#endif /* USE_JPWL */


		/* keep a link to cp so that we can destroy it later in j2k_destroy_decompress */
		j2k->cp = cp;
	}
}

void j2k_setup_decoder_v2(opj_j2k_v2_t *j2k, opj_dparameters_t *parameters)
{
	if(j2k && parameters) {
		j2k->m_cp.m_specific_param.m_dec.m_layer = parameters->cp_layer;
		j2k->m_cp.m_specific_param.m_dec.m_reduce = parameters->cp_reduce;

#ifdef USE_JPWL
		j2k->m_cp.correct = parameters->jpwl_correct;
		j2k->m_cp.exp_comps = parameters->jpwl_exp_comps;
		j2k->m_cp.max_tiles = parameters->jpwl_max_tiles;
#endif /* USE_JPWL */
	}
}

opj_image_t* j2k_decode(opj_j2k_t *j2k, opj_cio_t *cio, opj_codestream_info_t *cstr_info) {
	opj_image_t *image = NULL;

	opj_common_ptr cinfo = j2k->cinfo;	

	j2k->cio = cio;
	j2k->cstr_info = cstr_info;
	if (cstr_info)
		memset(cstr_info, 0, sizeof(opj_codestream_info_t));

	/* create an empty image */
	image = opj_image_create0();
	j2k->image = image;

	j2k->state = J2K_STATE_MHSOC;

	for (;;) {
		opj_dec_mstabent_t *e;
		int id = cio_read(cio, 2);

#ifdef USE_JPWL
		/* we try to honor JPWL correction power */
		if (j2k->cp->correct) {

			int orig_pos = cio_tell(cio);
			opj_bool status;

			/* call the corrector */
			status = jpwl_correct(j2k);

			/* go back to where you were */
			cio_seek(cio, orig_pos - 2);

			/* re-read the marker */
			id = cio_read(cio, 2);

			/* check whether it begins with ff */
			if (id >> 8 != 0xff) {
				opj_event_msg(cinfo, EVT_ERROR,
					"JPWL: possible bad marker %x at %d\n",
					id, cio_tell(cio) - 2);
				if (!JPWL_ASSUME) {
					opj_image_destroy(image);
					opj_event_msg(cinfo, EVT_ERROR, "JPWL: giving up\n");
					return 0;
				}
				/* we try to correct */
				id = id | 0xff00;
				cio_seek(cio, cio_tell(cio) - 2);
				cio_write(cio, id, 2);
				opj_event_msg(cinfo, EVT_WARNING, "- trying to adjust this\n"
					"- setting marker to %x\n",
					id);
			}

		}
#endif /* USE_JPWL */

		if (id >> 8 != 0xff) {
			opj_image_destroy(image);
			opj_event_msg(cinfo, EVT_ERROR, "%.8x: expected a marker instead of %x\n", cio_tell(cio) - 2, id);
			return 0;
		}
		e = j2k_dec_mstab_lookup(id);
		/* Check if the marker is known*/
		if (!(j2k->state & e->states)) {
			opj_image_destroy(image);
			opj_event_msg(cinfo, EVT_ERROR, "%.8x: unexpected marker %x\n", cio_tell(cio) - 2, id);
			return 0;
		}
		/* Check if the decoding is limited to the main header*/
		if (e->id == J2K_MS_SOT && j2k->cp->limit_decoding == LIMIT_TO_MAIN_HEADER) {
			opj_event_msg(cinfo, EVT_INFO, "Main Header decoded.\n");
			return image;
		}		

		if (e->handler) {
			(*e->handler)(j2k);
		}
		if (j2k->state & J2K_STATE_ERR) 
			return NULL;	

		if (j2k->state == J2K_STATE_MT) {
			break;
		}
		if (j2k->state == J2K_STATE_NEOC) {
			break;
		}
	}
	if (j2k->state == J2K_STATE_NEOC) {
		j2k_read_eoc(j2k);
	}

	if (j2k->state != J2K_STATE_MT) {
		opj_event_msg(cinfo, EVT_WARNING, "Incomplete bitstream\n");
	}
	return image;
}

/*
* Read a JPT-stream and decode file
*
*/
opj_image_t* j2k_decode_jpt_stream(opj_j2k_t *j2k, opj_cio_t *cio,  opj_codestream_info_t *cstr_info) {
	opj_image_t *image = NULL;
	opj_jpt_msg_header_t header;
	int position;
	opj_common_ptr cinfo = j2k->cinfo;

	OPJ_ARG_NOT_USED(cstr_info);

	j2k->cio = cio;

	/* create an empty image */
	image = opj_image_create0();
	j2k->image = image;

	j2k->state = J2K_STATE_MHSOC;
	
	/* Initialize the header */
	jpt_init_msg_header(&header);
	/* Read the first header of the message */
	jpt_read_msg_header(cinfo, cio, &header);
	
	position = cio_tell(cio);
	if (header.Class_Id != 6) {	/* 6 : Main header data-bin message */
		opj_image_destroy(image);
		opj_event_msg(cinfo, EVT_ERROR, "[JPT-stream] : Expecting Main header first [class_Id %d] !\n", header.Class_Id);
		return 0;
	}
	
	for (;;) {
		opj_dec_mstabent_t *e = NULL;
		int id;
		
		if (!cio_numbytesleft(cio)) {
			j2k_read_eoc(j2k);
			return image;
		}
		/* data-bin read -> need to read a new header */
		if ((unsigned int) (cio_tell(cio) - position) == header.Msg_length) {
			jpt_read_msg_header(cinfo, cio, &header);
			position = cio_tell(cio);
			if (header.Class_Id != 4) {	/* 4 : Tile data-bin message */
				opj_image_destroy(image);
				opj_event_msg(cinfo, EVT_ERROR, "[JPT-stream] : Expecting Tile info !\n");
				return 0;
			}
		}
		
		id = cio_read(cio, 2);
		if (id >> 8 != 0xff) {
			opj_image_destroy(image);
			opj_event_msg(cinfo, EVT_ERROR, "%.8x: expected a marker instead of %x\n", cio_tell(cio) - 2, id);
			return 0;
		}
		e = j2k_dec_mstab_lookup(id);
		if (!(j2k->state & e->states)) {
			opj_image_destroy(image);
			opj_event_msg(cinfo, EVT_ERROR, "%.8x: unexpected marker %x\n", cio_tell(cio) - 2, id);
			return 0;
		}
		if (e->handler) {
			(*e->handler)(j2k);
		}
		if (j2k->state == J2K_STATE_MT) {
			break;
		}
		if (j2k->state == J2K_STATE_NEOC) {
			break;
		}
	}
	if (j2k->state == J2K_STATE_NEOC) {
		j2k_read_eoc(j2k);
	}
	
	if (j2k->state != J2K_STATE_MT) {
		opj_event_msg(cinfo, EVT_WARNING, "Incomplete bitstream\n");
	}

	return image;
}

/* ----------------------------------------------------------------------- */
/* J2K encoder interface                                                       */
/* ----------------------------------------------------------------------- */

opj_j2k_t* j2k_create_compress(opj_common_ptr cinfo) {
	opj_j2k_t *j2k = (opj_j2k_t*) opj_calloc(1, sizeof(opj_j2k_t));
	if(j2k) {
		j2k->cinfo = cinfo;
	}
	return j2k;
}

opj_j2k_v2_t* j2k_create_compress_v2()
{
	opj_j2k_v2_t *l_j2k = (opj_j2k_v2_t*) opj_malloc(sizeof(opj_j2k_v2_t));
	if (!l_j2k) {
		return NULL;
	}

	memset(l_j2k,0,sizeof(opj_j2k_v2_t));

	l_j2k->m_is_decoder = 0;
	l_j2k->m_cp.m_is_decoder = 0;

	l_j2k->m_specific_param.m_encoder.m_header_tile_data = (OPJ_BYTE *) opj_malloc(J2K_DEFAULT_HEADER_SIZE);
	if (! l_j2k->m_specific_param.m_encoder.m_header_tile_data) {
		j2k_destroy(l_j2k);
		return NULL;
	}

	l_j2k->m_specific_param.m_encoder.m_header_tile_data_size = J2K_DEFAULT_HEADER_SIZE;

	/* validation list creation*/
	l_j2k->m_validation_list = opj_procedure_list_create();
	if (! l_j2k->m_validation_list) {
		j2k_destroy(l_j2k);
		return NULL;
	}

	/* execution list creation*/
	l_j2k->m_procedure_list = opj_procedure_list_create();
	if (! l_j2k->m_procedure_list) {
		j2k_destroy(l_j2k);
		return NULL;
	}

	return l_j2k;
}

void j2k_destroy_compress(opj_j2k_t *j2k) {
	int tileno;

	if(!j2k) return;
	if(j2k->cp != NULL) {
		opj_cp_t *cp = j2k->cp;

		if(cp->comment) {
			opj_free(cp->comment);
		}
		if(cp->matrice) {
			opj_free(cp->matrice);
		}
		for (tileno = 0; tileno < cp->tw * cp->th; tileno++) {
			opj_free(cp->tcps[tileno].tccps);
		}
		opj_free(cp->tcps);
		opj_free(cp);
	}

	opj_free(j2k);
}

void j2k_setup_encoder(opj_j2k_t *j2k, opj_cparameters_t *parameters, opj_image_t *image) {
	int i, j, tileno, numpocs_tile;
	opj_cp_t *cp = NULL;

	if(!j2k || !parameters || ! image) {
		return;
	}

	/* create and initialize the coding parameters structure */
	cp = (opj_cp_t*) opj_calloc(1, sizeof(opj_cp_t));

	/* keep a link to cp so that we can destroy it later in j2k_destroy_compress */
	j2k->cp = cp;

	/* set default values for cp */
	cp->tw = 1;
	cp->th = 1;

	/* 
	copy user encoding parameters 
	*/
	cp->cinema = parameters->cp_cinema;
	cp->max_comp_size =	parameters->max_comp_size;
	cp->rsiz   = parameters->cp_rsiz;
	cp->disto_alloc = parameters->cp_disto_alloc;
	cp->fixed_alloc = parameters->cp_fixed_alloc;
	cp->fixed_quality = parameters->cp_fixed_quality;

	/* mod fixed_quality */
	if(parameters->cp_matrice) {
		size_t array_size = parameters->tcp_numlayers * parameters->numresolution * 3 * sizeof(int);
		cp->matrice = (int *) opj_malloc(array_size);
		memcpy(cp->matrice, parameters->cp_matrice, array_size);
	}

	/* tiles */
	cp->tdx = parameters->cp_tdx;
	cp->tdy = parameters->cp_tdy;

	/* tile offset */
	cp->tx0 = parameters->cp_tx0;
	cp->ty0 = parameters->cp_ty0;

	/* comment string */
	if(parameters->cp_comment) {
		cp->comment = (char*)opj_malloc(strlen(parameters->cp_comment) + 1);
		if(cp->comment) {
			strcpy(cp->comment, parameters->cp_comment);
		}
	}

	/*
	calculate other encoding parameters
	*/

	if (parameters->tile_size_on) {
		cp->tw = int_ceildiv(image->x1 - cp->tx0, cp->tdx);
		cp->th = int_ceildiv(image->y1 - cp->ty0, cp->tdy);
	} else {
		cp->tdx = image->x1 - cp->tx0;
		cp->tdy = image->y1 - cp->ty0;
	}

	if(parameters->tp_on){
		cp->tp_flag = parameters->tp_flag;
		cp->tp_on = 1;
	}
	
	cp->img_size = 0;
	for(i=0;i<image->numcomps ;i++){
	cp->img_size += (image->comps[i].w *image->comps[i].h * image->comps[i].prec);
	}


#ifdef USE_JPWL
	/*
	calculate JPWL encoding parameters
	*/

	if (parameters->jpwl_epc_on) {
		int i;

		/* set JPWL on */
		cp->epc_on = OPJ_TRUE;
		cp->info_on = OPJ_FALSE; /* no informative technique */

		/* set EPB on */
		if ((parameters->jpwl_hprot_MH > 0) || (parameters->jpwl_hprot_TPH[0] > 0)) {
			cp->epb_on = OPJ_TRUE;
			
			cp->hprot_MH = parameters->jpwl_hprot_MH;
			for (i = 0; i < JPWL_MAX_NO_TILESPECS; i++) {
				cp->hprot_TPH_tileno[i] = parameters->jpwl_hprot_TPH_tileno[i];
				cp->hprot_TPH[i] = parameters->jpwl_hprot_TPH[i];
			}
			/* if tile specs are not specified, copy MH specs */
			if (cp->hprot_TPH[0] == -1) {
				cp->hprot_TPH_tileno[0] = 0;
				cp->hprot_TPH[0] = parameters->jpwl_hprot_MH;
			}
			for (i = 0; i < JPWL_MAX_NO_PACKSPECS; i++) {
				cp->pprot_tileno[i] = parameters->jpwl_pprot_tileno[i];
				cp->pprot_packno[i] = parameters->jpwl_pprot_packno[i];
				cp->pprot[i] = parameters->jpwl_pprot[i];
			}
		}

		/* set ESD writing */
		if ((parameters->jpwl_sens_size == 1) || (parameters->jpwl_sens_size == 2)) {
			cp->esd_on = OPJ_TRUE;

			cp->sens_size = parameters->jpwl_sens_size;
			cp->sens_addr = parameters->jpwl_sens_addr;
			cp->sens_range = parameters->jpwl_sens_range;

			cp->sens_MH = parameters->jpwl_sens_MH;
			for (i = 0; i < JPWL_MAX_NO_TILESPECS; i++) {
				cp->sens_TPH_tileno[i] = parameters->jpwl_sens_TPH_tileno[i];
				cp->sens_TPH[i] = parameters->jpwl_sens_TPH[i];
			}
		}

		/* always set RED writing to false: we are at the encoder */
		cp->red_on = OPJ_FALSE;

	} else {
		cp->epc_on = OPJ_FALSE;
	}
#endif /* USE_JPWL */


	/* initialize the mutiple tiles */
	/* ---------------------------- */
	cp->tcps = (opj_tcp_t*) opj_calloc(cp->tw * cp->th, sizeof(opj_tcp_t));

	for (tileno = 0; tileno < cp->tw * cp->th; tileno++) {
		opj_tcp_t *tcp = &cp->tcps[tileno];
		tcp->numlayers = parameters->tcp_numlayers;
		for (j = 0; j < tcp->numlayers; j++) {
			if(cp->cinema){
				if (cp->fixed_quality) {
					tcp->distoratio[j] = parameters->tcp_distoratio[j];
				}
				tcp->rates[j] = parameters->tcp_rates[j];
			}else{
				if (cp->fixed_quality) {	/* add fixed_quality */
					tcp->distoratio[j] = parameters->tcp_distoratio[j];
				} else {
					tcp->rates[j] = parameters->tcp_rates[j];
				}
			}
		}
		tcp->csty = parameters->csty;
		tcp->prg = parameters->prog_order;
		tcp->mct = parameters->tcp_mct; 

		numpocs_tile = 0;
		tcp->POC = 0;
		if (parameters->numpocs) {
			/* initialisation of POC */
			tcp->POC = 1;
			for (i = 0; i < parameters->numpocs; i++) {
				if((tileno == parameters->POC[i].tile - 1) || (parameters->POC[i].tile == -1)) {
					opj_poc_t *tcp_poc = &tcp->pocs[numpocs_tile];
					tcp_poc->resno0		= parameters->POC[numpocs_tile].resno0;
					tcp_poc->compno0	= parameters->POC[numpocs_tile].compno0;
					tcp_poc->layno1		= parameters->POC[numpocs_tile].layno1;
					tcp_poc->resno1		= parameters->POC[numpocs_tile].resno1;
					tcp_poc->compno1	= parameters->POC[numpocs_tile].compno1;
					tcp_poc->prg1		= parameters->POC[numpocs_tile].prg1;
					tcp_poc->tile		= parameters->POC[numpocs_tile].tile;
					numpocs_tile++;
				}
			}
			tcp->numpocs = numpocs_tile -1 ;
		}else{ 
			tcp->numpocs = 0;
		}

		tcp->tccps = (opj_tccp_t*) opj_calloc(image->numcomps, sizeof(opj_tccp_t));

		for (i = 0; i < image->numcomps; i++) {
			opj_tccp_t *tccp = &tcp->tccps[i];
			tccp->csty = parameters->csty & 0x01;	/* 0 => one precinct || 1 => custom precinct  */
			tccp->numresolutions = parameters->numresolution;
			tccp->cblkw = int_floorlog2(parameters->cblockw_init);
			tccp->cblkh = int_floorlog2(parameters->cblockh_init);
			tccp->cblksty = parameters->mode;
			tccp->qmfbid = parameters->irreversible ? 0 : 1;
			tccp->qntsty = parameters->irreversible ? J2K_CCP_QNTSTY_SEQNT : J2K_CCP_QNTSTY_NOQNT;
			tccp->numgbits = 2;
			if (i == parameters->roi_compno) {
				tccp->roishift = parameters->roi_shift;
			} else {
				tccp->roishift = 0;
			}

			if(parameters->cp_cinema)
			{
				/*Precinct size for lowest frequency subband=128*/
				tccp->prcw[0] = 7;
				tccp->prch[0] = 7;
				/*Precinct size at all other resolutions = 256*/
				for (j = 1; j < tccp->numresolutions; j++) {
					tccp->prcw[j] = 8;
					tccp->prch[j] = 8;
				}
			}else{
				if (parameters->csty & J2K_CCP_CSTY_PRT) {
					int p = 0;
					for (j = tccp->numresolutions - 1; j >= 0; j--) {
						if (p < parameters->res_spec) {
							
							if (parameters->prcw_init[p] < 1) {
								tccp->prcw[j] = 1;
							} else {
								tccp->prcw[j] = int_floorlog2(parameters->prcw_init[p]);
							}
							
							if (parameters->prch_init[p] < 1) {
								tccp->prch[j] = 1;
							}else {
								tccp->prch[j] = int_floorlog2(parameters->prch_init[p]);
							}

						} else {
							int res_spec = parameters->res_spec;
							int size_prcw = parameters->prcw_init[res_spec - 1] >> (p - (res_spec - 1));
							int size_prch = parameters->prch_init[res_spec - 1] >> (p - (res_spec - 1));
							
							if (size_prcw < 1) {
								tccp->prcw[j] = 1;
							} else {
								tccp->prcw[j] = int_floorlog2(size_prcw);
							}
							
							if (size_prch < 1) {
								tccp->prch[j] = 1;
							} else {
								tccp->prch[j] = int_floorlog2(size_prch);
							}
						}
						p++;
						/*printf("\nsize precinct for level %d : %d,%d\n", j,tccp->prcw[j], tccp->prch[j]); */
					}	/*end for*/
				} else {
					for (j = 0; j < tccp->numresolutions; j++) {
						tccp->prcw[j] = 15;
						tccp->prch[j] = 15;
					}
				}
			}

			dwt_calc_explicit_stepsizes(tccp, image->comps[i].prec);
		}
	}
}

opj_bool j2k_encode(opj_j2k_t *j2k, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t *cstr_info) {
	int tileno, compno;
	opj_cp_t *cp = NULL;

	opj_tcd_t *tcd = NULL;	/* TCD component */

	j2k->cio = cio;	
	j2k->image = image;

	cp = j2k->cp;

	/* INDEX >> */
	j2k->cstr_info = cstr_info;
	if (cstr_info) {
		int compno;
		cstr_info->tile = (opj_tile_info_t *) opj_malloc(cp->tw * cp->th * sizeof(opj_tile_info_t));
		cstr_info->image_w = image->x1 - image->x0;
		cstr_info->image_h = image->y1 - image->y0;
		cstr_info->prog = (&cp->tcps[0])->prg;
		cstr_info->tw = cp->tw;
		cstr_info->th = cp->th;
		cstr_info->tile_x = cp->tdx;	/* new version parser */
		cstr_info->tile_y = cp->tdy;	/* new version parser */
		cstr_info->tile_Ox = cp->tx0;	/* new version parser */
		cstr_info->tile_Oy = cp->ty0;	/* new version parser */
		cstr_info->numcomps = image->numcomps;
		cstr_info->numlayers = (&cp->tcps[0])->numlayers;
		cstr_info->numdecompos = (int*) opj_malloc(image->numcomps * sizeof(int));
		for (compno=0; compno < image->numcomps; compno++) {
			cstr_info->numdecompos[compno] = (&cp->tcps[0])->tccps->numresolutions - 1;
		}
		cstr_info->D_max = 0.0;		/* ADD Marcela */
		cstr_info->main_head_start = cio_tell(cio); /* position of SOC */
		cstr_info->maxmarknum = 100;
		cstr_info->marker = (opj_marker_info_t *) opj_malloc(cstr_info->maxmarknum * sizeof(opj_marker_info_t));
		cstr_info->marknum = 0;
	}
	/* << INDEX */

	j2k_write_soc(j2k);
	j2k_write_siz(j2k);
	j2k_write_cod(j2k);
	j2k_write_qcd(j2k);

	if(cp->cinema){
		for (compno = 1; compno < image->numcomps; compno++) {
			j2k_write_coc(j2k, compno);
			j2k_write_qcc(j2k, compno);
		}
	}

	for (compno = 0; compno < image->numcomps; compno++) {
		opj_tcp_t *tcp = &cp->tcps[0];
		if (tcp->tccps[compno].roishift)
			j2k_write_rgn(j2k, compno, 0);
	}
	if (cp->comment != NULL) {
		j2k_write_com(j2k);
	}

	j2k->totnum_tp = j2k_calculate_tp(cp,image->numcomps,image,j2k);
	/* TLM Marker*/
	if(cp->cinema){
		j2k_write_tlm(j2k);
		if (cp->cinema == CINEMA4K_24) {
			j2k_write_poc(j2k);
		}
	}

	/* uncomment only for testing JPSEC marker writing */
	/* j2k_write_sec(j2k); */

	/* INDEX >> */
	if(cstr_info) {
		cstr_info->main_head_end = cio_tell(cio) - 1;
	}
	/* << INDEX */
	/**** Main Header ENDS here ***/

	/* create the tile encoder */
	tcd = tcd_create(j2k->cinfo);

	/* encode each tile */
	for (tileno = 0; tileno < cp->tw * cp->th; tileno++) {
		int pino;
		int tilepartno=0;
		/* UniPG>> */
		int acc_pack_num = 0;
		/* <<UniPG */


		opj_tcp_t *tcp = &cp->tcps[tileno];
		opj_event_msg(j2k->cinfo, EVT_INFO, "tile number %d / %d\n", tileno + 1, cp->tw * cp->th);

		j2k->curtileno = tileno;
		j2k->cur_tp_num = 0;
		tcd->cur_totnum_tp = j2k->cur_totnum_tp[j2k->curtileno];
		/* initialisation before tile encoding  */
		if (tileno == 0) {
			tcd_malloc_encode(tcd, image, cp, j2k->curtileno);
		} else {
			tcd_init_encode(tcd, image, cp, j2k->curtileno);
		}

		/* INDEX >> */
		if(cstr_info) {
			cstr_info->tile[j2k->curtileno].start_pos = cio_tell(cio) + j2k->pos_correction;
			cstr_info->tile[j2k->curtileno].maxmarknum = 10;
			cstr_info->tile[j2k->curtileno].marker = (opj_marker_info_t *) opj_malloc(cstr_info->tile[j2k->curtileno].maxmarknum * sizeof(opj_marker_info_t));
			cstr_info->tile[j2k->curtileno].marknum = 0;
		}
		/* << INDEX */

		for(pino = 0; pino <= tcp->numpocs; pino++) {
			int tot_num_tp;
			tcd->cur_pino=pino;

			/*Get number of tile parts*/
			tot_num_tp = j2k_get_num_tp(cp,pino,tileno);
			tcd->tp_pos = cp->tp_pos;

			for(tilepartno = 0; tilepartno < tot_num_tp ; tilepartno++){
				j2k->tp_num = tilepartno;
				/* INDEX >> */
				if(cstr_info)
					cstr_info->tile[j2k->curtileno].tp[j2k->cur_tp_num].tp_start_pos =
					cio_tell(cio) + j2k->pos_correction;
				/* << INDEX */
				j2k_write_sot(j2k);

				if(j2k->cur_tp_num == 0 && cp->cinema == 0){
					for (compno = 1; compno < image->numcomps; compno++) {
						j2k_write_coc(j2k, compno);
						j2k_write_qcc(j2k, compno);
					}
					if (cp->tcps[tileno].numpocs) {
						j2k_write_poc(j2k);
					}
				}

				/* INDEX >> */
				if(cstr_info)
					cstr_info->tile[j2k->curtileno].tp[j2k->cur_tp_num].tp_end_header =
					cio_tell(cio) + j2k->pos_correction + 1;
				/* << INDEX */

				j2k_write_sod(j2k, tcd);

				/* INDEX >> */
				if(cstr_info) {
					cstr_info->tile[j2k->curtileno].tp[j2k->cur_tp_num].tp_end_pos =
						cio_tell(cio) + j2k->pos_correction - 1;
					cstr_info->tile[j2k->curtileno].tp[j2k->cur_tp_num].tp_start_pack =
						acc_pack_num;
					cstr_info->tile[j2k->curtileno].tp[j2k->cur_tp_num].tp_numpacks =
						cstr_info->packno - acc_pack_num;
					acc_pack_num = cstr_info->packno;
				}
				/* << INDEX */

				j2k->cur_tp_num++;
			}			
		}
		if(cstr_info) {
			cstr_info->tile[j2k->curtileno].end_pos = cio_tell(cio) + j2k->pos_correction - 1;
		}


		/*
		if (tile->PPT) { // BAD PPT !!! 
		FILE *PPT_file;
		int i;
		PPT_file=fopen("PPT","rb");
		fprintf(stderr,"%c%c%c%c",255,97,tile->len_ppt/256,tile->len_ppt%256);
		for (i=0;i<tile->len_ppt;i++) {
		unsigned char elmt;
		fread(&elmt, 1, 1, PPT_file);
		fwrite(&elmt,1,1,f);
		}
		fclose(PPT_file);
		unlink("PPT");
		}
		*/

	}

	/* destroy the tile encoder */
	tcd_free_encode(tcd);
	tcd_destroy(tcd);

	opj_free(j2k->cur_totnum_tp);

	j2k_write_eoc(j2k);

	if(cstr_info) {
		cstr_info->codestream_size = cio_tell(cio) + j2k->pos_correction;
		/* UniPG>> */
		/* The following adjustment is done to adjust the codestream size */
		/* if SOD is not at 0 in the buffer. Useful in case of JP2, where */
		/* the first bunch of bytes is not in the codestream              */
		cstr_info->codestream_size -= cstr_info->main_head_start;
		/* <<UniPG */
	}

#ifdef USE_JPWL
	/*
	preparation of JPWL marker segments
	*/
	if(cp->epc_on) {

		/* encode according to JPWL */
		jpwl_encode(j2k, cio, image);

	}
#endif /* USE_JPWL */

	return OPJ_TRUE;
}

static void j2k_add_mhmarker(opj_codestream_info_t *cstr_info, unsigned short int type, int pos, int len)
{
	assert(cstr_info != 00);

	/* expand the list? */
	if ((cstr_info->marknum + 1) > cstr_info->maxmarknum) {
		cstr_info->maxmarknum = 100 + (int) ((float) cstr_info->maxmarknum * 1.0F);
		cstr_info->marker = (opj_marker_info_t*)opj_realloc(cstr_info->marker, cstr_info->maxmarknum);
	}

	/* add the marker */
	cstr_info->marker[cstr_info->marknum].type = type;
	cstr_info->marker[cstr_info->marknum].pos = pos;
	cstr_info->marker[cstr_info->marknum].len = len;
	cstr_info->marknum++;

}

static void j2k_add_mhmarker_v2(opj_codestream_index_t *cstr_index, OPJ_UINT32 type, OPJ_OFF_T pos, OPJ_UINT32 len)
{
	assert(cstr_index != 00);

	/* expand the list? */
	if ((cstr_index->marknum + 1) > cstr_index->maxmarknum) {
		cstr_index->maxmarknum = 100 + (int) ((float) cstr_index->maxmarknum * 1.0F);
		cstr_index->marker = (opj_marker_info_t*)opj_realloc(cstr_index->marker, cstr_index->maxmarknum *sizeof(opj_marker_info_t));
	}

	/* add the marker */
	cstr_index->marker[cstr_index->marknum].type = (OPJ_UINT16)type;
	cstr_index->marker[cstr_index->marknum].pos = (OPJ_INT32)pos;
	cstr_index->marker[cstr_index->marknum].len = (OPJ_INT32)len;
	cstr_index->marknum++;

}

static void j2k_add_tlmarker( int tileno, opj_codestream_info_t *cstr_info, unsigned short int type, int pos, int len)
{
	opj_marker_info_t *marker;

	assert(cstr_info != 00);

	/* expand the list? */
	if ((cstr_info->tile[tileno].marknum + 1) > cstr_info->tile[tileno].maxmarknum) {
		cstr_info->tile[tileno].maxmarknum = 100 + (int) ((float) cstr_info->tile[tileno].maxmarknum * 1.0F);
		cstr_info->tile[tileno].marker = (opj_marker_info_t*)opj_realloc(cstr_info->tile[tileno].marker, cstr_info->maxmarknum);
	}

	marker = &(cstr_info->tile[tileno].marker[cstr_info->tile[tileno].marknum]);

	/* add the marker */
	marker->type = type;
	marker->pos = pos;
	marker->len = len;
	cstr_info->tile[tileno].marknum++;
}

static void j2k_add_tlmarker_v2(OPJ_UINT32 tileno, opj_codestream_index_t *cstr_index, OPJ_UINT32 type, OPJ_OFF_T pos, OPJ_UINT32 len)
{
	assert(cstr_index != 00);
	assert(cstr_index->tile_index != 00);

	/* expand the list? */
	if ((cstr_index->tile_index[tileno].marknum + 1) > cstr_index->tile_index[tileno].maxmarknum) {
		cstr_index->tile_index[tileno].maxmarknum = 100 + (int) ((float) cstr_index->tile_index[tileno].maxmarknum * 1.0F);
		cstr_index->tile_index[tileno].marker =
				(opj_marker_info_t*)opj_realloc(cstr_index->tile_index[tileno].marker,
												cstr_index->tile_index[tileno].maxmarknum *sizeof(opj_marker_info_t));
	}

	/* add the marker */
	cstr_index->tile_index[tileno].marker[cstr_index->tile_index[tileno].marknum].type = (OPJ_UINT16)type;
	cstr_index->tile_index[tileno].marker[cstr_index->tile_index[tileno].marknum].pos = (OPJ_INT32)pos;
	cstr_index->tile_index[tileno].marker[cstr_index->tile_index[tileno].marknum].len = (OPJ_INT32)len;
	cstr_index->tile_index[tileno].marknum++;

	if (type == J2K_MS_SOT) {
		OPJ_UINT32 l_current_tile_part = cstr_index->tile_index[tileno].current_tpsno;

		if (cstr_index->tile_index[tileno].tp_index)
			cstr_index->tile_index[tileno].tp_index[l_current_tile_part].start_pos = pos;

	}
}


/*
 * -----------------------------------------------------------------------
 * -----------------------------------------------------------------------
 * -----------------------------------------------------------------------
 */

/**
 * Ends the decompression procedures and possibiliy add data to be read after the
 * codestream.
 */
opj_bool j2k_end_decompress(
						opj_j2k_v2_t *p_j2k,
						opj_stream_private_t *p_stream,
						opj_event_mgr_t * p_manager)
{
	return OPJ_TRUE;
}

/**
 * Reads a jpeg2000 codestream header structure.

 *
 * @param p_stream the stream to read data from.
 * @param p_j2k the jpeg2000 codec.
 * @param p_manager the user event manager.
 *
 * @return true if the box is valid.
 */
opj_bool j2k_read_header(	struct opj_stream_private *p_stream,
							opj_j2k_v2_t* p_j2k,
							opj_image_t** p_image,
							struct opj_event_mgr* p_manager )
{
	/* preconditions */
	assert(p_j2k != 00);
	assert(p_stream != 00);
	assert(p_manager != 00);

	/* create an empty image header */
	p_j2k->m_private_image = opj_image_create0();
	if (! p_j2k->m_private_image) {
		return OPJ_FALSE;
	}

	/* customization of the validation */
	j2k_setup_decoding_validation(p_j2k);

	/* validation of the parameters codec */
	if (! j2k_exec(p_j2k, p_j2k->m_validation_list, p_stream,p_manager)) {
		opj_image_destroy(p_j2k->m_private_image);
		p_j2k->m_private_image = NULL;
		return OPJ_FALSE;
	}

	/* customization of the encoding */
	j2k_setup_header_reading(p_j2k);

	/* read header */
	if (! j2k_exec (p_j2k,p_j2k->m_procedure_list,p_stream,p_manager)) {
		opj_image_destroy(p_j2k->m_private_image);
		p_j2k->m_private_image = NULL;
		return OPJ_FALSE;
	}

	*p_image = opj_image_create0();
	if (! (*p_image)) {
		return OPJ_FALSE;
	}

	/* Copy codestream image information to the output image */
	opj_copy_image_header(p_j2k->m_private_image, *p_image);


	return OPJ_TRUE;
}

/**
 * Sets up the procedures to do on reading header. Developpers wanting to extend the library can add their own reading procedures.
 */
void j2k_setup_header_reading (opj_j2k_v2_t *p_j2k)
{
	/* preconditions*/
	assert(p_j2k != 00);

	opj_procedure_list_add_procedure(p_j2k->m_procedure_list,(void*)j2k_read_header_procedure);

	/* DEVELOPER CORNER, add your custom procedures */
	opj_procedure_list_add_procedure(p_j2k->m_procedure_list,(void*)j2k_copy_default_tcp_and_create_tcd);

}

/**
 * Sets up the validation ,i.e. adds the procedures to lauch to make sure the codec parameters
 * are valid. Developpers wanting to extend the library can add their own validation procedures.
 */
void j2k_setup_decoding_validation (opj_j2k_v2_t *p_j2k)
{
	/* preconditions*/
	assert(p_j2k != 00);

	opj_procedure_list_add_procedure(p_j2k->m_validation_list, (void*)j2k_build_decoder);
	opj_procedure_list_add_procedure(p_j2k->m_validation_list, (void*)j2k_decoding_validation);
	/* DEVELOPER CORNER, add your custom validation procedure */

}

/**
 * Builds the cp decoder parameters to use to decode tile.
 */
opj_bool j2k_build_decoder (
						opj_j2k_v2_t * p_j2k,
						opj_stream_private_t *p_stream,
						opj_event_mgr_t * p_manager
						)
{
	/* add here initialization of cp
	// copy paste of setup_decoder*/
	return OPJ_TRUE;
}

/**
 * The default decoding validation procedure without any extension.
 *
 * @param	p_j2k			the jpeg2000 codec to validate.
 * @param	p_stream				the input stream to validate.
 * @param	p_manager		the user event manager.
 *
 * @return true if the parameters are correct.
 */
opj_bool j2k_decoding_validation (
								opj_j2k_v2_t *p_j2k,
								opj_stream_private_t *p_stream,
								opj_event_mgr_t * p_manager
							  )
{
	opj_bool l_is_valid = OPJ_TRUE;

	/* preconditions*/
	assert(p_j2k != 00);
	assert(p_stream != 00);
	assert(p_manager != 00);


	/* STATE checking */
	/* make sure the state is at 0 */
#ifdef TODO_MSD
	l_is_valid &= (p_j2k->m_specific_param.m_decoder.m_state == J2K_DEC_STATE_NONE);
#endif
	l_is_valid &= (p_j2k->m_specific_param.m_decoder.m_state == 0x0000);

	/* POINTER validation */
	/* make sure a p_j2k codec is present */
	/* make sure a procedure list is present */
	l_is_valid &= (p_j2k->m_procedure_list != 00);
	/* make sure a validation list is present */
	l_is_valid &= (p_j2k->m_validation_list != 00);

	/* PARAMETER VALIDATION */
	return l_is_valid;
}

opj_bool j2k_read_header_procedure(	opj_j2k_v2_t *p_j2k,
									struct opj_stream_private *p_stream,
									struct opj_event_mgr * p_manager)
{
	OPJ_UINT32 l_current_marker;
	OPJ_UINT32 l_marker_size;
	const opj_dec_memory_marker_handler_t * l_marker_handler = 00;

	/* preconditions */
	assert(p_stream != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	/*  We enter in the main header */
	p_j2k->m_specific_param.m_decoder.m_state = J2K_STATE_MHSOC;

	/* Try to read the SOC marker, the codestream must begin with SOC marker */
	if (! j2k_read_soc_v2(p_j2k,p_stream,p_manager)) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Expected a SOC marker \n");
		return OPJ_FALSE;
	}

	/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer */
	if (opj_stream_read_data(p_stream,p_j2k->m_specific_param.m_decoder.m_header_data,2,p_manager) != 2) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
		return OPJ_FALSE;
	}

	/* Read 2 bytes as the new marker ID */
	opj_read_bytes(p_j2k->m_specific_param.m_decoder.m_header_data,&l_current_marker,2);

	/* Try to read until the SOT is detected */
	while (l_current_marker != J2K_MS_SOT) {

		/* Check if the current marker ID is valid */
		if (l_current_marker < 0xff00) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "We expected read a marker ID (0xff--) instead of %.8x\n", l_current_marker);
			return OPJ_FALSE;
		}

		/* Get the marker handler from the marker ID */
		l_marker_handler = j2k_get_marker_handler(l_current_marker);

		/* Manage case where marker is unknown */
		if (l_marker_handler->id == J2K_MS_UNK) {
			if (! j2k_read_unk_v2(p_j2k, p_stream, &l_current_marker, p_manager)){
				opj_event_msg_v2(p_manager, EVT_ERROR, "Unknow marker have been detected and generated error.\n");
				return OPJ_FALSE;
			}

			if (l_current_marker == J2K_MS_SOT)
				break; /* SOT marker is detected main header is completely read */
			else	/* Get the marker handler from the marker ID */
				l_marker_handler = j2k_get_marker_handler(l_current_marker);
		}

		/* Check if the marker is known and if it is the right place to find it */
		if (! (p_j2k->m_specific_param.m_decoder.m_state & l_marker_handler->states) ) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Marker is not compliant with its position\n");
			return OPJ_FALSE;
		}

		/* Try to read 2 bytes (the marker size) from stream and copy them into the buffer */
		if (opj_stream_read_data(p_stream,p_j2k->m_specific_param.m_decoder.m_header_data,2,p_manager) != 2) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
			return OPJ_FALSE;
		}

		/* read 2 bytes as the marker size */
		opj_read_bytes(p_j2k->m_specific_param.m_decoder.m_header_data,&l_marker_size,2);
		l_marker_size -= 2; /* Subtract the size of the marker ID already read */

		/* Check if the marker size is compatible with the header data size */
		if (l_marker_size > p_j2k->m_specific_param.m_decoder.m_header_data_size) {
			p_j2k->m_specific_param.m_decoder.m_header_data = (OPJ_BYTE*)
					opj_realloc(p_j2k->m_specific_param.m_decoder.m_header_data,l_marker_size);
			if (p_j2k->m_specific_param.m_decoder.m_header_data == 00) {
				return OPJ_FALSE;
			}
			p_j2k->m_specific_param.m_decoder.m_header_data_size = l_marker_size;
		}

		/* Try to read the rest of the marker segment from stream and copy them into the buffer */
		if (opj_stream_read_data(p_stream,p_j2k->m_specific_param.m_decoder.m_header_data,l_marker_size,p_manager) != l_marker_size) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
			return OPJ_FALSE;
		}

		/* Read the marker segment with the correct marker handler */
		if (! (*(l_marker_handler->handler))(p_j2k,p_j2k->m_specific_param.m_decoder.m_header_data,l_marker_size,p_manager)) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Marker handler function failed to read the marker segment\n");
			return OPJ_FALSE;
		}

		/* Add the marker to the codestream index*/
		j2k_add_mhmarker_v2(p_j2k->cstr_index,
							l_marker_handler->id,
							(OPJ_UINT32) opj_stream_tell(p_stream) - l_marker_size - 4,
							l_marker_size + 4 );

		/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer */
		if (opj_stream_read_data(p_stream,p_j2k->m_specific_param.m_decoder.m_header_data,2,p_manager) != 2) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
			return OPJ_FALSE;
		}

		/* read 2 bytes as the new marker ID */
		opj_read_bytes(p_j2k->m_specific_param.m_decoder.m_header_data,&l_current_marker,2);
	}

	opj_event_msg_v2(p_manager, EVT_INFO, "Main header has been correctly decoded.\n");

	/* Position of the last element if the main header */
	p_j2k->cstr_index->main_head_end = (OPJ_UINT32) opj_stream_tell(p_stream) - 2;

	/* Next step: read a tile-part header */
	p_j2k->m_specific_param.m_decoder.m_state = J2K_STATE_TPHSOT;

	return OPJ_TRUE;
}

/**
 * Excutes the given procedures on the given codec.
 *
 * @param	p_procedure_list	the list of procedures to execute
 * @param	p_j2k					the jpeg2000 codec to execute the procedures on.
 * @param	p_stream					the stream to execute the procedures on.
 * @param	p_manager			the user manager.
 *
 * @return	true				if all the procedures were successfully executed.
 */
opj_bool j2k_exec (	opj_j2k_v2_t * p_j2k,
					opj_procedure_list_t * p_procedure_list,
					opj_stream_private_t *p_stream,
					opj_event_mgr_t * p_manager )
{
	opj_bool (** l_procedure) (opj_j2k_v2_t * ,opj_stream_private_t *,opj_event_mgr_t *) = 00;
	opj_bool l_result = OPJ_TRUE;
	OPJ_UINT32 l_nb_proc, i;

	/* preconditions*/
	assert(p_procedure_list != 00);
	assert(p_j2k != 00);
	assert(p_stream != 00);
	assert(p_manager != 00);


	l_nb_proc = opj_procedure_list_get_nb_procedures(p_procedure_list);
	l_procedure = (opj_bool (**) (opj_j2k_v2_t * ,opj_stream_private_t *,opj_event_mgr_t *)) opj_procedure_list_get_first_procedure(p_procedure_list);

	for	(i=0;i<l_nb_proc;++i) {
		l_result = l_result && ((*l_procedure) (p_j2k,p_stream,p_manager));
		++l_procedure;
	}

	/* and clear the procedure list at the end.*/
	opj_procedure_list_clear(p_procedure_list);
	return l_result;
}

/* FIXME DOC*/
opj_bool j2k_copy_default_tcp_and_create_tcd
						(
						opj_j2k_v2_t * p_j2k,
						opj_stream_private_t *p_stream,
						opj_event_mgr_t * p_manager
						)
{
	opj_tcp_v2_t * l_tcp = 00;
	opj_tcp_v2_t * l_default_tcp = 00;
	OPJ_UINT32 l_nb_tiles;
	OPJ_UINT32 i,j;
	opj_tccp_t *l_current_tccp = 00;
	OPJ_UINT32 l_tccp_size;
	OPJ_UINT32 l_mct_size;
	opj_image_t * l_image;
	OPJ_UINT32 l_mcc_records_size,l_mct_records_size;
	opj_mct_data_t * l_src_mct_rec, *l_dest_mct_rec;
	opj_simple_mcc_decorrelation_data_t * l_src_mcc_rec, *l_dest_mcc_rec;
	OPJ_UINT32 l_offset;

	/* preconditions */
	assert(p_j2k != 00);
	assert(p_stream != 00);
	assert(p_manager != 00);

	l_image = p_j2k->m_private_image;
	l_nb_tiles = p_j2k->m_cp.th * p_j2k->m_cp.tw;
	l_tcp = p_j2k->m_cp.tcps;
	l_tccp_size = l_image->numcomps * sizeof(opj_tccp_t);
	l_default_tcp = p_j2k->m_specific_param.m_decoder.m_default_tcp;
	l_mct_size = l_image->numcomps * l_image->numcomps * sizeof(OPJ_FLOAT32);

	/* For each tile */
	for (i=0; i<l_nb_tiles; ++i) {
		/* keep the tile-compo coding parameters pointer of the current tile coding parameters*/
		l_current_tccp = l_tcp->tccps;
		/*Copy default coding parameters into the current tile coding parameters*/
		memcpy(l_tcp, l_default_tcp, sizeof(opj_tcp_v2_t));
		/* Initialize some values of the current tile coding parameters*/
		l_tcp->ppt = 0;
		l_tcp->ppt_data = 00;
		/* Reconnect the tile-compo coding parameters pointer to the current tile coding parameters*/
		l_tcp->tccps = l_current_tccp;

		/* Get the mct_decoding_matrix of the dflt_tile_cp and copy them into the current tile cp*/
		if (l_default_tcp->m_mct_decoding_matrix) {
			l_tcp->m_mct_decoding_matrix = (OPJ_FLOAT32*)opj_malloc(l_mct_size);
			if (! l_tcp->m_mct_decoding_matrix ) {
				return OPJ_FALSE;
			}
			memcpy(l_tcp->m_mct_decoding_matrix,l_default_tcp->m_mct_decoding_matrix,l_mct_size);
		}

		/* Get the mct_record of the dflt_tile_cp and copy them into the current tile cp*/
		l_mct_records_size = l_default_tcp->m_nb_max_mct_records * sizeof(opj_mct_data_t);
		l_tcp->m_mct_records = (opj_mct_data_t*)opj_malloc(l_mct_records_size);
		if (! l_tcp->m_mct_records) {
			return OPJ_FALSE;
		}
		memcpy(l_tcp->m_mct_records, l_default_tcp->m_mct_records,l_mct_records_size);

		/* Copy the mct record data from dflt_tile_cp to the current tile*/
		l_src_mct_rec = l_default_tcp->m_mct_records;
		l_dest_mct_rec = l_tcp->m_mct_records;

		for (j=0;j<l_default_tcp->m_nb_mct_records;++j) {

			if (l_src_mct_rec->m_data) {

				l_dest_mct_rec->m_data = (OPJ_BYTE*) opj_malloc(l_src_mct_rec->m_data_size);
				if(! l_dest_mct_rec->m_data) {
					return OPJ_FALSE;
				}
				memcpy(l_dest_mct_rec->m_data,l_src_mct_rec->m_data,l_src_mct_rec->m_data_size);
			}

			++l_src_mct_rec;
			++l_dest_mct_rec;
		}

		/* Get the mcc_record of the dflt_tile_cp and copy them into the current tile cp*/
		l_mcc_records_size = l_default_tcp->m_nb_max_mcc_records * sizeof(opj_simple_mcc_decorrelation_data_t);
		l_tcp->m_mcc_records = (opj_simple_mcc_decorrelation_data_t*) opj_malloc(l_mcc_records_size);
		if (! l_tcp->m_mcc_records) {
			return OPJ_FALSE;
		}
		memcpy(l_tcp->m_mcc_records,l_default_tcp->m_mcc_records,l_mcc_records_size);

		/* Copy the mcc record data from dflt_tile_cp to the current tile*/
		l_src_mcc_rec = l_default_tcp->m_mcc_records;
		l_dest_mcc_rec = l_tcp->m_mcc_records;

		for (j=0;j<l_default_tcp->m_nb_max_mcc_records;++j) {

			if (l_src_mcc_rec->m_decorrelation_array) {
				l_offset = l_src_mcc_rec->m_decorrelation_array - l_default_tcp->m_mct_records;
				l_dest_mcc_rec->m_decorrelation_array = l_tcp->m_mct_records + l_offset;
			}

			if (l_src_mcc_rec->m_offset_array) {
				l_offset = l_src_mcc_rec->m_offset_array - l_default_tcp->m_mct_records;
				l_dest_mcc_rec->m_offset_array = l_tcp->m_mct_records + l_offset;
			}

			++l_src_mcc_rec;
			++l_dest_mcc_rec;
		}

		/* Copy all the dflt_tile_compo_cp to the current tile cp */
		memcpy(l_current_tccp,l_default_tcp->tccps,l_tccp_size);

		/* Move to next tile cp*/
		++l_tcp;
	}

	/* Create the current tile decoder*/
	p_j2k->m_tcd = (opj_tcd_v2_t*)tcd_create_v2(OPJ_TRUE); /* FIXME why a cast ? */
	if (! p_j2k->m_tcd ) {
		return OPJ_FALSE;
	}

	if ( !tcd_init_v2(p_j2k->m_tcd, l_image, &(p_j2k->m_cp)) ) {
		tcd_destroy_v2(p_j2k->m_tcd);
		p_j2k->m_tcd = 00;
		opj_event_msg_v2(p_manager, EVT_ERROR, "Cannot decode tile, memory error\n");
		return OPJ_FALSE;
	}

	return OPJ_TRUE;
}

/**
 * Reads the lookup table containing all the marker, status and action, and returns the handler associated
 * with the marker value.
 * @param	p_id		Marker value to look up
 *
 * @return	the handler associated with the id.
*/
const opj_dec_memory_marker_handler_t * j2k_get_marker_handler (OPJ_UINT32 p_id)
{
	const opj_dec_memory_marker_handler_t *e;
	for (e = j2k_memory_marker_handler_tab; e->id != 0; ++e) {
		if (e->id == p_id) {
			break; /* we find a handler corresponding to the marker ID*/
		}
	}
	return e;
}


/**
 * Destroys a jpeg2000 codec.
 *
 * @param	p_j2k	the jpeg20000 structure to destroy.
 */
void j2k_destroy (opj_j2k_v2_t *p_j2k)
{
	if (p_j2k == 00) {
		return;
	}

	if (p_j2k->m_is_decoder) {

		if (p_j2k->m_specific_param.m_decoder.m_default_tcp != 00) {
			j2k_tcp_destroy(p_j2k->m_specific_param.m_decoder.m_default_tcp);
			opj_free(p_j2k->m_specific_param.m_decoder.m_default_tcp);
			p_j2k->m_specific_param.m_decoder.m_default_tcp = 00;
		}

		if (p_j2k->m_specific_param.m_decoder.m_header_data != 00) {
			opj_free(p_j2k->m_specific_param.m_decoder.m_header_data);
			p_j2k->m_specific_param.m_decoder.m_header_data = 00;
			p_j2k->m_specific_param.m_decoder.m_header_data_size = 0;
		}
	}
	else {

		if (p_j2k->m_specific_param.m_encoder.m_encoded_tile_data) {
			opj_free(p_j2k->m_specific_param.m_encoder.m_encoded_tile_data);
			p_j2k->m_specific_param.m_encoder.m_encoded_tile_data = 00;
		}

		if (p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer) {
			opj_free(p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer);
			p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_buffer = 00;
			p_j2k->m_specific_param.m_encoder.m_tlm_sot_offsets_current = 00;
		}

		if (p_j2k->m_specific_param.m_encoder.m_header_tile_data) {
			opj_free(p_j2k->m_specific_param.m_encoder.m_header_tile_data);
			p_j2k->m_specific_param.m_encoder.m_header_tile_data = 00;
			p_j2k->m_specific_param.m_encoder.m_header_tile_data_size = 0;
		}
	}

	tcd_destroy_v2(p_j2k->m_tcd);

	j2k_cp_destroy(&(p_j2k->m_cp));
	memset(&(p_j2k->m_cp),0,sizeof(opj_cp_v2_t));

	opj_procedure_list_destroy(p_j2k->m_procedure_list);
	p_j2k->m_procedure_list = 00;

	opj_procedure_list_destroy(p_j2k->m_validation_list);
	p_j2k->m_procedure_list = 00;

	j2k_destroy_cstr_index(p_j2k->cstr_index);
	p_j2k->cstr_index = NULL;

	opj_image_destroy(p_j2k->m_private_image);
	p_j2k->m_private_image = NULL;

	opj_image_destroy(p_j2k->m_output_image);
	p_j2k->m_output_image = NULL;

	opj_free(p_j2k);
}

void j2k_destroy_cstr_index (opj_codestream_index_t *p_cstr_ind)
{
	if (p_cstr_ind) {

		if (p_cstr_ind->marker) {
			opj_free(p_cstr_ind->marker);
			p_cstr_ind->marker = NULL;
		}

		if (p_cstr_ind->tile_index) {
			OPJ_UINT32 it_tile = 0;

			for (it_tile=0; it_tile < p_cstr_ind->nb_of_tiles; it_tile++) {

				if(p_cstr_ind->tile_index[it_tile].packet_index) {
					opj_free(p_cstr_ind->tile_index[it_tile].packet_index);
					p_cstr_ind->tile_index[it_tile].packet_index = NULL;
				}

				if(p_cstr_ind->tile_index[it_tile].tp_index){
					opj_free(p_cstr_ind->tile_index[it_tile].tp_index);
					p_cstr_ind->tile_index[it_tile].tp_index = NULL;
				}

				if(p_cstr_ind->tile_index[it_tile].marker){
					opj_free(p_cstr_ind->tile_index[it_tile].marker);
					p_cstr_ind->tile_index[it_tile].marker = NULL;

				}
			}

			opj_free( p_cstr_ind->tile_index);
			p_cstr_ind->tile_index = NULL;
		}

		opj_free(p_cstr_ind);
	}
}



/**
 * Destroys a tile coding parameter structure.
 *
 * @param	p_tcp		the tile coding parameter to destroy.
 */
void j2k_tcp_destroy (opj_tcp_v2_t *p_tcp)
{
	if (p_tcp == 00) {
		return;
	}

	if (p_tcp->ppt_buffer != 00) {
		opj_free(p_tcp->ppt_buffer);
		p_tcp->ppt_buffer = 00;
	}

	if (p_tcp->tccps != 00) {
		opj_free(p_tcp->tccps);
		p_tcp->tccps = 00;
	}

	if (p_tcp->m_mct_coding_matrix != 00) {
		opj_free(p_tcp->m_mct_coding_matrix);
		p_tcp->m_mct_coding_matrix = 00;
	}

	if (p_tcp->m_mct_decoding_matrix != 00) {
		opj_free(p_tcp->m_mct_decoding_matrix);
		p_tcp->m_mct_decoding_matrix = 00;
	}

	if (p_tcp->m_mcc_records) {
		opj_free(p_tcp->m_mcc_records);
		p_tcp->m_mcc_records = 00;
		p_tcp->m_nb_max_mcc_records = 0;
		p_tcp->m_nb_mcc_records = 0;
	}

	if (p_tcp->m_mct_records) {
		opj_mct_data_t * l_mct_data = p_tcp->m_mct_records;
		OPJ_UINT32 i;

		for (i=0;i<p_tcp->m_nb_mct_records;++i) {
			if (l_mct_data->m_data) {
				opj_free(l_mct_data->m_data);
				l_mct_data->m_data = 00;
			}

			++l_mct_data;
		}

		opj_free(p_tcp->m_mct_records);
		p_tcp->m_mct_records = 00;
	}

	if (p_tcp->mct_norms != 00) {
		opj_free(p_tcp->mct_norms);
		p_tcp->mct_norms = 00;
	}

	j2k_tcp_data_destroy(p_tcp);

}

/**
 * Destroys the data inside a tile coding parameter structure.
 *
 * @param	p_tcp		the tile coding parameter which contain data to destroy.
 */
void j2k_tcp_data_destroy (opj_tcp_v2_t *p_tcp)
{
	if (p_tcp->m_data) {
		opj_free(p_tcp->m_data);
		p_tcp->m_data = NULL;
		p_tcp->m_data_size = 0;
	}
}

/**
 * Destroys a coding parameter structure.
 *
 * @param	p_cp		the coding parameter to destroy.
 */
void j2k_cp_destroy (opj_cp_v2_t *p_cp)
{
	OPJ_UINT32 l_nb_tiles;
	opj_tcp_v2_t * l_current_tile = 00;
	OPJ_UINT32 i;

	if
		(p_cp == 00)
	{
		return;
	}
	if
		(p_cp->tcps != 00)
	{
		l_current_tile = p_cp->tcps;
		l_nb_tiles = p_cp->th * p_cp->tw;

		for
			(i = 0; i < l_nb_tiles; ++i)
		{
			j2k_tcp_destroy(l_current_tile);
			++l_current_tile;
		}
		opj_free(p_cp->tcps);
		p_cp->tcps = 00;
	}
	if
		(p_cp->ppm_buffer != 00)
	{
		opj_free(p_cp->ppm_buffer);
		p_cp->ppm_buffer = 00;
	}
	if
		(p_cp->comment != 00)
	{
		opj_free(p_cp->comment);
		p_cp->comment = 00;
	}
	if
		(! p_cp->m_is_decoder)
	{
		if
			(p_cp->m_specific_param.m_enc.m_matrice)
		{
			opj_free(p_cp->m_specific_param.m_enc.m_matrice);
			p_cp->m_specific_param.m_enc.m_matrice = 00;
		}
	}
}



/**
 * Reads a tile header.
 * @param	p_j2k		the jpeg2000 codec.
 * @param	p_stream			the stream to write data to.
 * @param	p_manager	the user event manager.
 */
opj_bool j2k_read_tile_header(	opj_j2k_v2_t * p_j2k,
					 	 	 	OPJ_UINT32 * p_tile_index,
					 	 	 	OPJ_UINT32 * p_data_size,
					 	 	 	OPJ_INT32 * p_tile_x0, OPJ_INT32 * p_tile_y0,
					 	 	 	OPJ_INT32 * p_tile_x1, OPJ_INT32 * p_tile_y1,
								OPJ_UINT32 * p_nb_comps,
								opj_bool * p_go_on,
								opj_stream_private_t *p_stream,
								opj_event_mgr_t * p_manager )
{
	OPJ_UINT32 l_current_marker = J2K_MS_SOT;
	OPJ_UINT32 l_marker_size;
	const opj_dec_memory_marker_handler_t * l_marker_handler = 00;
	opj_tcp_v2_t * l_tcp = NULL;
	OPJ_UINT32 l_nb_tiles;

	/* preconditions */
	assert(p_stream != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	/* Reach the End Of Codestream ?*/
	if (p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_EOC){
		l_current_marker = J2K_MS_EOC;
	}
	/* We need to encounter a SOT marker (a new tile-part header) */
	else if	(p_j2k->m_specific_param.m_decoder.m_state != J2K_STATE_TPHSOT){
		return OPJ_FALSE;
	}

	/* Read into the codestream until reach the EOC or ! can_decode ??? FIXME */
	while ( (!p_j2k->m_specific_param.m_decoder.m_can_decode) && (l_current_marker != J2K_MS_EOC) ) {

		/* Try to read until the Start Of Data is detected */
		while (l_current_marker != J2K_MS_SOD) {

			/* Try to read 2 bytes (the marker size) from stream and copy them into the buffer */
			if (opj_stream_read_data(p_stream,p_j2k->m_specific_param.m_decoder.m_header_data,2,p_manager) != 2) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
				return OPJ_FALSE;
			}

			/* Read 2 bytes from the buffer as the marker size */
			opj_read_bytes(p_j2k->m_specific_param.m_decoder.m_header_data,&l_marker_size,2);

			/* Why this condition? FIXME */
			if (p_j2k->m_specific_param.m_decoder.m_state & J2K_STATE_TPH){
				p_j2k->m_specific_param.m_decoder.m_sot_length -= (l_marker_size + 2);
			}
			l_marker_size -= 2; /* Subtract the size of the marker ID already read */

			/* Get the marker handler from the marker ID */
			l_marker_handler = j2k_get_marker_handler(l_current_marker);

			/* Check if the marker is known and if it is the right place to find it */
			if (! (p_j2k->m_specific_param.m_decoder.m_state & l_marker_handler->states) ) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Marker is not compliant with its position\n");
				return OPJ_FALSE;
			}
/* FIXME manage case of unknown marker as in the main header ? */

			/* Check if the marker size is compatible with the header data size */
			if (l_marker_size > p_j2k->m_specific_param.m_decoder.m_header_data_size) {
				p_j2k->m_specific_param.m_decoder.m_header_data = (OPJ_BYTE*)
					opj_realloc(p_j2k->m_specific_param.m_decoder.m_header_data,l_marker_size);
				if (p_j2k->m_specific_param.m_decoder.m_header_data == 00) {
					return OPJ_FALSE;
				}
				p_j2k->m_specific_param.m_decoder.m_header_data_size = l_marker_size;
			}

			/* Try to read the rest of the marker segment from stream and copy them into the buffer */
			if (opj_stream_read_data(p_stream,p_j2k->m_specific_param.m_decoder.m_header_data,l_marker_size,p_manager) != l_marker_size) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
				return OPJ_FALSE;
			}

			/* Read the marker segment with the correct marker handler */
			if (! (*(l_marker_handler->handler))(p_j2k,p_j2k->m_specific_param.m_decoder.m_header_data,l_marker_size,p_manager)) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Fail to read the current marker segment (%#x)\n", l_current_marker);
				return OPJ_FALSE;
			}

			/* Add the marker to the codestream index*/
			j2k_add_tlmarker_v2(p_j2k->m_current_tile_number,
								p_j2k->cstr_index,
								l_marker_handler->id,
								(OPJ_UINT32) opj_stream_tell(p_stream) - l_marker_size - 4,
								l_marker_size + 4 );

			/* Keep the position of the last SOT marker read */
			if ( l_marker_handler->id == J2K_MS_SOT ) {
				OPJ_UINT32 sot_pos = (OPJ_UINT32) opj_stream_tell(p_stream) - l_marker_size - 4 ;
				if (sot_pos > p_j2k->m_specific_param.m_decoder.m_last_sot_read_pos)
				{
					p_j2k->m_specific_param.m_decoder.m_last_sot_read_pos = sot_pos;
				}
			}


			if (p_j2k->m_specific_param.m_decoder.m_skip_data) {
				/* Skip the rest of the tile part header*/
				if (opj_stream_skip(p_stream,p_j2k->m_specific_param.m_decoder.m_sot_length,p_manager) != p_j2k->m_specific_param.m_decoder.m_sot_length) {
					opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
					return OPJ_FALSE;
				}
				l_current_marker = J2K_MS_SOD; /* Normally we reached a SOD */
			}
			else {
				/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer*/
				if (opj_stream_read_data(p_stream,p_j2k->m_specific_param.m_decoder.m_header_data,2,p_manager) != 2) {
					opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
					return OPJ_FALSE;
				}
				/* Read 2 bytes from the buffer as the new marker ID */
				opj_read_bytes(p_j2k->m_specific_param.m_decoder.m_header_data,&l_current_marker,2);
			}
		}

		/* If we didn't skip data before, we need to read the SOD marker*/
		if (! p_j2k->m_specific_param.m_decoder.m_skip_data) {
			/* Try to read the SOD marker and skip data ? FIXME */
			if (! j2k_read_sod_v2(p_j2k, p_stream, p_manager)) {
				return OPJ_FALSE;
			}



			if (! p_j2k->m_specific_param.m_decoder.m_can_decode){
				/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer */
				if (opj_stream_read_data(p_stream,p_j2k->m_specific_param.m_decoder.m_header_data,2,p_manager) != 2) {
					opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
					return OPJ_FALSE;
				}

				/* Read 2 bytes from buffer as the new marker ID */
				opj_read_bytes(p_j2k->m_specific_param.m_decoder.m_header_data,&l_current_marker,2);
			}
		}
		else {
			/* Indicate we will try to read a new tile-part header*/
			p_j2k->m_specific_param.m_decoder.m_skip_data = 0;
			p_j2k->m_specific_param.m_decoder.m_can_decode = 0;
			p_j2k->m_specific_param.m_decoder.m_state = J2K_STATE_TPHSOT;

			/* Try to read 2 bytes (the next marker ID) from stream and copy them into the buffer */
			if (opj_stream_read_data(p_stream,p_j2k->m_specific_param.m_decoder.m_header_data,2,p_manager) != 2) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
				return OPJ_FALSE;
			}

			/* Read 2 bytes from buffer as the new marker ID */
			opj_read_bytes(p_j2k->m_specific_param.m_decoder.m_header_data,&l_current_marker,2);
		}
	}

	/* Current marker is the EOC marker ?*/
	if (l_current_marker == J2K_MS_EOC) {
		if (p_j2k->m_specific_param.m_decoder.m_state != J2K_STATE_EOC ){
			p_j2k->m_current_tile_number = 0;
			p_j2k->m_specific_param.m_decoder.m_state = J2K_STATE_EOC;
		}
	}

	/* FIXME DOC ???*/
	if ( ! p_j2k->m_specific_param.m_decoder.m_can_decode) {
		l_tcp = p_j2k->m_cp.tcps + p_j2k->m_current_tile_number;
		l_nb_tiles = p_j2k->m_cp.th * p_j2k->m_cp.tw;

		while( (p_j2k->m_current_tile_number < l_nb_tiles) && (l_tcp->m_data == 00) ) {
			++p_j2k->m_current_tile_number;
			++l_tcp;
		}

		if (p_j2k->m_current_tile_number == l_nb_tiles) {
			*p_go_on = OPJ_FALSE;
			return OPJ_TRUE;
		}
	}

	/*FIXME ???*/
	if (! tcd_init_decode_tile(p_j2k->m_tcd, p_j2k->m_current_tile_number)) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Cannot decode tile, memory error\n");
		return OPJ_FALSE;
	}

	opj_event_msg_v2(p_manager, EVT_INFO, "Header of tile %d / %d has been read.\n",
			p_j2k->m_current_tile_number, (p_j2k->m_cp.th * p_j2k->m_cp.tw) - 1);

	*p_tile_index = p_j2k->m_current_tile_number;
	*p_go_on = OPJ_TRUE;
	*p_data_size = tcd_get_decoded_tile_size(p_j2k->m_tcd);
	*p_tile_x0 = p_j2k->m_tcd->tcd_image->tiles->x0;
	*p_tile_y0 = p_j2k->m_tcd->tcd_image->tiles->y0;
	*p_tile_x1 = p_j2k->m_tcd->tcd_image->tiles->x1;
	*p_tile_y1 = p_j2k->m_tcd->tcd_image->tiles->y1;
	*p_nb_comps = p_j2k->m_tcd->tcd_image->tiles->numcomps;

	 p_j2k->m_specific_param.m_decoder.m_state |= 0x0080;/* FIXME J2K_DEC_STATE_DATA;*/

	return OPJ_TRUE;
}


opj_bool j2k_decode_tile (	opj_j2k_v2_t * p_j2k,
							OPJ_UINT32 p_tile_index,
							OPJ_BYTE * p_data,
							OPJ_UINT32 p_data_size,
							opj_stream_private_t *p_stream,
							opj_event_mgr_t * p_manager )
{
	OPJ_UINT32 l_current_marker;
	OPJ_BYTE l_data [2];
	opj_tcp_v2_t * l_tcp;

	/* preconditions */
	assert(p_stream != 00);
	assert(p_j2k != 00);
	assert(p_manager != 00);

	if ( !(p_j2k->m_specific_param.m_decoder.m_state & 0x0080/*FIXME J2K_DEC_STATE_DATA*/)
		|| (p_tile_index != p_j2k->m_current_tile_number) ) {
		return OPJ_FALSE;
	}

	l_tcp = &(p_j2k->m_cp.tcps[p_tile_index]);
	if (! l_tcp->m_data) {
		j2k_tcp_destroy(l_tcp);
		return OPJ_FALSE;
	}

	if (! tcd_decode_tile_v2(	p_j2k->m_tcd,
								l_tcp->m_data,
								l_tcp->m_data_size,
								p_tile_index,
								p_j2k->cstr_index) ) {
		j2k_tcp_destroy(l_tcp);
		p_j2k->m_specific_param.m_decoder.m_state |= 0x8000;/*FIXME J2K_DEC_STATE_ERR;*/
		return OPJ_FALSE;
	}

	if (! tcd_update_tile_data(p_j2k->m_tcd,p_data,p_data_size)) {
		return OPJ_FALSE;
	}

	/* To avoid to destroy the tcp which can be useful when we try to decode a tile decoded before (cf j2k_random_tile_access)
	 * we destroy just the data which will be re-read in read_tile_header*/
	/*j2k_tcp_destroy(l_tcp);
	p_j2k->m_tcd->tcp = 0;*/
	j2k_tcp_data_destroy(l_tcp);

	p_j2k->m_specific_param.m_decoder.m_can_decode = 0;
	p_j2k->m_specific_param.m_decoder.m_state &= (~ (0x0080));/* FIXME J2K_DEC_STATE_DATA);*/

	if (p_j2k->m_specific_param.m_decoder.m_state != 0x0100){ /*FIXME J2K_DEC_STATE_EOC)*/
		if (opj_stream_read_data(p_stream,l_data,2,p_manager) != 2) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short\n");
			return OPJ_FALSE;
		}

		opj_read_bytes(l_data,&l_current_marker,2);

		if (l_current_marker == J2K_MS_EOC) {
			p_j2k->m_current_tile_number = 0;
			p_j2k->m_specific_param.m_decoder.m_state =  0x0100;/*FIXME J2K_DEC_STATE_EOC;*/
		}
		else if (l_current_marker != J2K_MS_SOT)
		{
			opj_event_msg_v2(p_manager, EVT_ERROR, "Stream too short, expected SOT\n");
			return OPJ_FALSE;
		}
	}

	return OPJ_TRUE;
}


opj_bool j2k_update_image_data (opj_tcd_v2_t * p_tcd, OPJ_BYTE * p_data, opj_image_t* p_output_image)
{
	OPJ_UINT32 i,j,k = 0;
	OPJ_UINT32 l_width_src,l_height_src;
	OPJ_UINT32 l_width_dest,l_height_dest;
	OPJ_INT32 l_offset_x0_src, l_offset_y0_src, l_offset_x1_src, l_offset_y1_src;
	OPJ_INT32 l_start_offset_src, l_line_offset_src, l_end_offset_src ;
	OPJ_UINT32 l_start_x_dest , l_start_y_dest;
	OPJ_UINT32 l_x0_dest, l_y0_dest, l_x1_dest, l_y1_dest;
	OPJ_INT32 l_start_offset_dest, l_line_offset_dest;

	opj_image_comp_t * l_img_comp_src = 00;
	opj_image_comp_t * l_img_comp_dest = 00;

	opj_tcd_tilecomp_v2_t * l_tilec = 00;
	opj_image_t * l_image_src = 00;
	OPJ_UINT32 l_size_comp, l_remaining;
	OPJ_INT32 * l_dest_ptr;
	opj_tcd_resolution_v2_t* l_res= 00;

	l_tilec = p_tcd->tcd_image->tiles->comps;
	l_image_src = p_tcd->image;
	l_img_comp_src = l_image_src->comps;

	l_img_comp_dest = p_output_image->comps;

	for (i=0; i<l_image_src->numcomps; i++) {

		/* Allocate output component buffer if necessary */
		if (!l_img_comp_dest->data) {

			l_img_comp_dest->data = (OPJ_INT32*) opj_calloc(l_img_comp_dest->w * l_img_comp_dest->h, sizeof(OPJ_INT32));
			if (! l_img_comp_dest->data) {
				return OPJ_FALSE;
			}
		}

		/* Copy info from decoded comp image to output image */
		l_img_comp_dest->resno_decoded = l_img_comp_src->resno_decoded;

		/*-----*/
		/* Compute the precision of the output buffer */
		l_size_comp = l_img_comp_src->prec >> 3; /*(/ 8)*/
		l_remaining = l_img_comp_src->prec & 7;  /* (%8) */
		l_res = l_tilec->resolutions + l_img_comp_src->resno_decoded;

		if (l_remaining) {
			++l_size_comp;
		}

		if (l_size_comp == 3) {
			l_size_comp = 4;
		}
		/*-----*/

		/* Current tile component size*/
		/*if (i == 0) {
		fprintf(stdout, "SRC: l_res_x0=%d, l_res_x1=%d, l_res_y0=%d, l_res_y1=%d\n",
				l_res->x0, l_res->x1, l_res->y0, l_res->y1);
		}*/

		l_width_src = (l_res->x1 - l_res->x0);
		l_height_src = (l_res->y1 - l_res->y0);

		/* Border of the current output component*/
		l_x0_dest = int_ceildivpow2(l_img_comp_dest->x0, l_img_comp_dest->factor);
		l_y0_dest = int_ceildivpow2(l_img_comp_dest->y0, l_img_comp_dest->factor);
		l_x1_dest = l_x0_dest + l_img_comp_dest->w;
		l_y1_dest = l_y0_dest + l_img_comp_dest->h;

		/*if (i == 0) {
		fprintf(stdout, "DEST: l_x0_dest=%d, l_x1_dest=%d, l_y0_dest=%d, l_y1_dest=%d (%d)\n",
				l_x0_dest, l_x1_dest, l_y0_dest, l_y1_dest, l_img_comp_dest->factor );
		}*/

		/*-----*/
		/* Compute the area (l_offset_x0_src, l_offset_y0_src, l_offset_x1_src, l_offset_y1_src)
		 * of the input buffer (decoded tile component) which will be move
		 * in the output buffer. Compute the area of the output buffer (l_start_x_dest,
		 * l_start_y_dest, l_width_dest, l_height_dest)  which will be modified
		 * by this input area.
		 * */
		if ( l_x0_dest < l_res->x0 ) {
			l_start_x_dest = l_res->x0 - l_x0_dest;
			l_offset_x0_src = 0;

			if ( l_x1_dest >= l_res->x1 ) {
				l_width_dest = l_width_src;
				l_offset_x1_src = 0;
			}
			else {
				l_width_dest = l_x1_dest - l_res->x0 ;
				l_offset_x1_src = l_width_src - l_width_dest;
			}
		}
		else {
			l_start_x_dest = 0 ;
			l_offset_x0_src = l_x0_dest - l_res->x0;

			if ( l_x1_dest >= l_res->x1 ) {
				l_width_dest = l_width_src - l_offset_x0_src;
				l_offset_x1_src = 0;
			}
			else {
				l_width_dest = l_img_comp_dest->w ;
				l_offset_x1_src = l_res->x1 - l_x1_dest;
			}
		}

		if ( l_y0_dest < l_res->y0 ) {
			l_start_y_dest = l_res->y0 - l_y0_dest;
			l_offset_y0_src = 0;

			if ( l_y1_dest >= l_res->y1 ) {
				l_height_dest = l_height_src;
				l_offset_y1_src = 0;
			}
			else {
				l_height_dest = l_y1_dest - l_res->y0 ;
				l_offset_y1_src =  l_height_src - l_height_dest;
			}
		}
		else {
			l_start_y_dest = 0 ;
			l_offset_y0_src = l_y0_dest - l_res->y0;

			if ( l_y1_dest >= l_res->y1 ) {
				l_height_dest = l_height_src - l_offset_y0_src;
				l_offset_y1_src = 0;
			}
			else {
				l_height_dest = l_img_comp_dest->h ;
				l_offset_y1_src = l_res->y1 - l_y1_dest;
			}
		}

		if( (l_offset_x0_src < 0 ) || (l_offset_y0_src < 0 ) || (l_offset_x1_src < 0 ) || (l_offset_y1_src < 0 ) ){
			return OPJ_FALSE;
		}
		/*-----*/

		/* Compute the input buffer offset */
		l_start_offset_src = l_offset_x0_src + l_offset_y0_src * l_width_src;
		l_line_offset_src = l_offset_x1_src + l_offset_x0_src;
		l_end_offset_src = l_offset_y1_src * l_width_src - l_offset_x0_src;

		/* Compute the output buffer offset */
		l_start_offset_dest = l_start_x_dest + l_start_y_dest * l_img_comp_dest->w;
		l_line_offset_dest = l_img_comp_dest->w - l_width_dest;

		/* Move the output buffer to the first place where we will write*/
		l_dest_ptr = l_img_comp_dest->data + l_start_offset_dest;

		/*if (i == 0) {
			fprintf(stdout, "COMPO[%d]:\n",i);
			fprintf(stdout, "SRC: l_start_x_src=%d, l_start_y_src=%d, l_width_src=%d, l_height_src=%d\n"
					"\t tile offset:%d, %d, %d, %d\n"
					"\t buffer offset: %d; %d, %d\n",
					l_res->x0, l_res->y0, l_width_src, l_height_src,
					l_offset_x0_src, l_offset_y0_src, l_offset_x1_src, l_offset_y1_src,
					l_start_offset_src, l_line_offset_src, l_end_offset_src);

			fprintf(stdout, "DEST: l_start_x_dest=%d, l_start_y_dest=%d, l_width_dest=%d, l_height_dest=%d\n"
					"\t start offset: %d, line offset= %d\n",
					l_start_x_dest, l_start_y_dest, l_width_dest, l_height_dest, l_start_offset_dest, l_line_offset_dest);
		}*/


		switch (l_size_comp) {
			case 1:
				{
					OPJ_CHAR * l_src_ptr = (OPJ_CHAR*) p_data;
					l_src_ptr += l_start_offset_src; /* Move to the first place where we will read*/

					if (l_img_comp_src->sgnd) {
						for (j = 0 ; j < l_height_dest ; ++j) {
							for ( k = 0 ; k < l_width_dest ; ++k) {
								*(l_dest_ptr++) = (OPJ_INT32) (*(l_src_ptr++)); /* Copy only the data needed for the output image */
							}

							l_dest_ptr+= l_line_offset_dest; /* Move to the next place where we will write */
							l_src_ptr += l_line_offset_src ; /* Move to the next place where we will read */
						}
					}
					else {
						for ( j = 0 ; j < l_height_dest ; ++j ) {
							for ( k = 0 ; k < l_width_dest ; ++k) {
								*(l_dest_ptr++) = (OPJ_INT32) ((*(l_src_ptr++))&0xff);
							}

							l_dest_ptr+= l_line_offset_dest;
							l_src_ptr += l_line_offset_src;
						}
					}

					l_src_ptr += l_end_offset_src; /* Move to the end of this component-part of the input buffer */
					p_data = (OPJ_BYTE*) l_src_ptr; /* Keep the current position for the next component-part */
				}
				break;
			case 2:
				{
					OPJ_INT16 * l_src_ptr = (OPJ_INT16 *) p_data;
					l_src_ptr += l_start_offset_src;

					if (l_img_comp_src->sgnd) {
						for (j=0;j<l_height_dest;++j) {
							for (k=0;k<l_width_dest;++k) {
								*(l_dest_ptr++) = *(l_src_ptr++);
							}

							l_dest_ptr+= l_line_offset_dest;
							l_src_ptr += l_line_offset_src ;
						}
					}
					else {
						for (j=0;j<l_height_dest;++j) {
							for (k=0;k<l_width_dest;++k) {
								*(l_dest_ptr++) = (*(l_src_ptr++))&0xffff;
							}

							l_dest_ptr+= l_line_offset_dest;
							l_src_ptr += l_line_offset_src ;
						}
					}

					l_src_ptr += l_end_offset_src;
					p_data = (OPJ_BYTE*) l_src_ptr;
				}
				break;
			case 4:
				{
					OPJ_INT32 * l_src_ptr = (OPJ_INT32 *) p_data;
					l_src_ptr += l_start_offset_src;

					for (j=0;j<l_height_dest;++j) {
						for (k=0;k<l_width_dest;++k) {
							*(l_dest_ptr++) = (*(l_src_ptr++));
						}

						l_dest_ptr+= l_line_offset_dest;
						l_src_ptr += l_line_offset_src ;
					}

					l_src_ptr += l_end_offset_src;
					p_data = (OPJ_BYTE*) l_src_ptr;
				}
				break;
		}

		++l_img_comp_dest;
		++l_img_comp_src;
		++l_tilec;
	}

	return OPJ_TRUE;
}

/**
 * Sets the given area to be decoded. This function should be called right after opj_read_header and before any tile header reading.
 *
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_start_x		the left position of the rectangle to decode (in image coordinates).
 * @param	p_end_x			the right position of the rectangle to decode (in image coordinates).
 * @param	p_start_y		the up position of the rectangle to decode (in image coordinates).
 * @param	p_end_y			the bottom position of the rectangle to decode (in image coordinates).
 * @param	p_manager		the user event manager
 *
 * @return	true			if the area could be set.
 */
opj_bool j2k_set_decode_area(	opj_j2k_v2_t *p_j2k,
								opj_image_t* p_image,
								OPJ_INT32 p_start_x, OPJ_INT32 p_start_y,
								OPJ_INT32 p_end_x, OPJ_INT32 p_end_y,
								struct opj_event_mgr * p_manager )
{
	opj_cp_v2_t * l_cp = &(p_j2k->m_cp);
	opj_image_t * l_image = p_j2k->m_private_image;

	OPJ_UINT32 it_comp;
	OPJ_INT32 l_comp_x1, l_comp_y1;
	opj_image_comp_t* l_img_comp = NULL;

	/* Check if we are read the main header */
	if (p_j2k->m_specific_param.m_decoder.m_state != J2K_STATE_TPHSOT) { /* FIXME J2K_DEC_STATE_TPHSOT)*/
		opj_event_msg_v2(p_manager, EVT_ERROR, "Need to decode the main header before begin to decode the remaining codestream");
		return OPJ_FALSE;
	}

	if ( !p_start_x && !p_start_y && !p_end_x && !p_end_y){
		opj_event_msg_v2(p_manager, EVT_INFO, "No decoded area parameters, set the decoded area to the whole image\n");

		p_j2k->m_specific_param.m_decoder.m_start_tile_x = 0;
		p_j2k->m_specific_param.m_decoder.m_start_tile_y = 0;
		p_j2k->m_specific_param.m_decoder.m_end_tile_x = l_cp->tw;
		p_j2k->m_specific_param.m_decoder.m_end_tile_y = l_cp->th;

		return OPJ_TRUE;
	}

	/* ----- */
	/* Check if the positions provided by the user are correct */

	/* Left */
	if (p_start_x > l_image->x1 ) {
		opj_event_msg_v2(p_manager, EVT_ERROR,
			"Left position of the decoded area (region_x0=%d) is outside the image area (Xsiz=%d).\n",
			p_start_x, l_image->x1);
		return OPJ_FALSE;
	}
	else if (p_start_x < l_image->x0){
		opj_event_msg_v2(p_manager, EVT_WARNING,
				"Left position of the decoded area (region_x0=%d) is outside the image area (XOsiz=%d).\n",
				p_start_x, l_image->x0);
		p_j2k->m_specific_param.m_decoder.m_start_tile_x = 0;
		p_image->x0 = l_image->x0;
	}
	else {
		p_j2k->m_specific_param.m_decoder.m_start_tile_x = (p_start_x - l_cp->tx0) / l_cp->tdx;
		p_image->x0 = p_start_x;
	}

	/* Up */
	if (p_start_y > l_image->y1){
		opj_event_msg_v2(p_manager, EVT_ERROR,
				"Up position of the decoded area (region_y0=%d) is outside the image area (Ysiz=%d).\n",
				p_start_y, l_image->y1);
		return OPJ_FALSE;
	}
	else if (p_start_y < l_image->y0){
		opj_event_msg_v2(p_manager, EVT_WARNING,
				"Up position of the decoded area (region_y0=%d) is outside the image area (YOsiz=%d).\n",
				p_start_y, l_image->y0);
		p_j2k->m_specific_param.m_decoder.m_start_tile_y = 0;
		p_image->y0 = l_image->y0;
	}
	else {
		p_j2k->m_specific_param.m_decoder.m_start_tile_y = (p_start_y - l_cp->ty0) / l_cp->tdy;
		p_image->y0 = p_start_y;
	}

	/* Right */
	if (p_end_x < l_image->x0) {
		opj_event_msg_v2(p_manager, EVT_ERROR,
			"Right position of the decoded area (region_x1=%d) is outside the image area (XOsiz=%d).\n",
			p_end_x, l_image->x0);
		return OPJ_FALSE;
	}
	else if (p_end_x > l_image->x1) {
		opj_event_msg_v2(p_manager, EVT_WARNING,
			"Right position of the decoded area (region_x1=%d) is outside the image area (Xsiz=%d).\n",
			p_end_x, l_image->x1);
		p_j2k->m_specific_param.m_decoder.m_end_tile_x = l_cp->tw;
		p_image->x1 = l_image->x1;
	}
	else {
		p_j2k->m_specific_param.m_decoder.m_end_tile_x = int_ceildiv((p_end_x - l_cp->tx0), l_cp->tdx);
		p_image->x1 = p_end_x;
	}

	/* Bottom */
	if (p_end_y < l_image->y0) {
		opj_event_msg_v2(p_manager, EVT_ERROR,
			"Bottom position of the decoded area (region_y1=%d) is outside the image area (YOsiz=%d).\n",
			p_end_y, l_image->y0);
		return OPJ_FALSE;
	}
	if (p_end_y > l_image->y1){
		opj_event_msg_v2(p_manager, EVT_WARNING,
			"Bottom position of the decoded area (region_y1=%d) is outside the image area (Ysiz=%d).\n",
			p_end_y, l_image->y1);
		p_j2k->m_specific_param.m_decoder.m_end_tile_y = l_cp->th;
		p_image->y1 = l_image->y1;
	}
	else{
		p_j2k->m_specific_param.m_decoder.m_end_tile_y = int_ceildiv((p_end_y - l_cp->ty0), l_cp->tdy);
		p_image->y1 = p_end_y;
	}
	/* ----- */

	p_j2k->m_specific_param.m_decoder.m_discard_tiles = 1;

	l_img_comp = p_image->comps;
	for (it_comp=0; it_comp < p_image->numcomps; ++it_comp)
	{
		OPJ_INT32 l_h,l_w;

		l_img_comp->x0 = int_ceildiv(p_image->x0, l_img_comp->dx);
		l_img_comp->y0 = int_ceildiv(p_image->y0, l_img_comp->dy);
		l_comp_x1 = int_ceildiv(p_image->x1, l_img_comp->dx);
		l_comp_y1 = int_ceildiv(p_image->y1, l_img_comp->dy);

		l_w = int_ceildivpow2(l_comp_x1, l_img_comp->factor)
				- int_ceildivpow2(l_img_comp->x0, l_img_comp->factor);
		if (l_w < 0){
			opj_event_msg_v2(p_manager, EVT_ERROR,
				"Size x of the decoded component image is incorrect (comp[%d].w=%d).\n",
				it_comp, l_w);
			return OPJ_FALSE;
		}
		l_img_comp->w = l_w;

		l_h = int_ceildivpow2(l_comp_y1, l_img_comp->factor)
				- int_ceildivpow2(l_img_comp->y0, l_img_comp->factor);
		if (l_h < 0){
			opj_event_msg_v2(p_manager, EVT_ERROR,
				"Size y of the decoded component image is incorrect (comp[%d].h=%d).\n",
				it_comp, l_h);
			return OPJ_FALSE;
		}
		l_img_comp->h = l_h;

		l_img_comp++;
	}

	opj_event_msg_v2( p_manager, EVT_INFO,"Setting decoding area to %d,%d,%d,%d\n",
			p_image->x0, p_image->y0, p_image->x1, p_image->y1);


	return OPJ_TRUE;
}


/* ----------------------------------------------------------------------- */
/* J2K / JPT decoder interface                                             */
/* ----------------------------------------------------------------------- */
/**
 * Creates a J2K decompression structure.
 *
 * @return a handle to a J2K decompressor if successful, NULL otherwise.
*/
opj_j2k_v2_t* j2k_create_decompress_v2()
{
	opj_j2k_v2_t *l_j2k = (opj_j2k_v2_t*) opj_malloc(sizeof(opj_j2k_v2_t));
	if (!l_j2k) {
		return 00;
	}
	memset(l_j2k,0,sizeof(opj_j2k_v2_t));

	l_j2k->m_is_decoder = 1;
	l_j2k->m_cp.m_is_decoder = 1;

	l_j2k->m_specific_param.m_decoder.m_default_tcp = (opj_tcp_v2_t*) opj_malloc(sizeof(opj_tcp_v2_t));
	if (!l_j2k->m_specific_param.m_decoder.m_default_tcp) {
		j2k_destroy(l_j2k);
		return 00;
	}
	memset(l_j2k->m_specific_param.m_decoder.m_default_tcp,0,sizeof(opj_tcp_v2_t));

	l_j2k->m_specific_param.m_decoder.m_header_data = (OPJ_BYTE *) opj_malloc(J2K_DEFAULT_HEADER_SIZE);
	if (! l_j2k->m_specific_param.m_decoder.m_header_data) {
		j2k_destroy(l_j2k);
		return 00;
	}

	l_j2k->m_specific_param.m_decoder.m_header_data_size = J2K_DEFAULT_HEADER_SIZE;

	l_j2k->m_specific_param.m_decoder.m_tile_ind_to_dec = -1 ;

	l_j2k->m_specific_param.m_decoder.m_last_sot_read_pos = 0 ;

	/* codestream index creation */
	l_j2k->cstr_index = j2k_create_cstr_index();

			/*(opj_codestream_index_t*) opj_malloc(sizeof(opj_codestream_index_t));
	if (!l_j2k->cstr_index){
		j2k_destroy(l_j2k);
		return NULL;
	}

	l_j2k->cstr_index->marker = (opj_marker_info_t*) opj_malloc(100 * sizeof(opj_marker_info_t));
*/

	/* validation list creation */
	l_j2k->m_validation_list = opj_procedure_list_create();
	if (! l_j2k->m_validation_list) {
		j2k_destroy(l_j2k);
		return 00;
	}

	/* execution list creation */
	l_j2k->m_procedure_list = opj_procedure_list_create();
	if (! l_j2k->m_procedure_list) {
		j2k_destroy(l_j2k);
		return 00;
	}

	return l_j2k;
}


opj_codestream_index_t* j2k_create_cstr_index(void)
{
	opj_codestream_index_t* cstr_index = (opj_codestream_index_t*)
			opj_calloc(1,sizeof(opj_codestream_index_t));
	if (!cstr_index)
		return NULL;

	cstr_index->maxmarknum = 100;
	cstr_index->marknum = 0;
	cstr_index->marker = (opj_marker_info_t*)
			opj_calloc(cstr_index->maxmarknum, sizeof(opj_marker_info_t));
	if (!cstr_index-> marker)
		return NULL;

	cstr_index->tile_index = NULL;

	return cstr_index;
}

/**
 * Reads a SPCod or SPCoc element, i.e. the coding style of a given component of a tile.
 * @param	p_header_data	the data contained in the COM box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the COM marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_SPCod_SPCoc(
						    opj_j2k_v2_t *p_j2k,
							OPJ_UINT32 compno,
							OPJ_BYTE * p_header_data,
							OPJ_UINT32 * p_header_size,
							struct opj_event_mgr * p_manager
							)
{
	OPJ_UINT32 i, l_tmp;
	opj_cp_v2_t *l_cp = NULL;
	opj_tcp_v2_t *l_tcp = NULL;
	opj_tccp_t *l_tccp = NULL;
	OPJ_BYTE * l_current_ptr = NULL;

	/* preconditions */
	assert(p_j2k != 00);
	assert(p_manager != 00);
	assert(p_header_data != 00);

	l_cp = &(p_j2k->m_cp);
	l_tcp = (p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH) ?
				&l_cp->tcps[p_j2k->m_current_tile_number] :
				p_j2k->m_specific_param.m_decoder.m_default_tcp;

	/* precondition again */
	assert(compno < p_j2k->m_private_image->numcomps);

	l_tccp = &l_tcp->tccps[compno];
	l_current_ptr = p_header_data;

	/* make sure room is sufficient */
	if (*p_header_size < 5) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading SPCod SPCoc element\n");
		return OPJ_FALSE;
	}

	opj_read_bytes(l_current_ptr, &l_tccp->numresolutions ,1);		/* SPcox (D) */
	++l_tccp->numresolutions;										/* tccp->numresolutions = read() + 1 */
	++l_current_ptr;

	/* If user wants to remove more resolutions than the codestream contains, return error */
	if (l_cp->m_specific_param.m_dec.m_reduce >= l_tccp->numresolutions) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error decoding component %d.\nThe number of resolutions to remove is higher than the number "
					"of resolutions of this component\nModify the cp_reduce parameter.\n\n", compno);
		p_j2k->m_specific_param.m_decoder.m_state |= 0x8000;/* FIXME J2K_DEC_STATE_ERR;*/
		return OPJ_FALSE;
	}

	opj_read_bytes(l_current_ptr,&l_tccp->cblkw ,1);		/* SPcoc (E) */
	++l_current_ptr;
	l_tccp->cblkw += 2;

	opj_read_bytes(l_current_ptr,&l_tccp->cblkh ,1);		/* SPcoc (F) */
	++l_current_ptr;
	l_tccp->cblkh += 2;

	opj_read_bytes(l_current_ptr,&l_tccp->cblksty ,1);		/* SPcoc (G) */
	++l_current_ptr;

	opj_read_bytes(l_current_ptr,&l_tccp->qmfbid ,1);		/* SPcoc (H) */
	++l_current_ptr;

	*p_header_size = *p_header_size - 5;

	/* use custom precinct size ? */
	if (l_tccp->csty & J2K_CCP_CSTY_PRT) {
		if (*p_header_size < l_tccp->numresolutions) {
			opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading SPCod SPCoc element\n");
			return OPJ_FALSE;
		}

		for	(i = 0; i < l_tccp->numresolutions; ++i) {
			opj_read_bytes(l_current_ptr,&l_tmp ,1);		/* SPcoc (I_i) */
			++l_current_ptr;
			l_tccp->prcw[i] = l_tmp & 0xf;
			l_tccp->prch[i] = l_tmp >> 4;
		}

		*p_header_size = *p_header_size - l_tccp->numresolutions;
	}
	else {
		/* set default size for the precinct width and height */
		for	(i = 0; i < l_tccp->numresolutions; ++i) {
			l_tccp->prcw[i] = 15;
			l_tccp->prch[i] = 15;
		}
	}

#ifdef WIP_REMOVE_MSD
	/* INDEX >> */
	if (p_j2k->cstr_info && compno == 0) {
		OPJ_UINT32 l_data_size = l_tccp->numresolutions * sizeof(OPJ_UINT32);

		p_j2k->cstr_info->tile[p_j2k->m_current_tile_number].tccp_info[compno].cblkh = l_tccp->cblkh;
		p_j2k->cstr_info->tile[p_j2k->m_current_tile_number].tccp_info[compno].cblkw = l_tccp->cblkw;
		p_j2k->cstr_info->tile[p_j2k->m_current_tile_number].tccp_info[compno].numresolutions = l_tccp->numresolutions;
		p_j2k->cstr_info->tile[p_j2k->m_current_tile_number].tccp_info[compno].cblksty = l_tccp->cblksty;
		p_j2k->cstr_info->tile[p_j2k->m_current_tile_number].tccp_info[compno].qmfbid = l_tccp->qmfbid;


		memcpy(p_j2k->cstr_info->tile[p_j2k->m_current_tile_number].pdx,l_tccp->prcw, l_data_size);
		memcpy(p_j2k->cstr_info->tile[p_j2k->m_current_tile_number].pdy,l_tccp->prch, l_data_size);
	}
	/* << INDEX */
#endif

	return OPJ_TRUE;
}

/**
 * Copies the tile component parameters of all the component from the first tile component.
 *
 * @param		p_j2k		the J2k codec.
 */
void j2k_copy_tile_component_parameters( opj_j2k_v2_t *p_j2k )
{
	/* loop */
	OPJ_UINT32 i;
	opj_cp_v2_t *l_cp = NULL;
	opj_tcp_v2_t *l_tcp = NULL;
	opj_tccp_t *l_ref_tccp = NULL, *l_copied_tccp = NULL;
	OPJ_UINT32 l_prc_size;

	/* preconditions */
	assert(p_j2k != 00);

	l_cp = &(p_j2k->m_cp);
	l_tcp = (p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH) ? /* FIXME J2K_DEC_STATE_TPH*/
				&l_cp->tcps[p_j2k->m_current_tile_number] :
				p_j2k->m_specific_param.m_decoder.m_default_tcp;

	l_ref_tccp = &l_tcp->tccps[0];
	l_copied_tccp = l_ref_tccp + 1;
	l_prc_size = l_ref_tccp->numresolutions * sizeof(OPJ_UINT32);

	for	(i=1; i<p_j2k->m_private_image->numcomps; ++i) {
		l_copied_tccp->numresolutions = l_ref_tccp->numresolutions;
		l_copied_tccp->cblkw = l_ref_tccp->cblkw;
		l_copied_tccp->cblkh = l_ref_tccp->cblkh;
		l_copied_tccp->cblksty = l_ref_tccp->cblksty;
		l_copied_tccp->qmfbid = l_ref_tccp->qmfbid;
		memcpy(l_copied_tccp->prcw,l_ref_tccp->prcw,l_prc_size);
		memcpy(l_copied_tccp->prch,l_ref_tccp->prch,l_prc_size);
		++l_copied_tccp;
	}
}

/**
 * Reads a SQcd or SQcc element, i.e. the quantization values of a band.
 *
 * @param	p_comp_no		the component being targeted.
 * @param	p_header_data	the data contained in the COM box.
 * @param	p_j2k			the jpeg2000 codec.
 * @param	p_header_size	the size of the data contained in the COM marker.
 * @param	p_manager		the user event manager.
*/
opj_bool j2k_read_SQcd_SQcc(
							opj_j2k_v2_t *p_j2k,
							OPJ_UINT32 p_comp_no,
							OPJ_BYTE* p_header_data,
							OPJ_UINT32 * p_header_size,
							struct opj_event_mgr * p_manager
							)
{
	/* loop*/
	OPJ_UINT32 l_band_no;
	opj_cp_v2_t *l_cp = 00;
	opj_tcp_v2_t *l_tcp = 00;
	opj_tccp_t *l_tccp = 00;
	OPJ_BYTE * l_current_ptr = 00;
	OPJ_UINT32 l_tmp, l_num_band;

	/* preconditions*/
	assert(p_j2k != 00);
	assert(p_manager != 00);
	assert(p_header_data != 00);

	l_cp = &(p_j2k->m_cp);
	/* come from tile part header or main header ?*/
	l_tcp = (p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH) ? /*FIXME J2K_DEC_STATE_TPH*/
				&l_cp->tcps[p_j2k->m_current_tile_number] :
				p_j2k->m_specific_param.m_decoder.m_default_tcp;

	/* precondition again*/
	assert(p_comp_no <  p_j2k->m_private_image->numcomps);

	l_tccp = &l_tcp->tccps[p_comp_no];
	l_current_ptr = p_header_data;

	if (*p_header_size < 1) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "Error reading SQcd or SQcc element\n");
		return OPJ_FALSE;
	}
	*p_header_size -= 1;

	opj_read_bytes(l_current_ptr, &l_tmp ,1);			/* Sqcx */
	++l_current_ptr;

	l_tccp->qntsty = l_tmp & 0x1f;
	l_tccp->numgbits = l_tmp >> 5;
	if (l_tccp->qntsty == J2K_CCP_QNTSTY_SIQNT) {
        l_num_band = 1;
	}
	else {
		l_num_band = (l_tccp->qntsty == J2K_CCP_QNTSTY_NOQNT) ?
			(*p_header_size) :
			(*p_header_size) / 2;

		if( l_num_band > J2K_MAXBANDS ) {
			opj_event_msg_v2(p_manager, EVT_WARNING, "While reading CCP_QNTSTY element inside QCD or QCC marker segment, "
				"number of subbands (%d) is greater to J2K_MAXBANDS (%d). So we limit the number of elements stored to "
				"J2K_MAXBANDS (%d) and skip the rest. \n", l_num_band, J2K_MAXBANDS, J2K_MAXBANDS);
			/*return OPJ_FALSE;*/
		}
	}

#ifdef USE_JPWL
	if (l_cp->correct) {

		/* if JPWL is on, we check whether there are too many subbands */
		if ((l_num_band < 0) || (l_num_band >= J2K_MAXBANDS)) {
			opj_event_msg_v2(p_manager, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
				"JPWL: bad number of subbands in Sqcx (%d)\n",
				l_num_band);
			if (!JPWL_ASSUME) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "JPWL: giving up\n");
				return OPJ_FALSE;
			}
			/* we try to correct */
			l_num_band = 1;
			opj_event_msg_v2(p_manager, EVT_WARNING, "- trying to adjust them\n"
				"- setting number of bands to %d => HYPOTHESIS!!!\n",
				l_num_band);
		};

	};
#endif /* USE_JPWL */

	if (l_tccp->qntsty == J2K_CCP_QNTSTY_NOQNT) {
		for	(l_band_no = 0; l_band_no < l_num_band; l_band_no++) {
			opj_read_bytes(l_current_ptr, &l_tmp ,1);			/* SPqcx_i */
			++l_current_ptr;
			if (l_band_no < J2K_MAXBANDS){
				l_tccp->stepsizes[l_band_no].expn = l_tmp>>3;
				l_tccp->stepsizes[l_band_no].mant = 0;
			}
		}
		*p_header_size = *p_header_size - l_num_band;
	}
	else {
		for	(l_band_no = 0; l_band_no < l_num_band; l_band_no++) {
			opj_read_bytes(l_current_ptr, &l_tmp ,2);			/* SPqcx_i */
			l_current_ptr+=2;
			if (l_band_no < J2K_MAXBANDS){
				l_tccp->stepsizes[l_band_no].expn = l_tmp >> 11;
				l_tccp->stepsizes[l_band_no].mant = l_tmp & 0x7ff;
			}
		}
		*p_header_size = *p_header_size - 2*l_num_band;
	}

	/* Add Antonin : if scalar_derived -> compute other stepsizes */
	if (l_tccp->qntsty == J2K_CCP_QNTSTY_SIQNT) {
		for (l_band_no = 1; l_band_no < J2K_MAXBANDS; l_band_no++) {
			l_tccp->stepsizes[l_band_no].expn =
				((l_tccp->stepsizes[0].expn) - ((l_band_no - 1) / 3) > 0) ?
					(l_tccp->stepsizes[0].expn) - ((l_band_no - 1) / 3) : 0;
			l_tccp->stepsizes[l_band_no].mant = l_tccp->stepsizes[0].mant;
		}
	}

	return OPJ_TRUE;
}

/**
 * Copies the tile component parameters of all the component from the first tile component.
 *
 * @param		p_j2k		the J2k codec.
 */
void j2k_copy_tile_quantization_parameters( opj_j2k_v2_t *p_j2k )
{
	OPJ_UINT32 i;
	opj_cp_v2_t *l_cp = NULL;
	opj_tcp_v2_t *l_tcp = NULL;
	opj_tccp_t *l_ref_tccp = NULL;
	opj_tccp_t *l_copied_tccp = NULL;
	OPJ_UINT32 l_size;

	/* preconditions */
	assert(p_j2k != 00);

	l_cp = &(p_j2k->m_cp);
	l_tcp = p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_TPH ?
			&l_cp->tcps[p_j2k->m_current_tile_number] :
			p_j2k->m_specific_param.m_decoder.m_default_tcp;

	l_ref_tccp = &l_tcp->tccps[0];
	l_copied_tccp = l_ref_tccp + 1;
	l_size = J2K_MAXBANDS * sizeof(opj_stepsize_t);

	for	(i=1;i<p_j2k->m_private_image->numcomps;++i) {
		l_copied_tccp->qntsty = l_ref_tccp->qntsty;
		l_copied_tccp->numgbits = l_ref_tccp->numgbits;
		memcpy(l_copied_tccp->stepsizes,l_ref_tccp->stepsizes,l_size);
		++l_copied_tccp;
	}
}

/**
 * Dump some elements from the J2K decompression structure .
 *
 *@param p_j2k				the jpeg2000 codec.
 *@param flag				flag to describe what elments are dump.
 *@param out_stream			output stream where dump the elements.
 *
*/
void j2k_dump (opj_j2k_v2_t* p_j2k, OPJ_INT32 flag, FILE* out_stream)
{
	/* Check if the flag is compatible with j2k file*/
	if ( (flag & OPJ_JP2_INFO) || (flag & OPJ_JP2_IND)){
		fprintf(out_stream, "Wrong flag\n");
		return;
	}

	/* Dump the image_header */
	if (flag & OPJ_IMG_INFO){
		if (p_j2k->m_private_image)
			j2k_dump_image_header(p_j2k->m_private_image, 0, out_stream);
	}

	/* Dump the codestream info from main header */
	if (flag & OPJ_J2K_MH_INFO){
		j2k_dump_MH_info(p_j2k, out_stream);
	}


	/* Dump the codestream info of the current tile */
	if (flag & OPJ_J2K_TH_INFO){

	}

	/* Dump the codestream index from main header */
	if (flag & OPJ_J2K_MH_IND){
		j2k_dump_MH_index(p_j2k, out_stream);
	}

	/* Dump the codestream index of the current tile */
	if (flag & OPJ_J2K_TH_IND){

	}

}

/**
 * Dump index elements of the codestream extract from the main header.
 *
 *@param p_j2k				the jpeg2000 codec.
 *@param out_stream			output stream where dump the elements.
 *
*/
void j2k_dump_MH_index(opj_j2k_v2_t* p_j2k, FILE* out_stream)
{
	opj_codestream_index_t* cstr_index = p_j2k->cstr_index;
	OPJ_UINT32 it_marker, it_tile, it_tile_part;

	fprintf(out_stream, "Codestream index from main header: {\n");

	fprintf(out_stream, "\t Main header start position=%" OPJ_OFF_F "d\n"
			            "\t Main header end position=%" OPJ_OFF_F "d\n",
			cstr_index->main_head_start, cstr_index->main_head_end);

	fprintf(out_stream, "\t Marker list: {\n");

	if (cstr_index->marker){
		for (it_marker=0; it_marker < cstr_index->marknum ; it_marker++){
			fprintf(out_stream, "\t\t type=%#x, pos=%" OPJ_OFF_F "d, len=%d\n",
					cstr_index->marker[it_marker].type,
					cstr_index->marker[it_marker].pos,
					cstr_index->marker[it_marker].len );
		}
	}

	fprintf(out_stream, "\t }\n");


	if (cstr_index->tile_index){
		fprintf(out_stream, "\t Tile index: {\n");

		for (it_tile=0; it_tile < cstr_index->nb_of_tiles ; it_tile++){
			OPJ_UINT32 nb_of_tile_part = cstr_index->tile_index[it_tile].nb_tps;

			fprintf(out_stream, "\t\t nb of tile-part in tile [%d]=%d\n", it_tile, nb_of_tile_part);

			if (cstr_index->tile_index[it_tile].tp_index){
				for (it_tile_part =0; it_tile_part < nb_of_tile_part; it_tile_part++){
					fprintf(out_stream, "\t\t\t tile-part[%d]: star_pos=%" OPJ_OFF_F "d, end_header=%" OPJ_OFF_F "d, end_pos=%" OPJ_OFF_F "d.\n",
							it_tile_part,
							cstr_index->tile_index[it_tile].tp_index[it_tile_part].start_pos,
							cstr_index->tile_index[it_tile].tp_index[it_tile_part].end_header,
							cstr_index->tile_index[it_tile].tp_index[it_tile_part].end_pos);
				}
			}

			if (cstr_index->tile_index[it_tile].marker){
				for (it_marker=0; it_marker < cstr_index->tile_index[it_tile].marknum ; it_marker++){
					fprintf(out_stream, "\t\t type=%#x, pos=%" OPJ_OFF_F "d, len=%d\n",
							cstr_index->tile_index[it_tile].marker[it_marker].type,
							cstr_index->tile_index[it_tile].marker[it_marker].pos,
							cstr_index->tile_index[it_tile].marker[it_marker].len );
				}
			}
		}
		fprintf(out_stream,"\t }\n");
	}

	fprintf(out_stream,"}\n");

}

/**
 * Dump info elements of the codestream extract from the main header.
 *
 *@param p_j2k				the jpeg2000 codec.
 *@param out_stream			output stream where dump the elements.
 *
*/
void j2k_dump_MH_info(opj_j2k_v2_t* p_j2k, FILE* out_stream)
{
	opj_tcp_v2_t * l_default_tile=NULL;

	fprintf(out_stream, "Codestream info from main header: {\n");

	fprintf(out_stream, "\t tx0=%d, ty0=%d\n", p_j2k->m_cp.tx0, p_j2k->m_cp.ty0);
	fprintf(out_stream, "\t tdx=%d, tdy=%d\n", p_j2k->m_cp.tdx, p_j2k->m_cp.tdy);
	fprintf(out_stream, "\t tw=%d, th=%d\n", p_j2k->m_cp.tw, p_j2k->m_cp.th);

	l_default_tile = p_j2k->m_specific_param.m_decoder.m_default_tcp;
	if (l_default_tile)
	{
		OPJ_INT32 compno;
		OPJ_INT32 numcomps = p_j2k->m_private_image->numcomps;

		fprintf(out_stream, "\t default tile {\n");
		fprintf(out_stream, "\t\t csty=%#x\n", l_default_tile->csty);
		fprintf(out_stream, "\t\t prg=%#x\n", l_default_tile->prg);
		fprintf(out_stream, "\t\t numlayers=%d\n", l_default_tile->numlayers);
		fprintf(out_stream, "\t\t mct=%x\n", l_default_tile->mct);

		for (compno = 0; compno < numcomps; compno++) {
			opj_tccp_t *l_tccp = &(l_default_tile->tccps[compno]);
			OPJ_INT32 resno, bandno, numbands;

			/* coding style*/
			fprintf(out_stream, "\t\t comp %d {\n", compno);
			fprintf(out_stream, "\t\t\t csty=%#x\n", l_tccp->csty);
			fprintf(out_stream, "\t\t\t numresolutions=%d\n", l_tccp->numresolutions);
			fprintf(out_stream, "\t\t\t cblkw=2^%d\n", l_tccp->cblkw);
			fprintf(out_stream, "\t\t\t cblkh=2^%d\n", l_tccp->cblkh);
			fprintf(out_stream, "\t\t\t cblksty=%#x\n", l_tccp->cblksty);
			fprintf(out_stream, "\t\t\t qmfbid=%d\n", l_tccp->qmfbid);

			fprintf(out_stream, "\t\t\t preccintsize (w,h)=");
			for (resno = 0; resno < l_tccp->numresolutions; resno++) {
				fprintf(out_stream, "(%d,%d) ", l_tccp->prcw[resno], l_tccp->prch[resno]);
			}
			fprintf(out_stream, "\n");

			/* quantization style*/
			fprintf(out_stream, "\t\t\t qntsty=%d\n", l_tccp->qntsty);
			fprintf(out_stream, "\t\t\t numgbits=%d\n", l_tccp->numgbits);
			fprintf(out_stream, "\t\t\t stepsizes (m,e)=");
			numbands = (l_tccp->qntsty == J2K_CCP_QNTSTY_SIQNT) ? 1 : l_tccp->numresolutions * 3 - 2;
			for (bandno = 0; bandno < numbands; bandno++) {
				fprintf(out_stream, "(%d,%d) ", l_tccp->stepsizes[bandno].mant,
					l_tccp->stepsizes[bandno].expn);
			}
			fprintf(out_stream, "\n");

			/* RGN value*/
			fprintf(out_stream, "\t\t\t roishift=%d\n", l_tccp->roishift);

			fprintf(out_stream, "\t\t }\n");
		} /*end of component of default tile*/
		fprintf(out_stream, "\t }\n"); /*end of default tile*/

	}

	fprintf(out_stream, "}\n");

}

/**
 * Dump an image header structure.
 *
 *@param img_header			the image header to dump.
 *@param dev_dump_flag		flag to describe if we are in the case of this function is use outside j2k_dump function
 *@param out_stream			output stream where dump the elements.
 */
void j2k_dump_image_header(opj_image_t* img_header, opj_bool dev_dump_flag, FILE* out_stream)
{
	char tab[2];

	if (dev_dump_flag){
		fprintf(stdout, "[DEV] Dump an image_header struct {\n");
		tab[0] = '\0';
	}
	else {
		fprintf(out_stream, "Image info {\n");
		tab[0] = '\t';tab[1] = '\0';
	}

	fprintf(out_stream, "%s x0=%d, y0=%d\n", tab, img_header->x0, img_header->y0);
	fprintf(out_stream,	"%s x1=%d, y1=%d\n", tab, img_header->x1, img_header->y1);
	fprintf(out_stream, "%s numcomps=%d\n", tab, img_header->numcomps);

	if (img_header->comps){
		int compno;
		for (compno = 0; compno < img_header->numcomps; compno++) {
			fprintf(out_stream, "%s\t component %d {\n", tab, compno);
			j2k_dump_image_comp_header(&(img_header->comps[compno]), dev_dump_flag, out_stream);
			fprintf(out_stream,"%s}\n",tab);
		}
	}

	fprintf(out_stream, "}\n");
}

/**
 * Dump a component image header structure.
 *
 *@param comp_header		the component image header to dump.
 *@param dev_dump_flag		flag to describe if we are in the case of this function is use outside j2k_dump function
 *@param out_stream			output stream where dump the elements.
 */
void j2k_dump_image_comp_header(opj_image_comp_t* comp_header, opj_bool dev_dump_flag, FILE* out_stream)
{
	char tab[3];

	if (dev_dump_flag){
		fprintf(stdout, "[DEV] Dump an image_comp_header struct {\n");
		tab[0] = '\0';
	}	else {
		tab[0] = '\t';tab[1] = '\t';tab[2] = '\0';
	}

	fprintf(out_stream, "%s dx=%d, dy=%d\n", tab, comp_header->dx, comp_header->dy);
	fprintf(out_stream, "%s prec=%d\n", tab, comp_header->prec);
	fprintf(out_stream, "%s sgnd=%d\n", tab, comp_header->sgnd);

	if (dev_dump_flag)
		fprintf(out_stream, "}\n");
}


/**
 * Get the codestream info from a JPEG2000 codec.
 *
 *@param	p_j2k				the component image header to dump.
 *
 *@return	the codestream information extract from the jpg2000 codec
 */
opj_codestream_info_v2_t* j2k_get_cstr_info(opj_j2k_v2_t* p_j2k)
{
	OPJ_UINT16 compno;
	OPJ_UINT16 numcomps = p_j2k->m_private_image->numcomps;
	opj_tcp_v2_t *l_default_tile;
	opj_codestream_info_v2_t* cstr_info = (opj_codestream_info_v2_t*) opj_calloc(1,sizeof(opj_codestream_info_v2_t));

	cstr_info->nbcomps = p_j2k->m_private_image->numcomps;

	cstr_info->tx0 = p_j2k->m_cp.tx0;
	cstr_info->ty0 = p_j2k->m_cp.ty0;
	cstr_info->tdx = p_j2k->m_cp.tdx;
	cstr_info->tdy = p_j2k->m_cp.tdy;
	cstr_info->tw = p_j2k->m_cp.tw;
	cstr_info->th = p_j2k->m_cp.th;

	cstr_info->tile_info = NULL; /* Not fill from the main header*/

	l_default_tile = p_j2k->m_specific_param.m_decoder.m_default_tcp;

	cstr_info->m_default_tile_info.csty = l_default_tile->csty;
	cstr_info->m_default_tile_info.prg = l_default_tile->prg;
	cstr_info->m_default_tile_info.numlayers = l_default_tile->numlayers;
	cstr_info->m_default_tile_info.mct = l_default_tile->mct;

	cstr_info->m_default_tile_info.tccp_info = (opj_tccp_info_t*) opj_calloc(cstr_info->nbcomps, sizeof(opj_tccp_info_t));

	for (compno = 0; compno < numcomps; compno++) {
		opj_tccp_t *l_tccp = &(l_default_tile->tccps[compno]);
		opj_tccp_info_t *l_tccp_info = &(cstr_info->m_default_tile_info.tccp_info[compno]);
		OPJ_INT32 bandno, numbands;

		/* coding style*/
		l_tccp_info->csty = l_tccp->csty;
		l_tccp_info->numresolutions = l_tccp->numresolutions;
		l_tccp_info->cblkw = l_tccp->cblkw;
		l_tccp_info->cblkh = l_tccp->cblkh;
		l_tccp_info->cblksty = l_tccp->cblksty;
		l_tccp_info->qmfbid = l_tccp->qmfbid;
		if (l_tccp->numresolutions < J2K_MAXRLVLS)
		{
			memcpy(l_tccp_info->prch, l_tccp->prch, l_tccp->numresolutions);
			memcpy(l_tccp_info->prcw, l_tccp->prcw, l_tccp->numresolutions);
		}

		/* quantization style*/
		l_tccp_info->qntsty = l_tccp->qntsty;
		l_tccp_info->numgbits = l_tccp->numgbits;

		numbands = (l_tccp->qntsty == J2K_CCP_QNTSTY_SIQNT) ? 1 : l_tccp->numresolutions * 3 - 2;
		if (numbands < J2K_MAXBANDS) {
			for (bandno = 0; bandno < numbands; bandno++) {
				l_tccp_info->stepsizes_mant[bandno] = l_tccp->stepsizes[bandno].mant;
				l_tccp_info->stepsizes_expn[bandno] = l_tccp->stepsizes[bandno].expn;
			}
		}

		/* RGN value*/
		l_tccp_info->roishift = l_tccp->roishift;
	}


	return cstr_info;
}

/**
 * Get the codestream index from a JPEG2000 codec.
 *
 *@param	p_j2k				the component image header to dump.
 *
 *@return	the codestream index extract from the jpg2000 codec
 */
opj_codestream_index_t* j2k_get_cstr_index(opj_j2k_v2_t* p_j2k)
{
 	opj_codestream_index_t* l_cstr_index = (opj_codestream_index_t*)
			opj_calloc(1,sizeof(opj_codestream_index_t));
	if (!l_cstr_index)
		return NULL;

	l_cstr_index->main_head_start = p_j2k->cstr_index->main_head_start;
	l_cstr_index->main_head_end = p_j2k->cstr_index->main_head_end;
	l_cstr_index->codestream_size = p_j2k->cstr_index->codestream_size;

	l_cstr_index->marknum = p_j2k->cstr_index->marknum;
	l_cstr_index->marker = (opj_marker_info_t*)opj_malloc(l_cstr_index->marknum*sizeof(opj_marker_info_t));
	if (!l_cstr_index->marker){
		opj_free( l_cstr_index);
		return NULL;
	}

	if (p_j2k->cstr_index->marker)
		memcpy(l_cstr_index->marker, p_j2k->cstr_index->marker, l_cstr_index->marknum * sizeof(opj_marker_info_t) );
	else{
		opj_free(l_cstr_index->marker);
		l_cstr_index->marker = NULL;
	}

	l_cstr_index->nb_of_tiles = p_j2k->cstr_index->nb_of_tiles;
	l_cstr_index->tile_index = (opj_tile_index_t*)opj_calloc(l_cstr_index->nb_of_tiles, sizeof(opj_tile_index_t) );
	if (!l_cstr_index->tile_index){
		opj_free( l_cstr_index->marker);
		opj_free( l_cstr_index);
		return NULL;
	}

	if (!p_j2k->cstr_index->tile_index){
		opj_free(l_cstr_index->tile_index);
		l_cstr_index->tile_index = NULL;
	}
	else {
		OPJ_UINT32 it_tile = 0;
		for (it_tile = 0; it_tile < l_cstr_index->nb_of_tiles; it_tile++ ){

			/* Tile Marker*/
			l_cstr_index->tile_index[it_tile].marknum = p_j2k->cstr_index->tile_index[it_tile].marknum;

			l_cstr_index->tile_index[it_tile].marker =
				(opj_marker_info_t*)opj_malloc(l_cstr_index->tile_index[it_tile].marknum*sizeof(opj_marker_info_t));

			if (!l_cstr_index->tile_index[it_tile].marker) {
				OPJ_UINT32 it_tile_free;

				for (it_tile_free=0; it_tile_free < it_tile; it_tile_free++){
					opj_free(l_cstr_index->tile_index[it_tile_free].marker);
				}

				opj_free( l_cstr_index->tile_index);
				opj_free( l_cstr_index->marker);
				opj_free( l_cstr_index);
				return NULL;
			}

			if (p_j2k->cstr_index->tile_index[it_tile].marker)
				memcpy(	l_cstr_index->tile_index[it_tile].marker,
						p_j2k->cstr_index->tile_index[it_tile].marker,
						l_cstr_index->tile_index[it_tile].marknum * sizeof(opj_marker_info_t) );
			else{
				opj_free(l_cstr_index->tile_index[it_tile].marker);
				l_cstr_index->tile_index[it_tile].marker = NULL;
			}

			/* Tile part index*/
			l_cstr_index->tile_index[it_tile].nb_tps = p_j2k->cstr_index->tile_index[it_tile].nb_tps;

			l_cstr_index->tile_index[it_tile].tp_index =
				(opj_tp_index_t*)opj_malloc(l_cstr_index->tile_index[it_tile].nb_tps*sizeof(opj_tp_index_t));

			if(!l_cstr_index->tile_index[it_tile].tp_index){
				OPJ_UINT32 it_tile_free;

				for (it_tile_free=0; it_tile_free < it_tile; it_tile_free++){
					opj_free(l_cstr_index->tile_index[it_tile_free].marker);
					opj_free(l_cstr_index->tile_index[it_tile_free].tp_index);
				}

				opj_free( l_cstr_index->tile_index);
				opj_free( l_cstr_index->marker);
				opj_free( l_cstr_index);
				return NULL;
			}

			if (p_j2k->cstr_index->tile_index[it_tile].tp_index){
				memcpy(	l_cstr_index->tile_index[it_tile].tp_index,
						p_j2k->cstr_index->tile_index[it_tile].tp_index,
						l_cstr_index->tile_index[it_tile].nb_tps * sizeof(opj_tp_index_t) );
			}
			else{
				opj_free(l_cstr_index->tile_index[it_tile].tp_index);
				l_cstr_index->tile_index[it_tile].tp_index = NULL;
			}

			/* Packet index (NOT USED)*/
			l_cstr_index->tile_index[it_tile].nb_packet = 0;
			l_cstr_index->tile_index[it_tile].packet_index = NULL;

		}
	}

	return l_cstr_index;
}

opj_bool j2k_allocate_tile_element_cstr_index(opj_j2k_v2_t *p_j2k)
{
	OPJ_UINT32 it_tile=0;

	p_j2k->cstr_index->nb_of_tiles = p_j2k->m_cp.tw * p_j2k->m_cp.th;
	p_j2k->cstr_index->tile_index = (opj_tile_index_t*)opj_calloc(p_j2k->cstr_index->nb_of_tiles, sizeof(opj_tile_index_t));
	if (!p_j2k->cstr_index->tile_index)
		return OPJ_FALSE;

	for (it_tile=0; it_tile < p_j2k->cstr_index->nb_of_tiles; it_tile++){
		p_j2k->cstr_index->tile_index[it_tile].maxmarknum = 100;
		p_j2k->cstr_index->tile_index[it_tile].marknum = 0;
		p_j2k->cstr_index->tile_index[it_tile].marker = (opj_marker_info_t*)
				opj_calloc(p_j2k->cstr_index->tile_index[it_tile].maxmarknum, sizeof(opj_marker_info_t));
		if (!p_j2k->cstr_index->tile_index[it_tile].marker)
			return OPJ_FALSE;
	}

	return OPJ_TRUE;
}

/**
 * Reads the tiles.
 */
opj_bool j2k_decode_tiles (	opj_j2k_v2_t *p_j2k,
							opj_stream_private_t *p_stream,
							opj_event_mgr_t * p_manager)
{
	opj_bool l_go_on = OPJ_TRUE;
	OPJ_UINT32 l_current_tile_no;
	OPJ_UINT32 l_data_size,l_max_data_size;
	OPJ_INT32 l_tile_x0,l_tile_y0,l_tile_x1,l_tile_y1;
	OPJ_UINT32 l_nb_comps;
	OPJ_BYTE * l_current_data;

	l_current_data = (OPJ_BYTE*)opj_malloc(1000);
	if (! l_current_data) {
		return OPJ_FALSE;
	}
	l_max_data_size = 1000;

	/*Allocate and initialize some elements of codestrem index*/
	if (!j2k_allocate_tile_element_cstr_index(p_j2k)){
		opj_free(l_current_data);
		return OPJ_FALSE;
	}

	while (OPJ_TRUE) {
		if (! j2k_read_tile_header(	p_j2k,
									&l_current_tile_no,
									&l_data_size,
									&l_tile_x0, &l_tile_y0,
									&l_tile_x1, &l_tile_y1,
									&l_nb_comps,
									&l_go_on,
									p_stream,
									p_manager)) {
			opj_free(l_current_data);
			return OPJ_FALSE;
		}

		if (! l_go_on) {
			break;
		}

		if (l_data_size > l_max_data_size) {
			l_current_data = (OPJ_BYTE*)opj_realloc(l_current_data,l_data_size);
			if (! l_current_data) {
				opj_free(l_current_data);
				return OPJ_FALSE;
			}

			l_max_data_size = l_data_size;
		}

		if (! j2k_decode_tile(p_j2k,l_current_tile_no,l_current_data,l_data_size,p_stream,p_manager)) {
			opj_free(l_current_data);
			return OPJ_FALSE;
		}
		opj_event_msg_v2(p_manager, EVT_INFO, "Tile %d/%d has been decoded.\n", l_current_tile_no +1, p_j2k->m_cp.th * p_j2k->m_cp.tw);

		if (! j2k_update_image_data(p_j2k->m_tcd,l_current_data, p_j2k->m_output_image)) {
			opj_free(l_current_data);
			return OPJ_FALSE;
		}
		opj_event_msg_v2(p_manager, EVT_INFO, "Image data has been updated with tile %d.\n\n", l_current_tile_no + 1);

	}

	opj_free(l_current_data);

	return OPJ_TRUE;
}

/**
 * Sets up the procedures to do on decoding data. Developpers wanting to extend the library can add their own reading procedures.
 */
void j2k_setup_decoding (opj_j2k_v2_t *p_j2k)
{
	/* preconditions*/
	assert(p_j2k != 00);

	opj_procedure_list_add_procedure(p_j2k->m_procedure_list,(void*)j2k_decode_tiles);
	/* DEVELOPER CORNER, add your custom procedures */

}

/*
 * Read and decode one tile.
 */
opj_bool j2k_decode_one_tile (	opj_j2k_v2_t *p_j2k,
								opj_stream_private_t *p_stream,
								opj_event_mgr_t * p_manager)
{
	opj_bool l_go_on = OPJ_TRUE;
	OPJ_UINT32 l_current_tile_no;
	OPJ_UINT32 l_tile_no_to_dec;
	OPJ_UINT32 l_data_size,l_max_data_size;
	OPJ_INT32 l_tile_x0,l_tile_y0,l_tile_x1,l_tile_y1;
	OPJ_UINT32 l_nb_comps;
	OPJ_BYTE * l_current_data;

	l_current_data = (OPJ_BYTE*)opj_malloc(1000);
	if (! l_current_data) {
		return OPJ_FALSE;
	}
	l_max_data_size = 1000;

	/*Allocate and initialize some elements of codestrem index if not already done*/
	if( !p_j2k->cstr_index->tile_index)
	{
		if (!j2k_allocate_tile_element_cstr_index(p_j2k)){
			opj_free(l_current_data);
			return OPJ_FALSE;
		}
	}
	/* Move into the codestream to the first SOT used to decode the desired tile */
	l_tile_no_to_dec = p_j2k->m_specific_param.m_decoder.m_tile_ind_to_dec;
	if (p_j2k->cstr_index->tile_index)
		if(p_j2k->cstr_index->tile_index->tp_index)
		{
			if ( ! p_j2k->cstr_index->tile_index[l_tile_no_to_dec].nb_tps) {
				/* the index for this tile has not been built,
				 *  so move to the last SOT read */
				if ( opj_stream_read_seek(p_stream, p_j2k->m_specific_param.m_decoder.m_last_sot_read_pos+2, p_manager) ){
					opj_event_msg_v2(p_manager, EVT_ERROR, "Problem with seek function\n");
					return OPJ_FALSE;
				}
			}
			else{
				if (opj_stream_read_seek(p_stream, p_j2k->cstr_index->tile_index[l_tile_no_to_dec].tp_index[0].start_pos+2, p_manager)) {
					opj_event_msg_v2(p_manager, EVT_ERROR, "Problem with seek function\n");
					return OPJ_FALSE;
				}
			}
			/* Special case if we have previously read the EOC marker (if the previous tile getted is the last ) */
			if(p_j2k->m_specific_param.m_decoder.m_state == J2K_STATE_EOC)
				p_j2k->m_specific_param.m_decoder.m_state = J2K_STATE_TPHSOT;
		}

	while (OPJ_TRUE) {
		if (! j2k_read_tile_header(	p_j2k,
									&l_current_tile_no,
									&l_data_size,
									&l_tile_x0, &l_tile_y0,
									&l_tile_x1, &l_tile_y1,
									&l_nb_comps,
									&l_go_on,
									p_stream,
									p_manager)) {
			opj_free(l_current_data);
			return OPJ_FALSE;
		}


		if (! l_go_on) {
			break;
		}

		if (l_data_size > l_max_data_size) {
			l_current_data = (OPJ_BYTE*)opj_realloc(l_current_data,l_data_size);
			if (! l_current_data) {
				opj_free(l_current_data);
				return OPJ_FALSE;
			}

			l_max_data_size = l_data_size;
		}



		if (! j2k_decode_tile(p_j2k,l_current_tile_no,l_current_data,l_data_size,p_stream,p_manager)) {
			opj_free(l_current_data);
			return OPJ_FALSE;
		}
		opj_event_msg_v2(p_manager, EVT_INFO, "Tile %d/%d has been decoded.\n", l_current_tile_no, (p_j2k->m_cp.th * p_j2k->m_cp.tw) - 1);

		if (! j2k_update_image_data(p_j2k->m_tcd,l_current_data, p_j2k->m_output_image)) {
			opj_free(l_current_data);
			return OPJ_FALSE;
		}
		opj_event_msg_v2(p_manager, EVT_INFO, "Image data has been updated with tile %d.\n\n", l_current_tile_no);

		if(l_current_tile_no == l_tile_no_to_dec)
		{
			/* move into the codestream to the the first SOT (FIXME or not move?)*/
			if (opj_stream_read_seek(p_stream, p_j2k->cstr_index->main_head_end + 2, p_manager) ) {
				opj_event_msg_v2(p_manager, EVT_ERROR, "Problem with seek function\n");
				return OPJ_FALSE;
			}
			break;
		}
		else {
			opj_event_msg_v2(p_manager, EVT_WARNING, "Tile read, decode and updated is not the desired (%d vs %d).\n", l_current_tile_no, l_tile_no_to_dec);
		}

	}

	opj_free(l_current_data);

	return OPJ_TRUE;
}


/**
 * Sets up the procedures to do on decoding one tile. Developpers wanting to extend the library can add their own reading procedures.
 */
void j2k_setup_decoding_tile (opj_j2k_v2_t *p_j2k)
{
	/* preconditions*/
	assert(p_j2k != 00);

	opj_procedure_list_add_procedure(p_j2k->m_procedure_list,(void*)j2k_decode_one_tile);
	/* DEVELOPER CORNER, add your custom procedures */

}


/**
 * Decodes the tiles of the stream.
 */
opj_bool j2k_decode_v2(	opj_j2k_v2_t * p_j2k,
						opj_stream_private_t * p_stream,
						opj_image_t * p_image,
						opj_event_mgr_t * p_manager)
{
	OPJ_UINT32 compno;

	if (!p_image)
		return OPJ_FALSE;

	p_j2k->m_output_image = opj_image_create0();
	if (! (p_j2k->m_output_image)) {
		return OPJ_FALSE;
	}
	opj_copy_image_header(p_image, p_j2k->m_output_image);

	/* customization of the decoding */
	j2k_setup_decoding(p_j2k);

	/* Decode the codestream */
	if (! j2k_exec (p_j2k,p_j2k->m_procedure_list,p_stream,p_manager)) {
		opj_image_destroy(p_j2k->m_private_image);
		p_j2k->m_private_image = NULL;
		return OPJ_FALSE;
	}

	/* Move data and copy one information from codec to output image*/
	for (compno = 0; compno < p_image->numcomps; compno++) {
		p_image->comps[compno].resno_decoded = p_j2k->m_output_image->comps[compno].resno_decoded;
		p_image->comps[compno].data = p_j2k->m_output_image->comps[compno].data;
		p_j2k->m_output_image->comps[compno].data = NULL;
	}

	return OPJ_TRUE;
}


/**
 * Get the decoded tile.
 *
 * @param	p_j2k			the jpeg2000 codestream codec.
 * @param	p_stream		input_stream
 * @param	p_image			output image.	.
 * @param	p_manager		the user event manager
 * @param	tile_index		index of the tile we want decode
 *
 * @return	true			if succeed.
 */
opj_bool j2k_get_tile(	opj_j2k_v2_t *p_j2k,
						opj_stream_private_t *p_stream,
						opj_image_t* p_image,
						struct opj_event_mgr * p_manager,
						OPJ_UINT32 tile_index )
{
	OPJ_UINT32 compno;
	OPJ_UINT32 l_tile_x, l_tile_y;
	opj_image_comp_t* l_img_comp;

	if (!p_image) {
		opj_event_msg_v2(p_manager, EVT_ERROR, "We need an image previously created.\n");
		return OPJ_FALSE;
	}

	if ( (tile_index < 0) && (tile_index >= p_j2k->m_cp.tw * p_j2k->m_cp.th) ){
		opj_event_msg_v2(p_manager, EVT_ERROR, "Tile index provided by the user is incorrect %d (max = %d) \n", tile_index, (p_j2k->m_cp.tw * p_j2k->m_cp.th) - 1);
		return OPJ_FALSE;
	}

	/* Compute the dimension of the desired tile*/
	l_tile_x = tile_index % p_j2k->m_cp.tw;
	l_tile_y = tile_index / p_j2k->m_cp.tw;

	p_image->x0 = l_tile_x * p_j2k->m_cp.tdx + p_j2k->m_cp.tx0;
	if (p_image->x0 < p_j2k->m_private_image->x0)
		p_image->x0 = p_j2k->m_private_image->x0;
	p_image->x1 = (l_tile_x + 1) * p_j2k->m_cp.tdx + p_j2k->m_cp.tx0;
	if (p_image->x1 > p_j2k->m_private_image->x1)
		p_image->x1 = p_j2k->m_private_image->x1;

	p_image->y0 = l_tile_y * p_j2k->m_cp.tdy + p_j2k->m_cp.ty0;
	if (p_image->y0 < p_j2k->m_private_image->y0)
		p_image->y0 = p_j2k->m_private_image->y0;
	p_image->y1 = (l_tile_y + 1) * p_j2k->m_cp.tdy + p_j2k->m_cp.ty0;
	if (p_image->y1 > p_j2k->m_private_image->y1)
		p_image->y1 = p_j2k->m_private_image->y1;

	l_img_comp = p_image->comps;
	for (compno=0; compno < p_image->numcomps; ++compno)
	{
		OPJ_INT32 l_comp_x1, l_comp_y1;

		l_img_comp->factor = p_j2k->m_private_image->comps[compno].factor;

		l_img_comp->x0 = int_ceildiv(p_image->x0, l_img_comp->dx);
		l_img_comp->y0 = int_ceildiv(p_image->y0, l_img_comp->dy);
		l_comp_x1 = int_ceildiv(p_image->x1, l_img_comp->dx);
		l_comp_y1 = int_ceildiv(p_image->y1, l_img_comp->dy);

		l_img_comp->w = int_ceildivpow2(l_comp_x1, l_img_comp->factor) - int_ceildivpow2(l_img_comp->x0, l_img_comp->factor);
		l_img_comp->h = int_ceildivpow2(l_comp_y1, l_img_comp->factor) - int_ceildivpow2(l_img_comp->y0, l_img_comp->factor);

		l_img_comp++;
	}

	/* Destroy the previous output image*/
	if (p_j2k->m_output_image)
		opj_image_destroy(p_j2k->m_output_image);

	/* Create the ouput image from the information previously computed*/
	p_j2k->m_output_image = opj_image_create0();
	if (! (p_j2k->m_output_image)) {
		return OPJ_FALSE;
	}
	opj_copy_image_header(p_image, p_j2k->m_output_image);

	p_j2k->m_specific_param.m_decoder.m_tile_ind_to_dec = tile_index;

	/* customization of the decoding */
	j2k_setup_decoding_tile(p_j2k);

	/* Decode the codestream */
	if (! j2k_exec (p_j2k,p_j2k->m_procedure_list,p_stream,p_manager)) {
		opj_image_destroy(p_j2k->m_private_image);
		p_j2k->m_private_image = NULL;
		return OPJ_FALSE;
	}

	/* Move data and copy one information from codec to output image*/
	for (compno = 0; compno < p_image->numcomps; compno++) {
		p_image->comps[compno].resno_decoded = p_j2k->m_output_image->comps[compno].resno_decoded;

		if (p_image->comps[compno].data)
			opj_free(p_image->comps[compno].data);

		p_image->comps[compno].data = p_j2k->m_output_image->comps[compno].data;

		p_j2k->m_output_image->comps[compno].data = NULL;
	}

	return OPJ_TRUE;
}

opj_bool j2k_set_decoded_resolution_factor(opj_j2k_v2_t *p_j2k, OPJ_UINT32 res_factor, opj_event_mgr_t * p_manager)
{
	OPJ_UINT32 it_comp;

	p_j2k->m_cp.m_specific_param.m_dec.m_reduce = res_factor;

	if (p_j2k->m_private_image) {
		if (p_j2k->m_private_image->comps) {
			if (p_j2k->m_specific_param.m_decoder.m_default_tcp) {
				if (p_j2k->m_specific_param.m_decoder.m_default_tcp->tccps) {
					for (it_comp = 0 ; it_comp < p_j2k->m_private_image->numcomps; it_comp++) {
						OPJ_UINT32 max_res = p_j2k->m_specific_param.m_decoder.m_default_tcp->tccps[it_comp].numresolutions;
						if ( res_factor >= max_res){
							opj_event_msg_v2(p_manager, EVT_ERROR, "Resolution factor is greater than the maximum resolution in the component.\n");
							return OPJ_FALSE;
						}
						p_j2k->m_private_image->comps[it_comp].factor = res_factor;
					}
					return OPJ_TRUE;
				}
			}
		}
	}

	return OPJ_FALSE;
}
