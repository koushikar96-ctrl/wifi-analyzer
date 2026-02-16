from flask import Flask, render_template, request, redirect, url_for, session, flash, jsonify
import mysql.connector
from flask_bcrypt import Bcrypt
# keep pywifi import if you want fallback; otherwise keep but can be unused
import pywifi
from pywifi import const
import time
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import subprocess
import json
import sys


app = Flask(__name__)
app.secret_key = "supersecretkey"
bcrypt = Bcrypt(app)

# MySQL connection
db = mysql.connector.connect(
    host="localhost",
    user="root",
    password="$Jesus25",
    database="wifi_analyzer_db"
)
cursor = db.cursor(dictionary=True)

def get_scan_results():
    exe_name = "wifi_analyzer.exe" if sys.platform.startswith("win") else "wifi_analyzer"
    exe_path = os.path.join(os.getcwd(), "src", exe_name)

    try:
        if os.path.exists(exe_path) and os.access(exe_path, os.X_OK):
            proc = subprocess.run([exe_path], capture_output=True, text=True, timeout=8)
            raw = proc.stdout.strip()
            parsed = json.loads(raw)
            wifi_list = []

            for net in parsed:
                ssid = net.get("ssid") or ""
                signal = net.get("signal") or -100
                try:
                    signal = int(signal)
                except:
                    signal = -100
                factor = round(signal / (len(ssid) + 1), 2) if ssid else 0

                # Include estimated distance
                wifi_list.append({
                    "ssid": ssid,
                    "signal": signal,
                    "factor": factor,
                    "distance_m": net.get("distance_m", 0)
                })

            wifi_list.sort(key=lambda x: x["signal"], reverse=True)
            return wifi_list
    except Exception:
        return []

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        username = request.form['username']
        password = request.form['password']

        cursor.execute("SELECT * FROM users WHERE username=%s", (username,))
        user = cursor.fetchone()

        # Check hashed password
        if user and bcrypt.check_password_hash(user['password_hash'], password):
            session['user'] = username
            return redirect("/dashboard")
        else:
            flash("Invalid credentials")
            return redirect("/login")
    return render_template("login.html")

@app.route("/signup", methods=["GET", "POST"])
def signup():
    if request.method == "POST":
        username = request.form['username']
        password = request.form['password']

        # Hash the password before storing
        hashed_pw = bcrypt.generate_password_hash(password).decode('utf-8')

        cursor.execute(
            "INSERT INTO users (username, password_hash) VALUES (%s, %s)",
            (username, hashed_pw)
        )
        db.commit()
        flash("Signup successful! Please login.")
        return redirect("/login")
    return render_template("signup.html")

# ---------------- DASHBOARD ----------------
@app.route("/dashboard")
def dashboard():
    if "user" not in session:
        return redirect(url_for("index"))
    return render_template("dashboard.html", user=session["user"])


# ---------------- SCAN WIFI ----------------
@app.route("/scan")
def scan():
    if "user" not in session:
        return redirect(url_for("index"))

    wifi_list = get_scan_results()
    best_network = wifi_list[0]["ssid"] if wifi_list else "No networks found"
    return render_template("scan.html", wifi_list=wifi_list, best=best_network)
    
# ---------------- VISUALIZE ----------------
@app.route("/visualize")
def visualize():
    if "user" not in session:
        return redirect(url_for("index"))

    wifi_list = get_scan_results()
    if not wifi_list:
        best_network = "No networks found"
        return render_template("visualize.html", chart=None, best=best_network)

    # Plot all networks (including duplicates)
    ssids = [f"{net['ssid']} ({i+1})" for i, net in enumerate(wifi_list)]
    strengths = [net['signal'] for net in wifi_list]

    plt.figure(figsize=(10, 6))
    plt.bar(ssids, strengths, color="dodgerblue")
    plt.xlabel("WiFi Networks")
    plt.ylabel("Signal Strength (dBm)")
    plt.title("WiFi Signal Strengths")
    plt.xticks(rotation=45)
    plt.tight_layout()

    img_path = os.path.join("static", "wifi_chart.png")
    plt.savefig(img_path)
    plt.close()

    best_network = wifi_list[0]['ssid'] if wifi_list else "None"

    return render_template("visualize.html", chart="wifi_chart.png", best=best_network)

# ---------------- LOGOUT ----------------
@app.route("/logout")
def logout():
    session.pop("user", None)
    return redirect(url_for("index"))


if __name__ == "__main__":
    app.run(debug=True)
