from flask import Flask, render_template
import os

app = Flask(__name__, template_folder="templates")

MQTT_HOST    = os.environ.get("MQTT_HOST",    "k1809f1f.ala.us-east-1.emqxsl.com")
MQTT_WS_PORT = int(os.environ.get("MQTT_WS_PORT", "8084"))
MQTT_PATH    = os.environ.get("MQTT_PATH",    "/mqtt")
MQTT_USER    = os.environ.get("MQTT_USER",    "alexslcd")
MQTT_PASS    = os.environ.get("MQTT_PASS",    "JsnbCUE82WW4hgn")

@app.route("/")
def index():
    return render_template("index.html",
        mqtt_host=MQTT_HOST,
        mqtt_ws_port=MQTT_WS_PORT,
        mqtt_path=MQTT_PATH,
        mqtt_user=MQTT_USER,
        mqtt_pass=MQTT_PASS,
        use_ssl=True
    )

@app.get("/health")
def health():
    return {"ok": True}, 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000, debug=True)
