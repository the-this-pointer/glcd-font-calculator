## GLCD Font Calculator

#### Description

![](screenshots/Capture.PNG)

This code is a simple SSD1306 font pixel calculator.
 You can use it for the following library:

<a href="https://github.com/4ilo/ssd1306-stm32HAL" target="_blank">ssd1306-stm32HAL</a>

It is under HEAVY development for now, 
but you can use it for creating/editing characters. 

#### Build Instruction

The project uses opengl, `glew`, `glfw` as the backend for ui, and uses `nuklear` library which is included in the project. At this time, just `Microsoft Windows` is supported.
You can build the project on windows using msys2.
* At this time, I don't remember the libraries I've installed for my msys environment, if you have any problems with build instructions just let me know!

Install these packages first:

``` bash
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-cmake

pacman -S mingw-w64-x86_64-glew
pacman -S mingw-w64-x86_64-glfw
```

Then run these commands to build the project:
``` bash
mkdir build
cd build
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

If you have any questions about how cmake works (or you want to use other compilers), you can read this <a href="https://www.msys2.org/docs/cmake/" target="_blank">link</a>.  

#### Usage

Keep in mind that if you plan to work on a font with different dimensions, you can change the dimension from `Settings` menu (Located in `MENU`).

You have some fonts in the `fonts.c` file of the library, that every character presented in a line there, like this:

``` c
0x0800, 0x1800, 0x2800, 0x2800, 0x4800, 0x7C00, 0x0800, 0x0800, 0x0000, 0x0000,
```

###### Editing a character
Just copy the line and press the `Read f. Clipboard` menu (Located in `VALUE` menu) in the generator, this will parse the character and show that to you.

###### Generating a character

After editing pixels when you are ready to use the character, you should press the `Calc & Copy`, this will show you the generated data and also copy the data to the clipboard ready to use in the library. The generated data is like below:

``` c
0x0800, 0x1800, 0x2800, 0x2800, 0x4800, 0x7C00, 0x0800, 0x0800, 0x0000, 0x0000,
```
This calculator just create the character for you and the rest of code such as declaring font is on yourself.

See the result below, the picture belongs to one of my projects named `Universal Timer` you can find <a href="https://github.com/the-this-pointer/timer-board-stm32f103" target="_blank">here</a>:
![LCD Image](screenshots/1683184317955_2.jpg)

#### Remaining Works

These are the remaining tasks I plan to do in my spare time:

- Edit multiple characters simultaneously
- Generate font characters to header file