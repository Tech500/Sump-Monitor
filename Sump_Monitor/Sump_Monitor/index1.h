const char HTML1[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <title>Check Sump Pit</title>
    <meta http-equiv="refresh" content="30">
</head>

<body>
    <h1>Sump Pit Water Level</h1>
    <h2>WATER:  Distance to floor %TOP% inches!</h2>
    <h2>%DATE%</h2>
    <br>
    <br><h3>Client IP: %CLIENTIP%</h3>
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

