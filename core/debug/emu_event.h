/***************************************************************************************
 *  Genesis Plus GX
 *  Emulator Event System - Unified event logging for instrumentation
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

#ifndef _EMU_EVENT_H_
#define _EMU_EVENT_H_

#include "types.h"


/* ======================================================================== */
/* Event Type Enumeration                                                   */
/* ======================================================================== */

typedef enum
{
  EMU_EVENT_FRAME_BEGIN = 0,
  EMU_EVENT_FRAME_END,

  EMU_EVENT_M68K_EXEC,
  EMU_EVENT_S68K_EXEC,
  EMU_EVENT_Z80_EXEC,

  EMU_EVENT_GATE_ARRAY_READ,
  EMU_EVENT_GATE_ARRAY_WRITE,

  EMU_EVENT_COMMUNICATION,

  EMU_EVENT_BIOS_EXECUTION,
  EMU_EVENT_BIOS_CALL,
  EMU_EVENT_BIOS_RETURN,

  EMU_EVENT_CDD_COMMAND,

  EMU_EVENT_CDC_DMA_START,
  EMU_EVENT_CDC_DMA_END
} emu_event_type_t;


/* ======================================================================== */
/* CPU Type Enumeration                                                     */
/* ======================================================================== */

typedef enum
{
  EMU_CPU_NONE = 0,
  EMU_CPU_M68K,
  EMU_CPU_S68K,
  EMU_CPU_Z80
} emu_cpu_t;


/* ======================================================================== */
/* Event Structure                                                           */
/* ======================================================================== */

typedef struct
{
  uint32 seq;         /* Sequence number (wraps around on overflow) */
  uint32 frame;       /* Frame number */
  uint32 pc;          /* Program counter or address */
  uint32 address;     /* Memory address (for memory/peripheral access) */
  uint32 value;       /* Data value */
  uint8 type;         /* Event type (emu_event_type_t) */
  uint8 cpu;          /* CPU type (emu_cpu_t) */
  uint8 width;        /* Access width (1, 2, 4 bytes) or 0 if not applicable */
  uint8 aux;          /* Auxiliary data (e.g., command number for CDD) */
} emu_event_t;


/* ======================================================================== */
/* Event Filter Bit Definitions                                             */
/* ======================================================================== */

/* Individual event type filters */
#define EMU_EVENT_FILTER_FRAME          (1 << 0)
#define EMU_EVENT_FILTER_M68K_EXEC      (1 << 1)
#define EMU_EVENT_FILTER_S68K_EXEC      (1 << 2)
#define EMU_EVENT_FILTER_Z80_EXEC       (1 << 3)
#define EMU_EVENT_FILTER_GATE_ARRAY     (1 << 4)
#define EMU_EVENT_FILTER_COMMUNICATION  (1 << 5)
#define EMU_EVENT_FILTER_BIOS           (1 << 6)
#define EMU_EVENT_FILTER_CDD            (1 << 7)
#define EMU_EVENT_FILTER_CDC            (1 << 8)

/* Combined masks */
#define EMU_EVENT_FILTER_ALL_CPU_EXEC   (EMU_EVENT_FILTER_M68K_EXEC | \
                                         EMU_EVENT_FILTER_S68K_EXEC | \
                                         EMU_EVENT_FILTER_Z80_EXEC)

#define EMU_EVENT_FILTER_NONE           0
#define EMU_EVENT_FILTER_DEFAULT        EMU_EVENT_FILTER_ALL_CPU_EXEC


/* ======================================================================== */
/* Ring Buffer Size Configuration                                           */
/* ======================================================================== */

#ifndef EMU_EVENT_BUFFER_SIZE
#define EMU_EVENT_BUFFER_SIZE 8192
#endif


/* ======================================================================== */
/* API Functions                                                            */
/* ======================================================================== */

/* Initialize the event system */
extern void emu_event_init(void);

/* Reset the event buffer (clears all events) */
extern void emu_event_reset(void);

/* Set the event filter mask */
extern void emu_event_set_mask(uint32 mask);

/* Get the current event filter mask */
extern uint32 emu_event_get_mask(void);

/* Get the number of events currently in the buffer */
extern uint32 emu_event_count(void);

/* Get the number of events that were dropped due to overflow */
extern uint32 emu_event_overflow_count(void);

/* Read an event from the buffer (returns 1 if event available, 0 if buffer empty) */
extern int emu_event_read(emu_event_t *event);

/* Push an event to the buffer */
extern void emu_event_push(emu_event_type_t type, emu_cpu_t cpu, uint32 pc,
                           uint32 address, uint32 value, uint8 width, uint8 aux);

/* Get the current write position cursor */
extern uint32 emu_event_cursor(void);

/* Clear all events from the buffer */
extern void emu_event_clear(void);


#endif /* _EMU_EVENT_H_ */
