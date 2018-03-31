


/*function send(ctx) {
   var byteArray = new Uint8Array(data);
  socket.send(byteArray.buffer); 
} 
*/

function concatenate(arr1, arr2) {
    let totalLength = arr1.length + arr2.length
    const result = new Uint8Array(totalLength);
	result.set(arr1, 0);
	result.set(arr2, arr1.length);
	return result;
}


var processbuffer = new Uint8Array();

function handleReceive(message) {
	var buffer = new Uint8Array(message.data);
	if( processbuffer.length > 0 )
	{
		var totalLength = processbuffer.length + buffer.length;
		var res = new Uint8Array(totalLength);
		console.log( buffer );
	}
	else
	{
		processbuffer = buffer;
	}

	//Now, process data out of processbuffer.
	if( processbuffer.length >= 4 )
	{
		
	}
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
