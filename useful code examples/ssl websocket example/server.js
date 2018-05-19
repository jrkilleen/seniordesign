// code from https://blog.zackad.com/en/2017/08/19/create-websocket-with-nodejs.html

var fs = require('fs');
var https= require('https');


var privateKey = fs.readFileSync('sslcerts/key.pem', 'utf8');
var certificate = fs.readFileSync('sslcerts/certificate.pem', 'utf8');




//var port = 1337,
//	WebSocketServer = require('ws').Server,
//	wss = new WebSocketServer({ port: port });
//
//console.log('listening on port: ' + port);

var credentials = { key: privateKey, cert: certificate };
var httpsServer = https.createServer(credentials);
httpsServer.listen(8443);

var WebSocketServer = require('ws').Server;
var wss = new WebSocketServer({
    server: httpsServer,
});




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

setInterval(wss.broadcast, 10000, "idk lol");


wss.on('connection', function connection(ws) {

	ws.on('message', function(message) {
		//console.log("incoming message from: " + wss.)
		console.log('message: ' + message);
		ws.send('echo: ' + message);

	});

	console.log('new client connected!');
	ws.send('connected!');

});