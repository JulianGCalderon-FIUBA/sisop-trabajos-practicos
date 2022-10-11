# TP: malloc

En primer lugar se definieron dos estructuras: `region` y `block`, donde la primera representa una sección contigua de memoria virtual que la biblioteca le devuelve al usuario, y la segunda es utilizada por la biblioteca para administrar la memoria de forma más ordenada. De esta forma, se trabaja con _regiones_ que quedan contenidas en _bloques_. Tanto _bloques_ como _regiones_ se trabajan como listas doblemente enlazadas.

También se definieron distintas _macros_ que ayudan a realizar aritmética de punteros de forma más ordenada y permiten el movimiento entre regiones o bloques sin tener que conocer sus tamaños. 

Luego, para poder administrar la memoria de forma más ordenada, se definieron tres listas de bloques, una por cada tamaño permitido: tamaño pequeño (16Kib), tamaño mediano (1Mib) y tamaño grande (32Mib). De esta forma, los bloques de distintos tamaños quedan separados y es más fácil encontrar el bloque adecuado para el tamaño de memoria que el usuario reserva. Por ejemplo, si se quieren reservar más de 1Mib (tamaño mediano) se recorrerá directamente la lista de bloques grandes buscando alguna región libre para devolver. Si el usuario quiere reservar una región de memoria de tamaño mayor al bloque grande la biblioteca fallará, y, por el contrario, hay un mínimo de tamaño para las regiones; si el usuario quiere reservar menos memoria se devuelve siempre una región con el tamaño mínimo. 

A la hora de buscar regiones libres para devolverle al usuario se pueden utilizar dos estrategias distintas: **first fit** o **best fit**; cuando se ejecuta el programa el usuario puede seleccionar la estrategia que prefiere. Si se elige **first fit** se recorrerán las listas de bloques de tamaño mayor o igual a la memoria que se quiere reservar (no tendría sentido recorrer la lista de bloques pequeños si se quieren reservar más de 16Kib, por ejemplo) hasta encontrar la primera región libre de tamaño mayor o igual al pedido. Esta estrategia tiene la ventaja de ser más rápida pero puede suceder que la región tenga más memoria de la que necesita el usuario, por lo que habrá que hacer un `split` para evitar la fragmentación interna. Por otro lado, con **best fit** se recorren las listas de bloques de tamaño mayor o igual al pedido por el usuario pero buscando la menor de las regiones con tamaño suficiente. Es un poco más lento ya que, además de recorrer todas las listas hasta el final, debe ir comparando el tamaño de las regiones y guardando la mejor región. 

¿¿faltaría mencionar:
  - los bloques se reservan con mmap
  - split
  - coalescing
  - algo de realloc o calloc
??
