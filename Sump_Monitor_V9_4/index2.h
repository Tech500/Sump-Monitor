//index2.h
const char HTML2[] PROGMEM = R"====(
<!DOCTYPE HTML>
<html lang="en">

<head>
    <title>SD Browse — Sump Monitor</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
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
            font-family: 'Segoe UI', Arial, sans-serif;
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
            font-size: 1.4rem;
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

        /* ── Card ── */
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
            margin-bottom: 12px;
        }

        /* ── Scrollable file list ── */
        .file-list {
            font-family: 'Courier New', monospace;
            font-size: 0.95rem;
            color: var(--green);
            line-height: 2;
            max-height: 420px;
            overflow-y: auto;
            padding-right: 8px;
        }

        /* scrollbar styling */
        .file-list::-webkit-scrollbar {
            width: 6px;
        }
        .file-list::-webkit-scrollbar-track {
            background: var(--bg);
            border-radius: 3px;
        }
        .file-list::-webkit-scrollbar-thumb {
            background: var(--border);
            border-radius: 3px;
        }
        .file-list::-webkit-scrollbar-thumb:hover {
            background: var(--muted);
        }

        .file-list a {
            color: var(--green);
            text-decoration: none;
            transition: color 0.2s;
        }

        .file-list a:hover { color: #6ee68a; }

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
            <span class="header-icon">🗂</span>
            <div>
                <h1>SD Browse</h1>
                <div class="subtitle">Collected Observations</div>
            </div>
        </div>

        <!-- File list -->
        <div class="card">
            <div class="card-label">Files</div>
            <div class="file-list">
                %URLLINK%
            </div>
        </div>

        <!-- Nav -->
        <div class="nav-links">
            <a href="/SdBrowse">🗂 File Reader</a>
            <a href="/sump">🏠 Main Menu</a>
        </div>

    </div>
</body>

</html>
)====";
