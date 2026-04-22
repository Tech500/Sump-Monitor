//index4.h
const char HTML4[] PROGMEM = R"====(
<!DOCTYPE HTML>
<html lang="en">

<head>
    <title>File Reader — Sump Monitor</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Exo+2:wght@300;600;800&display=swap');

        :root {
            --bg:      #0d1117;
            --surface: #161b22;
            --border:  #30363d;
            --text:    #c9d1d9;
            --muted:   #8b949e;
            --accent:  #58a6ff;
            --green:   #3fb950;
        }

        * { box-sizing: border-box; margin: 0; padding: 0; }

        body {
            background: var(--bg);
            color: var(--text);
            font-family: 'Exo 2', sans-serif;
            min-height: 100vh;
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
            max-width: 860px;
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
            font-size: 1.4rem;
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

        /* ── File content card ── */
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
            margin-bottom: 12px;
            display: flex;
            align-items: center;
            gap: 8px;
        }

        .loading-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background: var(--accent);
            animation: blink 1s infinite;
        }

        @keyframes blink {
            0%, 100% { opacity: 1; }
            50%       { opacity: 0.2; }
        }

        #file-content {
            font-family: 'Share Tech Mono', monospace;
            font-size: 0.82rem;
            line-height: 1.7;
            color: var(--green);
            white-space: pre-wrap;
            word-break: break-all;
            max-height: 65vh;
            overflow-y: auto;
            padding-right: 8px;
        }

        /* scrollbar styling */
        #file-content::-webkit-scrollbar { width: 6px; }
        #file-content::-webkit-scrollbar-track { background: var(--bg); }
        #file-content::-webkit-scrollbar-thumb { background: var(--border); border-radius: 3px; }
        #file-content::-webkit-scrollbar-thumb:hover { background: var(--muted); }

        /* ── Row count badge ── */
        #row-count {
            font-family: 'Share Tech Mono', monospace;
            font-size: 0.72rem;
            color: var(--muted);
            margin-top: 10px;
            text-align: right;
        }

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
            <span class="header-icon">📄</span>
            <div>
                <h1>File Reader</h1>
                <div class="subtitle">Sump Monitor — Raw Data</div>
            </div>
        </div>

        <!-- File content -->
        <div class="card">
            <div class="card-label" style="display:flex; justify-content:space-between; align-items:center;">
                <div style="display:flex; align-items:center; gap:8px;">
                    <div class="loading-dot" id="loading-dot"></div>
                    <span id="card-title">Loading file...</span>
                </div>
                <span id="filename-display" style="color:var(--green); font-size:0.85rem;">%FN%</span>
            </div>
            <div id="file-content"></div>
            <div id="row-count"></div>
        </div>

        <!-- Nav -->
        <div class="nav-links">
            <a href="/SdBrowse">🗂 File Browser</a>
            <a href="/sump">🏠 Main Menu</a>
        </div>

    </div>
</body>

<script>
    window.addEventListener('load', getFile);

    function getFile() {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
                var content = this.responseText;
                document.getElementById("file-content").textContent = content;

                // hide loading dot, update label
                document.getElementById("loading-dot").style.display = "none";
                document.getElementById("card-title").textContent = "File Contents";

                // row count
                var lines = content.trim().split('\n').length;
                document.getElementById("row-count").textContent = lines + " rows";
            }
        };
        xhr.open("GET", "/get-file", true);
        xhr.send();
    }
</script>

</html>
)====";
