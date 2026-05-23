var Clay = require('pebble-clay');
var clayConfig = require('./config');
var keys = require('message_keys');
var clay = new Clay(clayConfig);

function fetchWeather() {
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  var apiKey = settings[keys.API_KEY] || settings['API_KEY'];
  var unitType = settings[keys.UNITS] || 'imperial';
  var timeFormat = settings[keys.TIME_FORMAT] || '12h';

  if (!apiKey) return;

  navigator.geolocation.getCurrentPosition(function(pos) {
    var url = 'https://api.openweathermap.org/data/3.0/onecall?lat=' + 
              pos.coords.latitude + '&lon=' + pos.coords.longitude + 
              '&exclude=minutely,daily&units=' + unitType + '&appid=' + apiKey;

    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
      if (this.status === 200) {
        var json = JSON.parse(this.responseText);
        var tempSuffix = (unitType === 'metric') ? "C" : "°";
        
        var chunk1 = {};
        var chunk2 = {};
        chunk1[keys.UNITS] = unitType;

        for(var i = 0; i < 24; i++) {
          var h = json.hourly[i];
          var date = new Date(h.dt * 1000);
          var hours = date.getHours();
          var timeStr = "";

          if (timeFormat === '12h') {
            var ampm = hours >= 12 ? 'p' : 'a';
            hours = hours % 12 || 12;
            timeStr = hours + ampm;
          } else {
            timeStr = (hours < 10 ? '0' + hours : hours).toString();
          }

          var target = (i < 12) ? chunk1 : chunk2;
          target[keys.COLUMN_TIME + i] = timeStr;
          target[keys.COLUMN_TEMP + i] = Math.round(h.temp) + tempSuffix;
          target[keys.COLUMN_WIND + i] = Math.round(h.wind_speed).toString();
          target[keys.COLUMN_RAIN + i] = Math.round(h.pop * 100) + "%";
          target[keys.COLUMN_UV + i]   = Math.round(h.uvi);
          target[keys.COLUMN_COND + i] = h.weather[0].id;
          target[keys.IS_NIGHT + i]    = h.weather[0].icon.includes('n') ? 1 : 0;
        }

        // Send Chunks
        Pebble.sendAppMessage(chunk1, function() {
            Pebble.sendAppMessage(chunk2);
        });
      }
    };
    xhr.open('GET', url);
    xhr.send();
  });
}

Pebble.addEventListener('ready', fetchWeather);
Pebble.addEventListener('webviewclosed', function(e) {
  if (e && e.response) fetchWeather();
});