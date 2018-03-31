var wgcanvas = null
var wgl = null;

var wgcams = [];
var wgshades = [];
var curshad = null;
var wggeo = [];

function InitSystem( addy, canvas )
{
	wgcanvas = canvas;
	wgl = wgcanvas.getContext("webgl");
	wgl.viewportWidth = wgcanvas.width;
	wgl.viewportHeight = wgcanvas.height;

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
//		66 = UpdateCameraPerspective( uint8_t id, float perspective[16] );
//		67 = UpdateCameraView( uint8_t id, float view[16] );
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
			wgl.linkProgram(ts.program);

			if (!wgl.getProgramParameter(ts.program, wgl.LINK_STATUS)) {
				alert("Could not initialise shaders");
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
			curshad=wgshades[Pop32()];
			wgl.useProgram(curshad.shaderProgram);
			break;
		case 81:
			var v = PopMultiFloat(4);
			var loc = wgl.getUniformLocation(curshad.shaderProgram,PopStr() );
			wgl.uniform4fv( loc, v );
			break;
		case 82:
			var v = PopMultiFloat(16);
			var loc = wgl.getUniformLocation(curshad.shaderProgram,PopStr() );
			wgl.uniformMatrix4fv( loc, v );
			break;

		case 87:
			var gip = Pop8();
			while( wggeos.length <= cid ) wggeos.push( {} );
			wggeos[gip].nam = PopStr();
			wggeos[gip].rendertype = Pop32();
			wggeos[gip].verts = Pop32();
			var arrays = wggeos[gip].nr_arrays = Pop32();
			
//XXX TODO Pick up here.
//	SpreadMessage( spr, "geometry%d", "bbsiibvvv", ret->geo_in_parent, 87, ret->geo_in_parent, geoname, render_type, verts, nr_arrays,
//		sizeof(int)*nr_arrays, strides, 
//		sizeof(int)*nr_arrays, types, 
//		sizeof(int)*nr_arrays, typesizes );

			
		
//		87 = Create new geometry (complicated fields, read in spreadgine.c)
//		88 = PushNewArrayData( uint8_t geono, int arrayno, [VOID*] data);
//		89 = SpreadRenderGeometry( uint8_t geono, int offset_at, int nr_verts, float viewmatrix[16] );
//		90 = RemoveGeometry( uint8_t geono );	//Tricky: There is no call to remove children of geometries.  Client must do that.

		default:
			break;
			//discard
	}
}
