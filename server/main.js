const ws = require('ws');
const http = require('http');
const fs = require('fs');

const API_KEY = 'c1e0203f81c24f01ba5135649230511';

const startWatering = "A";
const stopWatering = "B";

var watering = false;

const server = http.createServer(async function (req, res) {

  switch (req.url) {
    case "/water":
      if (!watering) {
        water();
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end('Watering plants for 30s');
      } else if (watering) {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end('Plants are already being watered');
      }

      break;
    case "/":
      // Load HTML file
      var htmlData = fs.readFileSync('index.html', 'utf8');
      // Allow CORS
      res.setHeader('Access-Control-Allow-Origin', '*');
      res.writeHead(200, { 'Content-Type': 'text/html' });
      res.end(htmlData);
      //
      break;
    case "/api":
      res.writeHead(200, { 'Content-Type': 'application/json' });
      res.end(currentData ? JSON.stringify(currentData) : 'No data available');
      break;
    case '/forceCheckWeather':
      getWeatherData();
      res.writeHead(200, { 'Content-Type': 'text/plain' });
      res.end('Weather checked');
      break;
    }
});

const wss = new ws.Server({ server });

var currentData;

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
    try {
      currentData = JSON.parse(message);
      if (watering) ws.send(startWatering);
      else ws.send(stopWatering);
    } catch (e) {
    }
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
  fetch(`http://api.weatherapi.com/v1/forecast.json?key=${API_KEY}&q=Khon+Kaen&days=1&aqi=no&alerts=no`)
    .then((response) => response.json())
    .then((data) => {
      console.log('Weather data fetched successfully');
      console.log(data);
      console.log(data.forecast.forecastday[0].day.daily_chance_of_rain, currentData.soilMoisture);
      // If the weather is not rainy today, water the plants immediately for 30s
      if (data.forecast.forecastday[0].day.daily_chance_of_rain < 50 && currentData.soilMoisture < 20) {
        console.log('called');
        water();
      }


    })
    .catch((error) => {
      console.error('Error fetching weather data:', error);
    });

}

// Check the weather API at 18:00 everyday
async function checkWeather() {
  const now = new Date();
  const hour = now.getHours();
  const minute = now.getMinutes();
  // Start watering now
  getWeatherData();
  if (hour == 4 && minute <= 10) {
    getWeatherData();
  }

}

function water() {
  watering = true;
  setTimeout(() => {
    watering = false;
  }, 5000);
}

// Check the weather API every 10 minutes
checkWeather();
setInterval(checkWeather, 600000);

process.on('uncaughtException', console.log);