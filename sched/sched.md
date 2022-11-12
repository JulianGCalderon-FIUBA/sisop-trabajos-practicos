# sched.md

Lugar para respuestas en prosa, seguimientos con GDB y documentacion del TP

- Los procesos tienen un niceness que va de -19 a 20 (mientras más bajo más prioridad).
- MLFQ con 4 colas.
- Un proceso no puede aumentar su propia prioridad.
- Las niceness las modifica el usuario con las syscalls, nosotros modificamos su prioridad.

Estado de los registros:

1. Antes del cambio de contexto.

  ![image](https://user-images.githubusercontent.com/102059887/201488489-25dffab1-549c-469a-be93-fbf1e01f98d5.png)

2. Estado de los registros luego de ejecutar `iret`. Vemos que se restauraron los registros que requieren privilegios, como *cs*.

  ![image](https://user-images.githubusercontent.com/102059887/201489334-09632af7-f24c-4a31-b838-b1395d572a38.png)

Estado del *stack* durante el context switch:

1. Antes de empezar. 

  ![image](https://user-images.githubusercontent.com/102059887/201489465-ed17f274-e01e-4875-8c9b-efc1b32adc7e.png)

2. Primero se saltea la dirección de retorno.

  ![image](https://user-images.githubusercontent.com/102059887/201489514-a9d881a0-402e-4117-a589-b378a372f408.png)

3. Guardo en el registro *eax* la dirección del trapframe. Podemos usar el registro ya que todavía no se restauró el valor.

  ![image](https://user-images.githubusercontent.com/102059887/201489545-52f1da53-009b-4b88-8c4f-1bfa448684e6.png)

4. Muevo el *stack pointer* para que apunte al inicio del trapframe.

  ![image](https://user-images.githubusercontent.com/102059887/201489594-0324fd57-6c20-4f2f-9986-4e29a7a9199e.png)

5. Restauramos los registros de uso general.

  ![image](https://user-images.githubusercontent.com/102059887/201489648-2c436759-a7f9-4053-b984-5c72eb149a6c.png)

6. Restauramos el registro *es*.
  
  ![image](https://user-images.githubusercontent.com/102059887/201489685-b5915ecc-9035-402c-949f-09db9b6c60d2.png)

7. Restauramos el registro *ds*.
  
  ![image](https://user-images.githubusercontent.com/102059887/201489696-cfcd88f2-de75-4be2-a175-58e310b88dc5.png)

8. Salteamos los registros *trapno* y *errno*.

  ![image](https://user-images.githubusercontent.com/102059887/201489720-137fc5fe-bf34-43c2-bc98-d97a5567e308.png)

9. Finalmente, ejecutamos `iret`.
  
  ![image](https://user-images.githubusercontent.com/102059887/201489734-c2549939-c01b-4a3b-9d95-a2cf7ca04a3c.png)

  
