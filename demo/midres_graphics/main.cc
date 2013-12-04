#include "lib/armv7m/crt0.h"
#include "lib/armv7m/exception_table.h"
#include "lib/armv7m/instructions.h"
#include "lib/armv7m/scb.h"

#include "runtime/ramcode.h"

#include "vga/graphics_1.h"
#include "vga/vga.h"
#include "vga/mode/raster_640x480x1.h"

static vga::mode::Raster_640x480x1 mode;

static void set_ball(vga::Graphics1 &g, unsigned x, unsigned y) {
  g.set_pixel(x, y);
  g.set_pixel(x - 1, y);
  g.set_pixel(x + 1, y);
  g.set_pixel(x, y - 1);
  g.set_pixel(x, y + 1);
}

static void clear_ball(vga::Graphics1 &g, unsigned x, unsigned y) {
  g.clear_pixel(x, y);
  g.clear_pixel(x - 1, y);
  g.clear_pixel(x + 1, y);
  g.clear_pixel(x, y - 1);
  g.clear_pixel(x, y + 1);
}

static void step_ball(int &x, int &y,
                      int other_x, int other_y,
                      int &xi, int &yi) {
  vga::Graphics1 g = mode.make_bg_graphics();

  clear_ball(g, x, y);
  x = other_x + xi;
  y = other_y + yi;

  if (x < 0) {
    x = 0;
    xi = -xi;
  }

  if (y < 0) {
    y = 0;
    yi = -yi;
  }

  if (x >= 640) {
    x = 639;
    xi = -xi;
  }

  if (y >= 480) {
    y = 479;
    yi = -yi;
  }

  set_ball(g, x, y);

  ++yi;
  vga::sync_to_vblank();
  mode.flip();
}

void v7m_reset_handler() {
  armv7m::crt0_init();
  runtime::ramcode_init();

  // Enable fault reporting.
  armv7m::scb.write_shcsr(armv7m::scb.read_shcsr()
                          .with_memfaultena(true)
                          .with_busfaultena(true)
                          .with_usgfaultena(true));

  // Enable floating point automatic/lazy state preservation.
  // The CONTROL bit governing FP will be set automatically when first used.
  armv7m::scb_fp.write_fpccr(armv7m::scb_fp.read_fpccr()
                             .with_aspen(true)
                             .with_lspen(true));
  armv7m::instruction_synchronization_barrier();  // Now please.

  // Enable access to the floating point coprocessor.
  armv7m::scb.write_cpacr(armv7m::scb.read_cpacr()
                          .with_cp11(armv7m::Scb::CpAccess::full)
                          .with_cp10(armv7m::Scb::CpAccess::full));

  // It is now safe to use floating point.

  vga::init();

  vga::select_mode(&mode);

  mode.set_fg_color(0b111111);
  mode.set_bg_color(0b100000);

  int x[2], y[2];
  int xi = 5, yi = 1;

  x[0] = x[1] = y[0] = y[1] = 0;

  while (1) {
    step_ball(x[0], y[0], x[1], y[1], xi, yi);
    step_ball(x[1], y[1], x[0], y[0], xi, yi);
  }
}
