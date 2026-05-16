# utec — CRUD de Estudiantes

CRUD didáctico para el curso Principios de Programación.

- **Backend**: C + libmicrohttpd + SQLite + cJSON
- **Frontend**: React 18 + Vite
- **Persistencia**: SQLite local (`students.db`)
- **Sin autenticación**

## Estructura

```
.
├── backend/        # API REST en C
│   ├── Makefile
│   ├── Dockerfile
│   └── src/        # main.c, router, handlers, db, student, http_util
├── frontend/       # SPA React (Vite)
│   ├── package.json
│   ├── Dockerfile
│   ├── nginx.conf
│   └── src/        # App, api, components/
├── nginx/          # vhost para el reverse proxy del host
├── docker-compose.yml
└── docker-compose.prod.yml
```

## Correr en local sin Docker

### Backend

```bash
sudo apt install libmicrohttpd-dev libsqlite3-dev libcjson-dev
cd backend
make
./server          # escucha en :8080, crea students.db
```

Puerto personalizado: `./server 9000`.
DB en otra ruta: `STUDENTS_DB=/tmp/foo.db ./server`.

### Frontend

```bash
cd frontend
npm install
npm run dev       # Vite en :5173, apunta a http://localhost:8080
```

Variables:
- `VITE_API_BASE` (build/dev) — base URL del backend. Default `http://localhost:8080` para dev, `/api` para el build de producción.

## Correr con Docker (local)

```bash
docker compose up --build
# frontend en http://localhost:3030
```

El compose monta `./data` como volumen para persistir `students.db`.

## Despliegue en producción

El servicio está registrado en `/opt/autodeploy/services.conf`. Cada commit a `main` redespliega automáticamente vía el cron de `deploy` cada 10 min.

URL: `https://utec.tumvp.uy`
- `/` → SPA React
- `/api/students*` → backend C (proxied)

Forzar redeploy manual:
```bash
sudo -u deploy /opt/autodeploy/autodeploy.sh utec
```

## API

Base: `http://localhost:8080` (dev) o `https://utec.tumvp.uy/api` (prod).

| Método | Path | Descripción |
|---|---|---|
| GET | `/students` | Lista todos |
| GET | `/students/{id}` | Obtiene uno |
| POST | `/students` | Crea (201) |
| PUT | `/students/{id}` | Actualiza (200) |
| DELETE | `/students/{id}` | Elimina (204) |

Errores: `{"error": "mensaje"}` con status 400 (validación), 404 (no encontrado), 409 (`id_doc` duplicada).

Validaciones:
- `name`: 1–100 caracteres
- `id_doc`: 7–10 dígitos, único
- `email`: contiene `@` y `.` después del `@`, máx. 120

CORS abierto (`*`), preflight `OPTIONS` responde 204.

## Build limpio

```bash
cd backend && make clean
cd frontend && rm -rf node_modules dist
```
