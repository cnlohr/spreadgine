


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
var processbufferp = 0;
var packbuffer = new Uint8Array();
var packbufferp;

function Pop32()
{
	return packbuffer[packbufferp++]<<24 | packbuffer[packbufferp++]<<16 | packbuffer[packbufferp++]<<8 | packbuffer[packbufferp++];
}

function Pop16()
{
	return packbuffer[packbufferp++]<<8 | packbuffer[packbufferp++];
}

function PopMulti8Auto()
{
	nrtopop = Pop32();
	console.log( "Popping " + nrtopop );
	var ret = packbuffer.slice(packbufferp,packbufferp+nrtopop);
	packbufferp+=nrtopop;
	return ret;
}

function PopMulti8( nrtopop )
{
	var ret = packbuffer.slice(packbufferp,packbufferp+nrtopop);
	packbufferp+=nrtopop;
	return ret;
}


function Pop8()
{
	return packbuffer[packbufferp++];
}

function PopStr()
{
	var ret = "";
	for( var c = 0; c = packbuffer[packbufferp]; packbufferp++ )
	{
		ret += String.fromCharCode(c);
	}
	packbufferp++;
	return ret;
}

function PopFloat()
{
	var bb = new Uint8Array( 4 );
	bb.set( packbuffer.slice(packbufferp,packbufferp+4), 0 );
	var floatbuff = new Float32Array(bb.buffer,0 );
	packbufferp+=4;
	return floatbuff[0];
}

function PopMultiFloat( nrtopop )
{
	var bb = new Uint8Array( 4*nrtopop );
	bb.set( packbuffer.slice(packbufferp,packbufferp+4*nrtopop), 0 );
	var floatbuff = new Float32Array(bb.buffer,0 );
	packbufferp+=4*nrtopop;
	return floatbuff;
}


function handleReceive(message) {
	var buffer = new Uint8Array(message.data);

	if( processbuffer.length > 0 )
	{
		var totalLength = processbuffer.length + buffer.length;
		var res = new Uint8Array(totalLength);
		res.set( processbuffer, 0 );
		res.set( buffer, processbuffer.length );
		processbuffer = res;
	}
	else
	{
		processbuffer = buffer;
	}


	//Now, process data out of processbuffer.
	while( (processbuffer.length >= 4) && (processbufferp+4 <= processbuffer.length) )
	{
		var len = processbuffer[processbufferp+0]<<24 | processbuffer[processbufferp+1]<<16 | processbuffer[processbufferp+2]<<8 | processbuffer[processbufferp+3];
		if( len > 1000000 ) 
		{
			console.log( "Socket data stream disrupted." );
			CloseWS();
		}
		if( processbuffer.length >= len + 4 + processbufferp )
		{
			try {
				packbuffer = new Uint8Array(processbuffer.buffer, processbufferp+4, len );
				packbufferp = 0;
			}
			catch( e )
			{
				console.log( e );
				console.log( processbuffer.length );
				console.log( processbufferp );
				console.log( len );
				CloseWS();
				break;
			}
			ProcessPack();
			//console.log( packbuffer );
			processbufferp += len+4;
		}
		else
			break;
	}

/*	var vtmp = new Uint8Array( processbuffer.length - processbufferp );
	vtmp.set( processbuffer, 0 );
	processbuffer = vtmp;
	console.log( processbuffer );
*/
	processbuffer = new Uint8Array( processbuffer.buffer.slice( processbufferp ) );
	processbufferp = 0;
}


var sockettimeout = null;

function InitWebsocket( address )
{
	socket = new WebSocket(address);
	console.log( address );
	socket.binaryType = 'arraybuffer';
	socket.onopen = function() {
		console.log( "Socket opened." );
	}
	socket.onmessage = handleReceive;
	socket.onclose = function() {
		console.log( "Socket failed." );
		CloseWS();
	}
}


function CloseWS()
{
	socket.close();
	console.log( socket );
	if( sockettimeout ) clearTimeout( sockettimeout );
	sockettimeout = setTimeout( InitWebsocket(socket.url), 4000 );
	document.title = "Spreadgine Offline";

	processbuffer = new Uint8Array();
	processbufferp = 0;
	packbuffer = new Uint8Array();
	packbufferp = 0;

}
