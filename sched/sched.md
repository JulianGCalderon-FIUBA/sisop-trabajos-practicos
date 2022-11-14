# sched.md
## Scheduler

El scheduler que implementamos comparte ciertas similitudes con **MLFQ**. MLFQ consiste en tener a los procesos en distintas colas y su funcionamiento básico se basa en las siguientes reglas:

  1. Si la prioridad(A) > prioridad(B), entonces A se va a ejecutar antes.
  2. Si prioridad(A) = prioridad(B), se ejecutan en *round robin*.
  3. Cuando un proceso ingresa al sistema se le da la prioridad más alta.
  4. Si un proceso ocupó todo su timeslice, de forma continua o no, se le reduce la prioridad en uno, bajándolo a la siguiente cola.
  5. Cada un tiempo $S$ se hace un *boosting*, y se sube la prioridad a la máxima, haciendo que todos los procesos estén en la primera cola.

Para realizar nuestra implementación, agregamos en el struct *Env* tres campos: *niceness*, *priority*, y *vruntime*. El *niceness* es un valor que puede ir de -19 a 20, siendo -19 el mejor valor, y se inicializa en un valor default de 1. Luego, cada env puede modificar su *niceness* o el de sus procesos hijos a través de la syscall `sys_env_set_niceness` y obtener el *niceness* de un proceso con la syscall `sys_env_get_niceness`. Cabe aclarar que un proceso solo puede incrementar su *niceness* (disminuyendo su prioridad), o la de un proceso hijo, ya que de otra forma podría darse siempre la mejor prioridad y no dejar que otros se ejecuten. El campo de *priority* indica en qué cola de prioridad se ejecutó por última vez el env, para saber en una próxima corrida en qué cola debería estar (ya sea la misma o una menos). Nuestra implementación consiste de 4 colas.

Por otro lado, el *vruntime* se utiliza para saber cuándo un env debería disminuir de cola. Todas las colas tienen un mismo timeslice, y no contabilizamos cuánto tiempo se ejecutó un proceso, si no que en cada corrida se aumenta el *vruntime* realizando un cálculo entre el *niceness* y el peso correspondiente. Cada cola tiene un límite de *vruntime* (salvo la última, ya que cuando un proceso está ahí no puede ir a otra cola), que cuando un env lo supera significa que debe modificarse su prioridad y moverse a la siguiente cola. El cálculo que se realiza para el *vruntime* es de la siguiente forma: 

  $$ vruntime = vruntime + weight $$
  
siendo el weight un valor que varía según el *niceness* del proceso. El *vruntime* se setea en 0 cada vez que un proceso es movido de cola, ya sea porque se le redujo la prioridad o porque hubo *boosting*.

El *boosting*, en vez de realizarse en un tiempo $S$, se realiza cada 64 ejecuciones. Cada env tiene guardado cuántas corridas hubo cuando se lo ejecutó, por lo que si en una próxima corrida se superó el número de ejecuciones eso indica que debe haber un *boost*. El *boost* mueve todos los procesos a la primera cola, y les cambia la prioridad a la más baja. Esto evita que haya starvation y que procesos con peor *niceness* no se ejecuten nunca. Para poder hacerlo y que tenga complejidad $O(1)$ lo que se hace es concatenar todas las colas existentes en una sola, y sumar el contador de boosts, que luego se utiliza para actualizar de forma acorde el *vruntime* y la prioridad del proceso en ejecución.

Por otra parte, en el caso donde se realiza `fork` se tomó como convención que el proceso hijo iniciará con el mismo *niceness* que el padre.

Para las estadísticas del scheduler se optó por mostrar el historial de los procesos ejecutados con su correspondiente *env_id* y su ejecución actual. Para poder visualizar las estadísticas es necesario definir *VERBOSE* durante la compilación, de este modo se mostrará el reporte al finalizar la ejecución de todos los procesos.

## Tests

La forma de testear el scheduler es con distintos procesos de usuario. Para poder ver el resultado de cada ejemplo es necesario descomentar el test correspondiente en el archivo *kern/init.c/i386_init*. Se adjuntan en los tests capturas de las terminales donde se observa en qué orden se ejecutaron los procesos, analizando el *env_id*.

### Test 1

El objetivo de este test es mostrar que luego de 5 ejecuciones un proceso con el *niceness* default termina en la cola 3, y que luego cuando se hace `fork` el proceso hijo comenzará en la primera cola, e irá bajando su prioridad hasta terminar en la cola del padre, en donde se ejecutan en *round robin*.

![image](https://user-images.githubusercontent.com/102059887/201560966-8dd14790-a0f9-43fa-9ad3-6fb4d78234e9.png)

### Test 2

La idea de este segundo test es mostrar que dos procesos con igual *niceness* se ejecutarán en *round robin* hasta finalizar. 

![image](https://user-images.githubusercontent.com/102059887/201561238-eb465209-dd50-4760-a7b5-511c2fef03d9.png)

### Test 3

En este test se ejecutan dos procesos iguales, pero hay uno que se ejecuta hasta finalizar, sin ceder el CPU al otro, ya que tiene el mejor *niceness*.

![image](https://user-images.githubusercontent.com/102059887/201561564-8fcba53e-2c65-4b72-b8b3-4b6a7ca3973f.png)

### Test 4

Este test muestra el *boosting*. En una situación normal, el proceso con mayor prioridad debería monopolizar el CPU hasta que suceda un boost, momento en el cual el segundo proceso va a poder ejecutarse. Sin embargo, en este ejemplo se puede ver que ambos procesos se ejecutan en *round robin* justo antes de que sucede el boost, ya que para ese entonces ambos se encontraban en la última cola. Luego del boost, sucede que el proceso con mayor prioridad monopoliza el CPU hasta finalizar.

![image](https://user-images.githubusercontent.com/102059887/201562564-87690a72-e3ae-4f2b-b527-6423bbcb9f69.png)

### Test 5

En este quinto test la idea es demostrar el correcto funcionamiento de las syscalls implementadas `sys_env_set_niceness` y `sys_env_get_niceness`. Esto implica que un proceso solo puede disminuir su prioridad y la de sus procesos hijos; no puede aumentar su prioridad ni modificar la de un proceso padre.

![image](https://user-images.githubusercontent.com/102059887/201563208-e431755a-14eb-4936-b620-04de5b157480.png)

### Test 6

Finalmente, este último test tiene como objetivo mostrar que un proceso hijo se crea con el mismo *niceness* que su proceso padre.

![image](https://user-images.githubusercontent.com/102059887/201563431-c5d9d566-e7df-464f-a572-ac8ce221ef2f.png)


## Estado de los registros:

1. Antes del cambio de contexto.

  ![image](https://user-images.githubusercontent.com/102059887/201533872-10f02996-2307-47d0-928a-78a5eeba9287.png)

2. Estado de los registros luego de ejecutar `iret`. Vemos que se restauraron los registros que requieren privilegios, como *cs*.

  ![image](https://user-images.githubusercontent.com/102059887/201534183-e206e1fa-d260-43be-b21c-0f4bac5619d6.png)


## Estado del *stack* y el *stack frame* durante el context switch:

1. Antes de empezar. 

 Stack frame               |  Stack
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/102059887/201533900-f76ed6a5-d11e-4977-b1eb-14b3ab1ef5d1.png)|![image](https://user-images.githubusercontent.com/102059887/201533920-42f7959a-2d80-4a66-aa2a-2d588895041b.png)

2. Primero se saltea la dirección de retorno.

 Stack frame               |  Stack
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/102059887/201533966-6b3d26e8-0b8c-41d5-a37a-155c68655a87.png)|![image](https://user-images.githubusercontent.com/102059887/201533979-9560368d-62f7-4595-99ce-2b5892fce543.png)

3. Guardo en el registro *eax* la dirección del trapframe. Podemos usar el registro ya que todavía no se restauró el valor.

Stack frame               |  Stack
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/102059887/201534011-c30fbdef-1643-4936-aacf-38085106ad93.png)|![image](https://user-images.githubusercontent.com/102059887/201534022-68b735d8-0654-4995-8cc5-a354e18a8919.png)

4. Muevo el *stack pointer* para que apunte al inicio del trapframe.

Stack frame               |  Stack
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/102059887/201534039-d251b7c2-c0a1-45e9-bfbd-299b40ebca40.png)|![image](https://user-images.githubusercontent.com/102059887/201534050-24f4e12a-4a6d-46c6-aec6-e7e447bb12ce.png)

5. Restauramos los registros de uso general.

Stack frame               |  Stack
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/102059887/201534068-3da55ffe-5506-4375-94f1-f04a091bf791.png)|![image](https://user-images.githubusercontent.com/102059887/201534077-e1d82309-e524-4dc7-ab0e-d41d18ecc756.png)

6. Restauramos el registro *es*.

Stack frame               |  Stack
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/102059887/201534084-03809adb-aebc-4e4f-a7cb-3f95174b45d7.png)|![image](https://user-images.githubusercontent.com/102059887/201534094-e5b7ae5d-217a-4236-a892-20198616e6fa.png)
  
7. Restauramos el registro *ds*.

Stack frame               |  Stack
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/102059887/201534122-927b8daa-66c5-4b3e-8585-e2cf78e6b262.png)|![image](https://user-images.githubusercontent.com/102059887/201534138-7f1f1ada-2b81-4ec6-8e17-7585df9d95ec.png)

8. Salteamos los registros *trapno* y *errno*.

Stack frame               |  Stack
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/102059887/201534152-2220bbd5-eb1c-4a2d-a43a-196370a03cb6.png)|![image](https://user-images.githubusercontent.com/102059887/201534160-1fe9fb52-cdea-47ed-a240-7835db39500b.png)

9. Finalmente, ejecutamos `iret`.

Stack frame               |  Stack
:-------------------------:|:-------------------------:
![image](https://user-images.githubusercontent.com/102059887/201534193-cf46aa3a-c4d1-4976-b354-5a3532cb2358.png)|![image](https://user-images.githubusercontent.com/102059887/201534203-d67f46c4-eac8-42c5-a267-73a2e30e0ee5.png)
  
 Como ya se ejecutó `iret` se le devolvió el control al proceso de usuario, que no tiene permisos para ver el stack.
  
