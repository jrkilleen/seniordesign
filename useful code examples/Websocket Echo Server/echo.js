var port = 1337,
	WebSocketServer = require('ws').Server,
	wss = new WebSocketServer({ port: port });

console.log('listening on port: ' + port);





wss.broadcast = function broadcast(data) {
	time =  new Date().toLocaleTimeString();
	console.log(`broadcasting => ${time}`);
  wss.clients.forEach(function each(client) {
	  
    client.send(time);
  });
};

function myFunc(arg) {
  console.log(`arg was => ${arg}`);
}





function intervalFunc() {
  console.log('Cant stop me now!');
}

setInterval(wss.broadcast, 1000, "idk lol");


wss.on('connection', function connection(ws) {

	ws.on('message', function(message) {

		console.log('message: ' + message);
		ws.send('echo: ' + message);

	});

	console.log('new client connected!');
	ws.send('connected!');

});