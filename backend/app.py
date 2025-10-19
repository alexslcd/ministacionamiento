from flask import Flask, render_template, request, jsonify, make_response, abort
from datetime import datetime, UTC
import os

app = Flask(__name__)

API_TOKEN = os.environ.get("API_TOKEN", "")               # igual que antes
ALLOW_SIM = os.environ.get("ALLOW_SIM", "false").lower()  # demo opcional

# Helper para timestamp UTC ISO 8601
def now_utc_iso():
    return datetime.now(UTC).replace(microsecond=0).isoformat()

# Configura aquí la "capacidad" o la lista de puestos iniciales:
DEFAULT_SLOTS = ["S1", "S2", "S3", "S4", "S5", "S6"]

STATE = {
    "motor_desired": "off",                 # on | off
    "updated_at": now_utc_iso(),
    "slots": {s: 0 for s in DEFAULT_SLOTS}, # 0 libre, 1 ocupado
    "capacity": len(DEFAULT_SLOTS),
}

def require_token():
    token = request.headers.get("X-API-Key", "")
    if API_TOKEN and token != API_TOKEN:
        abort(401)

def _summary():
    occupied = sum(int(v) for v in STATE["slots"].values())
    free = STATE["capacity"] - occupied
    return occupied, free

@app.route("/")
def index():
    # La página no necesita saber ALLOW_SIM; si lo habilitas, podrás usar el endpoint de sim
    return render_template("index.html")

@app.get("/api/status")
def api_status():
    occupied, free = _summary()
    return jsonify({
        "motor_desired": STATE["motor_desired"],
        "updated_at": STATE["updated_at"],
        "slots": STATE["slots"],
        "capacity": STATE["capacity"],
        "occupied": occupied,
        "free": free
    })

@app.get("/api/cmd")
def api_cmd():
    require_token()
    return jsonify({"motor_desired": STATE["motor_desired"]})

@app.post("/api/motor")
def api_motor():
    data = request.get_json(silent=True) or {}
    st = str(data.get("state", "")).lower()
    if st not in ("on", "off"):
        return make_response({"error": "usa on|off"}, 400)
    STATE["motor_desired"] = st
    STATE["updated_at"] = now_utc_iso()
    return make_response(f"OK MOTOR {st.upper()}", 200)

@app.post("/api/telemetry")
def api_telemetry():
    # Usado por ESP32 (con token)
    require_token()
    data = request.get_json(silent=True) or {}
    prox = 1 if int(data.get("prox", 0)) else 0
    slot = str(data.get("slot", "S1"))  # por compat: si no envían slot, usa S1
    if slot not in STATE["slots"]:
        # si llega un slot nuevo, lo agregamos y ajustamos capacidad
        STATE["slots"][slot] = 0
        STATE["capacity"] = len(STATE["slots"])
    STATE["slots"][slot] = prox
    STATE["updated_at"] = now_utc_iso()
    return make_response({"ok": True}, 200)

# (OPCIONAL) Endpoint de simulación desde el navegador para demo en clase
@app.post("/api/sim_telemetry")
def api_sim_telemetry():
    if ALLOW_SIM != "true":
        abort(403)
    data = request.get_json(silent=True) or {}
    prox = 1 if int(data.get("prox", 0)) else 0
    slot = str(data.get("slot", "S1"))
    if slot not in STATE["slots"]:
        STATE["slots"][slot] = 0
        STATE["capacity"] = len(STATE["slots"])
    STATE["slots"][slot] = prox
    STATE["updated_at"] = now_utc_iso()
    return make_response({"ok": True, "slot": slot, "prox": prox}, 200)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000, debug=True)
