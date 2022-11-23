# fisopfs

Sistema de archivos tipo FUSE.

# Pruebas

- [ ] Crear un directorio y chequear que existe con `ls`
- [ ] Borrar un directorio y chequear que ya no existe
- [ ] Crear un archivo y chequear que existe 
- [ ] Borrar un directorio (y su contenido)
- [ ] Borrar un archivo
- [ ] Stats del archivo
- [ ] Directorio dentro de directorio
- [ ] Reutilización de inodo
- [ ] Acceder con un path
- [ ] Acceder con un inodo
- [ ] Leer directorios
- [ ] Leer directorios internos 
- [ ] Leer en un archivo
- [ ] Escribir en un archivo 

# Funcionalidad

Para buscar un archivo con el path '/root/directorio/archivo' se debe hacer lo siguiente:
1. Entrar al inodo 0 (root directory) y acceder a la información de root.
2. Buscar en la data de root 'directorio' para encontrar su inodo.
3. En la tabla de inodos acceder al inodo de 'directorio', para acceder a la data.
4. En la data de 'directorio' buscar el inodo de 'archivo'.
5. En la tabla de inodos buscar el inodo de 'archivo' para acceder a su data.
6. Entrar a las páginas de 'archivo' y realizar la operación correspondiente.

