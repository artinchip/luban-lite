#include <aic_core.h>
#include <boot_rom.h>

void jump_to_rom_upgmode_entry(void)
{
    u8 *p = (void *)0x66;
    void (*rom_upgmode_entry)(void);

    switch (*p) {
        case 0x32:
            rom_upgmode_entry = (void *)0x5c08;
            break;
        default:
            return;
    }

    aicos_dcache_clean_invalid();
    aicos_dcache_disable();
    aicos_icache_disable();
    asm volatile("li sp, 0x103000");
    rom_upgmode_entry();
}
