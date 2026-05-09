#!/usr/bin/env python3
"""
Local rover dev/log server.

Serves the static `web/` directory (replacing the plain
`python -m http.server`) AND exposes a tiny REST API the browser uses
to record per-session telemetry into a SQLite database.

Stdlib only — no pip install. Designed to be the long-running
collector behind autonomous-mode improvements: every tick (distance,
vision thirds, motor commands, decisions) is captured and queryable
later via SQL or any notebook.

Endpoints
=========
GET  /                              static index.html
GET  /<path>                        any file under web/
POST /api/sessions                  body: {rover_host, rover_name?, user_agent?, metadata?}
                                    → {id}
POST /api/sessions/<id>/events      body: [{t_ms, type, payload}, ...]
                                    → 204
POST /api/sessions/<id>/end         → 204
GET  /api/sessions                  → [{id, rover_host, rover_name, started_at_ms,
                                        ended_at_ms, event_count}, ...]
GET  /api/sessions/<id>/events      ?type=<type>&limit=<n>&since_t_ms=<n>
                                    → [{t_ms, type, payload}, ...]
GET  /api/health                    → {ok: true, db: <path>}

Run
===
  python3 server/app.py [--port 8000] [--web-dir web] [--db rover-logs.db]

Environment overrides: PORT, ROVER_DB_PATH, ROVER_WEB_DIR.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import sqlite3
import sys
import threading
import time
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import parse_qs, urlsplit

SCHEMA = """
CREATE TABLE IF NOT EXISTS sessions (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    rover_host    TEXT NOT NULL,
    rover_name    TEXT,
    user_agent    TEXT,
    metadata      TEXT,           -- JSON blob, free-form
    started_at_ms INTEGER NOT NULL,
    ended_at_ms   INTEGER
);

CREATE TABLE IF NOT EXISTS events (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id INTEGER NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,
    t_ms       INTEGER NOT NULL,    -- ms since session.started_at_ms
    type       TEXT NOT NULL,       -- 'tick'|'pivot'|'input'|'auto'|...
    payload    TEXT NOT NULL        -- JSON blob, type-specific
);

-- Camera frames captured at the time an event fired. JOIN with events
-- on (session_id, t_ms, type) to correlate. We don't FK to events.id
-- because event inserts are batched (POST /events) while snapshot
-- inserts are individual (POST /snapshots), and locking the two
-- together would slow logging without buying much.
CREATE TABLE IF NOT EXISTS snapshots (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id INTEGER NOT NULL REFERENCES sessions(id) ON DELETE CASCADE,
    t_ms       INTEGER NOT NULL,
    type       TEXT NOT NULL,
    bytes      INTEGER NOT NULL,
    rel_path   TEXT NOT NULL        -- '<session_id>/<sha>.jpg', under SNAPSHOTS_DIR
);

CREATE INDEX IF NOT EXISTS ix_events_session_t   ON events(session_id, t_ms);
CREATE INDEX IF NOT EXISTS ix_events_type        ON events(type);
CREATE INDEX IF NOT EXISTS ix_snapshots_session  ON snapshots(session_id, t_ms);
CREATE INDEX IF NOT EXISTS ix_snapshots_type     ON snapshots(type);
"""


# ---------------------------------------------------------------------------
# DB helpers — one connection per thread, reused across requests on that
# thread. SQLite + WAL is enough for one concurrent writer (the page) and
# many readers (analysis notebooks).
# ---------------------------------------------------------------------------

_thread_local = threading.local()


def db_path() -> str:
    return getattr(_thread_local, "db_path", "rover-logs.db")


def set_db_path(p: str) -> None:
    _thread_local.db_path = p


def db() -> sqlite3.Connection:
    conn = getattr(_thread_local, "conn", None)
    if conn is None:
        conn = sqlite3.connect(db_path(), isolation_level=None)  # autocommit
        conn.execute("PRAGMA journal_mode=WAL")
        conn.execute("PRAGMA synchronous=NORMAL")
        conn.execute("PRAGMA foreign_keys=ON")
        conn.row_factory = sqlite3.Row
        _thread_local.conn = conn
    return conn


def init_db(path: str) -> None:
    """Create schema if missing. Called once at startup."""
    conn = sqlite3.connect(path)
    try:
        conn.executescript(SCHEMA)
        conn.commit()
    finally:
        conn.close()


# ---------------------------------------------------------------------------
# HTTP handler
# ---------------------------------------------------------------------------


class Handler(SimpleHTTPRequestHandler):
    # Subclass-level config — set by run() before serve_forever().
    web_dir: str = "web"
    db_file: str = "rover-logs.db"
    snapshots_dir: str = "rover-snapshots"

    def __init__(self, *args, **kwargs):
        # Serve static files from web_dir.
        super().__init__(*args, directory=self.web_dir, **kwargs)

    # --- routing ----------------------------------------------------------

    def do_GET(self):
        path = urlsplit(self.path).path
        if path == "/api/health":
            return self._json(200, {"ok": True, "db": self.db_file})
        if path == "/api/sessions":
            return self._list_sessions()
        if path.startswith("/api/sessions/") and path.endswith("/events"):
            return self._list_events(self._parse_session_id(path, suffix="/events"))
        if path.startswith("/api/sessions/") and path.endswith("/snapshots"):
            return self._list_snapshots(self._parse_session_id(path, suffix="/snapshots"))
        if path.startswith("/snapshots/"):
            return self._serve_snapshot(path[len("/snapshots/"):])
        # Fall through to static file serving.
        super().do_GET()

    def do_POST(self):
        path = urlsplit(self.path).path
        if path == "/api/sessions":
            return self._create_session()
        if path.startswith("/api/sessions/") and path.endswith("/events"):
            return self._append_events(self._parse_session_id(path, suffix="/events"))
        if path.startswith("/api/sessions/") and path.endswith("/snapshots"):
            return self._upload_snapshot(self._parse_session_id(path, suffix="/snapshots"))
        if path.startswith("/api/sessions/") and path.endswith("/end"):
            return self._end_session(self._parse_session_id(path, suffix="/end"))
        self._json(404, {"error": "unknown route"})

    # --- handlers ---------------------------------------------------------

    def _create_session(self):
        body = self._read_json()
        if not isinstance(body, dict) or not body.get("rover_host"):
            return self._json(400, {"error": "rover_host required"})
        set_db_path(self.db_file)
        cur = db().execute(
            "INSERT INTO sessions(rover_host, rover_name, user_agent, metadata, started_at_ms) "
            "VALUES (?, ?, ?, ?, ?)",
            (
                str(body["rover_host"])[:255],
                _maybe_str(body.get("rover_name")),
                _maybe_str(body.get("user_agent"), 511),
                _dump_or_none(body.get("metadata")),
                int(time.time() * 1000),
            ),
        )
        return self._json(200, {"id": cur.lastrowid})

    def _append_events(self, session_id: int | None):
        if session_id is None:
            return self._json(400, {"error": "bad session id"})
        body = self._read_json()
        if not isinstance(body, list):
            return self._json(400, {"error": "expected JSON array of events"})
        set_db_path(self.db_file)
        rows = []
        for e in body:
            if not isinstance(e, dict):
                continue
            t_ms = int(e.get("t_ms") or 0)
            ev_type = str(e.get("type") or "")[:64]
            payload = e.get("payload")
            if not ev_type:
                continue
            rows.append((session_id, t_ms, ev_type, json.dumps(payload, default=str)))
        if rows:
            db().executemany(
                "INSERT INTO events(session_id, t_ms, type, payload) VALUES (?, ?, ?, ?)",
                rows,
            )
        return self._no_content()

    def _end_session(self, session_id: int | None):
        if session_id is None:
            return self._json(400, {"error": "bad session id"})
        set_db_path(self.db_file)
        db().execute(
            "UPDATE sessions SET ended_at_ms = ? WHERE id = ? AND ended_at_ms IS NULL",
            (int(time.time() * 1000), session_id),
        )
        return self._no_content()

    def _list_sessions(self):
        set_db_path(self.db_file)
        rows = db().execute(
            """
            SELECT s.id, s.rover_host, s.rover_name, s.started_at_ms, s.ended_at_ms,
                   (SELECT COUNT(*) FROM events e WHERE e.session_id = s.id) AS event_count
            FROM sessions s
            ORDER BY s.id DESC
            LIMIT 200
            """
        ).fetchall()
        return self._json(200, [dict(r) for r in rows])

    def _upload_snapshot(self, session_id: int | None):
        if session_id is None:
            return self._json(400, {"error": "bad session id"})
        qs = parse_qs(urlsplit(self.path).query)
        try:
            t_ms = int((qs.get("t_ms") or ["0"])[0])
        except ValueError:
            return self._json(400, {"error": "bad t_ms"})
        ev_type = (qs.get("type") or [""])[0][:64]
        if not ev_type:
            return self._json(400, {"error": "type required"})

        ctype = (self.headers.get("Content-Type") or "").lower()
        if "image/jpeg" not in ctype and "image/jpg" not in ctype:
            return self._json(415, {"error": "expected image/jpeg"})
        length = int(self.headers.get("Content-Length") or 0)
        if length <= 0 or length > 4 * 1024 * 1024:
            return self._json(413, {"error": "bad size"})

        data = self.rfile.read(length)
        # Hash → stable filename (de-dupes if the same frame arrives twice).
        sha = hashlib.sha1(data).hexdigest()[:16]
        rel = f"{session_id}/{sha}.jpg"
        target = Path(self.snapshots_dir) / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists():
            target.write_bytes(data)

        set_db_path(self.db_file)
        cur = db().execute(
            "INSERT INTO snapshots(session_id, t_ms, type, bytes, rel_path) "
            "VALUES (?, ?, ?, ?, ?)",
            (session_id, t_ms, ev_type, len(data), rel),
        )
        return self._json(200, {"id": cur.lastrowid, "rel_path": rel, "bytes": len(data)})

    def _list_snapshots(self, session_id: int | None):
        if session_id is None:
            return self._json(400, {"error": "bad session id"})
        qs = parse_qs(urlsplit(self.path).query)
        ev_type = (qs.get("type") or [None])[0]
        set_db_path(self.db_file)
        if ev_type:
            rows = db().execute(
                "SELECT id, t_ms, type, bytes, rel_path FROM snapshots "
                "WHERE session_id = ? AND type = ? ORDER BY t_ms",
                (session_id, ev_type),
            ).fetchall()
        else:
            rows = db().execute(
                "SELECT id, t_ms, type, bytes, rel_path FROM snapshots "
                "WHERE session_id = ? ORDER BY t_ms",
                (session_id,),
            ).fetchall()
        return self._json(
            200,
            [
                {
                    "id": r["id"], "t_ms": r["t_ms"], "type": r["type"],
                    "bytes": r["bytes"], "url": f"/snapshots/{r['rel_path']}",
                }
                for r in rows
            ],
        )

    def _serve_snapshot(self, rel: str):
        # Path safety: refuse any traversal up out of the snapshots dir.
        rel = rel.lstrip("/")
        candidate = (Path(self.snapshots_dir) / rel).resolve()
        root = Path(self.snapshots_dir).resolve()
        try:
            candidate.relative_to(root)
        except ValueError:
            return self._json(403, {"error": "forbidden"})
        if not candidate.is_file():
            return self._json(404, {"error": "not found"})
        data = candidate.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", "image/jpeg")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "max-age=86400")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data)

    def _list_events(self, session_id: int | None):
        if session_id is None:
            return self._json(400, {"error": "bad session id"})
        qs = parse_qs(urlsplit(self.path).query)
        ev_type = (qs.get("type") or [None])[0]
        limit = min(int((qs.get("limit") or ["10000"])[0]), 100000)
        since = int((qs.get("since_t_ms") or ["0"])[0])
        set_db_path(self.db_file)
        if ev_type:
            rows = db().execute(
                "SELECT t_ms, type, payload FROM events "
                "WHERE session_id = ? AND type = ? AND t_ms >= ? "
                "ORDER BY t_ms LIMIT ?",
                (session_id, ev_type, since, limit),
            ).fetchall()
        else:
            rows = db().execute(
                "SELECT t_ms, type, payload FROM events "
                "WHERE session_id = ? AND t_ms >= ? ORDER BY t_ms LIMIT ?",
                (session_id, since, limit),
            ).fetchall()
        return self._json(
            200,
            [
                {"t_ms": r["t_ms"], "type": r["type"], "payload": json.loads(r["payload"])}
                for r in rows
            ],
        )

    # --- helpers ----------------------------------------------------------

    def _parse_session_id(self, path: str, *, suffix: str) -> int | None:
        # path looks like /api/sessions/<id><suffix>
        head = "/api/sessions/"
        if not path.startswith(head) or not path.endswith(suffix):
            return None
        mid = path[len(head): -len(suffix)]
        try:
            return int(mid)
        except ValueError:
            return None

    def _read_json(self) -> Any:
        length = int(self.headers.get("Content-Length") or 0)
        if length <= 0 or length > 8 * 1024 * 1024:
            return None
        try:
            return json.loads(self.rfile.read(length).decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            return None

    def _json(self, status: int, payload: Any):
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        # Allow the page (served from same origin) to call /api/* freely.
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()
        self.wfile.write(body)

    def _no_content(self):
        self.send_response(HTTPStatus.NO_CONTENT)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()

    def do_OPTIONS(self):
        self._no_content()

    # Quieter logs — Python's default per-request log is noisy with the
    # logger flushing every few seconds.
    def log_message(self, fmt: str, *args) -> None:  # noqa: A003
        path = urlsplit(self.path).path
        if path.startswith("/api/sessions/") and path.endswith("/events"):
            return
        super().log_message(fmt, *args)


def _maybe_str(v: Any, max_len: int = 255) -> str | None:
    if v is None:
        return None
    return str(v)[:max_len]


def _dump_or_none(v: Any) -> str | None:
    if v is None:
        return None
    try:
        return json.dumps(v)
    except (TypeError, ValueError):
        return None


# ---------------------------------------------------------------------------
# Entrypoint
# ---------------------------------------------------------------------------


def run(port: int, web_dir: str, db_file: str, snapshots_dir: str) -> None:
    init_db(db_file)
    Path(snapshots_dir).mkdir(parents=True, exist_ok=True)
    Handler.web_dir = web_dir
    Handler.db_file = db_file
    Handler.snapshots_dir = snapshots_dir
    server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    print(f"rover server: http://127.0.0.1:{port}/")
    print(f"  serving   {web_dir}/")
    print(f"  logging   {db_file}")
    print(f"  snapshots {snapshots_dir}/")
    print("Press Ctrl-C to stop.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[server] shutting down")
    finally:
        server.server_close()


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="Rover web + log server")
    p.add_argument(
        "--port",
        type=int,
        default=int(os.environ.get("PORT", 8000)),
        help="listen port (default 8000)",
    )
    p.add_argument(
        "--web-dir",
        default=os.environ.get("ROVER_WEB_DIR", "web"),
        help="static directory to serve (default ./web)",
    )
    p.add_argument(
        "--db",
        default=os.environ.get("ROVER_DB_PATH", "rover-logs.db"),
        help="SQLite database path (default ./rover-logs.db)",
    )
    p.add_argument(
        "--snapshots-dir",
        default=os.environ.get("ROVER_SNAPSHOTS_DIR", "rover-snapshots"),
        help="directory for camera snapshots (default ./rover-snapshots)",
    )
    args = p.parse_args(argv)

    # Resolve paths relative to repo root (one level up from this file).
    here = Path(__file__).resolve().parent
    def _abs(p_str: str) -> Path:
        return (here / ".." / p_str).resolve() if not Path(p_str).is_absolute() else Path(p_str)
    web_dir       = _abs(args.web_dir)
    db_file       = _abs(args.db)
    snapshots_dir = _abs(args.snapshots_dir)
    if not web_dir.exists():
        print(f"error: web dir not found: {web_dir}", file=sys.stderr)
        return 2

    run(args.port, str(web_dir), str(db_file), str(snapshots_dir))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
