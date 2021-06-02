TP1: Memoria virtual en JOS
===========================

backtrace_func_names
--------------------

Salida del comando `backtrace`:

```
K> backtrace
  ebp f0115f48  eip f0100a55  args 00000001 f0115f70 00000000 0000000a 00000009
	     kern/monitor.c:122: runcmd+261

  ebp f0115fc8  eip f0100a9e  args 00010094 00010094 f0115ff8 f01000f7 00000000
	     kern/monitor.c:140: monitor+66

  ebp f0115fd8  eip f01000f7  args 00000000 00001aac 00000658 00000000 00000000
	     kern/init.c:49: i386_init+86

  ebp f0115ff8  eip f0100047  args 000000a1 00000000 00000000 00000000 00000000
	     kern/entry.S:90: entry+59
```


boot_alloc_pos
--------------

En la primera llamada a boot_alloc, como nextfree no está inicializado, se ejecuta el siguiente código: 

```
if (!nextfree) {
		extern char end[];
		nextfree = ROUNDUP((char *) end, PGSIZE);
	}
```
De esta manera, ```nextfree``` se inicializa con el valor de ```end ```alineado a PGSIZE. ```end ``` apunta al final del kernel, y esta dirección se puede ver ejecutando el comando ```nm obj/kern/kernel```, y toma el valor f0117958. 
Realizando un redondeo para que quede alineado a PGSIZE=0x00001000, el valor resultante es nextfree = f0118000. Este es el valor que retorna boot_alloc. 

```
0x0000fff0 in ?? ()
(gdb) b boot_alloc
Breakpoint 1 at 0xf0100ac4: file kern/pmap.c, line 98.
(gdb) c
Continuing.
The target architecture is assumed to be i386
=> 0xf0100ac4 <boot_alloc>:	cmpl   $0x0,0xf0117538

Breakpoint 1, boot_alloc (n=4096) at kern/pmap.c:98
98		if (!nextfree) {
(gdb) finish
Run till exit from #0  boot_alloc (n=4096) at kern/pmap.c:98
=> 0xf01026a6 <mem_init+24>:	mov    %eax,0xf0117950
mem_init () at kern/pmap.c:141
141		kern_pgdir = (pde_t *) boot_alloc(PGSIZE);
Value returned is $1 = (void *) 0xf0118000
```

Se observa que el valor obtenido en gdb es igual al que fue calculado en la sección anterior.

page_alloc
----------
page2pa() recibe un puntero a una struct PageInfo y devuelve la dirección física de la página correspondiente a esta struct. Sabiendo la dirección de comienzo del arreglo pages se calcula la posición relativa del struct PageInfo dentro del arreglo, y realizando un shift de 12 bits hacia la izquierda, se obtiene la dirección física de la página.

page2kva() recibe el mismo puntero pero devuelve la dirección virtual de la página. Por como está mapeada la memoria virtual en JOS, simplemente le suma KERNBASE a la dirección física obtenida con page2pa().


map_region_large
----------------

Dado que al usar large pages no es necesario reservar una page table, por cada large page utilizada (4 MiB), estamos ahorrando una page table que ocupa 4 KiB.
Este tamaño es fijo, por lo tanto el espacio ahorrado no depende de la memoria física de la computadora, sino de cuántas large pages se está alocando.

