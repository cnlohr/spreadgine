


function send(ctx) {
/*  // RAWデータをそのまま送信
  var data = ctx.getImageData(0, 0, 200, 200).data;
  var byteArray = new Uint8Array(data);
  socket.send(byteArray.buffer); */
} 

function handleReceive(message) {
/*
  // 受信したRAWデータをcanvasに
  var c = resultCanvas = document.getElementById('result');
  var ctx = c.getContext('2d');
  var imageData = ctx.createImageData(200, 200);
  var pixels = imageData.data;
  var buffer = new Uint8Array(message.data);
  for (var i=0; i < pixels.length; i++) {
    pixels[i] = buffer[i];
  }
  ctx.putImageData(imageData, 0, 0);
*/
  var buffer = new Uint8Array(message.data);

}

function InitWebsocket( address )
{
	socket = new WebSocket(address);
	console.log( address );
	socket.binaryType = 'arraybuffer';
	socket.onopen = function() {
		//send(ctx);
	}
	socket.onmessage = handleReceive;
}
