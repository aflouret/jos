TP3: Multitarea con desalojo
========================

env_return
---------

Al terminar la funcion `umain()`, la funcion `libmain()` llama a `exit()`, la cual a su vez llama a la syscall `sys_env_destroy(0)`, donde el parametro 0 indica que se quiere destruir el environment actual. En `sys_env_destroy()` se llama a `syscall()`, donde se realiza la interrupcion que devuelve el control al kernel. Luego se pasa por el trap handler, las funciones `trap()`, `trap_dispatch()`, y las funciones del kernel `syscall()` y `sys_env_destroy()`. 
En esta ultima se llama a `env_destroy()`, donde el kernel dispone del proceso.

En el TP2, `env_destroy()` simplemente liberaba el proceso y llamaba al monitor del kernel.
En el TP3, si el proceso esta corriendo en otras CPUs, se cambia el estado del proceso de RUNNING a DYING, dejandolo como zombie. La siguiente vez que se llama a `env_destroy()`, el proceso se libera.
Si el proceso que se quiere destruir es el actual, se cede la ejecucion a otro proceso mediante `sched_yield()`.


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