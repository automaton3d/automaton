# It from bit — a concrete attempt
Hi all,

I'm developing a toy model of the universe using the cellular automaton paradigm. The detailed research ideas are contained in this <A HREF="https://zenodo.org/record/7579718#.Y-p7LMfMK00">pdf</A> file.

Here I present my efforts to implement a tiny version (in computational terms only because the rules are independent of the memory size of the model) of the universal automaton described in the aforementioned document. Two versions are being developed. One in the C programming language using CDT. The other, mixing Cuda and OpenGL, being developed under Visual Studio with Cuda Toolkit 7.5 under Windows 11. The GPU used is an NVIDIA GTX 1050 super. Only the CDT version is updated (16/02/2023).

I'm an independent researcher. My link to the Research Gate portal is

https://www.researchgate.net/

The immediate goals of this computational implementation are
* Compute the Poincaré Cycle for sizes SIDE=8, SIDE=16 and SIDE=32 (the universe is SIDE=2 to 268 power);
* Check charge quantization for SIDE=32.

To achieve even these simple goals, I need lots of computational power, far beyond my limited budget. Notice that I do not receive funds from any university, so your help will be very valuable :)

You are visitor 

![Visitor Count](https://profile-counter.glitch.me/javaresende/count.svg)


Last update: 13/02/2023
