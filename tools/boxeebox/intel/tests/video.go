package main

/*
#cgo LDFLAGS: -lismd_viddec -lismd_vidrend -lismd_vidpproc -lismd_bufmon -lavformat -lavcodec -lavutil
#include <libgdl.h>
#include <ismd_bufmon.h>
#include <ismd_core.h>
#include <ismd_viddec.h>
#include <ismd_vidpproc.h>
#include <ismd_vidrend.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <math.h>
#include <osal_memmap.h>
#include <unistd.h>

void *mymap(ismd_physical_address_t base, int size)
{
	return OS_MAP_IO_TO_MEM_NOCACHE(base, size);
}
void myunmap(void* ptr, int size)
{
	OS_UNMAP_IO_FROM_MEM(ptr, size);
}
*/
import "C"

import (
	"flag"
	"log"
	"os"
	"time"
	"unsafe"
)

var (
	m_main_clock                                                                                                                                            C.ismd_clock_t
	m_video_bufmon, m_viddec, m_video_proc, m_video_render                                                                                                  C.ismd_dev_t
	m_viddec_input_port, m_viddec_output_port, m_video_input_port_proc, m_video_output_port_proc, m_video_input_port_renderer, m_video_output_port_renderer C.ismd_port_handle_t
	m_video_codec                                                                                                                                           C.ismd_codec_type_t
)

func CreateStartPacket(start_pts C.ismd_pts_t, buffer_handle C.ismd_buffer_handle_t) {
	var (
		newsegment_data C.ismd_newsegment_tag_t
	)
	var hack int = C.ISMD_NO_PTS
	newsegment_data.linear_start = 0
	newsegment_data.start = start_pts
	newsegment_data.stop = C.ismd_pts_t(hack)
	newsegment_data.requested_rate = C.ISMD_NORMAL_PLAY_RATE
	newsegment_data.applied_rate = C.ISMD_NORMAL_PLAY_RATE
	newsegment_data.rate_valid = true
	newsegment_data.segment_position = C.ismd_pts_t(hack)
	log.Printf("%128s: %02d", "ismd_tag_set_newsegment_position", C.ismd_tag_set_newsegment_position(buffer_handle, newsegment_data))
}

func SendStartPacket(start_pts C.ismd_pts_t, port C.ismd_port_handle_t) {
	var buffer C.ismd_buffer_handle_t
	log.Printf("%128s: %02d", "ismd_buffer_alloc", C.ismd_buffer_alloc(0, &buffer))
	CreateStartPacket(start_pts, buffer)
	log.Printf("%128s: %02d", "ismd_port_write", C.ismd_port_write(port, buffer))
}

func Init(codec_type C.ismd_codec_type_t, plane C.gdl_plane_id_t) {

	C.gdl_plane_reset(plane)
	C.gdl_plane_config_begin(plane)

	var policy C.gdl_vid_policy_t = C.GDL_VID_POLICY_RESIZE

	C.gdl_plane_set_attr(C.GDL_PLANE_VID_MISMATCH_POLICY, unsafe.Pointer(&policy))
	C.gdl_plane_config_end(C.GDL_FALSE)

	log.Printf("%128s: %02d", "ismd_clock_alloc", C.ismd_clock_alloc(C.ISMD_CLOCK_TYPE_FIXED, &m_main_clock))
	log.Printf("%128s: %02d", "ismd_clock_make_primary", C.ismd_clock_make_primary(m_main_clock))
	log.Printf("%128s: %02d", "ismd_viddec_open", C.ismd_viddec_open(codec_type, &m_viddec))
	log.Printf("%128s: %02d", "ismd_viddec_get_input_port", C.ismd_viddec_get_input_port(m_viddec, &m_viddec_input_port))
	log.Printf("%128s: %02d", "ismd_viddec_get_output_port", C.ismd_viddec_get_output_port(m_viddec, &m_viddec_output_port))
	log.Printf("%128s: %02d", "ismd_viddec_set_pts_interpolation_policy", C.ismd_viddec_set_pts_interpolation_policy(m_viddec, C.ISMD_VIDDEC_INTERPOLATE_MISSING_PTS, 0))
	log.Printf("%128s: %02d", "ismd_viddec_set_max_frames_to_decode", C.ismd_viddec_set_max_frames_to_decode(m_viddec, C.ISMD_VIDDEC_ALL_FRAMES))
	log.Printf("%128s: %02d", "ismd_vidpproc_open", C.ismd_vidpproc_open(&m_video_proc))
	log.Printf("%128s: %02d", "ismd_vidpproc_set_scaling_policy", C.ismd_vidpproc_set_scaling_policy(m_video_proc, C.SCALE_TO_FIT))
	log.Printf("%128s: %02d", "ismd_vidpproc_get_input_port", C.ismd_vidpproc_get_input_port(m_video_proc, &m_video_input_port_proc))
	log.Printf("%128s: %02d", "ismd_vidpproc_get_output_port", C.ismd_vidpproc_get_output_port(m_video_proc, &m_video_output_port_proc))
	log.Printf("%128s: %02d", "ismd_vidpproc_pan_scan_disable", C.ismd_vidpproc_pan_scan_disable(m_video_proc))
	log.Printf("%128s: %02d", "ismd_vidpproc_gaussian_enable", C.ismd_vidpproc_gaussian_enable(m_video_proc))
	log.Printf("%128s: %02d", "ismd_vidpproc_deringing_enable", C.ismd_vidpproc_deringing_enable(m_video_proc))
	log.Printf("%128s: %02d", "ismd_vidrend_open", C.ismd_vidrend_open(&m_video_render))
	log.Printf("%128s: %02d", "ismd_vidrend_get_input_port", C.ismd_vidrend_get_input_port(m_video_render, &m_video_input_port_renderer))
	log.Printf("%128s: %02d", "ismd_dev_set_clock", C.ismd_dev_set_clock(m_video_render, m_main_clock))
	log.Printf("%128s: %02d", "ismd_clock_set_vsync_pipe", C.ismd_clock_set_vsync_pipe(m_main_clock, 0))

	const width = 1920
	const height = 1080
	log.Printf("%128s: %02d", "ismd_vidpproc_set_dest_params2", C.ismd_vidpproc_set_dest_params(m_video_proc, width, height, width, height))
	log.Printf("%128s: %02d", "ismd_vidrend_set_video_plane", C.ismd_vidrend_set_video_plane(m_video_render, C.uint(plane)))
	log.Printf("%128s: %02d", "ismd_vidrend_set_flush_policy", C.ismd_vidrend_set_flush_policy(m_video_render, C.ISMD_VIDREND_FLUSH_POLICY_REPEAT_FRAME))
	log.Printf("%128s: %02d", "ismd_vidrend_set_stop_policy", C.ismd_vidrend_set_stop_policy(m_video_render, C.ISMD_VIDREND_STOP_POLICY_DISPLAY_BLACK))
	log.Printf("%128s: %02d", "ismd_vidrend_enable_max_hold_time", C.ismd_vidrend_enable_max_hold_time(m_video_render, 30, 1))
	log.Printf("%128s: %02d", "ismd_bufmon_open", C.ismd_bufmon_open(&m_video_bufmon))
	var underrun C.ismd_event_t
	log.Printf("%128s: %02d", "ismd_bufmon_add_renderer", C.ismd_bufmon_add_renderer(m_video_bufmon, m_video_render, &underrun))
	log.Printf("%128s: %02d", "ismd_dev_set_underrun_event", C.ismd_dev_set_underrun_event(m_video_render, underrun))
	log.Printf("%128s: %02d", "ismd_dev_set_clock", C.ismd_dev_set_clock(m_video_bufmon, m_main_clock))
	log.Printf("%128s: %02d", "ismd_port_connect", C.ismd_port_connect(m_viddec_output_port, m_video_input_port_proc))
	log.Printf("%128s: %02d", "ismd_port_connect", C.ismd_port_connect(m_video_output_port_proc, m_video_input_port_renderer))

	SetState(C.ISMD_DEV_STATE_PAUSE)
}

func Flush() {
	log.Printf("%128s: %02d", "ismd_vidrend_set_flush_policy", C.ismd_vidrend_set_flush_policy(m_video_render, C.ISMD_VIDREND_FLUSH_POLICY_REPEAT_FRAME))
	SetState(C.ISMD_DEV_STATE_PAUSE)

	log.Printf("%128s: %02d", "ismd_dev_flush", C.ismd_dev_flush(m_viddec))
	log.Printf("%128s: %02d", "ismd_dev_flush", C.ismd_dev_flush(m_video_proc))
	log.Printf("%128s: %02d", "ismd_dev_flush", C.ismd_dev_flush(m_video_render))
	log.Printf("%128s: %02d", "ismd_dev_flush", C.ismd_dev_flush(m_video_bufmon))
}

func SetBaseTime(time C.ismd_time_t) {
	log.Printf("%128s: %02d", "ismd_dev_set_stream_base_time", C.ismd_dev_set_stream_base_time(m_video_render, time))
}

func SetState(state C.ismd_dev_state_t) {
	log.Printf("%128s: %02d", "ismd_dev_set_state", C.ismd_dev_set_state(m_viddec, state))
	log.Printf("%128s: %02d", "ismd_dev_set_state", C.ismd_dev_set_state(m_video_proc, state))
	log.Printf("%128s: %02d", "ismd_dev_set_state", C.ismd_dev_set_state(m_video_render, state))
	log.Printf("%128s: %02d", "ismd_dev_set_state", C.ismd_dev_set_state(m_video_bufmon, state))
}

func GetCurrentTime() (ret C.ismd_time_t) {
	if err := C.ismd_clock_get_time(m_main_clock, &ret); err != C.ISMD_SUCCESS {
		log.Printf("%128s: %02d", "ismd_clock_get_time", err)
	}
	return
}

func SetCurrentTime(current C.ismd_time_t) {
	log.Printf("%128s: %02d", "ismd_clock_set_time", C.ismd_clock_set_time(m_main_clock, current))
}

func Destroy() {
	Flush()
	SetState(C.ISMD_DEV_STATE_STOP)

	log.Printf("%128s: %02d", "ismd_dev_close", C.ismd_dev_close(m_viddec))
	log.Printf("%128s: %02d", "ismd_dev_close", C.ismd_dev_close(m_video_proc))
	log.Printf("%128s: %02d", "ismd_dev_close", C.ismd_dev_close(m_video_render))
	log.Printf("%128s: %02d", "ismd_dev_close", C.ismd_dev_close(m_video_bufmon))
	log.Printf("%128s: %02d", "ismd_clock_free", C.ismd_clock_free(m_main_clock))
}

func open_codec_context(stream_idx *C.int, fmt_ctx *C.AVFormatContext, mtype C.enum_AVMediaType) C.int {
	if ret := C.av_find_best_stream(fmt_ctx, mtype, -1, -1, nil, 0); ret < 0 {
		log.Printf("Could not find %s stream in input file", C.GoString(C.av_get_media_type_string(mtype)))
		return ret
	} else {
		*stream_idx = ret
	}
	return 0
}

func DvdToIsmdPts(pts float64) C.ismd_pts_t {
	return (C.ismd_pts_t)((pts / 1000000) * 90000)
}

var (
	flushFlag = true
	start     C.ismd_pts_t
	counter   int
)

func WriteToInputPort(data uintptr, length C.size_t, pts float64, bufSize C.size_t) int {
	newPts := DvdToIsmdPts(pts)
	if flushFlag {
		start = newPts
		log.Printf("Flush %f", float64(newPts)/90000.0)

		nt := C.ismd_time_t(newPts)
		if GetCurrentTime() < nt {
			// Just initialize the time to a value that allows adjustments in both positive
			// and negative directions.
			SetCurrentTime(nt + 60*90000)
		}
		Flush()
		SendStartPacket(start, m_viddec_input_port)
		SetBaseTime(GetCurrentTime() + 90000)
		SetState(C.ISMD_DEV_STATE_PLAY)
		flushFlag = false
	}
	var (
		basetime C.ismd_time_t
	)
	C.ismd_vidrend_get_base_time(m_video_render, &basetime)
	current := start + C.ismd_pts_t(GetCurrentTime()-basetime)
	if GetCurrentTime() < basetime {
		current = 0
	}
	if int(newPts) != C.ISMD_NO_PTS && newPts < current {
		// Try to prevent out of order problems
		newPts = current
	}
	counter++

	if true || counter%30 == 0 {
		current := GetCurrentTime()
		currentVideo := start + C.ismd_pts_t(current-basetime)
		diff := (float64)(newPts) - (float64)(currentVideo)
		log.Printf("\ncurrent videorenderer time: %f, timestamp: %f (should be: videorenderer time + ~1 second), ffmpeg timestamp: %f, actual diff: %f", float64(currentVideo)/90000.0, float64(newPts)/90000.0, pts/1000000.0, diff/90000.0)
		// if diff > 2*90000 {
		// 	old := start

		// 	start = newPts - 1*90000 + C.ismd_pts_t(basetime-current)
		// 	log.Printf("Something's broken... adjusting sync %f -> %f", float64(old)/90000.0, float64(start)/90000.0)
		// 	SendStartPacket(start, m_viddec_input_port)
		// }
	}
	for length > 0 {
		var (
			buffer_handle C.ismd_buffer_handle_t
			buffer_desc   C.ismd_buffer_descriptor_t
		)

		if ismd_ret := C.ismd_buffer_alloc(bufSize, &buffer_handle); ismd_ret != C.ISMD_SUCCESS {
			log.Printf("CIntelSMDVideo::WriteToInputPort ismd_buffer_alloc failed <%d>", ismd_ret)
			return 0
		} else if ismd_ret = C.ismd_buffer_read_desc(buffer_handle, &buffer_desc); ismd_ret != C.ISMD_SUCCESS {
			log.Printf("CIntelSMDVideo::WriteToInputPort ismd_buffer_read_desc failed <%d>", ismd_ret)
			return 0
		}

		cp := length
		if cp > bufSize {
			cp = bufSize
		}

		ptr := C.mymap(buffer_desc.phys.base, buffer_desc.phys.size)
		C.memcpy(ptr, unsafe.Pointer(data), cp) //ptr to data should be the data being copied to viddec
		C.myunmap(ptr, buffer_desc.phys.size)

		buffer_desc.phys.level = C.int(cp)

		buf_attrs := (*C.ismd_es_buf_attr_t)(unsafe.Pointer(&buffer_desc.attributes[0]))
		buf_attrs.original_pts = newPts
		buf_attrs.local_pts = newPts
		buf_attrs.discontinuity = false

		if ismd_ret := C.ismd_buffer_update_desc(buffer_handle, &buffer_desc); ismd_ret != C.ISMD_SUCCESS {
			log.Printf("-- ismd_buffer_update_desc failed <%d>", ismd_ret)
			return 0
		}

		//the input port should be full frequently when doing file, or any other push mode
		var ismd_ret C.ismd_result_t
		for counter := 0; counter < 5; counter++ {
			if ismd_ret = C.ismd_port_write(m_viddec_input_port, buffer_handle); ismd_ret != C.ISMD_SUCCESS {
				time.Sleep(100 * time.Millisecond)
			} else {
				break
			}
		}

		if length > bufSize {
			length -= bufSize
			data += uintptr(bufSize)
		} else {
			length = 0
		}

		if ismd_ret != C.ISMD_SUCCESS {
			log.Printf("CIntelSMDVideo::WriteToInputPort failed. %d", ismd_ret)
			C.ismd_buffer_dereference(buffer_handle)
			return 0
		}
	}
	return 1
}

func main() {

	var (
		fmt_ctx          *C.AVFormatContext
		video_stream_idx C.int
		pkt              C.AVPacket
		fn               string
	)
	flag.StringVar(&fn, "i", fn, "Input filename")
	flag.Parse()
	if fn == "" {
		flag.PrintDefaults()
		os.Exit(1)
	}
	cfn := C.CString(fn)
	defer C.free(unsafe.Pointer(cfn))
	C.av_register_all()
	if err := C.avformat_open_input(&fmt_ctx, cfn, nil, nil); err < 0 {
		log.Fatalf("Could not open source file %s, %d\n", fn, err)
	}
	// The smd codecs aren't too happy with missing PTS
	fmt_ctx.flags |= C.AVFMT_FLAG_GENPTS
	defer C.avformat_close_input(&fmt_ctx)
	if err := C.avformat_find_stream_info(fmt_ctx, nil); err < 0 {
		log.Fatalf("Could not find stream information: %d", err)
	}
	if err := open_codec_context(&video_stream_idx, fmt_ctx, C.AVMEDIA_TYPE_VIDEO); err < 0 {
		log.Fatalf("Could not open codec context: %d", err)
	}
	log.Printf("fmt_ctx: %+v", fmt_ctx)
	log.Printf("video codec: %+v", (*fmt_ctx.streams).codec)

	Init(C.ISMD_CODEC_TYPE_MPEG2, C.GDL_PLANE_ID_UPP_C)
	defer Destroy()

	C.av_init_packet(&pkt)
	pkt.data = nil
	pkt.size = 0

	running := true
	go func() {
		os.Stdin.Read(make([]byte, 1))
		running = false
	}()
	frame := 0
	// var last float64
	// var lastdts float64
	for running && C.av_read_frame(fmt_ctx, &pkt) >= 0 {
		orig_pkt := pkt
		for pkt.stream_index == video_stream_idx && (pkt.size > 0) {
			pts := float64(pkt.pts)
			//			dts := float64(pkt.dts)
			// log.Printf("pts: %.2f, diff: %.2f, dts: %.2f, diff: %.2f", pts, pts-last, dts, dts-lastdts)
			// last = pts
			// lastdts = dts
			WriteToInputPort(uintptr(unsafe.Pointer(pkt.data)), C.size_t(pkt.size), 1000.0*(pts/(2*30.0)), 32*1024)
			// ret = decode_packet(&got_frame, 0)
			// if ret < 0 {
			// 	break
			// }
			// pkt.data += ret
			// pkt.size -= ret
			break
		}
		frame++
		if frame%100 == 0 {
			var stat C.ismd_vidrend_stats_t
			C.ismd_vidrend_get_stats(m_video_render, &stat)
			log.Printf("%+v", stat)
		}

		C.av_free_packet(&orig_pkt)
	}
	// pkt.data = NULL
	// pkt.size = 0
	// do {
	//     decode_packet(&got_frame, 1);
	// } while (got_frame);

}
