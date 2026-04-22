const char HTML1[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">

<head>
    <title>Sump Monitor</title>
    <meta http-equiv="refresh" content="30">
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Exo+2:wght@300;600;800&display=swap');

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
        }

        * { box-sizing: border-box; margin: 0; padding: 0; }

        body {
            background: var(--bg);
            color: var(--text);
            font-family: 'Exo 2', sans-serif;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 24px 16px;
        }

        /* subtle grid background */
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
            font-family: 'Share Tech Mono', monospace;
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
            font-family: 'Share Tech Mono', monospace;
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
            font-family: 'Share Tech Mono', monospace;
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
            font-family: 'Share Tech Mono', monospace;
            font-size: 0.9rem;
            color: var(--text);
            margin-top: 4px;
            word-break: break-all;
        }

        /* ── Alert status ── */
        .status-normal  { color: var(--green); font-weight: 600; font-size: 1.1rem; }
        .status-alert   { color: white; background: var(--red);     padding: 10px 16px; border-radius: 8px; font-weight: 700; display: inline-block; animation: pulse-red 1.5s infinite; }
        .status-flood   { color: white; background: var(--darkred); padding: 10px 16px; border-radius: 8px; font-weight: 700; font-size: 1.2em; display: inline-block; animation: pulse-red 0.8s infinite; }
        .status-amber   { color: white; background: var(--amber);   padding: 10px 16px; border-radius: 8px; font-weight: 700; display: inline-block; }
        .status-ack     { color: white; background: #5a4200;        padding: 10px 16px; border-radius: 8px; display: inline-block; }

        @keyframes pulse-red {
            0%, 100% { box-shadow: 0 0 0 0 rgba(248,81,73,0.5); }
            50%       { box-shadow: 0 0 0 8px rgba(248,81,73,0); }
        }

        /* ── Buttons ── */
        .btn-alert  { background: var(--red);   color: white; padding: 12px 24px; border: none; border-radius: 8px; font-size: 1em; font-family: 'Exo 2', sans-serif; font-weight: 600; cursor: pointer; letter-spacing: 0.04em; transition: opacity 0.2s; }
        .btn-resume { background: var(--amber); color: white; padding: 12px 24px; border: none; border-radius: 8px; font-size: 1em; font-family: 'Exo 2', sans-serif; font-weight: 600; cursor: pointer; letter-spacing: 0.04em; transition: opacity 0.2s; }
        .btn-alert:hover, .btn-resume:hover { opacity: 0.85; }

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
                <h1>Sump Monitor</h1>
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
        <div class="card" style="margin-bottom:20px; padding-top:35px; padding-bottom:35px;">
            <div class="card-label" style="color: var(--green); font-size: 1.1rem;">Alert Status</div>
            <div style="margin-top:8px;">
                %ALERTBUTTON%
            </div>
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
