var locationOptions = {
  enableHighAccuracy: false,
  maximumAge: 6000000,
  timeout: 10000
};
var long = 165;
var lat = 0;

var counter = 0;

// I flocation was correctly found
function locationSuccess(pos) {
  counter = counter + 1;
  console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
  long = Math.round((147 / 180 * pos.coords.longitude) + 215);
  lat = Math.round(-(pos.coords.latitude - 90) * 2 / 3 + 16);
  console.log('location found');
  sendLocation();

  // Don't remember why change the behaviour after 30 occurences...
  if (counter > 30) {
    locationOptions = {
      enableHighAccuracy: false,
      maximumAge: 1000,
      timeout: 10000
    };
    counter = 0;
  }
}

// Sending location to the watch
function sendLocation() {
  console.log('sending location latmod= ' + lat + ' lonmod= ' + long);
  Pebble.sendAppMessage({
    latitude: lat,
    longitude: long
  });
}

// Reveiving messages from the watch
Pebble.addEventListener('appmessage',
  function (e) {
    console.log('Received message: ' + JSON.stringify(e.payload));
  }
);

// Meh, the locaiton wasn't what was expected
function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

// Make an asynchronous request
navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);