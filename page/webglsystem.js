var wgcanvas = null
var wgl = null;

var wgcams = [];
var wgshades = [];

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
			console.log( shaderfrag );
			console.log( arblistlen );
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
			

//		69 = Setup New Shader (or update existing) ... See spreadgine_remote.c for mroe info.
//		70 = Remove Shader( uint8_t shader id );
//	
//
//		74 = glEnable( int )
//		75 = glDisable( int )
//		76 = glLineWidth( float )
//		77 = glSwap
//		78 = glClearColor( float, float, float, float )
//		79 = glClear( int )
//		80 = glUseProgram( Shader[int]->shader_in_parent );
//		81 = glUniform4fv( float[4] + plan_text_uniform_name );
//		82 = glUniformMatrix4fv( float[4] + plan_text_uniform_name );
//		83..86 reserved for other uniform operations.
//		87 = Create new geometry (complicated fields, read in spreadgine.c)
//		88 = PushNewArrayData( uint8_t geono, int arrayno, [VOID*] data);
//		89 = SpreadRenderGeometry( uint8_t geono, int offset_at, int nr_verts, float viewmatrix[16] );
//		90 = RemoveGeometry( uint8_t geono );	//Tricky: There is no call to remove children of geometries.  Client must do that.

		default:
			break;
			//discard
	}
}
