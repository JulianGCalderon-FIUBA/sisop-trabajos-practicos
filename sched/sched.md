# sched.md

Lugar para respuestas en prosa, seguimientos con GDB y documentacion del TP

- Los procesos tienen un niceness que va de -19 a 20 (mientras más bajo más prioridad).
- MLFQ con 4 colas.
- Un proceso no puede aumentar su propia prioridad.
- Las niceness las modifica el usuario con las syscalls, nosotros modificamos su prioridad.
