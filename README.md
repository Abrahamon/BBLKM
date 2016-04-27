BBLKM
===================


El codigo encontrado en este repositorio implementa un pequeño **intérprete** que genera instrucciones hacia el hardware. El código es generado desde una aplicacion movil(**Java**) o una computadora (**C++**) de escritorio, ambas terminales se conectan via socket con un servidor(**Python**) que interpreta el codigo y genera instrucciones enviadas por un puerto serial hacia un **arduino** que controla un lapiz con el cual se dibuja. Ver los [<i class="icon-refresh"></i> requisitos](#Requerimientos)  para ejecutar el repositorio completo.

----------


Creadores:
-------------
Abraham Arias Chinchilla. <br />
Lenin Torres Valverde. <br />
2016

-------------

Requerimientos
-------------

La configuracion del servidor es automatica, basta con correr el script y el mismo encuentra la ip y puerto disponibles. Para correr todo se debe contar con:

> - Computador de escritorio corriendo la "desktop app" o aplicacion movil con la respactiva app.
> - Servidor ejecutando script de python.
> - Placa de arduino con conexion inalambrica, ejecutando el script respectivo. 
> - La placa de arduino debe controlar algun tipo de hardware que controle un lapiz de acuerdo a las instrucciones recibidas.

-------------

Aplicacion para el usuario
-------------

La aplicacion de escritorio cuenta con linea de comandos y editor de texto, los comandos para la terminal son los mismos comandos explicados en la seccion de comandos en el editor de texto.

Para el editor de texto existen las siguientes opciones:

#### <i class="icon-file"></i> Comandos:

| Item     | Value | Qty   |
| :------- | ----: | :---: |
| Computer | $1600 |  5    |
| Phone    | $12   |  12   |
| Pipe     | $1    |  234  |


#### <i class="icon-folder-open"></i> Nuevo y Abrir, Copiar y Pegar

A <kbd>Ctrl+[</kbd> and <kbd>Ctrl+]</kbd>.

#### <i class="icon-pencil"></i> Guardar

You can rename the current document by clicking the document title in the navigation bar.


#### <i class="icon-hdd"></i> Verificar y ejecutar

A.

> **Tip:** [<i class="icon-upload"></i> Verificar](#Verificaryejecutar) el codigo antes de enviar como codigo final.

----------


### Table of contents

You can insert a table of contents using the marker `[TOC]`:

[TOC]





### UML diagrams

You can also render sequence diagrams like this:

```sequence
Alice->Bob: Hello Bob, how are you?
Note right of Bob: Bob thinks
Bob-->Alice: I am good thanks!
```

And flow charts like this:

```flow
st=>start: Start
e=>end
op=>operation: My Operation
cond=>condition: Yes or No?

st->op->cond
cond(yes)->e
cond(no)->op
```

> **Note:** You can find more information:

> - Readme creado [here][1],




  [1]: https://stackedit.io/
