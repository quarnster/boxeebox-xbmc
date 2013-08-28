/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2009 Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.

  Contact Information:

  Intel Corporation
  2200 Mission College Blvd.
  Santa Clara, CA  97052

  BSD LICENSE

  Copyright(c) 2009 Intel Corporation. All rights reserved.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/** @weakgroup dvr_doc SMD DVR and indexing documentation

\brief High level design and usage of a DVR using Intel Streaming Media Drivers

This document is intended to provide Software Engineers
with the necessary context and information to interface with Intel Corporation
Digital Home Group (DHG) Streaming Media Drivers (SMD) modules to meet specific
product requirements for Digital Video Recorder (DVR) with trick play indexing.

\anchor Contents
<H2>Contents</H2>
- <A HREF="#Assumptions">Assumptions</A>
- <A HREF="#Related_Documents">Related Documents</A>
- <A HREF="#Software_Design">Software Design</A>
 - <A HREF="#High_level_use_case">High level use case</A>
 - <A HREF="#Transitions">Transitions</A>
  - <A HREF="#Trans_Opt1">Option 1</A>
     - <A HREF="#Trans_Steps">Steps</A>
     .
  - <A HREF="#Trans_Opt2">Option 2</A>
  .
 .
- <A HREF="#Index_Data">Index data</A>
- <A HREF="#Configuring">Configuring</A>
 - <A HREF="#Memory">Memory</A>
- <A HREF="#Sample_Function_Calls">Sample Function Calls</A>
.

\anchor Assumptions
<H2>Assumptions</H2>

-# The SMD Demux will provide real time indexing of MPEG-2 Video ES bitstreams.

\anchor Related_Documents
<H2>See Also (Related_Documents)</H2>
-# DVR_feature_planning.xls (requrements): <A HREF="http://moss.amr.ith.intel.com/sites/CPE/CE/USSeg/Shared%20Documents/Forms/AllItems.aspx?RootFolder=%2fsites%2fCPE%2fCE%2fUSSeg%2fShared%20Documents%2fFeature%20Requirements%20and%20HLDs&FolderCTID=&View=%7b95A057F4%2d44DE%2d45CF%2dA0D7%2d9F45F2AD547D%7d">Feature Requirements and HLDs</A>

\anchor Software_Design
<H2>Software Design</H2>
\anchor High_level_use_case
<H3>High level use case</H3>

\image html DVR_Max_Use_Case.png "DVR Indexing Intel CE 31xx stress use case"

<B>Figure 1: DVR Indexing Intel CE 31xx stress use case</B>
Figure 1 above shows a possible stress use case for the CE 31xx HW and SW.
There are 4 MPTS streams input to the system with five instances of the SMD
Demux, two programs being displayed on a connected TV/Monitor in a PIP
configurations and four programs being recorded and indexed.  In one case (E.)
the recorded TS is being streamed out to the IP network.

There are a few options not shown in the figure above as it is not practical to
show all possible combinations.   Example: It is possible to stream the recorded
TS in options C. and D. out to the IP network as well.   However this figure
provides a good example to discuss the various use cases of DVR time-shifted
playback while doing other things in the system.    The various boxes can be
combined in other configurations up to the HW limitations.   An example would be
a use case containing four of the option D. HDD Save only boxes representing the
recording of 4 programs for later playback.

Also not shown is the optional encryption of the TS going to and from the HDD.
Please refer to the Intel&reg; Media Processor CE 3100 documentation on security API's.

\anchor Transitions
<H3>Transition to/from live</H3>
The Use case above shows a steady state and some DVR designs never change from 
timeshifted playback, and instead always have a slight delay (~1 sec) even when 
they report to the user interface that they are in a Live mode. However some 
systems may want to change the state of the DVR system from time-shifted 
playback to a live mode that bypasses the HDD or go from live mode to a 
time-shifted mode.  The following two diagrams show two ways of doing this.

\anchor Trans_Opt1
<H4>Option 1</H4>
This is the recommended option.

\image html DVR_Transition1.png "DVR Transition to/from live (option 1)"

\anchor Trans_Steps
During the transitions never pause the first Demux from recording the SPTS and 
index info to HDD.  Steps to transition from recorded playback to live playback:
-#	Pause playback and disconnect the file source from the 2nd Demux.
-#	Flush the pipelines
-#	Create a 2nd SPTS filter in the 1st Demux with the same PIDs (Video, Audio 
 and PCR) as the first SPTS filter that is being recording to the HDD (this 
 filter could have been created earlier.
-#	Connect the 2nd SPTS filter output port to the input port of the 2nd Demux
-#	Set new base time to current time
-#	Wait for the 2nd Demux to buffer
-#	Enter Play mode.

Steps to transition from live playback to recorded playback:
-#	Pause playback and disconnect the input from the 2nd Demux.
-#	Query ismd_vidrend_get_stream_position() for the last_segment_pts if needed 
   for locating the same time in the index.  
   <b>Note:</b> lsb of last_segment_pts will always be zero.
-#	Flush the pipelines
-#	Start reading the recorded SPTS file from the HDD (optionally use the index 
   file to locate a specific time by byte offset in the file using PTS values).  
   This becomes the input to the 2nd Demux.
   If entering a trick mode please refer to documentaion on \ref Trick_modes "Trick modes".
-#	Set new base time to current time
-#	Wait for the 2nd Demux to buffer
-#	Enter Play mode.

\anchor Trans_Opt2
<H4>Option 2</H4>
This is an alternative option.

\image html DVR_Transition2.png "DVR Transition to/from live (option 2)"

\anchor Index_Data
<H3>Index data of SPTS</H3>
The data coming out of the demux index filter is specified below.   It is
expected the application will use this data as it sees fit and store it along
side the TS stream in a method of it's choosing.

\code
struct {
  char   zero; // unused
  char   one;  // reserved
  char   index_type; // PSI=2, PES, SEQ, GOP, PIC, PCEH, PDE
  char   offset_in_ts_packet;
  char   pic_type; // I (b01), P (b10), B(b11)
  char   five; // unused
  char   six;  // unused
  char   pts_hi_and_valid; // bit7: is PTS Valid; bit 0: 33rd bit of the PTS
  uint32 pts_lo;
  uint32 packet_number; // TS Packet number
} index_data  // 16 bytes

// Indexing Header type values.
#define TSD_INDEX_ID_PSI  (0x2)
#define TSD_INDEX_ID_PES  (0x3)
#define TSD_INDEX_ID_SEQ  (0x4)
#define TSD_INDEX_ID_GOP  (0x5)
#define TSD_INDEX_ID_PIC  (0x6)
#define TSD_INDEX_ID_PCEH (0x7)
#define TSD_INDEX_ID_PDE  (0x8)

// pic_type values, only valid with index_type = PIC (6)
// Value of 0 indicates pic_type undetermined
#define PIC_TYPE_I  (0x1)
#define PIC_TYPE_P  (0x2)
#define PIC_TYPE_B  (0x3)
\endcode

<H4>Demux Indexing Algorithm Notes</H4>
The SMD demux indexing design comes from the initial design from the Intel&reg;
Media Processor CE 21xx documentation.   Indexing is done just prior to the
section re-assembling which is just before the output stage of the demux firmware.
When a packet has a pid matching the one selected for indexing, the firmware
will output an index record for that packet.   Refer to smd/demux/fw/tsd_fw_indexing.c
for details.

Currently the firmware only supports indexing MPEG-2 video bitstreams.

\note Do not attempt to use the indexing feature of the SMD demux to create an
index of an existing TS file. If there are any packet errors in the existing
TS file, the new index offsets will not match.  Instead use the SMD demux to
create a new SPTS file and new index from the existing TS file.  The new index
will match the new SPTS file.

\anchor Configuring
<H2>Configuring</H2>

<b>Software Configuration of the demux SPTS and Index Filters</b>

This is a simple case (see Figure 1, box D) as the client only needs to open an
SMD demux stream instance, configure it, and start the input of the MPTS data.
Software Configuration of the demux SPTS and Index Filters
This does not show transitions, but that must be done in the stop state and
doesn't change the operations much.  Basic record and index steps:
-#	open demux stream
-#	open SPTS output filter
-#	create pids for a program (This often includes video, one or more audio, PSI, private data and Teletext pids).
-#	map pids to filter
-#	open and map indexer to TS output filter
-#	configure indexing of video pid
-#	enable stream

\note Optionally the output of filters in the demux can be created as leaky when
it is expected that one of the filters may be stalled to allow the SPTS filter
to avoid a buffer overflow and allow the recording of the SPTS to remain 
uninterrupted.  Currently this feature should be used with caution as it is a global
feature for all ports on a given instance of a demux.

\anchor Memory
<H3>Memory</H3>
The etc/platform_config/memory_layout_512M.hcfg is designed for just enough
memory to decode and display 2 streams (with various restrictions like 1.5HD, up to
h.264 level 4.1, etc...).   For other use cases, it will need to be edited and more
memory (buffers) reserved.   It is recommended that at least 14 buffers of size
24KB - 32KB are reserved per SPTS stream to be recorded.  This buffer size can be
specified during the call to ismd_demux_filter_open().

Two stream addition to memory layout example:
\code
smd_buffers_DMX_SPTS1  { buf_size = 0x5E00        base = 0x12380000    size =    0x52400 } // (329KB) 14 x 128x188B Demux SPTS even packet sized buffers for stream 1 
smd_buffers_DMX_SPTS2  { buf_size = 0x5E00        base = 0x123D2400    size =    0x52400 } // (329KB) 14 x 128x188B Demux SPTS even packet sized buffers for stream 2
\endcode

\anchor Sample_Function_Calls
<H3>Sample Function Calls</H3>

This code would appear in an application that is configuring an instance of the
SMD demux to output two filters one for a SPTS (demuxed from a MPTS) and an
associated index filter.

\code
////////////////////////////////////////////////////////////////////////////////
// local vars
ismd_result_t              result           = ISMD_SUCCESS;
ismd_dev_t                 demux_handle     = ISMD_DEV_HANDLE_INVALID;
ismd_port_handle_t         demux_input_port = ISMD_PORT_HANDLE_INVALID;
ismd_port_handle_t         demux_SPTS_output_port    = ISMD_PORT_HANDLE_INVALID;
ismd_port_handle_t         demux_index_output_port   = ISMD_PORT_HANDLE_INVALID;
ismd_demux_filter_handle_t demux_SPTS_filter_handle  = 0;
ismd_demux_filter_handle_t demux_index_filter_handle = 0;
ismd_clock_t               clock_handle     = ISMD_CLOCK_HANDLE_INVALID;
ismd_buffer_handle_t       buf              = ISMD_BUFFER_HANDLE_INVALID;
ismd_demux_pid_list_t      pid_list;
unsigned int               pid_list_size;


// open demux stream
result = ismd_demux_stream_open(ISMD_DEMUX_STREAM_TYPE_MPEG2_TRANSPORT_STREAM_188,
                                &demux_handle,
                                &demux_input_port);
OS_ASSERT(result == ISMD_SUCCESS);


// open SPTS output filter
result = ismd_demux_filter_open(demux_handle,
                                ISMD_DEMUX_OUTPUT_WHOLE_TS_PACKET,
                                0x5E00, // 24,064B (128 x 188B) for an even multpile of TS packet size
                                &demux_SPTS_filter_handle,
                                &demux_SPTS_output_port);
OS_ASSERT(result == ISMD_SUCCESS);


// map pids to SPTS filter
pid_list_size = 0;
pid_list.pid_array[pid_list_size].pid_val  = program[0].psi_info_pid;
pid_list.pid_array[pid_list_size].pid_type = ISMD_DEMUX_PID_TYPE_PSI;
pid_list_size++;

pid_list.pid_array[pid_list_size].pid_val  = program[0].video_pid;
pid_list.pid_array[pid_list_size].pid_type = ISMD_DEMUX_PID_TYPE_PES;
pid_list_size++;

pid_list.pid_array[pid_list_size].pid_val  = program[0].audio_pid;
pid_list.pid_array[pid_list_size].pid_type = ISMD_DEMUX_PID_TYPE_PES;
pid_list_size++;

pid_list.pid_array[pid_list_size].pid_val  = program[0].sap_pid;
pid_list.pid_array[pid_list_size].pid_type = ISMD_DEMUX_PID_TYPE_PES;
pid_list_size++;

pid_list.pid_array[pid_list_size].pid_val  = program[0].teletext_pid;
pid_list.pid_array[pid_list_size].pid_type = ISMD_DEMUX_PID_TYPE_PES;
pid_list_size++;

result = ismd_demux_filter_map_to_pids(demux_handle,
                                       demux_SPTS_filter_handle,
                                       pid_list,
                                       pid_list_size);
OS_ASSERT(result == ISMD_SUCCESS);


// open and map indexer to TS output filter (Defaults to 1K size buffers)
result = ismd_demux_filter_open_indexer(demux_handle,
                                        demux_SPTS_filter_handle,
                                        &demux_index_filter_handle,
                                        &demux_index_output_port);
OS_ASSERT(result == ISMD_SUCCESS);


// configure indexing of video pid
result = ismd_demux_ts_set_indexing_on_pid(demux_handle,
                                           program[0].video_pid,
                                           true);
OS_ASSERT(result == ISMD_SUCCESS);

// Make filters leaky
result = ismd_demux_enable_leaky_filters(demux_handle);
OS_ASSERT(result == ISMD_SUCCESS);

// Enable stream
result = ismd_dev_set_state(demux_handle, ISMD_DEV_STATE_PLAY);
OS_ASSERT(result == ISMD_SUCCESS);

// Loop to feed stream data into demux
while (put_ts_data_into_buffer(&buf) == ISMD_SUCCESS){// local fcn call
   result = ismd_port_write(demux_input_port, buf);

   if (result == ISMD_ERROR_NO_SPACE_AVAILABLE) {
      wait_for_the_port_to_have_space();  // local function call
   } else if (result != ISMD_SUCCESS) {
      OS_ASSERT(0);
   }
}

// Shutdown
result = ismd_demux_filter_close(demux_handle, demux_SPTS_filter_handle);
OS_ASSERT(result == ISMD_SUCCESS);
result = ismd_demux_filter_close(demux_handle, demux_index_filter_handle);
OS_ASSERT(result == ISMD_SUCCESS);
result = ismd_demux_stream_close(demux_handle);
OS_ASSERT(result == ISMD_SUCCESS);

\endcode
*/
