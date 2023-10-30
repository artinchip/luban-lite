#include "mpp_ge.h"
#include "mpp_fb.h"

#include "artinchip_fb.h"

/* use double frambuffer */
#define APP_FB_NUM 2

/* screen_info structure */
struct ge_fb_info {
    struct mpp_fb *fb;
    struct aicfb_screeninfo fb_data;
    int swap_flag;
};

struct ge_fb_info * fb_open(void);
void fb_close(struct ge_fb_info *info);

void fb_swap_frame(struct ge_fb_info *info);
unsigned int fb_get_cur_frame(struct ge_fb_info *info);
int fb_start_and_wait(struct ge_fb_info *info);
