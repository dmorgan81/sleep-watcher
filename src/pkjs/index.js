Pebble.addEventListener('appmessage', function(e) {
    console.log('AppMessage received');
    var sleeping = e.payload['SLEEPING'];
    console.log('sleeping = ' + sleeping);
    // Now make your call
    Pebble.sendAppMessage({ 'MSG_PROCESSED' : 1 });
});

Pebble.addEventListener('ready', function() {
    console.log('App ready');
    Pebble.sendAppMessage({ 'APP_READY' : 1 });
});
