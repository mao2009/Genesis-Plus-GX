/***************************************************************************************
 *  Genesis Plus GX
 *  Emulator Event System Unit Tests
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "debug/emu_event.h"

/* Test counters */
static int test_passed = 0;
static int test_failed = 0;

/* Test macros */
#define ASSERT_EQ(a, b, fmt) \
  do { \
    if ((a) != (b)) { \
      printf("FAIL: %s line %d: " fmt "\n", __FILE__, __LINE__, (a), (b)); \
      test_failed++; \
      return 0; \
    } \
  } while(0)

#define ASSERT_TRUE(cond) \
  do { \
    if (!(cond)) { \
      printf("FAIL: %s line %d: condition false\n", __FILE__, __LINE__); \
      test_failed++; \
      return 0; \
    } \
  } while(0)

#define TEST_START printf("Test: %s\n", __func__); test_passed++
#define TEST_END return 1

/* ======================================================================== */
/* Unit Tests                                                               */
/* ======================================================================== */

static int test_init(void)
{
  TEST_START;

  emu_event_init();
  ASSERT_EQ(emu_event_count(), 0, "Count=%u, expected 0");
  ASSERT_EQ(emu_event_overflow_count(), 0, "Overflow=%u, expected 0");
  ASSERT_EQ(emu_event_get_mask(), EMU_EVENT_FILTER_DEFAULT, "Mask=%u, expected %u");

  TEST_END;
}

static int test_push_read(void)
{
  TEST_START;

  emu_event_init();
  emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, 0x1000, 0, 0, 0, 0);

  ASSERT_EQ(emu_event_count(), 1, "Count=%u, expected 1");

  emu_event_t event;
  ASSERT_TRUE(emu_event_read(&event));
  ASSERT_EQ(event.type, EMU_EVENT_M68K_EXEC, "Type=%u, expected %u");
  ASSERT_EQ(event.cpu, EMU_CPU_M68K, "CPU=%u, expected %u");
  ASSERT_EQ(event.pc, 0x1000, "PC=%u, expected %u");
  ASSERT_EQ(event.seq, 0, "Seq=%u, expected 0");

  ASSERT_EQ(emu_event_count(), 0, "Count=%u after read, expected 0");

  TEST_END;
}

static int test_wraparound(void)
{
  TEST_START;

  emu_event_init();

  /* Fill buffer to wrap-around point */
  int i;
  for (i = 0; i < EMU_EVENT_BUFFER_SIZE + 10; i++)
  {
    emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, i, 0, 0, 0, 0);
  }

  /* Buffer should contain exactly EMU_EVENT_BUFFER_SIZE events */
  ASSERT_EQ(emu_event_count(), EMU_EVENT_BUFFER_SIZE, "Count=%u, expected %u");

  /* Read some events and verify sequence numbers are sequential */
  emu_event_t event;
  for (i = 0; i < 5; i++)
  {
    ASSERT_TRUE(emu_event_read(&event));
    ASSERT_EQ(event.seq, i + 10, "Seq=%u at pos %d, expected %u");
  }

  TEST_END;
}

static int test_overflow_detection(void)
{
  TEST_START;

  emu_event_init();
  ASSERT_EQ(emu_event_overflow_count(), 0, "Initial overflow=%u, expected 0");

  /* Push more than buffer capacity */
  int i;
  for (i = 0; i < EMU_EVENT_BUFFER_SIZE + 5; i++)
  {
    emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, 0, 0, 0, 0, 0);
  }

  /* Should detect overflow */
  ASSERT_TRUE(emu_event_overflow_count() > 0);

  TEST_END;
}

static int test_sequence_tracking(void)
{
  TEST_START;

  emu_event_init();

  /* Push multiple events */
  int i;
  for (i = 0; i < 100; i++)
  {
    emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, i, 0, 0, 0, 0);
  }

  /* Verify sequence is incremental */
  emu_event_t event;
  for (i = 0; i < 100; i++)
  {
    ASSERT_TRUE(emu_event_read(&event));
    ASSERT_EQ(event.seq, i, "Seq=%u at pos %d, expected %u");
  }

  TEST_END;
}

static int test_filter_all_events(void)
{
  TEST_START;

  emu_event_init();
  emu_event_set_mask(EMU_EVENT_FILTER_NONE);

  /* Push with filter disabled */
  emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, 0, 0, 0, 0, 0);

  /* Should not be stored */
  ASSERT_EQ(emu_event_count(), 0, "Count=%u, expected 0 with filter disabled");

  TEST_END;
}

static int test_filter_specific_type(void)
{
  TEST_START;

  emu_event_init();
  emu_event_set_mask(EMU_EVENT_FILTER_M68K_EXEC);

  /* Push M68K execution event (should be stored) */
  emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, 0, 0, 0, 0, 0);
  ASSERT_EQ(emu_event_count(), 1, "Count=%u, expected 1 for M68K_EXEC");

  /* Push S68K event (should not be stored) */
  emu_event_push(EMU_EVENT_S68K_EXEC, EMU_CPU_S68K, 0, 0, 0, 0, 0);
  ASSERT_EQ(emu_event_count(), 1, "Count=%u, expected 1 (S68K should be filtered)");

  TEST_END;
}

static int test_clear(void)
{
  TEST_START;

  emu_event_init();

  /* Push events */
  int i;
  for (i = 0; i < 50; i++)
  {
    emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, 0, 0, 0, 0, 0);
  }

  ASSERT_EQ(emu_event_count(), 50, "Count=%u before clear, expected 50");

  /* Clear */
  emu_event_clear();

  ASSERT_EQ(emu_event_count(), 0, "Count=%u after clear, expected 0");
  ASSERT_EQ(emu_event_overflow_count(), 0, "Overflow=%u after clear, expected 0");

  TEST_END;
}

static int test_cursor(void)
{
  TEST_START;

  emu_event_init();

  uint32 cursor1 = emu_event_cursor();
  ASSERT_EQ(cursor1, 0, "Initial cursor=%u, expected 0");

  emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, 0, 0, 0, 0, 0);
  uint32 cursor2 = emu_event_cursor();
  ASSERT_TRUE(cursor2 != cursor1);

  emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, 0, 0, 0, 0, 0);
  uint32 cursor3 = emu_event_cursor();
  ASSERT_TRUE(cursor3 != cursor2);

  TEST_END;
}

static int test_reset(void)
{
  TEST_START;

  emu_event_init();

  /* Push events */
  int i;
  for (i = 0; i < 30; i++)
  {
    emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, 0, 0, 0, 0, 0);
  }

  emu_event_reset();

  ASSERT_EQ(emu_event_count(), 0, "Count=%u after reset, expected 0");

  /* Verify sequence counter resets */
  emu_event_push(EMU_EVENT_M68K_EXEC, EMU_CPU_M68K, 0, 0, 0, 0, 0);
  emu_event_t event;
  ASSERT_TRUE(emu_event_read(&event));
  ASSERT_EQ(event.seq, 0, "Seq=%u after reset, expected 0");

  TEST_END;
}

static int test_event_data_preservation(void)
{
  TEST_START;

  emu_event_init();

  /* Push event with specific data */
  emu_event_push(EMU_EVENT_GATE_ARRAY_WRITE, EMU_CPU_M68K, 0xABCD, 0x1234, 0x5678, 2, 0xEF);

  emu_event_t event;
  ASSERT_TRUE(emu_event_read(&event));
  ASSERT_EQ(event.type, EMU_EVENT_GATE_ARRAY_WRITE, "Type=%u, expected %u");
  ASSERT_EQ(event.cpu, EMU_CPU_M68K, "CPU=%u, expected %u");
  ASSERT_EQ(event.pc, 0xABCD, "PC=%u, expected %u");
  ASSERT_EQ(event.address, 0x1234, "Address=%u, expected %u");
  ASSERT_EQ(event.value, 0x5678, "Value=%u, expected %u");
  ASSERT_EQ(event.width, 2, "Width=%u, expected %u");
  ASSERT_EQ(event.aux, 0xEF, "Aux=%u, expected %u");

  TEST_END;
}


/* ======================================================================== */
/* Test Runner                                                              */
/* ======================================================================== */

int main(void)
{
  printf("=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
  printf("Emulator Event System Unit Tests\n");
  printf("=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n\n");

  /* Run all tests */
  test_init();
  test_push_read();
  test_wraparound();
  test_overflow_detection();
  test_sequence_tracking();
  test_filter_all_events();
  test_filter_specific_type();
  test_clear();
  test_cursor();
  test_reset();
  test_event_data_preservation();

  /* Report results */
  printf("\n" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
  printf("Tests passed: %d\n", test_passed);
  printf("Tests failed: %d\n", test_failed);
  printf("=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");

  return test_failed == 0 ? 0 : 1;
}

#endif /* HOOK_CPU */
