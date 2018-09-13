var wgcanvas = null
var wgl = null;
var screenw, screenh;
var wgcams = [];
var wgshades = [];
var wgcurshad = 0;
var wggeos = [];
var wgtexs = [];

var sprviewpoints = 1;
var perspectivematrix = [];
var viewmatrix = [];
var currentcam = 0;
var squareVertexPositionBuffer = null;

function InitSystem( addy, canvas )
{
	wgcanvas = canvas;
	wgl = wgcanvas.getContext("webgl");
//	mat4.perspective(45, wgl.viewportWidth / wgl.viewportHeight, 0.1, 100.0, perspectivematrix);
//	mat4.identity( viewmatrix );

	InitWebsocket( addy );
}

var StoredSceneOperations = [];

var TotalProcessedBytes = 0;
var ProcessedBytesLastFrame = 0;
var TotalProcessedMessages = 0;
var ProcessedMessagesLastFrame = 0;

function ProcessPack()
{
	//In calling this function, packbuffer = Uint8Array of packet and packbufferp should be 0.
	//Queue up the messages until we see a 77 (page flip)
	StoredSceneOperations.push( packbuffer );
	TotalProcessedBytes += packbuffer.length;

	if( packbuffer[0] != 77 ) return;

	TotalProcessedMessages += StoredSceneOperations.length;
	document.getElementById( "BytesProcessedDiv" ).innerHTML = Math.round(TotalProcessedBytes/1024) + " KB";
	document.getElementById( "BytesProcessedFrameDiv" ).innerHTML = TotalProcessedBytes-ProcessedMessagesLastFrame;
	document.getElementById( "MsgProcessedDiv" ).innerHTML = TotalProcessedMessages;
	document.getElementById( "MsgProcessedFrameDiv" ).innerHTML = StoredSceneOperations.length;
	ProcessedMessagesLastFrame = TotalProcessedBytes;

	screenw = wgl.viewportWidth = wgcanvas.width;
	screenh = wgl.viewportHeight = wgcanvas.height;
	wgcanvas.style.width ='100%';
	wgcanvas.style.height='100%';
	wgcanvas.width  = wgcanvas.offsetWidth;
	wgcanvas.height = wgcanvas.offsetHeight;

	wgl.viewport( 0, 0, wgl.viewportWidth, wgl.viewportHeight );

	try {
		for( var s in StoredSceneOperations )
		{
			packbuffer = StoredSceneOperations[s];
			packbufferp = 0;
			InternalProcessPack();
		}
	}
	catch(error) {
		console.error(error);
	}


	StoredSceneOperations = [];
}

function UpdatePerspective( force )
{
	var cid = currentcam;

	if( !wgcams[cid] ) return;
	//perspectivematrix[cid] = PopMultiFloat(16);
	var fov = wgcams[cid].fov*2.0;
	var aspect = screenw/screenh;//wgcams[cid].aspect;
	var near = wgcams[cid].near;
	var far = wgcams[cid].far;
	var name = wgcams[cid].nam;
	var f = 1./Math.tan(fov/2);

	var m = new Float32Array(16);
	m[0] = -f/aspect;
	m[5] = -f;
	m[10] = (far+near)/(near-far);
	m[14] = (2*far*near)/(near-far);
	m[11] = -1;
	perspectivematrix[0] = m;
}

function InternalProcessPack()
{
	var cal = Pop8();
	switch( cal )
	{
		case 64:		//Setup function ii w,h (discarded)
			Pop32(); Pop32(); 
			document.title = PopStr();
			console.log( "Initialized: " + document.title );
			UpdatePerspective();
			break;
		case 65: //		65 = UpdateCameraName( uint8_t id, char name[...] );
			//XXX TODO
			break;
		case 66:	//XXX Wrong... sort of... the spreadgine gives the webpage the perspective matrix but we want to throw it away.
			var cid = Pop8();
			//Would get the perspective matrix.
			break;
		case 67:
			var cid = Pop8();
			viewmatrix[cid] = PopMultiFloat(16);
			break;
		case 68:  //Setup camera
			var cid = Pop8();
			var fov = PopFloat();
			var aspect = PopFloat();
			var near = PopFloat();
			var far = PopFloat();
			var name = PopStr();
			console.log( "Cam " + cid + "  \"" +name + "\" adding: " + fov + " " + aspect + " " + near +" " + far + "/" + sprviewpoints );
			while( wgcams.length <= cid ) wgcams.push( {} );
			wgcams[cid].fov = fov;
			wgcams[cid].aspect = aspect;
			wgcams[cid].near = near;
			wgcams[cid].far = far;
			wgcams[cid].nam = name;

			var sel = document.getElementById( "cameraselect" );
			var so = wgcams[cid].selopt = document.createElement("option");
			so.value = cid;
			so.text  = name;
			so.onclick = function () { currentcam = cid; };
			sel.appendChild(so);

			break;
		case 69:  //Setup new shader.
			//SpreadMessage( spr, "shader%d", "bbsssS", ret->shader_in_parent, 69, ret->shader_in_parent, shadername, fragmentShader, vertexShader, attriblistlength, attriblist );
			var shaderid = Pop8();
			while( wgshades.length <= shaderid ) wgshades.push( {} );
			var ts = wgshades[shaderid];
			var shadername = ts.nam = PopStr();
			var shaderfrag = ts.fragSource = PopStr();
			var shadervert = ts.vertSource = PopStr();
			var shadergeo = ts.geoSource = PopStr();

			ts.vert = wgl.createShader(wgl.VERTEX_SHADER);
			wgl.shaderSource(ts.vert, ts.vertSource);
			wgl.compileShader(ts.vert);
			if (!wgl.getShaderParameter(ts.vert, wgl.COMPILE_STATUS)) {
					alert(wgl.getShaderInfoLog(ts.vert));
					return null;
			}

			ts.frag = wgl.createShader(wgl.FRAGMENT_SHADER);
			wgl.shaderSource(ts.frag, ts.fragSource);
			wgl.compileShader(ts.frag);
			if (!wgl.getShaderParameter(ts.frag, wgl.COMPILE_STATUS)) {
					alert(wgl.getShaderInfoLog(ts.frag));
					return null;
			}

			if( shadergeo.length > 1 )
			{
				ts.geo = wgl.createShader(wgl.GEOMETRY_SHADER);
				wgl.shaderSource(ts.geo, ts.geoSource);
				wgl.compileShader(ts.geo);
				if (!wgl.getShaderParameter(ts.geo, wgl.COMPILE_STATUS)) {
						alert(wgl.getShaderInfoLog(ts.geo));
						return null;
				}
			}
			else
				ts.geo = null;


			ts.program = wgl.createProgram();
			wgl.attachShader(ts.program, ts.frag);
			wgl.attachShader(ts.program, ts.vert);
			if( ts.geo )
			{
				wgl.attachShader(ts.program, ts.geo );
			}

			for( i = 0; i < 8; i++ )
			{
				wgl.bindAttribLocation( ts.program, i, "attrib" + i );
			}

			wgl.linkProgram(ts.program);

			if (!wgl.getProgramParameter(ts.program, wgl.LINK_STATUS)) {
				alert("Could not initialise shaders");
			}
			else
			{
				wgl.useProgram(ts.program);

				ts.mindex = wgl.getUniformLocation(ts.program, "mmatrix" );
				ts.vindex = wgl.getUniformLocation(ts.program, "vmatrix" );
				ts.pindex = wgl.getUniformLocation(ts.program, "pmatrix" );

				for( i = 0; i < 8; i++ )
				{
					indx = wgl.getUniformLocation( ts.program, "texture" + i );
					wgl.uniform1i(indx, i);
				}
			}

			break;
		case 70:
			var shaderid = Pop8();
			while( wgshades.length <= shaderid ) wgshades.push( {} );
			wgshades[shaderid] = null;
			break;
		case 74:
			wgl.enable( Pop32() );
			break;
		case 75:
			wgl.disable( Pop32() );
			break;
		case 76:
			wgl.lineWidth( PopFloat() );
			break;
		case 77:
			UpdatePerspective();
			//"Swap Buffers"  this is actually the last thing that happens...
			break;
		case 78:
			var r, g, b, a;
			r = PopFloat();
			g = PopFloat();
			b = PopFloat();
			a = PopFloat();
			wgl.clearColor( r,g ,b, a);
			break;
		case 79:
			var bits = Pop32();
			wgl.clear( bits );
			break;
		case 80:
			wgcurshad=Pop32();
			break;
		case 81:
			var v = PopMultiFloat(4);
			var pgm = wgshades[wgcurshad].program;
			var loc = wgl.getUniformLocation(pgm,PopStr() );
			wgl.useProgram(pgm);
			wgl.uniform4fv( loc, v );
			break;
		case 82:
			var v = PopMultiFloat(16);
			var pgm = wgshades[wgcurshad].program;
			var loc = wgl.getUniformLocation(pgm,PopStr() );
			wgl.useProgram(pgm);
			wgl.uniformMatrix4fv( loc, v );
			break;

		case 87:		//Update geo object but not the geometry.
			var gip = Pop8();
			while( wggeos.length <= gip ) wggeos.push( {} );
			wggeos[gip].nam = PopStr();
			wggeos[gip].rendertype = Pop32();
			wggeos[gip].verts = Pop32();
			var arrays = wggeos[gip].nr_arrays = Pop8();
			wggeos[gip].strides = PopMulti8Auto();
			wggeos[gip].types = PopMulti8Auto();
			wggeos[gip].arraybuffer = [];
			var indices = wggeos[gip].indices = Pop32()/2;
			var iarray = wggeos[gip].indexarray = new Uint16Array( indices );
			console.log( "Indices: " + indices );
			for( var i = 0; i < indices; i++ )
			{
				iarray[i] = Pop16();
			}
			var ibo = wggeos[gip].indexbuffer = wgl.createBuffer();
			wgl.bindBuffer(wgl.ELEMENT_ARRAY_BUFFER, ibo);
			wggeos[gip].laststart = 0;
			wgl.bufferData(wgl.ELEMENT_ARRAY_BUFFER, wggeos[gip].indexarray, wgl.STATIC_DRAW);
			break;
		case 88:		//Update geometry
			var gip = Pop8();
			while( wggeos.length <= gip ) wggeos.push( {} );
			var arrayno = Pop8();
			if( !wggeos[gip].arraydata ) wggeos[gip].arraydata = [];
			var nrv = Pop32();
			var ad = wggeos[gip].arraydata[arrayno] = PopMultiFloat(nrv/4);
			console.log( wggeos[gip] );
			if( !wggeos[gip].arraybuffer ) { console.log( "Warning: Geometry updated on geo that was never initialized.\n" ); wggeos[gip].arraybuffer = []; }
			if( !wggeos[gip].strides ) wggeos[gip].strides = [];
			if( !wggeos[gip].arraybuffer[arrayno] ) wggeos[gip].arraybuffer[arrayno] = wgl.createBuffer();
			var ab = wggeos[gip].arraybuffer[arrayno];
			wgl.bindBuffer(wgl.ARRAY_BUFFER, ab);
			wgl.bufferData(wgl.ARRAY_BUFFER, ad, wgl.STATIC_DRAW);
			ab.itemSize = wggeos[gip].strides[arrayno];
			ab.numItems = nrv;
			break;
		case 89:
			var gip = Pop16();
			var start = Pop16();
			var num = Pop16();
			var ge = wggeos[gip];
			var mmatrix = PopMultiFloat(16);

			var curshad = wgshades[wgcurshad];
			if( !curshad ) break;

			wgl.useProgram(curshad.program);

			//Now, how do we render this mess?
			wgl.uniformMatrix4fv( curshad.mindex, wgl.FALSE, mmatrix );
			for( var i = 0; i < ge.nr_arrays; i++ )
			{
				wgl.bindBuffer(wgl.ARRAY_BUFFER, ge.arraybuffer[i]);
				wgl.vertexAttribPointer(i, ge.strides[i], (ge.types[i]==0)?wgl.FLOAT:wgl.UNSIGNED_BYTE, false, 0, 0);
				wgl.enableVertexAttribArray(i);
			}

			wgl.bindBuffer(wgl.ELEMENT_ARRAY_BUFFER, ge.indexbuffer);
			if( wggeos[gip].laststart != start )
			{
				wgl.bufferData(wgl.ELEMENT_ARRAY_BUFFER, new Uint16Array( wggeos[gip].indexarray.buffer, start*2 ), wgl.STATIC_DRAW);
				wggeos[gip].laststart = start;
			}

			if( sprviewpoints > 1 )
			{
				for( var i = 0; i < 2; i++ )
				{
					wgl.viewport( i*wgl.viewportWidth/2, 0, wgl.viewportWidth/2, wgl.viewportHeight );
					wgl.uniformMatrix4fv( curshad.vindex, wgl.FALSE, viewmatrix[i]);
					wgl.uniformMatrix4fv( curshad.pindex, wgl.FALSE, perspectivematrix[i] );		
					wgl.drawElements(ge.rendertype,	(num!=65535)?num:ge.indices, wgl.UNSIGNED_SHORT, 0 );
				}
			}
			else
			{
				if( perspectivematrix[0] )
				{
					wgl.uniformMatrix4fv( curshad.vindex, wgl.FALSE, viewmatrix[currentcam]);
					wgl.uniformMatrix4fv( curshad.pindex, wgl.FALSE, perspectivematrix[0] );			
					wgl.drawElements(ge.rendertype,	(num!=65535)?num:ge.indices, wgl.UNSIGNED_SHORT, 0 );
				}
			}

			break;
		case 90:
			var gip = Pop8();
			wggeos[gip] = null;
			break;

		case 97:
			var tip = Pop8();
			while( wgtexs.length <= tip ) wgtexs.push( {} );
			wgtexs[tip].nam = PopStr();
			var typ = wgtexs[tip].typ = Pop32();
			var cs = wgtexs[tip].channels  = Pop32();
			var fmt = 0;
			switch( cs )
			{
				default: fmt = 0;
				case 1: fmt = wgl.RED; break;
				case 2: fmt = wgl.RG; break;
				case 3: fmt = wgl.RGB; break;
				case 4: fmt = wgl.RGBA; break;
			}
			wgtexs[tip].fmt = fmt;
			var w = wgtexs[tip].w = Pop32();
			var h = wgtexs[tip].h = Pop32();

			var tex = wgtexs[tip].tex = wgl.createTexture();
			wgl.bindTexture(wgl.TEXTURE_2D,tex);

			var pxsiz = cs*((typ==wgl.GL_FLOAT)?4:1);
			var buff = new Uint8Array( pxsiz*w*h );
			wgl.texImage2D( wgl.TEXTURE_2D, 0, fmt, w, h, 0, fmt, typ, buff );

			wgl.bindTexture( wgl.TEXTURE_2D, tex );

			wgl.texParameteri(wgl.TEXTURE_2D, wgl.TEXTURE_MAG_FILTER, wgl.NEAREST );
			wgl.texParameteri(wgl.TEXTURE_2D, wgl.TEXTURE_MIN_FILTER, wgl.NEAREST );
			wgl.texParameteri(wgl.TEXTURE_2D, wgl.TEXTURE_WRAP_S, wgl.CLAMP_TO_EDGE );
			wgl.texParameteri(wgl.TEXTURE_2D, wgl.TEXTURE_WRAP_T, wgl.CLAMP_TO_EDGE );  //Required for np2 textures.

			//console.log( fmt, typ, wgl.RGBA, wgl.UNSIGNED_BYTE, wgl.RGBA );
			break;
		case 96:
			var tip = Pop8();
			var wgt = wgtexs[tip];
			var minmag_lin = Pop32();
			var clamping = Pop32();
			var max_miplevel = Pop32();

			wgl.bindTexture( wgl.TEXTURE_2D, wgt.tex );

			if( minmag_lin < 0 || minmag_lin > 2 ) minmag_lin = 0;

			var mmmode = [ wgl.NEAREST, wgl.LINEAR, wgl.NEAREST_MIPMAP_LINEAR ];
			var mamode = [ wgl.NEAREST, wgl.LINEAR, wgl.LINEAR ];

			wgl.texParameteri(wgl.TEXTURE_2D, wgl.TEXTURE_MAG_FILTER, mamode[minmag_lin] );
			wgl.texParameteri(wgl.TEXTURE_2D, wgl.TEXTURE_MIN_FILTER, mmmode[minmag_lin] );
			wgl.texParameteri(wgl.TEXTURE_2D, wgl.TEXTURE_WRAP_S, clamping?wgl.CLAMP_TO_EDGE:wgl.REPEAT);
			wgl.texParameteri(wgl.TEXTURE_2D, wgl.TEXTURE_WRAP_T, clamping?wgl.CLAMP_TO_EDGE:wgl.REPEAT);

			if( minmag_lin == 2 )
			{
				/*wgl.texParameteri(wgl.TEXTURE_2D, wgl.GENERATE_MIPMAP, wgl.TRUE ); 
				wgl.texParameteri(wgl.TEXTURE_2D, wgl.TEXTURE_BASE_LEVEL, 0);*/
				//wgl.texParameteri(wgl.TEXTURE_2D, wgl.TEXTURE_MAX_LEVEL, max_miplevel);
				wgl.generateMipmap( wgl.TEXTURE_2D );
				//At least in firefox, much else seems busted.
			}

			break;
		case 99:
			var tip = Pop8();
			var slot = Pop8();
			var wgt = wgtexs[tip];
			//wgl.enable(wgl.TEXTURE_2D);
			wgl.activeTexture(wgl.TEXTURE0+slot);
			wgl.bindTexture( wgl.TEXTURE_2D, wgt.tex );
			// Tell the shader we bound the texture to texture unit...
			//console.log( wgshades[wgcurshad].program );
			//wgl.uniform1i(wgshades[wgcurshad].program.programInfo.uniformLocations.uSampler, slot);
			break;
		case 98:
			var tip = Pop8();
			var x = Pop32();
			var y = Pop32();
			var w = Pop32();
			var h = Pop32();
			var wgt = wgtexs[tip];
			wgl.bindTexture( wgl.TEXTURE_2D, wgt.tex );
			var ntopop = Pop32();
			var buff = PopMulti8( ntopop );
			var mmpl = Pop8();
			wgl.texSubImage2D( wgl.TEXTURE_2D, mmpl,  x, y, w, h, wgt.fmt, wgt.typ, buff );
			break;
		case 100:
			var tip = Pop8();
			wgtexs[tip] = null;
			break;
		default:
			break;
			//discard
	}
}
