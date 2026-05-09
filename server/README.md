# Rover dev + telemetry server

A small Python (stdlib only) HTTP server that:

1. Serves the static `web/` app (replaces `python -m http.server`).
2. Accepts session telemetry from the browser and persists it to SQLite.

The browser-side `web/js/logger.js` opens a session per page load,
records every interesting event (manual inputs, autonomous decisions,
vision samples, motor commands, pivot outcomes), and ships them in
~3-second batches. SQLite + WAL means you can query the database while
the rover is running.

---

## Run

```bash
./web/serve.sh
# or directly:
python3 server/app.py --port 8000
```

Open `http://localhost:8000/`. Drive the rover. Each session is rowed
into `rover-logs.db` (created on first run, in the repo root).

---

## API

| Method | Path | Notes |
|---|---|---|
| `POST` | `/api/sessions` | body `{rover_host, rover_name?, user_agent?, metadata?}` → `{id}` |
| `POST` | `/api/sessions/<id>/events` | body: array of `{t_ms, type, payload}` |
| `POST` | `/api/sessions/<id>/end` | mark ended |
| `GET`  | `/api/sessions` | list latest 200 sessions w/ event counts |
| `GET`  | `/api/sessions/<id>/events` | `?type=<t>&limit=<n>&since_t_ms=<n>` |
| `POST` | `/api/sessions/<id>/snapshots` | `?t_ms=<n>&type=<t>`, body: image/jpeg |
| `GET`  | `/api/sessions/<id>/snapshots` | `?type=<t>` → `[{id, t_ms, type, bytes, url}]` |
| `GET`  | `/snapshots/<rel_path>` | serves the JPEG file |
| `GET`  | `/api/health` | `{ok: true, db: ...}` |

The browser logger uses the first three; the latter two are for
analysis tooling.

---

## Schema

```sql
CREATE TABLE sessions (
  id            INTEGER PRIMARY KEY,
  rover_host    TEXT,
  rover_name    TEXT,
  user_agent    TEXT,
  metadata      TEXT,           -- JSON
  started_at_ms INTEGER,        -- wall-clock unix ms
  ended_at_ms   INTEGER         -- NULL while in progress
);

CREATE TABLE events (
  id         INTEGER PRIMARY KEY,
  session_id INTEGER REFERENCES sessions(id),
  t_ms       INTEGER,           -- ms since session.started_at_ms
  type       TEXT,
  payload    TEXT               -- JSON, type-specific
);

CREATE TABLE snapshots (
  id         INTEGER PRIMARY KEY,
  session_id INTEGER REFERENCES sessions(id),
  t_ms       INTEGER,           -- aligned with the matching event row
  type       TEXT,              -- e.g. 'pivot_start', 'pivot_end'
  bytes      INTEGER,
  rel_path   TEXT               -- under rover-snapshots/
);
```

Snapshots are JOINed with events on `(session_id, t_ms, type)`:

```sql
SELECT e.t_ms, e.type, e.payload, s.url
FROM events e
LEFT JOIN snapshots s
  ON s.session_id = e.session_id AND s.t_ms = e.t_ms AND s.type = e.type
WHERE e.session_id = ? AND e.type IN ('pivot_start','pivot_end')
ORDER BY e.t_ms;
```

### Event types

| `type` | `payload` shape | When |
|---|---|---|
| `ws`           | `{state: "open"\|"close"}` | WS connection lifecycle |
| `auto`         | `{state: "start"\|"stop"\|"cancel"}` | Auto-mode lifecycle |
| `tick`         | `{distance, decision: "drive"\|"pivot", pwm?}` | Per autonomous tick |
| `pivot_start`  | `{entryDistance, visionHint, side, thirds}` | Pivot decision |
| `pivot_end`    | `{entryDistance, outcome: "ok"\|"gave_up"}` | Pivot result |
| `vision`       | `{left, center, right}` | Camera frame analysis |
| `input`        | `{held, speed, cmd}` | User input → motor commands |

---

## Querying the data

Use any SQLite client. SQL examples to bootstrap analysis:

### Session list
```sql
SELECT id, rover_host, started_at_ms,
       (ended_at_ms - started_at_ms) / 1000 AS duration_s,
       (SELECT COUNT(*) FROM events e WHERE e.session_id = s.id) AS events
FROM sessions s ORDER BY id DESC LIMIT 20;
```

### Pivot success rate by entry-distance bucket
```sql
SELECT
  CAST(json_extract(p1.payload, '$.entryDistance') / 5 AS INT) * 5 AS bucket_cm,
  COUNT(*) AS attempts,
  SUM(CASE WHEN json_extract(p2.payload, '$.outcome') = 'ok' THEN 1 ELSE 0 END) AS escaped,
  ROUND(100.0 * SUM(CASE WHEN json_extract(p2.payload, '$.outcome') = 'ok' THEN 1 ELSE 0 END) / COUNT(*), 1) AS pct
FROM events p1
JOIN events p2 ON p2.session_id = p1.session_id AND p2.t_ms > p1.t_ms
WHERE p1.type = 'pivot_start' AND p2.type = 'pivot_end'
  AND NOT EXISTS (
    SELECT 1 FROM events m WHERE m.session_id = p1.session_id
      AND m.type = 'pivot_end' AND m.t_ms > p1.t_ms AND m.t_ms < p2.t_ms
  )
GROUP BY bucket_cm ORDER BY bucket_cm;
```

### Vision-hint accuracy — does the picked side correlate with success?
```sql
SELECT
  json_extract(s.payload, '$.visionHint') AS hint,
  json_extract(s.payload, '$.side')        AS picked_side,
  COUNT(*)                                 AS attempts,
  SUM(CASE WHEN json_extract(e.payload, '$.outcome') = 'ok' THEN 1 ELSE 0 END) AS escaped
FROM events s
JOIN events e ON e.session_id = s.session_id AND e.t_ms > s.t_ms AND e.type = 'pivot_end'
WHERE s.type = 'pivot_start'
GROUP BY hint, picked_side
ORDER BY attempts DESC;
```

### Distance distribution per session (CSV-style for plotting)
```sql
SELECT t_ms, json_extract(payload, '$.distance') AS distance_cm
FROM events
WHERE session_id = ?            -- pick one session
  AND type = 'tick'
ORDER BY t_ms;
```

### Find oscillation patterns — same entry distance ±2 cm within 10 s
```sql
SELECT a.session_id, a.t_ms AS first, b.t_ms AS second,
       (b.t_ms - a.t_ms) AS gap_ms,
       json_extract(a.payload, '$.entryDistance') AS entry
FROM events a JOIN events b
  ON b.session_id = a.session_id
 AND b.type = 'pivot_start'
 AND b.t_ms > a.t_ms
 AND b.t_ms - a.t_ms < 10000
 AND ABS(CAST(json_extract(a.payload, '$.entryDistance') AS REAL) -
         CAST(json_extract(b.payload, '$.entryDistance') AS REAL)) < 2
WHERE a.type = 'pivot_start'
ORDER BY a.session_id, a.t_ms LIMIT 50;
```

---

## Pulling sessions into a notebook

```python
import sqlite3, json, pandas as pd

conn = sqlite3.connect('rover-logs.db')
events = pd.read_sql_query(
    "SELECT * FROM events WHERE session_id = ?",
    conn, params=(LATEST_SESSION_ID,))
events['payload'] = events['payload'].apply(json.loads)
ticks = events[events.type == 'tick']
ticks['distance_cm'] = ticks.payload.apply(lambda p: p.get('distance'))
ticks.plot(x='t_ms', y='distance_cm')
```

---

## Privacy / footprint

- DB file lives in the **repo root** (`rover-logs.db`); gitignored.
- Camera snapshots are stored in `rover-snapshots/<session_id>/<sha>.jpg`
  (also gitignored). One snapshot fires per `pivot_start` and `pivot_end`
  — typical session: 10–50 snapshots, ~10 KB each → under 1 MB per session.
- Vision-analysis frames (the small 80×60 used for L/C/R clutter) are
  **not** stored — only their numeric scores.
- One typical autonomous-mode session emits ~5–15 events/sec → roughly
  1–3 MB of event JSON per hour of driving.
- WAL mode (default): you can `sqlite3 rover-logs.db` and run queries
  while the page is logging without blocking either side.

---

## Resetting

Stop the server, then:

```bash
rm -f rover-logs.db rover-logs.db-shm rover-logs.db-wal
rm -rf rover-snapshots/
```

Next launch will re-create an empty schema and snapshots dir.
