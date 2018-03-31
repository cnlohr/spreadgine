


/*function send(ctx) {
   var byteArray = new Uint8Array(data);
  socket.send(byteArray.buffer); 
} 
*/

function handleReceive(message) {
	var buffer = new Uint8Array(message.data);
	
	console.log( buffer );
}

function InitWebsocket( address )
{
	socket = new WebSocket(address);
	console.log( address );
	socket.binaryType = 'arraybuffer';
	socket.onopen = function() {
		console.log( "Socket opened." );
	}
	socket.onmessage = handleReceive;
}
