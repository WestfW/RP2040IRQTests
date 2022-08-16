#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define dGPIO_IRQ ( 0 )
#define dGPIO_OUT1 ( 2 )
#define dGPIO_OUT2 ( 3 )
#define dPWMOUT 10

void GPIO_IRQHandlerFunc( uint gpio, uint32_t events );
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
    gpio_set_irq_enabled_with_callback( dGPIO_IRQ, 0x04, true, &GPIO_IRQHandlerFunc);
    mypwm_init();
    
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
    padsbank0_hw->io[dGPIO_OUT1] ^= PADS_BANK0_GPIO0_OD_BITS;
    padsbank0_hw->io[dGPIO_OUT2] ^= PADS_BANK0_GPIO0_OD_BITS;
}
