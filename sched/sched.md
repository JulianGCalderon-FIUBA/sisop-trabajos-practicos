# sched.md

Lugar para respuestas en prosa, seguimientos con GDB y documentacion del TP

- Los procesos tienen un niceness que va de -19 a 20 (mientras más bajo más prioridad).
- MLFQ con 4 colas.
- Un proceso no puede aumentar su propia prioridad.
- Las niceness las modifica el usuario con las syscalls, nosotros modificamos su prioridad.

## Estado de los registros:

1. Antes del cambio de contexto.

  ![image](https://user-images.githubusercontent.com/102059887/201533872-10f02996-2307-47d0-928a-78a5eeba9287.png)

2. Estado de los registros luego de ejecutar `iret`. Vemos que se restauraron los registros que requieren privilegios, como *cs*.

  ![image](https://user-images.githubusercontent.com/102059887/201534183-e206e1fa-d260-43be-b21c-0f4bac5619d6.png)

## Estado del *stack* y el *stack frame* durante el context switch:

1. Antes de empezar. 

  ![image](https://user-images.githubusercontent.com/102059887/201533900-f76ed6a5-d11e-4977-b1eb-14b3ab1ef5d1.png)
  ![image](https://user-images.githubusercontent.com/102059887/201533920-42f7959a-2d80-4a66-aa2a-2d588895041b.png)

2. Primero se saltea la dirección de retorno.

  ![image](https://user-images.githubusercontent.com/102059887/201533966-6b3d26e8-0b8c-41d5-a37a-155c68655a87.png)
  ![image](https://user-images.githubusercontent.com/102059887/201533979-9560368d-62f7-4595-99ce-2b5892fce543.png)

3. Guardo en el registro *eax* la dirección del trapframe. Podemos usar el registro ya que todavía no se restauró el valor.

  ![image](https://user-images.githubusercontent.com/102059887/201534011-c30fbdef-1643-4936-aacf-38085106ad93.png)
  ![image](https://user-images.githubusercontent.com/102059887/201534022-68b735d8-0654-4995-8cc5-a354e18a8919.png)

4. Muevo el *stack pointer* para que apunte al inicio del trapframe.

  ![image](https://user-images.githubusercontent.com/102059887/201534039-d251b7c2-c0a1-45e9-bfbd-299b40ebca40.png)
  ![image](https://user-images.githubusercontent.com/102059887/201534050-24f4e12a-4a6d-46c6-aec6-e7e447bb12ce.png)

5. Restauramos los registros de uso general.

  ![image](https://user-images.githubusercontent.com/102059887/201534068-3da55ffe-5506-4375-94f1-f04a091bf791.png)
  ![image](https://user-images.githubusercontent.com/102059887/201534077-e1d82309-e524-4dc7-ab0e-d41d18ecc756.png)

6. Restauramos el registro *es*.
  
  ![image](https://user-images.githubusercontent.com/102059887/201534084-03809adb-aebc-4e4f-a7cb-3f95174b45d7.png)
  ![image](https://user-images.githubusercontent.com/102059887/201534094-e5b7ae5d-217a-4236-a892-20198616e6fa.png)

7. Restauramos el registro *ds*.
  
  ![image](https://user-images.githubusercontent.com/102059887/201534122-927b8daa-66c5-4b3e-8585-e2cf78e6b262.png)
  ![image](https://user-images.githubusercontent.com/102059887/201534138-7f1f1ada-2b81-4ec6-8e17-7585df9d95ec.png)

8. Salteamos los registros *trapno* y *errno*.

  ![image](https://user-images.githubusercontent.com/102059887/201534152-2220bbd5-eb1c-4a2d-a43a-196370a03cb6.png)
  ![image](https://user-images.githubusercontent.com/102059887/201534160-1fe9fb52-cdea-47ed-a240-7835db39500b.png)

9. Finalmente, ejecutamos `iret`.
  
  ![image](https://user-images.githubusercontent.com/102059887/201534193-cf46aa3a-c4d1-4976-b354-5a3532cb2358.png)
  ![image](https://user-images.githubusercontent.com/102059887/201534203-d67f46c4-eac8-42c5-a267-73a2e30e0ee5.png)
  
  Como ya se ejecutó `iret` se le devolvió el control al proceso de usuario, que no tiene permisos para ver el stack.
  
