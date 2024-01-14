// CaptivePortalHTML.h
#ifndef CaptivePortalHTML_h
#define CaptivePortalHTML_h

#include <Arduino.h>

const char captivePortalHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Rover</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            background-color: #f4f4f4;
        }
        h1 {
            text-align: center;
            margin-bottom: 20px;
        }
        .container {
            width: 60%;
            padding: 20px;
            background: #fff;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
            border-radius: 5px;
            text-align: left; /* Align container content to the left */
        }
        button, input[type='password'] {
            width: 100%;
            padding: 10px;
            margin-top: 5px;
            margin-bottom: 5px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box; /* Fixes padding and border issues */
        }
        button {
            background-color: #007bff;
            color: white;
            cursor: pointer;
            border: none;
            transition: background-color 0.3s;
            width: auto; /* Make buttons auto width */
            padding: 5px 10px; /* Reduce button padding */
        }
        button:hover {
            background-color: #0056b3;
        }
        .status {
            margin-top: 10px;
            padding: 5px; /* Reduce status padding */
            border-radius: 4px;
        }
        .connecting {
            color: blue;
        }
        .success {
            color: green;
        }
        .error {
            color: red;
        }
        .radio-group {
            display: flex;
            flex-direction: column; /* Align radio buttons vertically */
        }
        .radio-item {
            margin-right: 10px; /* Add some space between radio buttons */
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Plant Irrigator WI-FI Setup</h1>
        <div id="networkList"></div> <!-- Network list will be populated here -->
        <button onclick="scanNetworks()">Refresh Networks</button>
        <div>
            Password: <input type="password" id="password">
        </div>
        <button onclick="connect()">Connect</button>
        <label id="status" class="status">Status: Idle</label>
    </div>

    <script>
        function scanNetworks() {
            fetch('/scan')
                .then(response => response.json())
                .then(data => {
                    const networkList = document.getElementById('networkList');
                    networkList.innerHTML = ''; // Clear existing list

                    data.forEach((net, index) => {
                        const radioItem = document.createElement('input');
                        radioItem.type = 'radio';
                        radioItem.id = 'net' + index;
                        radioItem.name = 'network';
                        radioItem.value = net.SSID;

                        const label = document.createElement('label');
                        label.htmlFor = 'net' + index;
                        label.textContent = net.SSID;

                        const lineBreak = document.createElement('br');

                        networkList.appendChild(radioItem);
                        networkList.appendChild(label);
                        networkList.appendChild(lineBreak);
                    });
                })
                .catch(error => {
                    console.error('There has been a problem with your fetch operation:', error);
                    alert('Error: ' + error.message);
                });
        }

        function connect() {
            const selectedNetwork = document.querySelector('input[name="network"]:checked');
            if (!selectedNetwork) {
                alert('Please select a network');
                return;
            }

            const ssid = selectedNetwork.value;
            const password = document.getElementById('password').value;
            const statusLabel = document.getElementById('status');

            statusLabel.innerText = 'Connecting...';
            statusLabel.className = 'status connecting';

            fetch('/connect', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
            })
            .then(response => {
                if (response.ok) {
                    statusLabel.innerText = 'Connected successfully';
                    statusLabel.className = 'status success';
                    setTimeout(() => {
                        window.close(); // Close the captive portal window
                    }, 2000); // Close after 2 seconds to allow user to see the message
                } else {
                    statusLabel.innerText = 'Connection failed';
                    statusLabel.className = 'status error';
                }
            })
            .catch(error => {
                statusLabel.innerText = 'Connection failed';
                statusLabel.className = 'status error';
            });
        }

        window.onload = scanNetworks;
    </script>
</body>
</html>
)rawliteral";

#endif
