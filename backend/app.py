from flask import Flask, render_template, request, jsonify, make_response
from datetime import datetime

app = Flask(__name__)

STATE = {
    "prox": 0,                 # 0 libre, 1 ocupado
    "motor_desired": "off",    # "on" | "off"
    "updated_at": datetime.utcnow().isoformat()
}

@app.route("/")
def index():
    return render_template("index.html")

@app.get("/api/status")
def api_status():
    return jsonify(STATE)

@app.get("/api/cmd")
def api_cmd():
    # El ESP32 consulta qué debe hacer con el motor
    return jsonify({"motor_desired": STATE["motor_desired"]})

@app.post("/api/motor")
def api_motor():
    data = request.get_json(silent=True) or {}
    st = str(data.get("state", "")).lower()
    if st not in ("on", "off"):
        return make_response({"error": "usa on|off"}, 400)
    STATE["motor_desired"] = st
    STATE["updated_at"] = datetime.utcnow().isoformat()
    return make_response(f"OK MOTOR {st.upper()}", 200)

@app.post("/api/telemetry")
def api_telemetry():
    data = request.get_json(silent=True) or {}
    prox = int(data.get("prox", 0))
    STATE["prox"] = 1 if prox else 0
    STATE["updated_at"] = datetime.utcnow().isoformat()
    return make_response({"ok": True}, 200)

if __name__ == "__main__":
    # Para pruebas locales
    app.run(host="0.0.0.0", port=8000, debug=True)
