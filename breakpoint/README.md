实际测试时，在下面代码的 puts() 位置下断：

```c
printf ("check one : total size %d, location %p\n", ph_t->p_memsz, p);
```

```bash
check_breakpoint:000087BE LDR             R2, =0xFFFFEA36
check_breakpoint:000087C0 ADDS            R0, R2, R0
check_breakpoint:000087C2 MOV             R2, R5
check_breakpoint:000087C4 BL              puts
check_breakpoint:000087C8 LDRB            R0, [R5]
check_breakpoint:000087CA CMP             R0, #0xF0
check_breakpoint:000087CC BEQ             loc_87E6
```

将 check_breakpoint 内存 dump 出来，puts 处内存(000007C4)并没有替换为断点指令，原因未知。

可能参考文章本身有错误？
http://www.droidsec.cn/anti-debugging-skills-in-apk/


使用IDA 6.8，dump 脚本：

```c
static main(void)
{
  auto fp, byte;
  fp = fopen("d:\\check_dump", "wb");

  for ( byte = 0x00008000; byte < 0x00008000+0x00001000; byte ++ )
      fputc(Byte(byte), fp);
}

```