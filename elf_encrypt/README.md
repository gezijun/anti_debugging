参考了ThomasKing在看雪的贴子，但查找符号位置使用了一种新方法。

原代码使用的 `DT_HASH`，老版本 GCC 和现在的安卓都在使用这个结构，它比较简单。在 Ubuntu 14.04 上测试时发现新版 GCC 并没有用 `DT_HASH` ，而是使用的 `DT_GUN_HASH` ，它使用 `BloomFilter` 算法针对符号不存在的情况做了效率优化。

这个结构比较复杂，如果再按照 ELF 加载器的流程来做就比较麻烦，所以选择了遍历的方法。但也有个缺点，当查找的符号不存在时程序会崩溃 ，当然要加密的函数名我们事先都知道的，也不是什么大问题 ^_^ 

运行结果如下：

```bash
kiiim@ubuntu :~/_elf/m2$ gcc shell.c 
kiiim@ubuntu :~/_elf/m2$ gcc loader.c -o loader -ldl
kiiim@ubuntu :~/_elf/m2$ gcc demo.c -fPIC -shared -o libdemo.so
kiiim@ubuntu :~/_elf/m2$ ./a.out libdemo.so 
Find section .dynamic, size = 0x1c0, addr = 0xe18
Find .dynstr, addr = 0x488
Find .dynsym, addr = 0x230
Find .dynstr size, size = 0x10f
Find: say_hello, offset = 0x9f5, size = 0x12
Complete!
kiiim@ubuntu :~/_elf/m2$ ./loader 
hello elf.
kiiim@ubuntu:~/_elf/m2$ 
```

原贴中还有另一种加固方法，将要加固函数写入新的节区，如 .mytext，然后针对节区整体加密。这种方法实现更简单。但评论里有个问题值得讨论下。

`有回复说实现了.text整体加密方案，但我分析了下，觉得不可信`

观察 `.init_array` 节，发现在解密函数 `__init()` 执行前，还要执行一个 `frame_dummy()` 的系统函数：

```bash
.init_array:0000000000200DF8 _init_array     segment para public 'DATA' use64
.init_array:0000000000200DF8                 assume cs:_init_array
.init_array:0000000000200DF8                 ;org 200DF8h
.init_array:0000000000200DF8 __frame_dummy_init_array_entry dq offset frame_dummy
.init_array:0000000000200E00                 dq offset __init  ;解密函数
```

而这个函数是在 `.text` 中实现的：

```bash
.text:00000000000009C0 frame_dummy     proc near
.text:00000000000009C0                 cmp     cs:__JCR_LIST__, 0
.text:00000000000009C8                 jz      short loc_9F0
.text:00000000000009CA                 mov     rax, cs:_Jv_RegisterClasses_ptr
.text:00000000000009D1                 test    rax, rax
.text:00000000000009D4                 jz      short loc_9F0
.text:00000000000009D6                 push    rbp
.text:00000000000009D7                 lea     rdi, __JCR_LIST__
.text:00000000000009DE                 mov     rbp, rsp
.text:00000000000009E1                 call    rax ; _Jv_RegisterClasses
.text:00000000000009E3                 pop     rbp
.text:00000000000009E4                 jmp     register_tm_clones
.text:00000000000009E4 ; ---------------------------------------------------------------------------
.text:00000000000009E9                 align 10h
.text:00000000000009F0
.text:00000000000009F0 loc_9F0:                                ; CODE XREF: frame_dummy+8 j
.text:00000000000009F0                                         ; frame_dummy+14 j
.text:00000000000009F0                 jmp     register_tm_clones

.text:00000000000009F0 frame_dummy     endp
```

也就是说，在解密函数 `__init()` 执行之前， `frame_dummy()` 运行就会失败。也就不能对 `.text` 整体加密。
