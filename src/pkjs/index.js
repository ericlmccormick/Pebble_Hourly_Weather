// ==============================================================================
// DEPENDENCIES & CONFIGURATION
// ==============================================================================
var Clay = require('pebble-clay');            // Include Clay framework for settings UI
var clayConfig = require('./config');         // Load settings page structure
var keys = require('message_keys');           // Import CloudPebble message key indices
var clay = new Clay(clayConfig);              // Initialize settings

// ==============================================================================
// CORE DATA FETCHING FUNCTION
// ==============================================================================
function fetchWeather() {
  // 1. Retrieve user preferences from local phone storage
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  var apiKey = settings[keys.API_KEY] || settings['API_KEY'];
  var unitType = settings[keys.UNITS] || 'imperial';
  var timeFormat = settings[keys.TIME_FORMAT] || '12h';

  if (!apiKey) return; // Halt if no API key is provided

  // 2. Request current GPS coordinates from the phone OS
  navigator.geolocation.getCurrentPosition(function(pos) {
    var lat = pos.coords.latitude;
    var lon = pos.coords.longitude;

    // Build the OpenWeather 3.0 One Call URL
    var url = 'https://api.openweathermap.org/data/3.0/onecall?lat=' + 
              lat + '&lon=' + lon + '&exclude=minutely,daily&units=' + unitType + '&appid=' + apiKey;

    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
      if (this.status === 200) {
        var json = JSON.parse(this.responseText);
        var tempSuffix = (unitType === 'metric') ? "C" : "°";
        
        // Prepare two separate dictionaries to prevent Bluetooth MTU overflow
        var chunk1 = {};
        var chunk2 = {};
        chunk1[keys.UNITS] = unitType;

        // 3. Process the next 24 hours of forecast data
        for(var i = 0; i < 24; i++) {
          var h = json.hourly[i];
          var date = new Date(h.dt * 1000);
          var hours = date.getHours();
          var timeStr = "";

          // Time formatting logic (12h vs 24h)
          if (timeFormat === '12h') {
            var ampm = hours >= 12 ? 'p' : 'a';
            hours = hours % 12 || 12;
            timeStr = hours + ampm;
          } else {
            timeStr = (hours < 10 ? '0' + hours : hours).toString();
          }

          // Determine which transmission chunk this hour belongs to
          var target = (i < 12) ? chunk1 : chunk2;
          
          // Populate the specific key-value pairs utilizing index offsets
          target[keys.COLUMN_TIME + i] = timeStr;
          target[keys.COLUMN_TEMP + i] = Math.round(h.temp) + tempSuffix;
          target[keys.COLUMN_WIND + i] = Math.round(h.wind_speed).toString();
          target[keys.COLUMN_RAIN + i] = Math.round(h.pop * 100) + "%";
          target[keys.COLUMN_UV + i]   = Math.round(h.uvi);
          
          // Index 0 accesses the primary weather condition array block
          target[keys.COLUMN_COND + i] = h.weather[0].id;
          target[keys.IS_NIGHT + i]    = h.weather[0].icon.includes('n') ? 1 : 0;
        }

        // ==============================================================================
        // REVERSE GEOCODING & FALLBACK LOGIC
        // ==============================================================================
        
        // Primary fallback: Extract city from generic timezone string (e.g., "America/New_York")
        var fallbackName = "GPS Fix";
        if (json.timezone) {
          var tzParts = json.timezone.split('/');
          if (tzParts.length > 1) {
            fallbackName = tzParts[1].replace('_', ' '); // Converts "New_York" to "New York"
          }
        }

        // Attempt secondary API call for precise local city name
        var geoUrl = 'https://api.openweathermap.org/geo/1.0/reverse?lat=' + lat + '&lon=' + lon + '&limit=1&appid=' + apiKey;
        var geoXhr = new XMLHttpRequest();
        
        geoXhr.onload = function() {
          var locationName = fallbackName;
          if (this.status === 200) {
            var geoJson = JSON.parse(this.responseText);
            if (geoJson && geoJson.length > 0 && geoJson[0].name) {
              locationName = geoJson[0].name; // Successfully pulled true city name
            }
          }
          
          // Insert location into the first chunk and initiate transfer
          chunk1[keys.LOCATION_NAME] = locationName;
          sendWeatherChunks(chunk1, chunk2);
        };

        // If the secondary API drops or times out, immediately push the fallback to avoid a stuck UI
        geoXhr.onerror = function() {
          chunk1[keys.LOCATION_NAME] = fallbackName;
          sendWeatherChunks(chunk1, chunk2);
        };

        geoXhr.open('GET', geoUrl);
        geoXhr.send();
      }
    };
    xhr.open('GET', url);
    xhr.send();
  });
}

// ==============================================================================
// TRANSMISSION CHAIN
// ==============================================================================

// Ensures the first 12 hours complete transmission before triggering the remaining 12
function sendWeatherChunks(c1, c2) {
  Pebble.sendAppMessage(c1, function() {
    Pebble.sendAppMessage(c2);
  });
}

// ==============================================================================
// LIFECYCLE LISTENERS
// ==============================================================================

// Triggers immediately when the watch app boots up
Pebble.addEventListener('ready', fetchWeather);

// Triggers when the user closes the Clay Settings menu on their phone
Pebble.addEventListener('webviewclosed', function(e) {
  if (e && e.response) fetchWeather(); // Refresh data with new user settings
});