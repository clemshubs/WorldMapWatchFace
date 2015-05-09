var locationOptions = {
  enableHighAccuracy: true, 
  maximumAge: 10000, 
  timeout: 10000
};

function locationSuccess(pos) {
  
  console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
  var long = Math.round((147/180 * pos.coords.longitude)+215);
  var lat = Math.round(-(pos.coords.latitude - 90)*2/3 + 16);
  
  Pebble.sendAppMessage({latitude :  lat, 
                         longitude : long});
    console.log('latmod= ' + lat + ' lonmod= ' + long);


}

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

// Make an asynchronous request
navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
