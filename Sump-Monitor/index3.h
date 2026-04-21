//index3.h
const char HTML3[] PROGMEM = R"====(
<!DOCTYPE html>
<html>
<head>
  <title>Sump-Monitor -- Graphs</title>
  <meta http-equiv="refresh" content="120" />   
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { 
      background-color: #111; 
      color: #eee; 
      font-family: Arial, sans-serif; 
      text-align: center;
      height: 100vh;
      overflow: hidden;
    }
    h1 { 
      color: #4CAF50; 
      padding: 10px 0 2px 0;
      font-size: 1.6em;
    }
    p { 
      color: #aaa; 
      padding-bottom: 5px;
      font-size: 0.9em;
    }
    .wrapper {
      max-width: 1600px;
      margin: 0 auto;
      padding: 0 24px;
    }
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      grid-template-rows: auto auto auto;
      gap: 12px;
      padding: 10px 0;
    }
    .panel {
      background: #1a1a1a;
      border-radius: 8px;
      padding: 8px;
    }
    .panel h3 { 
      margin: 3px 0 6px 0;
      font-size: 0.95em;
    }
    .all-events {
      grid-column: 1 / -1;
    }
    .all-events iframe {
      height: 24vh;
    }
    iframe { 
      width: 100%; 
      height: 27vh;
      border: none; 
      border-radius: 4px; 
    }
    a { 
      color: #4CAF50; 
      text-decoration: none;
      font-size: 0.9em;
    }
    a:hover {
      text-decoration: underline;
    }
    .footer {
      padding: 8px 0;
    }
    @media (max-width: 600px) {
      body { overflow: auto; }
      .wrapper { padding: 0 12px; }
      .grid { grid-template-columns: 1fr; }
      iframe { height: 250px; }
      .all-events iframe { height: 250px; }
    }
  </style>
</head>
<body>

  <h1>&#x1F4CA; Sump-Monitor -- Graphs</h1>
  <p>Auto-refreshes every 30 seconds</p>

  <div class="wrapper">
    <div class="grid">

      <div class="panel all-events">
        <h3>All Events</h3>
        <iframe src="http://192.168.12.146:3000/d-solo/adqlkqm/sump-monitor?orgId=1&timezone=browser&panelId=panel-1" frameborder="0"></iframe>
      </div>

      <div class="panel">
        <h3 style="color:#FDD835;">&#x26A0;&#xFE0F; High Water Events</h3>
        <iframe src="http://192.168.12.146:3000/d-solo/adqlkqm/sump-monitor?orgId=1&timezone=browser&panelId=panel-3" frameborder="0"></iframe>
      </div>

      <div class="panel">
        <h3 style="color:#e53935;">&#x1F6A8; Flooding Events</h3>
        <iframe src="http://192.168.12.146:3000/d-solo/adqlkqm/sump-monitor?orgId=1&timezone=browser&panelId=panel-2" frameborder="0"></iframe>
      </div>

      <div class="panel">
        <h3 style="color:#4CAF50;">&#x2705; All Clear Events</h3>
        <iframe src="http://192.168.12.146:3000/d-solo/adqlkqm/sump-monitor?orgId=1&timezone=browser&panelId=panel-4" frameborder="0"></iframe>
      </div>

      <div class="panel">
        <h3 style="color:#64B5F6;">&#x1F4CF; Pit Activity -- Distance To Target</h3>
        <iframe src="http://192.168.12.146:3000/d-solo/adqlkqm/sump-monitor?orgId=1&timezone=browser&panelId=panel-6" frameborder="0"></iframe>
      </div>

    </div>
  </div>

  <div class="footer">
    <a href="/sump">&#x2190; Back to Sump Monitor</a>
  </div>

</body>
</html>

)====";
