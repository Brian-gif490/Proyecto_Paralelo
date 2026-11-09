# TEL420 - Hito 2: Clúster MPI para Optimización de Rutas de Recolección de Residuos Sólidos en Yacuiba

## 1. Descripción

Este repositorio contiene la implementación del **Hito 2** del proyecto final de la asignatura **TEL420 - Sistemas Paralelos**. El objetivo es construir la infraestructura base de un sistema paralelo que permita optimizar rutas de recolección de residuos sólidos en la ciudad de Yacuiba, Bolivia.

El entregable incluye:

- Un clúster contenerizado de **3 nodos** (1 maestro y 2 trabajadores) con Docker y Docker Compose.
- Una red virtual interna tipo `bridge` con direcciones IP estáticas.
- Acceso SSH sin contraseña entre los nodos mediante generación automática de llaves.
- Un programa secuencial de referencia para el problema del viajante de comercio (TSP) con medición de tiempo `T1`.
- Un programa MPI preliminar (*smoke test*) que verifica la comunicación entre los tres nodos.
- Datos sintéticos de puntos geográficos de Yacuiba.
- Scripts de compilación, ejecución y diagnóstico.

## 2. Arquitectura del clúster

| Nodo | Contenedor | Hostname | IP interna | Puerto SSH en el host | Rol |
|------|------------|----------|------------|-----------------------|-----|
| Maestro | `tel420-master` | `master` | `172.20.0.2` | `2222` | Orquesta `mpirun` |
| Trabajador 1 | `tel420-worker1` | `worker1` | `172.20.0.3` | `2223` | Ejecuta procesos MPI |
| Trabajador 2 | `tel420-worker2` | `worker2` | `172.20.0.4` | `2224` | Ejecuta procesos MPI |

- Red Docker: `tel420_cluster_net` (`bridge`, subred `172.20.0.0/16`).
- Imagen base: `ubuntu:22.04`.
- Software instalado: `openssh-server`, `openssh-client`, `openmpi-bin`, `libopenmpi-dev`, `build-essential`.
- Volumen compartido `tel420_ssh_keys` para intercambiar llaves públicas entre nodos.
- Los directorios `src/`, `data/`, `scripts/`, `config/` y `bin/` se montan como volúmenes para facilitar el desarrollo.

## 3. Estructura del proyecto

```text
Proyecto/
├── Dockerfile
├── docker-compose.yml
├── Makefile
├── README.md
├── .dockerignore
├── .gitignore
├── bin/
│   └── .gitkeep
├── config/
│   └── hostfile
├── data/
│   └── puntos_yacuiba.csv
├── scripts/
│   ├── build.sh
│   ├── cluster_status.sh
│   ├── compile.sh
│   ├── entrypoint.sh
│   ├── run_mpi.sh
│   └── run_seq.sh
└── src/
    ├── parallel/
    │   └── tsp_mpi.c
    └── sequential/
        └── tsp_seq.c
```

## 4. Requisitos previos

- Docker Engine 20.10 o superior.
- Docker Compose v2 (`docker compose`).
- Al menos 4 GB de RAM y 2 GB de espacio libre.
- Conexión a Internet para construir la imagen por primera vez.
- Sistema operativo Linux, macOS o Windows con Docker Desktop.

Verificación:

```bash
docker --version
docker compose version
```

## 5. Clonación del repositorio

```bash
git clone <URL_DEL_REPOSITORIO> tel420-hito2
cd tel420-hito2
```

Si el proyecto se entrega como archivo comprimido, descomprímalo y acceda al directorio:

```bash
unzip tel420-hito2.zip
cd Proyecto
```

Si los scripts perdieron el permiso de ejecución, restáurelo:

```bash
chmod +x scripts/*.sh
```

## 6. Levantamiento del clúster

Construya la imagen del clúster:

```bash
docker compose build
```

Inicie los tres nodos en segundo plano:

```bash
docker compose up -d
```

Como atajo, puede construir y levantar el clúster en un solo paso:

```bash
docker compose up -d --build
```

Verifique el estado de los contenedores:

```bash
docker compose ps
```

Debe observar los tres servicios en estado `running` o `healthy`. Los nodos tardan unos segundos en generar sus llaves SSH y sincronizarlas.

También puede ejecutar el diagnóstico integrado:

```bash
bash scripts/cluster_status.sh
```

Este script muestra los contenedores, las direcciones IP asignadas y prueba la conexión SSH desde `master` hacia `worker1` y `worker2`.

### Verificación manual de la red y SSH

```bash
docker compose exec master bash -lc 'getent hosts master worker1 worker2'
docker compose exec master bash -lc 'ssh worker1 hostname && ssh worker2 hostname'
```

La salida esperada es:

```text
worker1
worker2
```

## 7. Compilación

Con el clúster en ejecución, compile ambos programas dentro del nodo maestro:

```bash
bash scripts/build.sh
```

El script ejecuta `make clean && make all` dentro de `/app` y muestra los binarios generados en `bin/`:

```text
bin/tsp_seq
bin/tsp_mpi
```

También puede compilar manualmente desde una shell del maestro:

```bash
docker compose exec master bash
cd /app
make
```

## 8. Ejecución de la versión secuencial (T1)

Ejecute el programa secuencial con el archivo de datos por defecto:

```bash
bash scripts/run_seq.sh
```

O indique otro archivo CSV:

```bash
bash scripts/run_seq.sh data/puntos_yacuiba.csv
```

Salida esperada (los tiempos varían según el hardware):

```text
============================================================
 TSP secuencial - Recoleccion de residuos solidos (Yacuiba)
============================================================
 Archivo de datos  : data/puntos_yacuiba.csv
 Puntos cargados   : 20
 Metodo            : vecino mas cercano + 2-opt
------------------------------------------------------------
 Ruta encontrada (deposito -> ... -> deposito):
   [ 0] id=0   Deposito Central                 (-22.037000, -63.677000)
   [ 1] id=1   Barrio San Jose                  (-22.033100, -63.681200)
   ...
   [20] regreso al deposito (id=0)
------------------------------------------------------------
 Distancia total   : 17.047472 km
 Pasadas 2-opt     : 3
 T1 (secuencial)   : 0.000048943 s
 T1 (milisegundos) : 0.048943 ms
============================================================
```

La medición de `T1` utiliza `clock_gettime(CLOCK_MONOTONIC)`, que es un reloj monotónico de alta resolución y no se ve afectado por ajustes del reloj del sistema. `T1` incluye la construcción de la matriz de distancias y la fase de optimización (vecino más cercano + 2-opt), pero excluye la lectura del archivo CSV.

## 9. Ejecución del smoke test MPI

Ejecute el programa MPI con 3 procesos distribuidos entre los nodos:

```bash
bash scripts/run_mpi.sh
```

Para lanzar una cantidad distinta de procesos:

```bash
bash scripts/run_mpi.sh 6
```

El script utiliza el archivo `config/hostfile` y las opciones necesarias para ejecutar OpenMPI como `root` dentro de contenedores:

```bash
mpirun --allow-run-as-root \
  --hostfile /app/config/hostfile \
  --map-by node \
  --bind-to none \
  --mca btl ^openib \
  --oversubscribe \
  -np 3 /app/bin/tsp_mpi
```

Salida esperada (el orden de las líneas puede variar):

```text
[MPI] rank 0/3 | host=master | pid=...
[MPI] rank 1/3 | host=worker1 | pid=...
[MPI] rank 2/3 | host=worker2 | pid=...

[MPI] Nodos participantes (3):
  - master
  - worker1
  - worker2
[MPI] Allreduce(rank) = 3 (esperado 3) -> OK
[MPI] Smoke test completado: 3 procesos en 3 nodo(s)
```

La verificación es correcta si aparecen los tres hostnames distintos y si la suma de rangos calculada con `MPI_Allreduce` coincide con el valor esperado.

## 10. Scripts disponibles

| Script | Descripción |
|--------|-------------|
| `scripts/build.sh` | Compila los programas dentro del nodo maestro. |
| `scripts/compile.sh` | Ejecuta `make clean && make all` dentro del contenedor. |
| `scripts/run_seq.sh` | Ejecuta el TSP secuencial en el nodo maestro. |
| `scripts/run_mpi.sh` | Ejecuta el smoke test MPI con el número de procesos indicado. |
| `scripts/cluster_status.sh` | Muestra el estado del clúster, las IPs y prueba el acceso SSH. |
| `scripts/entrypoint.sh` | Inicializa SSH, genera llaves y arranca `sshd` en cada nodo. |

## 11. Formato de los datos

El archivo `data/puntos_yacuiba.csv` contiene coordenadas geográficas sintéticas alrededor de la ciudad de Yacuiba:

```csv
id,nombre,lat,lon
0,Deposito Central,-22.037000,-63.677000
1,Barrio San Jose,-22.033100,-63.681200
2,Barrio El Palmar,-22.041200,-63.673500
```

- `id`: identificador numérico del punto.
- `nombre`: nombre del punto de recolección.
- `lat`: latitud en grados decimales.
- `lon`: longitud en grados decimales.

Las distancias se calculan con la fórmula de Haversine y se expresan en kilómetros. El punto con `id=0` se utiliza como depósito de inicio y fin de la ruta.

## 12. Detalles de implementación

### Red y direccionamiento

La red `tel420_cluster_net` utiliza el driver `bridge` y la subred `172.20.0.0/16`. Cada contenedor tiene una IP estática asignada mediante `ipv4_address` en `docker-compose.yml`.

### SSH sin contraseña

En cada arranque, el script `scripts/entrypoint.sh`:

1. Genera un par de llaves RSA de 4096 bits propio del nodo si aún no existe.
2. Publica la llave pública en el volumen compartido `/ssh-keys/pubkeys`.
3. Espera las llaves públicas de los demás nodos.
4. Construye `authorized_keys` con todas las llaves recolectadas.
5. Configura el cliente SSH con `StrictHostKeyChecking no` para evitar confirmaciones interactivas.
6. Arranca `sshd`.
7. En el nodo maestro, espera a que los workers acepten conexiones SSH antes de finalizar.

La autenticación por contraseña está deshabilitada; solo se permite autenticación por llave pública.

### OpenMPI

El archivo `config/hostfile` define los tres nodos con dos slots cada uno:

```text
master slots=2
worker1 slots=2
worker2 slots=2
```

La opción `--map-by node` distribuye los procesos entre nodos y `--bind-to none` evita conflictos de afinidad dentro de los contenedores. `--mca btl ^openib` deshabilita InfiniBand, que no está disponible en este entorno, y fuerza el uso de TCP.

## 13. Solución de problemas

### El puerto 2222, 2223 o 2224 ya está en uso

Edite `docker-compose.yml` y cambie los puertos del host, por ejemplo:

```yaml
ports:
  - "12222:22"
```

### Error `Permission denied (publickey)` al usar SSH

1. Verifique que los tres contenedores estén en ejecución:

   ```bash
   docker compose ps
   ```

2. Revise los logs del nodo afectado:

   ```bash
   docker compose logs master
   docker compose logs worker1
   ```

3. Reinicie los servicios:

   ```bash
   docker compose restart
   ```

4. Si el problema persiste, elimine el volumen de llaves y vuelva a levantar el clúster:

   ```bash
   docker compose down -v
   docker compose up -d --build
   ```

### `mpirun` indica que no hay suficientes slots

El script ya incluye `--oversubscribe`. Si modifica el `hostfile`, ajuste la cantidad de slots o mantenga esta opción para permitir más procesos que slots disponibles.

### OpenMPI no puede comunicarse entre contenedores

Compruebe que los hostnames resuelvan a las IPs correctas:

```bash
docker compose exec master bash -lc 'getent hosts master worker1 worker2'
```

Si la interfaz de red no es `eth0`, agregue el parámetro correspondiente:

```bash
--mca btl_tcp_if_include eth0
```

### La compilación falla por falta de paquetes

Reconstruya la imagen sin caché:

```bash
docker compose build --no-cache
docker compose up -d
```

### Archivos de `bin/` propiedad de `root`

Si compiló dentro de los contenedores, los binarios pueden quedar propiedad de `root` en el host. Corrija los permisos:

```bash
sudo chown -R "$USER":"$USER" bin
```

## 14. Limpieza

Detener y eliminar los contenedores y la red:

```bash
docker compose down
```

Eliminar además el volumen de llaves SSH:

```bash
docker compose down -v
```

Eliminar contenedores, red, volumen e imagen local:

```bash
docker compose down -v --rmi local
```

## 15. Entregables del Hito 2

- [x] Clúster de 3 nodos con Docker Compose.
- [x] Red bridge interna con IPs estáticas `172.20.0.2`, `172.20.0.3` y `172.20.0.4`.
- [x] Contenedores Ubuntu con OpenSSH Server y OpenMPI.
- [x] Generación automática de llaves y acceso SSH sin contraseña.
- [x] Programa secuencial `src/sequential/tsp_seq.c` con medición de `T1` mediante `clock_gettime`.
- [x] Programa MPI preliminar `src/parallel/tsp_mpi.c` con verificación de rangos y hostnames.
- [x] Datos sintéticos en `data/puntos_yacuiba.csv`.
- [x] Scripts de compilación, ejecución y diagnóstico.
- [x] Documentación de clonación, despliegue y ejecución.

## 16. Próximos pasos (Hito 3)

- Paralelizar el algoritmo TSP con OpenMPI.
- Distribuir la construcción de la matriz de distancias o la exploración de vecindarios 2-opt.
- Calcular métricas de desempeño: `T1`, `Tp`, speedup y eficiencia.
- Ejecutar experimentos con distintos tamaños de instancia y número de procesos.
- Comparar la calidad de la solución y el tiempo de cómputo entre la versión secuencial y la paralela.
