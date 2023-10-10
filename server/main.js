const ws = require('ws');
const http = require('http');

const server = http.createServer(function (req, res) {
  res.writeHead(200, { 'Content-Type': 'application/json' });
  res.end(currentData ? JSON.stringify(currentData) : 'No data available');
});
const wss = new ws.Server({ server });

var currentData;

var wateringQueued = false;

// Keep track of connected clients
const clients = new Set();

// Function to check if a WebSocket connection is alive
function isAlive(client) {
  return client.isAlive;
}

// Function to terminate a WebSocket connection
function terminateConnection(client) {
  client.terminate();
}

var wsID = 1;

// Set up a periodic ping to check if connections are alive
const pingInterval = setInterval(() => {
  wss.clients.forEach((client) => {
    if (!isAlive(client)) {
      console.log('Client did not respond to ping. Terminating connection.');
      terminateConnection(client);
    } else {
      client.isAlive = false;
      client.send('$SERVERPING');
    }
  });
}, 30000);

wss.on('connection', (ws) => {

  wsID++;

  // Request client identification
  ws.send("$IDENTIFICATION_REQUEST");
  ws.isAlive = true;
  ws.isESPClient = false;
  clients.add(ws);

  console.log('New client connected');

  ws.on('message', (message) => {
    // Ignore ping response message
    if (message.toString() == "$CLIENTPONG") return ws.isAlive = true;
    if (message.toString() == "$IDENTIFY ESP32") return ws.isESPClient = true;

    // Broadcast message to all connected clients
    console.log(`Received message: ${message}`);
    currentData = JSON.parse(message);
    wss.clients.forEach((client) => {
      if (client != ws && client.readyState === ws.OPEN) {
        client.send(message);
      }
    });
  });

  ws.on('close', () => {
   clients.delete(ws); 
  });
  
  ws.on('error', () => {
    console.error('Client error. Terminating connection.');
    terminateConnection(ws);
  });
});

server.listen(42070, () => {
  console.log('Server started on port 42070');
});

// Check the weather API at 18:00 everyday
function getWeatherData() {
  fetch()
}

process.on('uncaughtException', console.log);