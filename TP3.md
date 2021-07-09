TP3: Multitarea con desalojo
========================

env_return
---------

Al terminar la funcion `umain()`, la funcion `libmain()` llama a `exit()`, la cual a su vez llama a la syscall `sys_env_destroy(0)`, donde el parametro 0 indica que se quiere destruir el environment actual. En `sys_env_destroy()` se llama a `syscall()`, donde se realiza la interrupcion que devuelve el control al kernel. Luego se pasa por el trap handler, las funciones `trap()`, `trap_dispatch()`, y las funciones del kernel `syscall()` y `sys_env_destroy()`. 
En esta ultima se llama a `env_destroy()`, donde el kernel dispone del proceso.

En el TP2, `env_destroy()` simplemente liberaba el proceso y llamaba al monitor del kernel.
En el TP3, si el proceso esta corriendo en otras CPUs, se cambia el estado del proceso de RUNNING a DYING, dejandolo como zombie. La siguiente vez que se llama a `env_destroy()`, el proceso se libera.
Si el proceso que se quiere destruir es el actual, se cede la ejecucion a otro proceso mediante `sched_yield()`.

envid2env
---------

Cuando se llama a `sys_env_destroy(0)`, dentro de esta función se realiza un llamado a `envid2env()` a la cual se le pasa el parámetro envid con valor 0. Esta función especifica que cuando envid vale 0, devolverá el curenv.
```C
	// If envid is zero, return the current environment.
	if (envid == 0) {
		*env_store = curenv;
		return 0;
	}
```

Por lo tanto, en `sys_env_destroy()` la variable `e` será igual a curenv, y mediante `env_destroy(e)` se destruirá el proceso actual.

sys_yield
---------
```
Booting from Hard Disk..6828 decimal is 15254 octal!
Physical memory: 131072K available, base = 640K, extended = 130432K
check_page_free_list() succeeded!
check_page_alloc() succeeded!
check_page() succeeded!
check_kern_pgdir() succeeded!
check_page_free_list() succeeded!
check_page_installed_pgdir() succeeded!
SMP: CPU 0 found 1 CPU(s)
enabled interrupts: 1 2
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000.
Hello, I am environment 00001001.
Hello, I am environment 00001002.
Back in environment 00001000, iteration 0.
Back in environment 00001001, iteration 0.
Back in environment 00001002, iteration 0.
Back in environment 00001000, iteration 1.
Back in environment 00001001, iteration 1.
Back in environment 00001002, iteration 1.
Back in environment 00001000, iteration 2.
Back in environment 00001001, iteration 2.
Back in environment 00001002, iteration 2.
Back in environment 00001000, iteration 3.
Back in environment 00001001, iteration 3.
Back in environment 00001002, iteration 3.
Back in environment 00001000, iteration 4.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001001, iteration 4.
All done in environment 00001001.
[00001001] exiting gracefully
[00001001] free env 00001001
Back in environment 00001002, iteration 4.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
No runnable environments in the system!
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
K> 
```

El programa yield.c realiza una iteracion de 5 ciclos donde en cada uno se llama a la syscall sys_yield y se imprime el numero del proceso y de la iteracion.
Hay 3 procesos creados en estado RUNNABLE, y el scheduler ejecuta el primero que encuentra en ese estado (env 1000). Este primer proceso, ya en estado RUNNING, comienza a iterar y llama a sys_yield. 
Al hacerlo, el scheduler toma el proximo proceso RUNNABLE (env 1001) y lo ejecuta. Como este proceso tambien esta corriendo el programa yield.c, realizara lo mismo que el proceso 1000 y llamara a sys_yield. Por lo tanto, el scheduler levantara el siguiente proceso RUNNABLE disponible (env 1002), que tambien esta corriendo una instancia de yield.c. Cuando este ultimo proceso llame a sys_yield, el scheduler va a recorrer circularmente el arreglo hasta llegar nuevamente al env 1000.
Esto se repite en las siguientes 4 iteraciones de yield.c para los 3 procesos.


multicore_init
---------
1. `memmove(code, mpentry_start, mpentry_end - mpentry_start);` copia el código que empieza en `mpentry_start` y termina en `mpentry_end`, a su destino que es `code`, que sería la virtual address de `MPENTRY_PADDR` (0x7000). Este código se usa para bootear los APs y se encuentra en el archivo mpentry.S . El memmove es necesario de realizar ya que el código de booteo de los APs debe ser accesible por debajo de los 2^16 bytes de memoria física.

2. `mpentry_kstack` guarda el tope del stack del kernel para cada CPU. Esta variable se setea en init y no en mpentry.S porque si se realizara en este último, todos los CPUs compartirían stack ya que el código es común para todos los CPUs, y el código en init.c contempla que hay N CPUs por lo que la memoria de cada stack estará reservada en direcciones diferentes.

3. 

```
(gdb) watch mpentry_kstack
Hardware watchpoint 1: mpentry_kstack
(gdb) continue
Continuing.
The target architecture is assumed to be i386
=> 0xf010015f <boot_aps+92>:    mov    %esi,%ecx

Thread 1 hit Hardware watchpoint 1: mpentry_kstack

Old value = (void *) 0x0
New value = (void *) 0xf0254000 <percpu_kstacks+65536>
boot_aps () at kern/init.c:109
109                     lapic_startap(c->cpu_id, PADDR(code));
(gdb) bt
#0  boot_aps () at kern/init.c:109
#1  0xf0100210 in i386_init () at kern/init.c:55
#2  0xf0105e66 in ?? ()
#3  0xf0100047 in entry () at kern/entry.S:88
(gdb) info threads
  Id   Target Id                    Frame 
* 1    Thread 1.1 (CPU#0 [running]) boot_aps () at kern/init.c:109
  2    Thread 1.2 (CPU#1 [halted ]) 0x000fd0ae in ?? ()
  3    Thread 1.3 (CPU#2 [halted ]) 0x000fd0ae in ?? ()
  4    Thread 1.4 (CPU#3 [halted ]) 0x000fd0ae in ?? ()
(gdb) continue
Continuing.
=> 0xf010015f <boot_aps+92>:    mov    %esi,%ecx

Thread 1 hit Hardware watchpoint 1: mpentry_kstack

Old value = (void *) 0xf0254000 <percpu_kstacks+65536>
New value = (void *) 0xf025c000 <percpu_kstacks+98304>
boot_aps () at kern/init.c:109
109                     lapic_startap(c->cpu_id, PADDR(code));
(gdb) info threads
  Id   Target Id                    Frame 
* 1    Thread 1.1 (CPU#0 [running]) boot_aps () at kern/init.c:109
  2    Thread 1.2 (CPU#1 [running]) 0xf01002a2 in mp_main () at kern/init.c:127
  3    Thread 1.3 (CPU#2 [halted ]) 0x000fd0ae in ?? ()
  4    Thread 1.4 (CPU#3 [halted ]) 0x000fd0ae in ?? ()
(gdb) thread 2
[Switching to thread 2 (Thread 1.2)]
#0  0xf01002a2 in mp_main () at kern/init.c:127
127             xchg(&thiscpu->cpu_status, CPU_STARTED);  // tell boot_aps() we're up
(gdb) bt
#0  0xf01002a2 in mp_main () at kern/init.c:127
#1  0x00000000 in ?? ()
(gdb) p cpunum()
$1 = 1
(gdb) thread 1
[Switching to thread 1 (Thread 1.1)]
#0  boot_aps () at kern/init.c:111
111                     while (c->cpu_status != CPU_STARTED)
(gdb) p cpunum()
$2 = 0
(gdb) continue
Continuing.
=> 0xf010015f <boot_aps+92>:    mov    %esi,%ecx

Thread 1 hit Hardware watchpoint 1: mpentry_kstack

Old value = (void *) 0xf025c000 <percpu_kstacks+98304>
New value = (void *) 0xf0264000 <percpu_kstacks+131072>
boot_aps () at kern/init.c:109
109                     lapic_startap(c->cpu_id, PADDR(code));
```

4. Esta línea se encuentra algunas lineas después de mpentry_start, por lo tanto si se redondea a 12 bits, y sabiendo que el código de mpentry.S fue copiado a la dirección física 0x7000 (la cual corresponde a mpentry_start), el eip estará aproximadamente en la dirección 0x7000. Esto es teniendo en cuenta que mediante el lapic se configuraron los APs para que el eip inicien en esa dirección.

Si se pone un breakpoint en mpentry_start, no se detendrá la ejecución ya que el valor del eip no coincide con la dirección del símbolo mpentry_start dado que fue copiado a la dirección mencionada anteriormente.


dumbfork
---------

1. Si una pagina no es modificable en el padre, lo es en el hijo, porque cuando se duplican las paginas en duppage() se llama a sys_page_alloc() con permisos de escritura.

2.

Para verificar los permisos de una pagina, tenemos que acceder a la page table entry correspondiente. En espacio de usuario, esto se puede hacer a traves del arreglo uvpt[].

```C
envid_t dumbfork(void) {
    // ...
    for (addr = UTEXT; addr < end; addr += PGSIZE) {
        bool readonly;
        
        pte_t pte = uvpt[PGNUM(addr)];
        if(pte & PTE_W == 0)
        	readonly = true;
        else readonly = false;

        duppage(envid, addr, readonly);
    }
```

3.


ipc_recv
---------

Si la syscall sys_ipc_recv() falla, ipc_recv() guarda un 0 en los parametros from_env_store y perm_store, si no son NULL.
En este caso, from_env_store == &src. Entonces, hay que verificar si src == 0.

```C
envid_t src = -1;
int r = ipc_recv(&src, 0, NULL);

if (r < 0)
  if (src == 0)
    puts("Hubo error.");
  else
    puts("Valor negativo correcto.")

```