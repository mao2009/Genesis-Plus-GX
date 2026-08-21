/***************************************************************************************
 *  Genesis Plus GX
 *  Emulator Event System - Implementation
 *
 *  Copyright (C) 2024
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************************/

#ifdef HOOK_CPU

#include <string.h>
#include "emu_event.h"


/* ======================================================================== */
/* Ring Buffer State                                                        */
/* ======================================================================== */

static struct
{
  emu_event_t buffer[EMU_EVENT_BUFFER_SIZE];
  uint32 write_idx;
  uint32 seq;
  uint32 overflow_count;
  uint32 filter_mask;
  uint32 frame_count;
} emu_event_state =
{
  .write_idx = 0,
  .seq = 0,
  .overflow_count = 0,
  .filter_mask = EMU_EVENT_FILTER_DEFAULT,
  .frame_count = 0
};

/* Read position for emu_event_read() */
static uint32 emu_event_read_idx = 0;


/* ======================================================================== */
/* Internal Helpers                                                         */
/* ======================================================================== */

/* Check if an event type matches the current filter */
static int emu_event_filtered(emu_event_type_t type)
{
  uint32 mask = 0;

  switch (type)
  {
    case EMU_EVENT_FRAME_BEGIN:
    case EMU_EVENT_FRAME_END:
      mask = EMU_EVENT_FILTER_FRAME;
      break;

    case EMU_EVENT_M68K_EXEC:
      mask = EMU_EVENT_FILTER_M68K_EXEC;
      break;

    case EMU_EVENT_S68K_EXEC:
      mask = EMU_EVENT_FILTER_S68K_EXEC;
      break;

    case EMU_EVENT_Z80_EXEC:
      mask = EMU_EVENT_FILTER_Z80_EXEC;
      break;

    case EMU_EVENT_GATE_ARRAY_READ:
    case EMU_EVENT_GATE_ARRAY_WRITE:
      mask = EMU_EVENT_FILTER_GATE_ARRAY;
      break;

    case EMU_EVENT_COMMUNICATION:
      mask = EMU_EVENT_FILTER_COMMUNICATION;
      break;

    case EMU_EVENT_BIOS_EXECUTION:
    case EMU_EVENT_BIOS_CALL:
    case EMU_EVENT_BIOS_RETURN:
      mask = EMU_EVENT_FILTER_BIOS;
      break;

    case EMU_EVENT_CDD_COMMAND:
      mask = EMU_EVENT_FILTER_CDD;
      break;

    case EMU_EVENT_CDC_DMA_START:
    case EMU_EVENT_CDC_DMA_END:
      mask = EMU_EVENT_FILTER_CDC;
      break;
  }

  return (emu_event_state.filter_mask & mask) != 0;
}


/* ======================================================================== */
/* Public API Implementation                                                */
/* ======================================================================== */

void emu_event_init(void)
{
  memset(&emu_event_state.buffer, 0, sizeof(emu_event_state.buffer));
  emu_event_state.write_idx = 0;
  emu_event_state.seq = 0;
  emu_event_state.overflow_count = 0;
  emu_event_state.filter_mask = EMU_EVENT_FILTER_DEFAULT;
  emu_event_state.frame_count = 0;
  emu_event_read_idx = 0;
}

void emu_event_reset(void)
{
  emu_event_state.write_idx = 0;
  emu_event_state.seq = 0;
  emu_event_state.overflow_count = 0;
  emu_event_state.frame_count = 0;
  emu_event_read_idx = 0;
}

void emu_event_set_mask(uint32 mask)
{
  emu_event_state.filter_mask = mask;
}

uint32 emu_event_get_mask(void)
{
  return emu_event_state.filter_mask;
}

uint32 emu_event_count(void)
{
  if (emu_event_read_idx <= emu_event_state.write_idx)
  {
    return emu_event_state.write_idx - emu_event_read_idx;
  }
  else
  {
    return EMU_EVENT_BUFFER_SIZE - emu_event_read_idx + emu_event_state.write_idx;
  }
}

uint32 emu_event_overflow_count(void)
{
  return emu_event_state.overflow_count;
}

int emu_event_read(emu_event_t *event)
{
  if (event == NULL)
    return 0;

  if (emu_event_read_idx == emu_event_state.write_idx)
    return 0;  /* Buffer empty */

  *event = emu_event_state.buffer[emu_event_read_idx];
  emu_event_read_idx = (emu_event_read_idx + 1) % EMU_EVENT_BUFFER_SIZE;

  return 1;
}

void emu_event_push(emu_event_type_t type, emu_cpu_t cpu, uint32 pc,
                    uint32 address, uint32 value, uint8 width, uint8 aux)
{
  /* Check filter before pushing */
  if (!emu_event_filtered(type))
    return;

  emu_event_t *event = &emu_event_state.buffer[emu_event_state.write_idx];

  event->seq = emu_event_state.seq++;
  event->frame = emu_event_state.frame_count;
  event->pc = pc;
  event->address = address;
  event->value = value;
  event->type = (uint8)type;
  event->cpu = (uint8)cpu;
  event->width = width;
  event->aux = aux;

  /* Advance write pointer */
  uint32 next_idx = (emu_event_state.write_idx + 1) % EMU_EVENT_BUFFER_SIZE;

  /* Detect overflow: if next write position catches up to read position */
  if (next_idx == emu_event_read_idx)
  {
    emu_event_state.overflow_count++;
    /* Advance read pointer to make room (losing oldest event) */
    emu_event_read_idx = (emu_event_read_idx + 1) % EMU_EVENT_BUFFER_SIZE;
  }

  emu_event_state.write_idx = next_idx;
}

uint32 emu_event_cursor(void)
{
  return emu_event_state.write_idx;
}

void emu_event_clear(void)
{
  memset(&emu_event_state.buffer, 0, sizeof(emu_event_state.buffer));
  emu_event_state.write_idx = 0;
  emu_event_state.seq = 0;
  emu_event_state.overflow_count = 0;
  emu_event_read_idx = 0;
}

#endif /* HOOK_CPU */
