package main

/*
#include <ismd_audio.h>
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
	"log"
	"math"
	"time"
	"unsafe"
)

func main() {
	var (
		m_audioProcessor C.ismd_audio_processor_t
		device           C.ismd_dev_t
		input_port       C.ismd_port_handle_t
	)

	log.Printf("open:  %x, %x", C.ismd_audio_open_global_processor(&m_audioProcessor), m_audioProcessor)
	defer C.ismd_audio_close_processor(m_audioProcessor)

	log.Printf("conf clock: %d", C.ismd_audio_configure_master_clock(m_audioProcessor, 36864000, C.ISMD_AUDIO_CLK_SRC_EXTERNAL))

	log.Printf("add: %d", C.ismd_audio_add_input_port(m_audioProcessor, false, &device, &input_port))
	defer C.ismd_audio_remove_input(m_audioProcessor, device)
	log.Printf("set data format: %d", C.ismd_audio_input_set_data_format(m_audioProcessor, device, C.ISMD_AUDIO_MEDIA_FMT_PCM))
	var tmp int64 = C.AUDIO_CHAN_CONFIG_2_CH
	log.Printf("set format: %d", C.ismd_audio_input_set_pcm_format(m_audioProcessor, device, 16, 48000, C.int(tmp)))
	log.Printf("en2: %d", C.ismd_audio_input_enable(m_audioProcessor, device))
	defer C.ismd_audio_input_disable(m_audioProcessor, device)

	for _, a := range []C.int{C.GEN3_HW_OUTPUT_HDMI /*, C.GEN3_HW_OUTPUT_SPDIF, C.GEN3_HW_OUTPUT_I2S0, C.GEN3_HW_OUTPUT_I2S1*/} {
		var (
			output_config C.ismd_audio_output_config_t
			audio_output  C.ismd_audio_output_t = -1
		)
		if a == C.GEN3_HW_OUTPUT_SPDIF {
			output_config.out_mode = C.ISMD_AUDIO_OUTPUT_PASSTHROUGH
		} else {
			output_config.out_mode = C.ISMD_AUDIO_OUTPUT_PCM
		}
		output_config.stream_delay = 0
		output_config.sample_size = 16
		output_config.ch_config = C.ISMD_AUDIO_STEREO
		output_config.ch_map = 0
		output_config.sample_rate = 48000
		log.Printf("addp: %x", C.ismd_audio_add_phys_output(m_audioProcessor, a, output_config, &audio_output))
		defer C.ismd_audio_remove_output(m_audioProcessor, audio_output)
		log.Printf("Enable: %x", C.ismd_audio_output_enable(m_audioProcessor, audio_output))
		defer C.ismd_audio_output_disable(m_audioProcessor, audio_output)
	}
	var passthrough_config C.ismd_audio_input_pass_through_config_t

	log.Printf("set primary: %d", C.ismd_audio_input_set_as_primary(m_audioProcessor, device, passthrough_config))
	log.Printf("Play: %d", C.ismd_dev_set_state(device, C.ISMD_DEV_STATE_PLAY))
	defer C.ismd_dev_set_state(device, C.ISMD_DEV_STATE_STOP)

	const pitch = (440.0 * 2 * math.Pi) / 48000.0
	const lrpitch = 2 * math.Pi / 48000.0
	var pos float64
	var lrpos float64
	for i := 0; i < 1000; i++ {
		var (
			buffer C.ismd_buffer_handle_t
			desc   C.ismd_buffer_descriptor_t
		)
		if al := C.ismd_buffer_alloc(4096, &buffer); al != 0 {
			log.Println("Alloc failed: ", al)
		} else if d := C.ismd_buffer_read_desc(buffer, &desc); d != 0 {
			log.Println("read desc failed")
		}
		raw := C.mymap(desc.phys.base, 4096)
		ptr := (*[2048]int16)(unsafe.Pointer(raw))
		for j := 0; j < 2048; j += 2 {
			p := 0.5 + 0.5*math.Sin(lrpos)
			ptr[j] = (int16)(p * 16767 * math.Sin(pos))
			ptr[j+1] = (int16)((1 - p) * 16767 * math.Sin(pos))

			pos += pitch
			lrpos += lrpitch
		}
		desc.phys.level = 4096
		if u := C.ismd_buffer_update_desc(buffer, &desc); u != 0 {
			log.Println("Error updating desc: %d", u)
		}
		C.myunmap(raw, 4096)
		for counter := 0; counter < 20; counter++ {
			if result := C.ismd_port_write(input_port, buffer); result != 0 {
				time.Sleep(time.Millisecond * 100)
			} else {
				break
			}
		}
		// ismd_port_write takes the reference
		//C.ismd_buffer_dereference(buffer)
	}
}
