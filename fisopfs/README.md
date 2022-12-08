# fisopfs

Sistema de archivos tipo FUSE.

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

    Se pudo escribir correctamente los 10 números generados por seq en el archivo prueba_archivo.
    
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
        [debug] fisopfs_create(/padre/hijo/archivo, 0100664)
        [debug] fisopfs_getattr(/padre/hijo/archivo)


        [debug] fisopfs_getattr(/padre)
        [debug] fisopfs_getattr(/padre/hijo)
        [debug] fisopfs_getattr(/padre/hijo/archivo)
        [debug] fisopfs_read(/padre/hijo/archivo, 0, 4096)

    Se pudo leer correctamente del archivo que está en un directorio interno.



- [ ] Reutilización de inodo

# Funcionalidad

Para buscar un archivo con el path '/directorio/archivo' se debe hacer lo siguiente:
1. Entrar al inodo 0 (root directory) y acceder a la información del directorio root '/'.
2. Buscar 'directorio' entre los dir_entries de '/' para encontrar su inodo.
3. Conseguir la tabla de inodos correspondiente al inodo indexando el arreglo `inode_tables`.
4. En la tabla de inodos acceder al inodo de 'directorio', para acceder a su data.
5. En las páginas de '/directorio' buscar la dir_entry de 'archivo' para obtener su inodo.
6. Conseguir la tabla de inodos correspondiente al inodo indexando el arreglo `inode_tables`.
7. En la tabla de inodos buscar el inodo de 'archivo' para acceder a su data.
8. Entrar a las páginas de 'archivo' y realizar la operación correspondiente.

