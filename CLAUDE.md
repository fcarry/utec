# utec — CRUD de Estudiantes

PRD canónico: [/home/ubuntu/claude/utec/prd0.md](../../home/ubuntu/claude/utec/prd0.md) (en el server: `/home/ubuntu/claude/utec/prd0.md`).

Aplicación CRUD didáctica para el curso **Principios de Programación** (UTEC). El alumno toca un backend en C real (libmicrohttpd + SQLite + cJSON) y un frontend React/Vite mínimo, viendo el contrato HTTP/JSON entre ambos. Sin autenticación, sin paginación, sin búsqueda — sólo lo que pide el PRD.

## Estado

**Desplegado** en `https://utec.tumvp.uy` desde 2026-05-16 (commit `db8aeeb`). 2 containers Docker corriendo, integrado al autodeploy. Cert LE expira 2026-08-14 (autorrenueva).

PRD §9 (criterios de aceptación) cumplido end-to-end:
- `GET /students` con DB vacía devuelve `[]`.
- Alta / listado / edición / baja funcionan desde la UI sin recargar.
- Cédula duplicada → 409 + mensaje "Ya existe un estudiante con esa cédula".
- Validaciones de email y cédula en front y back (back sigue siendo la fuente de verdad).
- Mutex en `db.c` serializa el acceso a SQLite — soporta requests concurrentes.
- DB persiste entre reinicios (volumen `./data:/app/data`).

## Arquitectura

```
host:443 (nginx host)
  └── 127.0.0.1:3030 → utec-frontend (nginx:alpine)
        ├── /        → SPA estática React (dist/)
        └── /api/    → rewrite /api/(.*) → /$1, proxy_pass http://backend:8080
              └── utec-backend (Debian + ./server)
                    └── /app/data/students.db (SQLite, persistido en host)
```

- 2 containers en la red `utec_default` de docker compose (puente interno).
- Sólo el frontend expone puerto al host (`127.0.0.1:3030`). El backend queda en la red interna; el frontend nginx hace el `proxy_pass http://backend:8080` por nombre DNS de compose.
- El vhost host (`/etc/nginx/sites-enabled/utec.tumvp.uy.conf`) hace HTTPS + reverse proxy → `127.0.0.1:3030`. Es decir: TLS termina en el host, no en el container.

## Backend — C

| Aspecto | Detalle |
|---|---|
| Lenguaje | C11 (`-Wall -Wextra -O2`) |
| Server HTTP | libmicrohttpd con `MHD_USE_INTERNAL_POLLING_THREAD` |
| DB | SQLite 3 (un solo archivo, mutex `pthread_mutex_t` en `db.c`) |
| JSON | cJSON |
| Build | `Makefile` simple, salida `./server` |
| Puerto | `8080` (override CLI: `./server 9000`) |
| DB path | env `STUDENTS_DB` (default `students.db`; en container `/app/data/students.db`) |
| Logging | stdout, 1 línea por request: `<ISO ts> <method> <url> <status>` |

### Estructura
```
backend/
├── Makefile
├── Dockerfile          # multi-stage: build (gcc + libs -dev) → runtime (libs runtime + useradd app)
└── src/
    ├── main.c          # MHD_start_daemon + signal handlers + access_handler + log_request
    ├── router.c/.h     # despacho por método+path; parse_id; preflight OPTIONS
    ├── handlers.c/.h   # un handler por endpoint, parsea body, mapea códigos DB→HTTP
    ├── db.c/.h         # wrappers sqlite3 + mutex global (prepare/bind/step/finalize en cada op)
    ├── student.c/.h    # struct Student + JSON in/out + validaciones (name 1-100, id_doc 7-10 dígitos, email @ + . post @)
    └── http_util.c/.h  # BodyBuf, send_json, send_error, send_no_content, send_cors_preflight
```

### Códigos de retorno DB
- `DB_OK = 0`, `DB_ERR = -1`, `DB_NOT_FOUND = -2`, `DB_CONFLICT = -3`.
- Mapeo en `handlers.c`: NOT_FOUND→404, CONFLICT→409, ERR→500.

### Endpoints
| Método | Path | Status OK | Notas |
|---|---|---|---|
| GET | `/students` | 200 | Array completo, orden por `id` ASC |
| GET | `/students/{id}` | 200 / 404 | |
| POST | `/students` | 201 / 400 / 409 | Devuelve el objeto con `id` asignado |
| PUT | `/students/{id}` | 200 / 400 / 404 / 409 | Reemplazo total |
| DELETE | `/students/{id}` | 204 / 404 | Sin body |
| OPTIONS | `*` | 204 | Preflight CORS |

Errores: `{"error": "mensaje"}` con header `Access-Control-Allow-Origin: *` siempre.

## Frontend — React 18 + Vite

| Aspecto | Detalle |
|---|---|
| Stack | React 18.3 + Vite 5 (`@vitejs/plugin-react`) |
| Estado | `useState` / `useEffect` — sin Redux/Zustand |
| HTTP | `fetch` nativo, sin axios |
| Estilos | `app.css` plano, sin Tailwind |
| Base API | `VITE_API_BASE` env (dev: `http://localhost:8080`; build prod: `/api`) |

### Estructura
```
frontend/
├── package.json
├── vite.config.js          # plugin react + host 0.0.0.0
├── nginx.conf              # SPA fallback try_files + /api/ rewrite + proxy_pass http://backend:8080
├── Dockerfile              # multi-stage: node:20-alpine npm run build (VITE_API_BASE=/api) → nginx:alpine
├── index.html
└── src/
    ├── main.jsx            # createRoot
    ├── App.jsx             # state, refresh, openCreate/openEdit, askDelete/confirmDelete
    ├── api.js              # listStudents/createStudent/updateStudent/deleteStudent + parseJsonOrError
    ├── app.css
    └── components/
        ├── StudentsGrid.jsx     # tabla + botones ✎ 🗑
        ├── StudentForm.jsx      # modal alta/edición + validación inline
        └── ConfirmDialog.jsx    # diálogo "¿Eliminar a X?"
```

### Manejo de errores en el front
- 409 en POST/PUT → "Ya existe un estudiante con esa cédula".
- 400 → muestra el `error` del backend.
- `Failed to fetch` (red) → banner "No se puede conectar al servidor".

### Build de producción
`VITE_API_BASE=/api` se setea en el Dockerfile del frontend, así que el bundle compilado apunta a rutas relativas `/api/students`. El nginx del container hace `rewrite ^/api/(.*)$ /$1 break;` antes del `proxy_pass http://backend:8080` (necesario porque `proxy_pass` con un literal sin `/` final NO strippea el prefix automáticamente cuando hay `rewrite` involucrado).

## Docker

- `docker-compose.yml` y `docker-compose.prod.yml` son idénticos (un solo entorno).
- `backend`: build local, `expose: 8080`, volumen `./data:/app/data`. **No publica puerto al host.**
- `frontend`: build local, `depends_on: backend`, publica `127.0.0.1:3030:80`.
- Healthcheck: ninguno configurado (PRD no lo pide; el `restart: unless-stopped` cubre el caso de crash).

### Backend Dockerfile — observaciones
- Multi-stage: stage `build` con `debian:bookworm-slim` + `gcc make libc6-dev libmicrohttpd-dev libsqlite3-dev libcjson-dev`, stage runtime con sólo las libs runtime (`libmicrohttpd12 libsqlite3-0 libcjson1`) + un usuario `app` (uid 1001).
- `STUDENTS_DB=/app/data/students.db` en runtime. Si rompés permisos del volumen: `sudo chown -R 1001:1001 /opt/repos/utec/data`.

## Nginx (host)

`/etc/nginx/sites-enabled/utec.tumvp.uy.conf` (sincronizado por el autodeploy desde `/opt/repos/utec/nginx/utec.tumvp.uy.conf`):
- :80 → 301 a HTTPS, con `/.well-known/acme-challenge/` rooteado en `/var/www/html` para que certbot renueve.
- :443 con HTTP/2, HSTS, X-Content-Type-Options, X-Frame-Options SAMEORIGIN, Referrer-Policy.
- `proxy_pass http://127.0.0.1:3030` con headers estándar (`Host`, `X-Real-IP`, `X-Forwarded-For`, `X-Forwarded-Proto`).
- `client_max_body_size 2M` (un POST de student ronda los ~150 bytes; cualquier valor razonable alcanza).

## Autodeploy

Registrado en `/opt/autodeploy/services.conf`:
```
utec|git@github.com:fcarry/utec.git|main|false|docker-compose.prod.yml|true
```
- Rama: `main`
- `HAS_MIGRATIONS=false` (SQLite + DDL `IF NOT EXISTS` desde `db.c`)
- `SKIP_BUILD=true` — el compose construye con `--build`, no se hace `docker build` aparte.

Forzar redeploy: `sudo -u deploy /opt/autodeploy/autodeploy.sh utec`.

## Operaciones comunes

```bash
# Smoke
curl -s https://utec.tumvp.uy/api/students | jq .
curl -s -X POST https://utec.tumvp.uy/api/students \
  -H 'Content-Type: application/json' \
  -d '{"name":"Ana","id_doc":"12345678","email":"ana@example.com"}'

# Logs
sudo docker logs --tail 100 utec-backend
sudo docker logs --tail 100 utec-frontend

# Inspeccionar DB desde el host (archivo SQLite local)
sudo sqlite3 /opt/repos/utec/data/students.db "SELECT * FROM students;"

# Forzar recreate sin rebuild
sudo -u deploy docker compose -f /opt/repos/utec/docker-compose.prod.yml \
  up -d --force-recreate backend frontend

# Build/run local sin Docker
cd /opt/repos/utec/backend && make && ./server
cd /opt/repos/utec/frontend && npm install && npm run dev
```

## Decisiones que se apartan del PRD

- `OPTIONS *` siempre devuelve 204 — el PRD lo pide en preflight, pero acá se generaliza para cualquier OPTIONS.
- `nginx.conf` del frontend container hace `rewrite ^/api/(.*)$ /$1 break;` además del `proxy_pass`. Sin el rewrite, la ruta `/api/students` llegaría al backend como `/api/students` (no `/students`) y devolvería 404.
- `VITE_API_BASE=/api` en build de prod → la SPA no llama nunca a `http://localhost:8080` ni necesita CORS cross-origin en producción (porque sale del mismo origen via nginx). El CORS abierto del backend queda igual para el modo dev (Vite en :5173 → backend en :8080).
- `STUDENTS_DB` env var permite mover la DB sin recompilar. No estaba en el PRD pero es práctico para tests/dev.
- DB en `./data/students.db` montado como volumen, no en `backend/students.db` como sugiere el PRD §10. El `.gitignore` cubre ambos.

## Lo que NO está implementado (fuera de alcance del PRD)

- Autenticación / autorización / roles.
- Paginación, búsqueda, ordenamiento por columna.
- Importación/exportación CSV/Excel.
- Tests automatizados (ni unit, ni integration). El PRD §9 son criterios de aceptación manuales.
- Healthchecks docker.
- Métricas / observability.

## Referencias

- PRD canónico: `/home/ubuntu/claude/utec/prd0.md`
- README del repo: `/opt/repos/utec/README.md`
- Repo GitHub: `git@github.com:fcarry/utec.git` (cuenta `fcarry`)
- Infra global del server (puertos, dominios, autodeploy): `/home/ubuntu/claude/CLAUDE.md`
