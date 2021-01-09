vs_2_x


//c4-c6: 2d texture scaling values
//c7.x: water level
//c7.y: highest level
//c7.z: proportional snow level
//c7.w: 
//c8.xyz: camera position
//c8.w: camera depth, 0 - 1 (1=surface and above)
//c9: underwater color
//c10.x: minskning av transp
//c11.xy: 2d tex scaling

//boolean constants-----
//b0: is camera above water
//b1: no reflection drawing(deal with ordinary stuff)
//b2: deal with destination alpha blending
//b3: clip above water

def c14, 0, 0, 0, 0
def c15, 1, 1, 1, 1

//vattenberäkningar
def c16, 0.5f, 0.007f, 50, 0.1
def c17, 3, 0.33, 0, 0
def c18, 0, 1, 0, 0   //vattnets normal

def c20, 0.005, 30, 0, 0

def c30, 0, 0, 0.005, 80   //dest transparency, .x= min transp
def c31, -15, 0, 0, 0   //x=klippgräns 

dcl_position v0
//dcl_texcoord v7
//dcl_blendweight v1

mov oT6, v0   //vertex pos
/*mov r11, c14
if b1
	nop
else
	if_lt v0.y, c31.x
		mov r11, c15
	endif
	if b3   //clip above shore
		if_gt v0.y, -c31.x
			mov r11, c15
		endif
	endif
endif*/

//if_ne r11.x, c15.x
	//geo transformations
	m4x4 r0, v0, c0
	mov oPos, r0

	//texture transformations-------
	mov r1.xyzw, v0.xzxz

	mul r2, r1, c4
	mul r3, r1, c5
	mul r4, r1, c6
	//mul r5, r1, c11
	//mov r4.xy, r5.xy

	mov oT0, r4.zwxy  //texmap + distance textures
	mov oT1, r2
	mov oT2, r2.zw
	mov oT3, r3
	mov oT4, r3.zw
	mov oT5, r4
	//------------------------------

	if b1
		//distance to vertex---------
		sub r0, c8, v0
		abs r0, r0
		//hitta koordinaten med det största avståndet
		max r1.x, r0.x, r0.y
		max r2.x, r1.x, r0.z

		sub r2.x, r2.x, c20.y  //if d<c20.y then d=0
		mul r2.x, r2.x, c20.x  //if d>c20.x then x=200

		max  r2.x, r2.x, c14
		min  r12.y, r2.x, c15
		//----------------------

		//water transparency-----------------
		//beräkna avstånd till vertex
		if_lt v0.y, c7.x   //vertex under water
			call l0
			call l1
		else
			if b0   //camera and vertex above water
				mov oD0, c15
			else
				call l0
				call l1
			endif
			//mov r4.x, c15.x
		endif
		//l0:
		//r0.x=distance
		//r0.y=vertex depth
		
		//l1:
		//r2.x=lum
		//------------------------------------
			
		if b2
			//Computing dest alpha----------------
			sub r0, c8.xyz, v0.xyz	    //view vector(avstånd till vertex)
			nrm r1, r0 
			dp3 r5.a, c18.xyz, r1.xyz   //view-normal angle
			sub r5.a, c15.x, r5.a       //1-angle
				
			//minska vattnets genomskinlighet
			/*mov r0.x, c8.y              
			sub r0.x, r0.x, c10.x     //cam.pos.y - gräns
			max r0.x, r0.x, c14.x     //>0
			mul r0.x, r0.x, c30.z*/     // *transpförändringens hastighet
			//add r0.x, r0.x, c30.x   //+min transp
			
			add r5.a, r5.a, c10.x
			min r5.a, r5.a, c15.x
			mov r12.x, r5.a
		else
			mov r12.x, c14
		endif
		mov oT7, r12.xy
	endif

	//mov oFog, r4.x 

/*else  //clipping has occured
	mov oPos, c14
endif*/

ret

//End of main-----------------------------

//Camera under water,
//calc distance from vertex to water
label l0

	//distance from camera to vertex
	  sub r0, v0, c8
	 
	 //distance from vertex to water level
	   sub r1.y, v0.y, c7.x
	   abs r1.y, r1.y
	 		  
	//camera above water
	if b0
		//x and z
		mul r3, r0, r1.y    //delta-x * dy
		rcp r4.x, r0.y      
		mul r5, r3, r4.xx
		mov r0, r5
		mov r0.z, r1.y
		//mov r0.y, r4.y
	endif
	   
	//distance
	  dp3 r2, r0, r0
	  //sqrt
		pow r2.x, r2.x, c16.x 
		add r2.x, r2.x, c16.w
	 
	if_lt c16.z, r2.x
		mov r4, c14
	else
		//grumlighet
		sub r3.x, c16.z, r2.x
		mul r4.x, r3.x, c16.y
	 endif
	if_lt r1.y, c17.x //vertex nära ytan
		if b0
			//sub r3.x, c17.x, r1.y
			//mul r3.x,  r3.x, c17.y
			//add r4.x, r4.x, r3.x
			//mov r4.x, c15.x
		endif
	endif				
	min r0.x, r4.x, c15.x  //distance
	mov r0.y, r1.y	//vertex depth
ret

label l1

	min r0.y, r0.y, c16.z   //vertex depth<100
	if_gt v0.y, c7.x  //vertex above water
		mov r3.y, c15
	else
		sub r3.y, c16.z, r0.y   //100=vid ytan
		rcp r5.x, c16.z
		mul r3.y, r3.y, r5.x   //100=1
	endif
	
	//mul r1.x, c8.w, r3.y    //L2
	//lrp r2.x, r0.x, c8.w, r1.x
	
	//mul r1, r3.y, c9  //modulera dimfärg med djup
	mov oD0, r0.x  //dimavstånd
	mov oD1, c9  //dimfärg
	
ret
