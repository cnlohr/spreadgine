var wgcanvas = null
var wgl = null;

var wgcams = [];
var wgshades = [];
var wgcurshad = 0;
var wggeos = [];

var perspectivematrix;
var viewmatrix;// = {1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.}

function InitSystem( addy, canvas )
{
	wgcanvas = canvas;
	wgl = wgcanvas.getContext("webgl");
	wgl.viewportWidth = wgcanvas.width;
	wgl.viewportHeight = wgcanvas.height;

//	mat4.perspective(45, wgl.viewportWidth / wgl.viewportHeight, 0.1, 100.0, perspectivematrix);
//	mat4.identity( viewmatrix );


	InitWebsocket( addy );
}

function ProcessPack()
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
			perspectivematrix = PopMultiFloat(16);
			break;
		case 67:
			var cid = Pop8();
			viewmatrix = PopMultiFloat(16);
			console.log( "VIEW MATRIX" );
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
			//???
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

			//XXX TODO: Switch thing over to using index buffers everywhere.
//			wggeos[gip].indexbuffer = gl.createBuffer();
//			wggeos[gip].indexarray = new Uint16Array( 65536 )
//			for( var i = 0; i < 65536; i++ )
//			{
//				wggeos[gip].indexarray[i] = i;
//			}
//			gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, wggeos[gip].indexbuffer);
//			gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(glIndices), gl.STATIC_DRAW);
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
			var offset = Pop32();
			var verts = Pop32();
			var mmatrix = PopMultiFloat(16);

			var curshad = wgshades[wgcurshad];
			wgl.useProgram(curshad.program);

			//Now, how do we render this mess?
			wgl.uniformMatrix4fv( curshad.mindex, wgl.GL_FALSE, mmatrix );
			for( var i = 0; i < ge.nr_arrays; i++ )
			{
//				console.log( ge.arraydata[i] );
				//wgl.vertexAttribPointer( i, (ge.types==0)?4:1, (ge.types==0)?wgl.GL_FLOAT:wgl.GL_UNSIGNED_BYTE, wgl.GL_FALSE, ge.strides, 0 );
				//wgl.bufferData( wgl.ARRAY_BUFFER,ge.arraydata[i],wgl.STATIC_DRAW);
//				wgl.vertexAttribPointer( i, 4, wgl.GL_FLOAT, wgl.GL_FALSE, 4, 0 );
				// ge.arraydata[i]
				wgl.bindBuffer(wgl.ARRAY_BUFFER, ge.arraybuffer[i]);
				//wgl.vertexAttribPointer( i, (ge.types==0)?4:1, (ge.types==0)?wgl.GL_FLOAT:wgl.GL_UNSIGNED_BYTE, wgl.GL_FALSE, ge.strides[i], 0 );
				wgl.vertexAttribPointer( i, (ge.types==0)?4:1, (ge.types==0)?wgl.FLOAT:wgl.UNSIGNED_BYTE, wgl.GL_FALSE, ge.strides[i], 0 );
				wgl.enableVertexAttribArray(i);
				//console.log(ge.arraydata[i]);
				//console.log( ge.arraydata[i] );
			}

			wgl.uniformMatrix4fv( curshad.vindex, wgl.GL_FALSE, viewmatrix);
			wgl.uniformMatrix4fv( curshad.pindex, wgl.GL_FALSE, perspectivematrix );

			wgl.drawArrays(ge.rendertype, offset, verts);

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
