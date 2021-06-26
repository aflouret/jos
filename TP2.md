TP2: Procesos de usuario
========================

env_alloc
---------

1. Los id de los procesos serían 0x00001000, 0x00001001, 0x00001002, 0x00001003 y 0x00001004. Los 12 bits menos significativos representan a la posición en el arreglo envs y el resto de los bits corresponden a la generación, es decir el numero de veces que se usó la misma posición en el arreglo.
2. El primer proceso que se encuenta en `env[630]` tiene como id 0x00001276. Al destruirlo y crear otro en la misma posición, aumentará su generación en 1000 por lo que su id será 0x00002276. Por lo que para las siguientes 3 iteraciones será: 0x0003276, 0x0004276 y 0x0005276. 


env_init_percpu
---------------

La función `lgdt` escribe 6 bytes en el registro `GDTR`, los cuales 4 bytes representan la base address de la GDT y los otros 2 son el limit de la GDT 


env_pop_tf
----------
Antes de llamar a `popal`, se mueve el stack pointer al tope del trapframe. En el tope está el struct PushRegs, que contiene el estado de los registros edi, esi, ebp, oesp, ebx, edx, ecx y eax.
Justo antes de `iret`, el tope de la pila contiene el instruction pointer `eip`.
Si consideramos que el primer elemento de la pila es `eip` y que cada elemento tiene un tamaño de 4 bytes, el tercer elemento de la pila es `eflags`.


El CPU compara el nivel de privilegio RPL (Requestor Privilege Level) del cs del trapframe contenido en el stack con el nivel de privilegio actual (CPL) del registro cs del CPU. Si el del stack es mayor, se realiza un cambio de ring. Esto se puede ver en la pagina 3-478 del manual IA32-2A:
```
IF CS(RPL) > CPL
	THEN GOTO RETURN-TO-OUTER-PRIVILEGE-LEVEL;
	ELSE GOTO RETURN-TO-SAME-PRIVILEGE-LEVEL; FI;
```

Para almacenar el nivel de privilegio en el registro cs se utilizan 2 bits.
...


gdb_hello
---------

1.
```
Continuing.
The target architecture is assumed to be i386
=> 0xf0102fa7 <env_pop_tf>:	endbr32 

Breakpoint 1, env_pop_tf (tf=0xf01c7000) at kern/env.c:466


```

2.
```
EAX=003bc000 EBX=f01c7000 ECX=f03bc000 EDX=00000200
ESI=00010094 EDI=00000000 EBP=f0119fd8 ESP=f0119fbc
EIP=f0102fa7 EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]

```

3.
```
$1 = (struct Trapframe *) 0xf01c7000

```

4.
```
0xf01c7000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c7010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c7020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c7030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c7040:	0x00000023

```
5.
```
(gdb) disas
Dump of assembler code for function env_pop_tf:
=> 0xf0102fa7 <+0>:	endbr32 
   0xf0102fab <+4>:	push   %ebp
   0xf0102fac <+5>:	mov    %esp,%ebp
   0xf0102fae <+7>:	sub    $0xc,%esp
   0xf0102fb1 <+10>:	mov    0x8(%ebp),%esp
   0xf0102fb4 <+13>:	popa   
   0xf0102fb5 <+14>:	pop    %es
   0xf0102fb6 <+15>:	pop    %ds
   0xf0102fb7 <+16>:	add    $0x8,%esp
   0xf0102fba <+19>:	iret   
   0xf0102fbb <+20>:	push   $0xf010553d
   0xf0102fc0 <+25>:	push   $0x1dc
   0xf0102fc5 <+30>:	push   $0xf01054c6
   0xf0102fca <+35>:	call   0xf01000ad <_panic>
End of assembler dump.
(gdb) si 5
=> 0xf0102fb4 <env_pop_tf+13>:	popa 

```
6.
```
x/17x $sp
0xf01c7000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c7010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c7020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c7030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c7040:	0x00000023

```
7.
Los valores corresponden a los siguientes campos del trapframe:
```
0xf01c7000:	reg_edi		reg_esi		reg_ebp		reg_oesp
0xf01c7010:	reg_ebx		reg_edx		reg_ecx		reg_eax
0xf01c7020:	tf_es		tf_ds		tf_trapno	tf_err
0xf01c7030:	tf_eip		tf_cs		tf_eflags	tf_esp
0xf01c7040:	tf_ss
```

El campo tf_eip contiene la direccion del entry point del binario.
El campo tf_esp contiene la direccion USTACKTOP, que es la direccion de memoria virtual donde se mapea el stack de un proceso de usuario.
Estos valores fueron configurados en la funcion load_icode().

Los campos tf_es, tf_ds, tf_ss y tf_cs, correspondientes a los registros de extra segment, data segment, stack segment y code segment, son configurados en la funcion env_alloc().
Los valores que tienen son:

```
tf_ds = GD_UD | 3;
tf_es = GD_UD | 3;
tf_ss = GD_UD | 3;
tf_cs = GD_UT | 3;
```
Donde GD_UD = 0x20 es el descriptor de user data en la GDT, y GD_UT = 0x18 es el descriptor de user text en la GDT. El 3 representa el nivel de privilegio RPL (Requestor Privilege Level) de este proceso.

Los registros restantes valen 0 ya que no fueron configurados.

8.

```
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=f01c7030
EIP=f0102fba EFL=00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]

```
Antes de ejecutar `iret` se ejecutaron las instrucciones `popal`, `pop es` y `pop ds`. Por lo tanto, vemos que los valores de los registros de proposito general y el `es` ahora tienen los valores que estaban en el trapframe. El stack pointer aumento 0x30 posiciones como resultado de los pops, y el instruction pointer aumento 19 respecto de su posicion original.

9.

```
p $pc
$1 = (void (*)()) 0x800020
```

El program counter apunta al entry point, casteado a puntero a funcion.

```
(gdb) add-symbol-file obj/user/hello 0x800020
add symbol table from file "obj/user/hello" at
	.text_addr = 0x800020
(y or n) y
Reading symbols from obj/user/hello...
(gdb) p $pc
$2 = (void (*)()) 0x800020 <_start>

```

Se puede ver que al cargar la tabla de simbolos, la direccion del entry point corresponde al simbolo `_start`.

```
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=eebfe000
EIP=00800020 EFL=00000002 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]

```

Como resultado de ejecutar `iret`, se observa que ahora todos los registros contienen los valores que guardaba el trapframe. El stack pointer apunta a USTACKTOP, el instruction pointer apunta al entry point y el code segment tiene el nivel de privilegio 3.

10. 
La instruccion `int 0x30` intenta ejecutar una syscall, pero la syscall no esta implementada. Por lo tanto, ocurre un error SIGQUIT.

user_evil_hello
---------------

La diferencia entre evilhello.c y el código mencionado es que en este último en la línea ` char first = *entry; ` se intenta desreferenciar un puntero que contiene una dirección del kernel y la mmu no lo permite porque el programa está corriendo en modo usuario.
Es decir, durante la ejecución del programa la cpu se encuenra en ring 3 (user mode) mientras que la página a la que se intenta acceder no tiene permisos de user ya que solo puede ser utilizada cuando la cpu se encuentra en ring 0 (kernel mode).
En cambio, en `evilhello.c` se logra acceder a la dirección porque se realiza dentro de la syscall (y no antes), por lo que la cpu se encuentra en ring 0 para este momento.
En el código de ejemplo, se intenta acceder a la dirección `0xf010000c` la cual es la dirección del primer caracter del código del kernel.
En el código de `evilhello.c`, se logra acceder a la misma dirección y sus 99 caracteres contiguos, ya que sys_cputs se llama con un size de 100 caracteres, por lo que se accederá hasta la dirección `0xf0100070`