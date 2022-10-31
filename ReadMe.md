## GLCD Font Calculator

![](screenshots/Capture.PNG)

This code is a simple SSD1306 font pixel calculator. I made this to use for my project <a href="https://github.com/the-this-pointer/timer-board-stm32f103" target="_blank">here</a>.

 You can use it for the following library also:

<a href="https://github.com/4ilo/ssd1306-stm32HAL" target="_blank">ssd1306-stm32HAL</a>

It is under HEAVY development for now, 
but you can use it for creating/editing characters. 

Keep in mind that if you plan to work on a font with different dimensions, you should edit these three lines in the code:

``` c
#define COL_W 11
#define COL_H 18
#define CELL_SIZE 15
```

It doesn't support font generation based on true-type fonts yet. 

These are the remaining tasks I plan to do in my spare time:

- Character dimension settings in UI
- Edit multiple characters simultaneously
- Create chars from true-type fonts
- UI/UX Improvements