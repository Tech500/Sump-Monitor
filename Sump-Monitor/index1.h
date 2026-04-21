const char HTML1[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <title>Check Sump Pit</title>
    <meta http-equiv="refresh" content="30">
    <style>
        .status-normal { color: green; font-weight: bold; }
        .status-alert { color: white; background-color: #e63000; padding: 10px; border-radius: 6px; font-weight: bold; }
        .status-flood { color: white; background-color: #cc0000; padding: 10px; border-radius: 6px; font-weight: bold; font-size: 1.2em; }
        .status-amber { color: white; background-color: #cc7700; padding: 10px; border-radius: 6px; font-weight: bold; }
        .status-ack { color: white; background-color: #886600; padding: 10px; border-radius: 6px; }
        .btn-alert { background-color: #e63000; color: white; padding: 10px 20px; border: none; border-radius: 6px; font-size: 1em; cursor: pointer; }
        .btn-resume { background-color: #cc7700; color: white; padding: 10px 20px; border: none; border-radius: 6px; font-size: 1em; cursor: pointer; }
    </style>
</head>

<body>
    <h1>Sump-Monitor --Water Level</h1>
    <h2>WATER:  Distance to floor %TOP% inches!</h2>
    <h2>%DATE%</h2>
    <br>
    <br><h3>Client IP: %CLIENTIP%</h3>
    <p><h3>Last Update: %LASTUPDATE%</h3></p>
    <br>

    <div class="alert-container">
        %ALERTBUTTON%
    </div>

    <br><br>

    <h2><a href="/SdBrowse">Filereader</a></h2>
    <h2><a href="/graph">Graph</a></h2>
</body>

</html>
)rawliteral";