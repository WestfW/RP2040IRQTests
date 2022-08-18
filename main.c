#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/structs/xip_ctrl.h"
#include "hardware/gpio.h"
#include "hardware/structs/iobank0.h"
#include "hardware/structs/sio.h"

#define dGPIO_IRQ ( 0 )
#define dGPIO_OUT1 ( 2 )
#define dGPIO_OUT2 ( 3 )
#define dPWMOUT 10

// define EXCLUSIVEIRQ non-zero to have the code add the IRQ directly
// to vector table.  set it to zero to use the per-pin dispatch via
// gpio_set_irq_enabled_with_callbac
#define EXCLUSIVEIRQ 1

// define USESIO non-zero to try to use the one-cycle SIO instead of GPIO
#define USESIO 1

void GPIO_IRQHandlerFunc( uint gpio, uint32_t events );
void GPIO_exclusiveIRQ(void);
bool bSetting = false;

void mypwm_init() {
  gpio_set_drive_strength(dPWMOUT, GPIO_DRIVE_STRENGTH_8MA);
// Tell GPIO it's allocated to the PWM
  gpio_set_function(dPWMOUT, GPIO_FUNC_PWM);

// Find out which PWM slice is connected to GPIO (it's slice 0)
  uint slice_num = pwm_gpio_to_slice_num(dPWMOUT);
  uint chan = pwm_gpio_to_channel(dPWMOUT);
    
// set clock divisor assuming 120MHz clock.  We want 500kHz
  pwm_set_clkdiv_int_frac(slice_num, 240, 0);  // 120MHz/240 = 500kHz.
// Set period of 500 cycles to get ~1kHz
  pwm_set_wrap(slice_num, 499);
// Set channel A output high for one cycle before dropping
  pwm_set_chan_level(slice_num, chan, 100);
// Set the PWM running
  pwm_set_enabled(slice_num, true);

// Note we could also use pwm_set_gpio_level(gpio, x) which looks up the
// correct slice and channel for a given GPIO.
}

int main()
{
//  set_sys_clock_khz(266000, true);
    gpio_init(dGPIO_OUT1);
    gpio_set_dir(dGPIO_OUT1, true);
    gpio_put(dGPIO_OUT1, true);
    gpio_set_pulls(dGPIO_OUT1, false, true);

    gpio_init(dGPIO_OUT2);
    gpio_set_dir(dGPIO_OUT2, true);
    gpio_put(dGPIO_OUT2, true);
    gpio_set_pulls(dGPIO_OUT2, false, true);

    gpio_init(dGPIO_IRQ);
    gpio_set_dir(dGPIO_IRQ, false);
    gpio_set_pulls(dGPIO_IRQ, true, false);
    /* Use falling slope as trigger source */
#if EXCLUSIVEIRQ
    irq_set_exclusive_handler(IO_IRQ_BANK0, GPIO_exclusiveIRQ);
    gpio_set_irq_enabled(dGPIO_IRQ, 0x4, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
#else
    gpio_set_irq_enabled_with_callback(dGPIO_IRQ, 0x04, true, &GPIO_IRQHandlerFunc);
#endif    
    mypwm_init();
    xip_ctrl_hw->ctrl = 0; // disable cache
    while(1)
    {
    }

    return 0;
}

// hal_gpio_irq_handler 

//void __not_in_flash_func()
// __attribute__((__section__(".scratch_x.GPIO_IRQHandlerFunc")))
void GPIO_IRQHandlerFunc( uint gpio, uint32_t events )
{
    padsbank0_hw->io[dGPIO_OUT1] = PADS_BANK0_GPIO0_OD_BITS;
    padsbank0_hw->io[dGPIO_OUT2] ^= PADS_BANK0_GPIO0_OD_BITS;
}

void GPIO_exclusiveIRQ(void)
{
#if USESIO
  sio_hw->gpio_oe_togl = 1<<dGPIO_OUT1;
  sio_hw->gpio_oe_togl = 1<<dGPIO_OUT2;
#else
    padsbank0_hw->io[dGPIO_OUT1] ^= PADS_BANK0_GPIO0_OD_BITS;
    padsbank0_hw->io[dGPIO_OUT2] ^= PADS_BANK0_GPIO0_OD_BITS;
#endif
    // clear interrupt cause
    iobank0_hw->intr[dGPIO_IRQ / 8] = 0xF << 4 * (dGPIO_IRQ % 8);
}
