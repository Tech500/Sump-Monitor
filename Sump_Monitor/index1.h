const char HTML1[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">

<head>
    <title>Sump-Monitor</title>
    <meta http-equiv="refresh" content="30">
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        :root {
            --bg:        #0d1117;
            --surface:   #161b22;
            --border:    #30363d;
            --text:      #c9d1d9;
            --muted:     #8b949e;
            --green:     #3fb950;
            --amber:     #d29922;
            --red:       #f85149;
            --darkred:   #b91c1c;
            --accent:    #58a6ff;
            --yellow:    #FFD700;
        }

        * { box-sizing: border-box; margin: 0; padding: 0; }

        body {
            background: var(--bg);
            color: var(--text);
            font-family: 'Segoe UI', 'Arial', sans-serif;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 24px 16px;
        }

        body::before {
            content: '';
            position: fixed;
            inset: 0;
            background-image:
                linear-gradient(rgba(48,54,61,0.3) 1px, transparent 1px),
                linear-gradient(90deg, rgba(48,54,61,0.3) 1px, transparent 1px);
            background-size: 40px 40px;
            pointer-events: none;
            z-index: 0;
        }

        .container {
            position: relative;
            z-index: 1;
            max-width: 640px;
            margin: 0 auto;
        }

        /* ── Header ── */
        .header {
            display: flex;
            align-items: center;
            gap: 12px;
            margin-bottom: 28px;
            padding-bottom: 16px;
            border-bottom: 1px solid var(--border);
        }

        .header-icon {
            font-size: 2rem;
            filter: drop-shadow(0 0 8px rgba(88,166,255,0.5));
        }

        .header h1 {
            font-size: 2.8rem;
            font-weight: 800;
            letter-spacing: 0.05em;
            text-transform: uppercase;
            color: var(--accent);
        }

        .header .subtitle {
            font-size: 0.75rem;
            color: var(--muted);
            font-family: 'Courier New', monospace;
            margin-top: 2px;
        }

        /* ── Cards ── */
        .card {
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: 10px;
            padding: 20px 24px;
            margin-bottom: 16px;
        }

        .card-label {
            font-size: 0.7rem;
            font-family: 'Courier New', monospace;
            text-transform: uppercase;
            letter-spacing: 0.12em;
            color: var(--muted);
            margin-bottom: 6px;
        }

        /* ── Water level ── */
        .water-card {
            border-left: 4px solid var(--accent);
        }

        .water-value {
            font-size: 3.2rem;
            font-weight: 800;
            font-family: 'Courier New', monospace;
            color: var(--accent);
            line-height: 1;
            filter: drop-shadow(0 0 12px rgba(88,166,255,0.4));
        }

        .water-value .unit {
            font-size: 1.2rem;
            color: var(--muted);
            margin-left: 6px;
        }

        /* ── Meta row ── */
        .meta-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
            margin-bottom: 16px;
        }

        .meta-card {
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: 8px;
            padding: 24px 18px;
        }

        .meta-value {
            font-family: 'Courier New', monospace;
            font-size: 0.9rem;
            color: var(--text);
            margin-top: 4px;
            word-break: break-all;
        }

        /* ── Alert key block ── */
        .alert-key {
            display: flex;
            align-items: center;
            justify-content: center;
            width: 100%;
            height: 80px;
            border-radius: 8px;
            font-size: 1.8rem;
            font-weight: 800;
            letter-spacing: 0.18em;
            text-transform: uppercase;
            margin-top: 10px;
        }

        .key-normal  { background: var(--green);   color: #fff; }
        .key-high    { background: var(--yellow);  color: #1a1a1a; animation: pulse-yellow 1.5s infinite; }
        .key-flood   { background: var(--darkred); color: #fff;    animation: pulse-red   0.8s infinite; }
        .key-pumpoff { background: var(--muted);   color: #fff; }

        @keyframes pulse-yellow {
            0%, 100% { box-shadow: 0 0 0 0   rgba(255,215,0,0.6); }
            50%       { box-shadow: 0 0 0 10px rgba(255,215,0,0);   }
        }

        @keyframes pulse-red {
            0%, 100% { box-shadow: 0 0 0 0   rgba(248,81,73,0.6); }
            50%       { box-shadow: 0 0 0 10px rgba(248,81,73,0);   }
        }

        /* ── Pump status ── */
        .status-normal { color: var(--green); font-weight: 600; font-size: 1.1rem; }
        .status-idle   { color: var(--muted); font-weight: 600; font-size: 1.1rem; }

        /* ── Nav links ── */
        .nav-links {
            display: flex;
            gap: 12px;
            margin-top: 8px;
        }

        .nav-links a {
            flex: 1;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: 8px;
            padding: 14px;
            color: var(--accent);
            text-decoration: none;
            font-weight: 600;
            font-size: 0.95rem;
            letter-spacing: 0.03em;
            transition: background 0.2s, border-color 0.2s;
        }

        .nav-links a:hover {
            background: #1f2937;
            border-color: var(--accent);
        }

    </style>
</head>

<body>
    <div class="container">

        <!-- Header -->
        <div class="header">
            <span class="header-icon">💧</span>
            <div>
                <h1>Sump-Monitor</h1>
                <div class="subtitle">Auto-refresh every 30s &nbsp;·&nbsp; %DATE%</div>
            </div>
        </div>

        <!-- Water level -->
        <div class="card water-card">
            <div class="card-label">Distance to Water Surface</div>
            <div class="water-value">%TOP%<span class="unit">in</span></div>
        </div>

        <!-- Meta -->
        <div class="meta-grid">
            <div class="meta-card">
                <div class="card-label">Client IP</div>
                <div class="meta-value">%CLIENTIP%</div>
            </div>
            <div class="meta-card">
                <div class="card-label">Last Event</div>
                <div class="meta-value">%LASTUPDATE%</div>
            </div>
        </div>

        <!-- Alert -->
        <div class="card" style="margin-bottom:16px; padding-top:28px; padding-bottom:28px;">
            <div class="card-label" style="font-size:1.1rem; color:%ALERTCOLOR%;">Alert Status</div>
            <div class="alert-key %ALERTKEYCLASS%" style="background:%ALERTBG%; color:%ALERTFG%;">%ALERTKEYWORD%</div>
        </div>

        <!-- Sump Pump Status -->
        <div class="card" style="margin-bottom:16px;">
            <div class="card-label">Pump Status</div>
            <div style="margin-top:8px;">%SUMPPUMP%</div>
        </div>

        <!-- Nav -->
        <div class="nav-links">
            <a href="/SdBrowse">🗂 File Reader</a>
            <a href="/graph">📈 Graphs</a>
        </div>

    </div>
</body>

</html>
)rawliteral";
