# fisopfs

Nuestro sistema de archivos tipo FUSE consiste de un **superbloque** que guarda las distintas **tablas de inodos** y un bitmap que indica las tablas libres. Cada tabla guarda una cantidad de **inodos** y un bitmap que indica qué inodos están libres. Los inodos son estructuras que guardan metadata tanto sobre archivos como sobre directorios, y tienen una relación uno a uno con los archivos/directorios, es decir, hay un inodo por archivo/directorio. A su vez, los inodos contienen referencias a las páginas de contenido cada archivo/directorio. En el caso de un archivo, el contenido será un stream de bytes, y en el caso de un directorio el contenido de la página serán distintas **dir_entry**, que guardan información sobre los archivos o directorios dentro del directorio al que se está accediendo.

El superbloque reside en stack, y se inicializa cuando comienza el proceso del sistema de archivos. Las demás estructuras residen en el heap y se va reservando memoria para ellas a medida que sea necesario. Cada estructura tiene un tamaño máximo establecido; se pueden tener hasta 128 tablas de inodos, y cada tabla mide lo mismo que una página (4096 bytes), por lo que la cantidad de inodos que puede sostener una tabla dependerá del tamaño de las páginas y de la estructura del inodo. Por otro lado, cada archivo/directorio tiene dedicadas hasta 5 páginas, cada una de 4096 bytes. El hecho de que las páginas tengan ese tamaño facilita reservar memoria utilizando `mmap` y `munmap`. El número 5 para la cantidad de páginas fue elegido para evitar tener archivos muy extensos. Luego, si bien hay soporte para múltiples niveles de directorios, la cantidad máxima de niveles dependerá del tamaño máximo establecido para el largo de un *path*. 

Por otra parte, el sistema de archivos también ofrece soporte para *hard links*. Esto implica que utilizando la syscall `link` se pueden crear archivos nuevos linkeados a archivos existentes, nuevos punteros al archivo.

Cuando se cierra el filesystem o se llama a `flush` se guarda toda la información que luego se recupera cuando se inicializa el sistema de archivos, permitiendo así persistencia en disco. El guardado se realiza en el siguiente orden: 

1. Se escribe el bitmap de las tablas de inodos, para saber cuáles estaban ocupadas.
2. Se escribe cada tabla ocupada.
3. Por cada tabla, escribe las páginas de los inodos ocupados en orden.

Luego, para recuperar la información se tiene que seguir el mismo orden:

1. Se guarda en el bitmap del superbloque qué tablas están ocupadas.
2. Se reserva memoria por cada tabla ocupada y se guardan en ella los inodos.
3. Por cada tabla, si el inodo no estaba vacío se reservan las páginas correspondientes y se escribe el contenido indicado.
    
De esta forma, se puede guardar y recuperar la información del sistema de archivos.
    

## Funcionalidad

Para buscar un archivo con el path '/directorio/archivo' se debe hacer lo siguiente:

1. Entrar al inodo 0 (root directory) y acceder a la información del directorio root '/'.
2. Buscar 'directorio' entre los dir_entries de '/' para encontrar su inodo.
3. Conseguir la tabla de inodos correspondiente al inodo indexando el arreglo `inode_tables`.
4. En la tabla de inodos acceder al inodo de 'directorio', para acceder a su data.
5. En las páginas de '/directorio' buscar la dir_entry de 'archivo' para obtener su inodo.
6. Conseguir la tabla de inodos correspondiente al inodo indexando el arreglo `inode_tables`.
7. En la tabla de inodos buscar el inodo de 'archivo' para acceder a su data.
8. Entrar a las páginas de 'archivo' y realizar la operación correspondiente.


# Pruebas


- [ ] Creación de un archivo

    Creación del archivo con touch:
    
        gaby@gaby:~/fisopfs$ touch to_mount/prueba_archivo

    Debug:
    
        [debug] fisopfs_getattr(/prueba_archivo)
        [debug] fisopfs_getattr(/prueba_archivo)
        [debug] fisopfs_getattr(/)
 
    Verificación con ls de la creación:
    
        gaby@gaby:~/fisopfs$ ls to_mount/
        prueba_archivo

    Debug:
    
        [debug] fisopfs_getattr(/)
        [debug] fisopfs_readdir(/)
        [debug] fisopfs_getattr(/prueba_archivo)
        [debug] fisopfs_readdir(/)
        [debug] fisopfs_getattr(/)

    El archivo fue creado exitosamente, pudo observarse en la salida del comando ls.

- [ ] Stats del archivo
    Estadísticas del archivo prueba_archivo con stat:
    
        gaby@gaby:~/fisopfs$ stat to_mount/prueba_archivo 
          File: to_mount/prueba_archivo
          Size: 0         	Blocks: 0          IO Block: 4096   regular empty file
        Device: 31h/49d	Inode: 2           Links: 1
        Access: (0664/-rw-rw-r--)  Uid: ( 1000/    gaby)   Gid: ( 1000/    gaby)
        Access: 2022-12-07 16:41:35.408322500 -0300
        Modify: 2022-12-07 16:41:35.408322500 -0300
        Change: 2022-12-07 16:35:48.000000000 -0300
         Birth: -
    
    Debug:
    
        [debug] fisopfs_getattr(/prueba_archivo)
    

    Se pudieron obtener las estadísticas correctamente, el inodo asignado fue el 2 (siguiente al reservado para el directorio raíz 1) al ser el único archivo en el filesystem.


- [ ] Escribir en un archivo 
    Escritura en el archivo prueba_archivo:
    
        gaby@gaby:~/fisopfs$ seq 10 > to_mount/prueba_archivo
    

    Debug:
            
        [debug] fisopfs_getattr(/prueba_archivo)
        [debug] fisopfs_truncate(/prueba_archivo, 0)
        [debug] fisopfs_getattr(/prueba_archivo)
        [debug] fisopfs_flush
        [debug] fisopfs_write(/prueba_archivo, 0, 21)
        [debug] fisopfs_flush


    Verificación con stat de la escritura:

        gaby@gaby:~/fisopfs$ stat to_mount/prueba_archivo 
          File: to_mount/prueba_archivo
          Size: 21        	Blocks: 1          IO Block: 4096   regular file
        Device: 31h/49d	Inode: 1           Links: 1
        Access: (0664/-rw-rw-r--)  Uid: ( 1000/    gaby)   Gid: ( 1000/    gaby)
        Access: 2022-12-07 16:41:35.408322500 -0300
        Modify: 2022-12-07 16:41:35.408322500 -0300
        Change: 2022-12-07 16:35:48.000000000 -0300
        Birth: -

    Se pudo escribir correctamente los 10 números generados por seq en el archivo prueba_archivo. Al ya existir el archivo donde se deseaba escribir, se llama a truncate en lugar de create, para la serialización se llama además a la operación flush.
    
- [ ] Leer en un archivo
    Lectura del archivo prueba_archivo con cat:
    
        gaby@gaby:~/fisopfs$ cat to_mount/prueba_archivo 
        1
        2
        3
        4
        5
        6
        7
        8
        9
        10
    
    Debug:
    
        [debug] fisopfs_getattr(/prueba_archivo)
        [debug] fisopfs_read(/prueba_archivo, 0, 4096)
    
    El contenido de prueba_archivo es el esperado mostrándose los números previamente descritos.


- [ ] Borrar un archivo
    
    Eliminación del archivo con rm:
    
        gaby@gaby:~/fisopfs$ rm to_mount/prueba_archivo
    
    Debug:
    
        [debug] fisopfs_getattr(/prueba_archivo)
        [debug] fisopfs_unlink(/prueba_archivo)

    Verificación con ls:
    
        gaby@gaby:~/fisopfs$ ls to_mount/
    
    Debug:
    
        [debug] fisopfs_getattr(/)
        [debug] fisopfs_readdir(/)
        [debug] fisopfs_readdir(/)

    El archivo fue eliminado exitosamente, ls no muestra a prueba_archivo ni se intentan obtener sus atributos.


- [ ] Crear un directorio

    Creación de un directorio prueba con mkdir:
    
        gaby@gaby:~/fisopfs$ mkdir to_mount/prueba

    Debug:
    
        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_mkdir(/prueba)
        [debug] fisopfs_getattr(/prueba)

    Verificación con ls -al:
    
        gaby@gaby:~/fisopfs$ ls -al to_mount/
        total 5
        drwxrwxrwx 13 gaby gaby  396 Dec  6 23:01 .
        drwxrwxr-x  5 gaby gaby 4096 Dec  7 16:58 ..
        drwxrwxr-x  2 gaby gaby  264 Dec  7 17:09 prueba

    Debug:
    
        [debug] fisopfs_getattr(/)
        [debug] fisopfs_readdir(/)
        [debug] fisopfs_getattr(/)
        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_readdir(/)
      
    El directorio prueba fue creado correctamente, puede listarse desde el directorio raíz con ls.

- [ ] Borrar un directorio
    Eliminación del directorio con rm:
    
        gaby@gaby:~/fisopfs$ rm -r to_mount/prueba/

    Debug:
    
        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_readdir(/prueba)
        [debug] fisopfs_readdir(/prueba)
        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_rmdir(/prueba)
        [debug] fisopfs_unlink(/prueba)
 
    Verificación con ls -al:
    
        gaby@gaby:~/fisopfs$ ls -al to_mount/
        total 5
        drwxrwxrwx 14 gaby gaby  264 Dec  6 23:01 .
        drwxrwxr-x  5 gaby gaby 4096 Dec  7 16:58 ..

    Debug:
    
        [debug] fisopfs_getattr(/)
        [debug] fisopfs_readdir(/)
        [debug] fisopfs_getattr(/)
        [debug] fisopfs_readdir(/)

    Se eliminó el directorio, al ejecutar ls no se intenta obtener los atributos del directorio eliminado ni leerlo.

- [ ] Crear un archivo dentro de un directorio

    Creación de "prueba_archivo" en la carpeta "prueba":
    
        gaby@gaby:~/fisopfs$ touch to_mount/prueba/prueba_archivo
    
    Debug:
    
        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_getattr(/prueba/prueba_archivo)
        [debug] fisopfs_create(/prueba/prueba_archivo, 0100664)
        [debug] fisopfs_getattr(/prueba/prueba_archivo)
        [debug] fisopfs_getattr(/prueba/prueba_archivo)

    Verificación con ls dentro de la carpeta prueba:
    
        gaby@gaby:~/fisopfs$ ls to_mount/prueba/
        prueba_archivo

    Debug:
    
        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_readdir(/prueba)
        [debug] fisopfs_getattr(/prueba/prueba_archivo)
        [debug] fisopfs_readdir(/prueba)


    Al hacer ls de la carpeta se lista el nuevo archivo y se obtienen sus atributos con getattr.


- [ ] Borrar un directorio (y su contenido)

    Eliminación del directorio prueba que contiene al archivo prueba_archivo:
    
        gaby@gaby:~/fisopfs$ rm -r to_mount/prueba/

    Debug:
    
        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_readdir(/prueba)
        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_readdir(/prueba)
        [debug] fisopfs_readdir(/prueba)
        [debug] fisopfs_getattr(/prueba/prueba_archivo)
        [debug] fisopfs_unlink(/prueba/prueba_archivo)
        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_rmdir(/prueba)
        [debug] fisopfs_unlink(/prueba)

    Verificación con ls de la eliminación:
    
        gaby@gaby:~/fisopfs$ ls to_mount/
    
    Debug:
    
        [debug] fisopfs_getattr(/)
        [debug] fisopfs_readdir(/)
        [debug] fisopfs_readdir(/)

    Al eliminar el directorio se hace un unlink del contenido de la carpeta y luego de la carpeta, para finalmente un rmdir de la carpeta. Se confirma la eliminación al ejecutar ls, dado que no se lista la carpeta.


- [ ] Crear un directorio dentro de directorio

    Creación del directorio hijo dentro del directorio padre:   
    
        gaby@gaby:~/fisopfs$ mkdir to_mount/padre
        gaby@gaby:~/fisopfs$ mkdir to_mount/padre/hijo

    Debug:
    
        [debug] fisopfs_getattr(/padre)
        [debug] fisopfs_mkdir(/padre)
        [debug] fisopfs_getattr(/padre)

       
        [debug] fisopfs_getattr(/padre)
        [debug] fisopfs_getattr(/padre/hijo)
        [debug] fisopfs_mkdir(/padre/hijo)
        [debug] fisopfs_getattr(/padre/hijo)


    Verificación con ls de la creación del directorio con stat:
    
        gaby@gaby:~/fisopfs$ stat to_mount/padre/hijo
          File: to_mount/padre/hijo
          Size: 264       	Blocks: 1          IO Block: 4096   directory
        Device: 31h/49d	Inode: 3           Links: 2
        Access: (0775/drwxrwxr-x)  Uid: ( 1000/    gaby)   Gid: ( 1000/    gaby)
        Access: 2022-12-08 16:27:04.000000000 -0300
        Modify: 2022-12-08 16:27:04.000000000 -0300
        Change: 2022-12-08 16:27:04.000000000 -0300
         Birth: -

    Debug:
    
        [debug] fisopfs_getattr(/padre)
        [debug] fisopfs_getattr(/padre/hijo)

    El directorio fue creado exitosamente dentro de la carpeta padre.


- [ ] Leer directorios internos 

    Lectura del archivo dentro del directorio hijo:
    
        gaby@gaby:~/fisopfs$ seq 10 > to_mount/padre/hijo/archivo
        gaby@gaby:~/fisopfs$ cat to_mount/padre/hijo/archivo 
        1
        2
        3
        4
        5
        6
        7
        8
        9
        10
    
    Debug:
    
        [debug] fisopfs_getattr(/padre)
        [debug] fisopfs_getattr(/padre/hijo)
        [debug] fisopfs_getattr(/padre/hijo/archivo)
        [debug] error: No such file or directory
        [debug] fisopfs_create(/padre/hijo/archivo, 0100664)
        [debug] fisopfs_getattr(/padre/hijo/archivo)
        [debug] fisopfs_flush
        [debug] fisopfs_write(/padre/hijo/archivo, 0, 21)
        [debug] fisopfs_flush



        [debug] fisopfs_getattr(/padre)
        [debug] fisopfs_getattr(/padre/hijo)
        [debug] fisopfs_getattr(/padre/hijo/archivo)
        [debug] fisopfs_read(/padre/hijo/archivo, 0, 4096)

    Se pudo leer correctamente del archivo que está en un directorio interno.
    Al no existir el archivo durante el redireccionamiento, se crea para luego permitir la escritura.



-----------------------------------------------------------------------------------
    
    Errores

- [ ] Intentar eliminar un directorio:
    Se intenta eliminar un directorio con rm sin el parámetro correspondiente:
    
        gaby@gaby:~/fisopfs$ rm to_mount/prueba/
        rm: cannot remove 'to_mount/prueba/': Is a directory

    Debug:

        [debug] fisopfs_getattr(/prueba)


    Al obtener las estadísticas del objetivo a eliminar, no coincide el tipo con el de un archivo, siendo un directorio impide la eliminación.


- [ ] Intentar eliminar un directorio no vacio con rmdir:

    Eliminación del directorio prueba que contiene al archivo prueba:

        gaby@gaby:~/fisopfs$ touch to_mount/prueba/archivo_prueba
        gaby@gaby:~/fisopfs$ rmdir to_mount/prueba/
        rmdir: failed to remove 'to_mount/prueba/': Directory not empty

    Debug:
        
        [...]        

        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_rmdir(/prueba)


    El objetivo coincide con el tipo directorio, pero al no estar vacío falla durante rmdir.


- [ ] Lectura sobre un archivo inexistente:

    Ejecución de cat sobre archivo prueba previamente eliminado:

        gaby@gaby:~/fisopfs$ rm to_mount/prueba/archivo_prueba 
        gaby@gaby:~/fisopfs$ cat to_mount/prueba/archivo_prueba
        cat: to_mount/prueba/archivo_prueba: No such file or directory

    Debug:

        [...]


        [debug] fisopfs_getattr(/prueba)
        [debug] fisopfs_getattr(/prueba/archivo_prueba)
        [debug] error: No such file or directory

        
    Al intentar obtener las estadísticas del objetivo para posteriormente poder leer, falla por no existir el archivo.


------------------------------

    Hardlinks:

- [ ] Creación de hardlink:

    Ejecución de ln sobre un archivo existente para crear un hardlink:
    
        gaby@gaby:~/fisopfs$ echo hola > to_mount/source
        gaby@gaby:~/fisopfs$ ln to_mount/source to_mount/link1

    Debug:
    
        [...]

        [debug] fisopfs_getattr(/link1)
        [debug] error: No such file or directory
        [debug] fisopfs_getattr(/source)
        [debug] fisopfs_getattr(/link1)
        [debug] error: No such file or directory
        [debug] fisopfs_link(/source, /link1)
        [debug] fisopfs_getattr(/link1)

    Verificación del link creado con ls -li:

        gaby@gaby:~/fisopfs$ ls -li to_mount/source to_mount/link1 
        3 -rw-rw-r-- 2 gaby gaby 0 Dec  9 14:04 to_mount/link1
        2 -rw-rw-r-- 2 gaby gaby 0 Dec  9 14:04 to_mount/source


    Debug:

        [debug] fisopfs_getattr(/source)
        [debug] fisopfs_getattr(/link1)


    Lectura del contenido del link:

    
        gaby@gaby:~/fisopfs$ cat to_mount/link1 
        hola


    El link fue creado exitosamente, en la tercer columna de ls -li se observa la cantidad de hardlinks actualmente en cada archivo. El contenido del link coincide con el del archivo original.


- [ ] Creación de varios hardlink:

   Ejecución de ln dos veces sobre un archivo existente para crear los hardlink:

        gaby@gaby:~/fisopfs$ ln to_mount/source to_mount/link1
        gaby@gaby:~/fisopfs$ ln to_mount/source to_mount/link2


    Debug:
    
        [debug] fisopfs_getattr(/link1)
        [debug] error: No such file or directory
        [debug] fisopfs_getattr(/source)
        [debug] fisopfs_getattr(/link1)
        [debug] error: No such file or directory
        [debug] fisopfs_link(/source, /link1)
        [debug] fisopfs_getattr(/link1)


        [debug] fisopfs_getattr(/link2)
        [debug] error: No such file or directory
        [debug] fisopfs_getattr(/source)
        [debug] fisopfs_getattr(/link2)
        [debug] error: No such file or directory
        [debug] fisopfs_link(/source, /link2)
        [debug] fisopfs_getattr(/link2)

    Verificación con ls -li de los hardlink creados:

        gaby@gaby:~/fisopfs$ ls -li to_mount/source to_mount/link1 to_mount/link2 
        3 -rw-rw-r-- 3 gaby gaby 0 Dec  9 14:04 to_mount/link1
        4 -rw-rw-r-- 3 gaby gaby 0 Dec  9 14:04 to_mount/link2
        2 -rw-rw-r-- 3 gaby gaby 0 Dec  9 14:04 to_mount/source

    
    Debug:

        [debug] fisopfs_getattr(/source)
        [debug] fisopfs_getattr(/link1)
        [debug] fisopfs_getattr(/link2)
            

    Se pudo crear correctamente el segundo hardlink y la cantidad aumentó correspondientemente.


- [ ] Eliminación de hardlink:

    Eliminación del segundo link con rm:

        gaby@gaby:~/fisopfs$ rm to_mount/link2
    
    Debug:

        [debug] fisopfs_getattr(/link2)
        [debug] fisopfs_unlink(/link2)
        [debug] fisopfs_getattr(/)

    Verificación con ls -li de la eliminación:

        gaby@gaby:~/fisopfs$ ls -li to_mount/source 
        2 -rw-rw-r-- 2 gaby gaby 5 Dec  9 14:04 to_mount/source

    Debug:
    
        [debug] fisopfs_getattr(/)


    El link fue eliminado correctamente, 
    

- [ ] Eliminación del archivo parcial:
    
    Eliminación del archivo sin eliminar su link:

        gaby@gaby:~/fisopfs$ rm to_mount/source 

    Debug:

        [debug] fisopfs_getattr(/source)
        [debug] fisopfs_unlink(/source)
        [debug] fisopfs_getattr(/)

    Verificación del contenido en el link:

        gaby@gaby:~/fisopfs$ cat to_mount/link1 
        hola

    Debug:

        [debug] fisopfs_getattr(/link1)
        [debug] fisopfs_read(/link1, 0, 4096)
        [debug] fisopfs_flush


    El archivo no fue realmente eliminado dado que aún tenía un hardlink "link1", el cual al leerlo mantiene el contenido del archivo.



