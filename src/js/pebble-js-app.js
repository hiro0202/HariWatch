
function iconFromWeatherId(weatherId) {
  if (weatherId < 600) {
    return 2;
  } else if (weatherId < 700) {
    return 3;
  } else if (weatherId > 800) {
    return 1;
  } else {
    return 0;
  }
}

function fetchWeather() {
  var req = new XMLHttpRequest();
  req.open('GET', 'サーバーのipアドレスを入力', true);
  //req.open('GET', 'http://192.168.1.101:8180/', true);
  req.onload = function () {
    if (req.readyState === 4) {
      if (req.status === 200) {
        console.log(req.responseText);
        var response = JSON.parse(req.responseText);
        var temp1 = Math.round(response.temp1);
        var key = iconFromWeatherId(response.humid);
        var temp2 = Math.round(response.temp2);
        var humid = Math.round(response.humid);
        console.log(temp1);
        console.log(key);
        console.log(temp2);
        console.log(humid);
        Pebble.sendAppMessage({
          'WEATHER_ICON_KEY': key,
          'WEATHER_TEMPERATURE_KEY': temp1 + '\xB0C',
          'WEATHER_CITY_KEY': temp2　+ '\xB0C',
          'WEATHER_HUMID_KEY':humid + '％'
        });
      } else {
        console.log('Error');
      }
    }
  };
  req.send(null);
}


Pebble.addEventListener('ready', function (e) {
  console.log('connect!' + e.ready);
  fetchWeather();
  console.log(e.type);
});

Pebble.addEventListener('appmessage', function (e) {

  fetchWeather();
  console.log(e.type);
  console.log(e.payload.temperature);
  console.log('message!');
});

Pebble.addEventListener('webviewclosed', function (e) {
  console.log('webview closed');
  console.log(e.type);
  console.log(e.response);
});
