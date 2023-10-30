#include <aic_core.h>
#include <boot_rom.h>

void jump_to_rom_upgmode_entry(void)
{
    u8 *p = (void *)0x30000066;
    void (*rom_upgmode_entry)(void);

    switch (*p) {
        case 0x33:
            rom_upgmode_entry = (void *)0x30007be6;
            break;
        case 0x37:
            rom_upgmode_entry = (void *)0x30007dd0;
            break;
        default:
            return;
    }

    aicos_dcache_clean_invalid();
    aicos_dcache_disable();
    aicos_icache_disable();
    asm volatile("li sp, 0x30044000");
    rom_upgmode_entry();
}
