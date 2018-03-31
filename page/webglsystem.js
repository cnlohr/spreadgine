var wgcanvas = null
var wgl = null;
var screenw, screenh;
var wgcams = [];
var wgshades = [];
var wgcurshad = 0;
var wggeos = [];

var perspectivematrix = [];
var viewmatrix = [];
var doubleview = true;

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
function ProcessPack()
{
	//In calling this function, packbuffer = Uint8Array of packet and packbufferp should be 0.
	//Queue up the messages until we see a 77 (page flip)
	StoredSceneOperations.push( packbuffer );
	if( packbuffer[0] != 77 ) return;

	screenw = wgl.viewportWidth = wgcanvas.width;
	screenh = wgl.viewportHeight = wgcanvas.height;
	wgcanvas.style.width ='100%';
	wgcanvas.style.height='100%';
	wgcanvas.width  = wgcanvas.offsetWidth;
	wgcanvas.height = wgcanvas.offsetHeight;

	wgl.viewport( 0, 0, wgl.viewportWidth, wgl.viewportHeight );

	for( var s in StoredSceneOperations )
	{
		packbuffer = StoredSceneOperations[s];
		packbufferp = 0;
		InternalProcessPack();
	}
	StoredSceneOperations = [];
}

function InternalProcessPack()
{
	var cal = Pop8();
	switch( cal )
	{
		case 64:		//Setup function iii w,h,cams (discarded)
			Pop32(); Pop32(); Pop32();
			document.title = PopStr();
			console.log( "Initialized" );
			break;
		case 65: //		65 = UpdateCameraName( uint8_t id, char name[...] );
			//XXX TODO
			break;
		case 66:	//XXX Wrong, but we'll use it for now.
			var cid = Pop8();
			perspectivematrix[cid] = PopMultiFloat(16);
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
			console.log( "Cam " + cid + "  \"" +name + "\" adding: " + fov + " " + aspect + " " + near +" " + far );
			while( wgcams.length <= cid ) wgcams.push( {} );
			wgcams[cid].fov = fov;
			wgcams[cid].aspect = aspect;
			wgcams[cid].near = near;
			wgcams[cid].far = far;
			wgcams[cid].nam = name;
			break;
		case 69:  //Setup new shader.
			//SpreadMessage( spr, "shader%d", "bbsssS", ret->shader_in_parent, 69, ret->shader_in_parent, shadername, fragmentShader, vertexShader, attriblistlength, attriblist );
			var shaderid = Pop8();
			while( wgshades.length <= shaderid ) wgshades.push( {} );
			var ts = wgshades[shaderid];
			var shadername = ts.nam = PopStr();
			var shaderfrag = ts.fragSource = PopStr();
			var shadervert = ts.vertSource = PopStr();
			var arblistlen = Pop32();
	
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


			ts.program = wgl.createProgram();
			wgl.attachShader(ts.program, ts.frag);
			wgl.attachShader(ts.program, ts.vert);

			for( i = 0; i < arblistlen; i++ )
			{
				wgl.bindAttribLocation( ts.program, i, PopStr() );
			}

			wgl.linkProgram(ts.program);

			if (!wgl.getProgramParameter(ts.program, wgl.LINK_STATUS)) {
				alert("Could not initialise shaders");
			}

			ts.mindex = wgl.getUniformLocation(ts.program, "mmatrix" );
			ts.vindex = wgl.getUniformLocation(ts.program, "vmatrix" );
			ts.pindex = wgl.getUniformLocation(ts.program, "pmatrix" );


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
			var loc = wgl.getUniformLocation(wgshades[wgcurshad].program,PopStr() );
			wgl.uniform4fv( loc, v );
			break;
		case 82:
			var v = PopMultiFloat(16);
			var loc = wgl.getUniformLocation(wgshades[wgcurshad].program,PopStr() );
			wgl.uniformMatrix4fv( loc, v );
			break;

		case 87:		//Update geo objec but not the geometry.
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
			wgl.bufferData(wgl.ELEMENT_ARRAY_BUFFER, wggeos[gip].indexarray, wgl.STATIC_DRAW);
			break;
		case 88:		//Update geometry
			var gip = Pop8();
			while( wggeos.length <= gip ) wggeos.push( {} );
			var arrayno = Pop8();
			if( !wggeos[gip].arraydata ) wggeos[gip].arraydata = [];
			var nrv = Pop32();
			var ad = wggeos[gip].arraydata[arrayno] = PopMultiFloat(nrv/4);
			if( !wggeos[gip].arraybuffer[arrayno] ) wggeos[gip].arraybuffer[arrayno] = wgl.createBuffer();
			var ab = wggeos[gip].arraybuffer[arrayno];
			wgl.bindBuffer(wgl.ARRAY_BUFFER, ab);
			wgl.bufferData(wgl.ARRAY_BUFFER, ad, wgl.STATIC_DRAW);
			ab.itemSize = wggeos[gip].strides[arrayno];
			ab.numItems = nrv;
			break;
		case 89:
			var gip = Pop32();
			var ge = wggeos[gip];
			var mmatrix = PopMultiFloat(16);

			var curshad = wgshades[wgcurshad];
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

			if( doubleview )
			{
				for( var i = 0; i < 2; i++ )
				{
					wgl.viewport( i*wgl.viewportWidth/2, 0, wgl.viewportWidth/2, wgl.viewportHeight );
					wgl.uniformMatrix4fv( curshad.vindex, wgl.FALSE, viewmatrix[i]);
					wgl.uniformMatrix4fv( curshad.pindex, wgl.FALSE, perspectivematrix[i] );			
					wgl.drawElements(ge.rendertype,	ge.indices, wgl.UNSIGNED_SHORT, 0 );
				}
			}
			else
			{
				wgl.uniformMatrix4fv( curshad.vindex, wgl.FALSE, viewmatrix[0]);
				wgl.uniformMatrix4fv( curshad.pindex, wgl.FALSE, perspectivematrix[1] );			
				wgl.drawElements(ge.rendertype,	ge.indices, wgl.UNSIGNED_SHORT, 0 );
			}


			break;
		case 90:
			var gip = Pop8();
			wggeos[gip] = null;
			break;
		default:
			break;
			//discard
	}
}
