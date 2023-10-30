# beep

## 介绍
基于 rt-thread 的 pin 和 pwm 驱动的蜂鸣器控制软件包，可以容易地驱动有源蜂鸣器或无源蜂鸣器，产生各种间隔长短的鸣叫声。  
对于使用无源蜂鸣器，还支持PM（电源管理）组件，能设置使得MCU运行频率发生变化时，有正确的发声频率；也可以设置在发声期间，阻止MCU进入STOP模式，维持正常的发声。

## 驱动原理

 **有源蜂鸣器** ：  
直接驱动 GPIO 引脚产生各种间隔长短的电平信号。

 **无源蜂鸣器：**   
硬件上必须连接到 STM32 定时器的 PWM 输出通道，利用 rt-thread 的 PWM 驱动，产生相应频率的方波脉冲，驱动无源蜂鸣器。  
发声时 PM 组件省电原理，比如发2声脉冲宽度500ms，周期1000ms，频率2500Hz，默认PM_SLEEP_MODE_IDLE。则在发声时请求SLEEP模式，以维持声音不断，在不发声的间隙释放SLEEP模式，MCU 进入 STOP 模式，以节省电流：

```
时间点                0ms          500ms        1000ms       1500ms       2000ms
                      _____________             _____________             
脉冲              ____|            |____________|            |____________

载波                  |   2500Hz   |   无输出    |   2500Hz   |   无输出    |

MCU PM模式            | SLEEP MODE | STOP  MODE | SLEEP MODE | STOP  MODE |
```


## 获取软件包

使用 beep 软件包需要在 RT-Thread 的包管理中选中它，ENV 运行 menuconfig 后具体路径如下：

```
RT-Thread online packages
    peripheral libraries and drivers  --->
         [*] beep: Control the buzzer to make beeps at different intervals.  --->
               (X) Active buzzer
               ( ) Passive buzzer
```

选中 beep 软件包后先选择有源蜂鸣器或无源蜂鸣器。

#### 有源蜂鸣器设置界面：
 

```
--- beep: Control the buzzer to make beeps at different intervals.
      Buzzer type (Active buzzer)  --->
[*]   Use heap with the beep thread stack created.
      Version (latest)  --->
```

若选中“Use heap with the beep thread stack created.”（只有在内核中选中“ Using memory heap object” 选项才会出现），则从动态堆内存中分配一个beep线程句柄以及按照参数中指定的栈大小从动态堆内存中分配相应的空间，否则，初始化的是静态线程。  
具体的线程栈大小及优先级设置，请查看 beep.h 头文件。

#### 无源蜂鸣器设置界面：

```
--- beep: Control the buzzer to make beeps at different intervals.
      Buzzer type (Passive buzzer)  --->
(pwm1) Setting current PWM device name.
(4)   Setting current PWM device channel.
(2700) Setting the best frequency(Hz) for buzzer.
[*]   Block the MCU power enter to stop mode on beeping.
[*]   Beep with the original frequency at MCU run frequency changed.
[*]   Use heap with the beep thread stack created.
      Version (latest)  --->
```

 **Setting current PWM device name：**   
对应使用的TIM号，TIM1为 pwm1，TIM2为pwm2，如此类推，同时，在ENV菜单的片上外设中要开启相应的TIM。  

 **Setting current PWM device channel：**   
实际使用的TIM通道号，同时，在ENV菜单的片上外设中要开启相应的TIM通道号码。  

 **Setting the best frequency(Hz) for buzzer：**   
输入无源蜂鸣器的最佳发声频率，单位Hz，也是蜂鸣器的共振频率。  

 **Block the MCU power enter to stop mode on beeping：**   
此项能在使用PM组件时，在发“哔”声时，阻止MCU进入STOP模式，以防止正发声时因关断时钟而发声中断。  
默认是 pm_request 了 PM_SLEEP_MODE_IDLE 模式，若要修改，进 beep.h 修改 PKG_BEEP_REQUEST_PM_MODE 宏。  

 **Beep with the original frequency at MCU run frequency changed：**   
此项能在使用PM组件时，当MCU运行频率发生改变时，维持原来的最佳发声频率，不使发声频率随运行频率而改变。  

## API 简介

`void beep_init(rt_base_t pin, rt_base_t reset_level)`

beep的初始化函数，必须的。  
 **pin：**  为蜂鸣器驱动引脚号，可查看 drv_gpio.c，或使用 GET_PIN() 宏得到。  
 **reset_level：**  为蜂鸣器关断时引脚电平，PIN_LOW 或 PIN_HIGH。  
对无源蜂鸣器来说，这2个参数在初始化函数内部都没有使用到。

`void beep(rt_uint32_t nums, rt_uint32_t period, rt_uint32_t percent, rt_uint32_t freq)`

 **nums：**  闪鸣次数。  
 **period：**  蜂鸣器闪鸣的周期，以毫秒为单位，[10-100000]，鸣叫和间歇即一个完整的周期。  
 **percent：**  鸣叫的占空比，鸣叫的时间和周期的百分比，[1-100]。  
 **freq：**  蜂鸣器鸣叫的发声频率，单位Hz，[0, 500-10000]（0：维持上次的发声频率），此参数对有源蜂鸣器无作用。

`void beep_stop(void)`

主动停止蜂鸣器。

`void beep_deinit(void)`

几乎没什么用。

## 使用示例

```
#include <beep.h>
  ...
/* 比如可以在main.c中使用，此为无源蜂鸣器使用，有源的一样，就发声频率不能控制 */
int main(void)
{
  ...
    /* 先初始化 beep */
    /* 第一个参数为蜂鸣器驱动引脚号，可查看drv_gpio.c，或使用GET_PIN()宏得到，如下面为PA11脚 */
    /* 第二个参数为蜂鸣器关断时引脚电平，PIN_LOW或PIN_HIGH */
    /* 对无源蜂鸣器来说，这2个参数在初始化函数内部都没有使用到 */
    beep_init(GET_PIN(A, 11), PIN_LOW);

    /* 鸣叫3声，周期1000ms，鸣叫占空比50%，默认发声频率 */
    beep(3, 1000, 50, 0);

    rt_thread_mdelay(8000);

    /* 鸣叫3声，周期1000ms，鸣叫占空比70%，2500Hz */
    beep(3, 1000, 70, 2500);

    rt_thread_mdelay(8000);

    /* 鸣叫1声，周期800ms，鸣叫占空比100%，上一次的发声频率，即2500Hz */
    beep(1, 800, 100, 0);

    rt_thread_mdelay(5000);

    /* 鸣叫20声，周期1000ms，鸣叫占空比50%，2000Hz */
    beep(20, 1000, 50, 2000);

    rt_thread_mdelay(5000);

    /* 中途打断，停止鸣叫 */
    beep_stop();

    while(1)
        rt_thread_mdelay(10000);
}
```

在 Msh 控制台，可以使用 beep 命令发声：

`beep <nums> <period> [prcent] [freq]`

前2个参数必选，后2个可以省略，可以输入 2、3、或 4 个参数，prcent默认50%，频率默认 0 。

## 最后
如果这个软件包对你有用的话，请点个赞！
