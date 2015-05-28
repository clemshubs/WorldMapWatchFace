var locationOptions = {
  enableHighAccuracy: true, 
  maximumAge: 1000, 
  timeout: 10000
};
var long = 165;
var lat = 0;

var counter = 0;




function locationSuccess(pos) {
  counter = counter + 1;
  // console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
  long = Math.round((147/180 * pos.coords.longitude)+215);
  lat = Math.round(-(pos.coords.latitude - 90)*2/3 + 16);
      console.log('location found');
     sendLocation();
  
  if(counter>30){
    locationOptions = {
    enableHighAccuracy: true, 
    maximumAge: 1000, 
    timeout: 10000
    };
    counter = 0;
  }
}

function sendLocation(){
        console.log('sending location latmod= ' + lat + ' lonmod= ' + long);
    Pebble.sendAppMessage({latitude :  lat, 
                         longitude : long});
}

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Received message: ' + JSON.stringify(e.payload));
  }
);

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

// Make an asynchronous request
navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);


